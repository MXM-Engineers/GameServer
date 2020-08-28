#include "channel.h"
#include "coordinator.h"
#include <EAThread/eathread_thread.h>

intptr_t ThreadChannel(void* pData)
{
	Channel& channel = *(Channel*)pData;

	thread_local timept t0 = TimeNow();
	thread_local f64 accumulatorMs = 0.0;
	const f64 UPDATE_RATE_MS = (1.0/60.0) * 1000.0;

	while(channel.server->running)
	{
		timept t1 = TimeNow();
		accumulatorMs += TimeDurationMs(t0, t1);
		t0 = t1;

		// limit accumulation to max 2 frames
		if(accumulatorMs > (UPDATE_RATE_MS * 2)) {
			accumulatorMs = UPDATE_RATE_MS * 2;
		}

		if(accumulatorMs > UPDATE_RATE_MS) {
			do
			{
				channel.Update(UPDATE_RATE_MS);
				accumulatorMs -= UPDATE_RATE_MS;
			}
			while(accumulatorMs > UPDATE_RATE_MS);
		}
		else {
			EA::Thread::ThreadSleep(UPDATE_RATE_MS - accumulatorMs); // yield
			// EA::Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			// Sleep on windows is notoriously innacurate, we'll probably need to "just yield"
		}
	}

	return 0;
}

bool Channel::Init(Server* server_)
{
	server = server_;
	replication.Init(server_);
	game.Init(&replication);

	packetDataQueue.Init(10 * (1024*1024)); // 10 MB
	processPacketQueue.Init(10 * (1024*1024)); // 10 MB

	EA::Thread::Thread Thread;
	Thread.Begin(ThreadChannel, this);
	return true;
}

void Channel::Update(f64 delta)
{
	// clients disconnected
	{
		const LockGuard lock(mutexClientDisconnectedList);
		foreach(it, clientDisconnectedList) {
			const i32 clientID = *it;
			game.OnPlayerDisconnect(clientID);
			replication.EventClientDisconnect(clientID);
		}
		clientDisconnectedList.clear();
	}

	// clients connected
	{
		const LockGuard lock(mutexNewPlayerQueue);
		foreach(it, newPlayerQueue) {
			game.OnPlayerConnect(it->clientID, it->accountData);
			replication.EventPlayerConnect(it->clientID);
		}
		newPlayerQueue.clear();
	}

	// process packets
	processPacketQueue.Clear();
	{
		const LockGuard lock(mutexPacketDataQueue);
		processPacketQueue.Append(packetDataQueue.data, packetDataQueue.size);
		packetDataQueue.Clear();
	}

	ConstBuffer buff(processPacketQueue.data, processPacketQueue.size);
	while(buff.CanRead(4)) {
		const i32 clientID = buff.Read<i32>();
		const NetHeader& header = buff.Read<NetHeader>();
		const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));
		ClientHandlePacket(clientID, header, packetData);
	}

	game.Update(delta);
	replication.FrameEnd();
}

void Channel::CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData)
{
	LOG("[client%03d] Channel:: New player :: '%S'", clientID, accountData->nickname.data());

	const LockGuard lock(mutexNewPlayerQueue);
	newPlayerQueue.push_back(EventOnClientConnect{ clientID, accountData });
}

void Channel::CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const LockGuard lock(mutexPacketDataQueue);
	packetDataQueue.Append(&clientID, sizeof(clientID));
	packetDataQueue.Append(&header, sizeof(header));
	packetDataQueue.Append(packetData, header.size - sizeof(NetHeader));
}

void Channel::CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count)
{
	const LockGuard lock(mutexClientDisconnectedList);
	eastl::copy(clientIDList, clientIDList+count, eastl::back_inserter(clientDisconnectedList));
}

void Channel::ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const i32 packetSize = header.size - sizeof(NetHeader);

#define HANDLE_CASE(PACKET) case Cl::PACKET::NET_ID: { HandlePacket_##PACKET(clientID, header, packetData, packetSize); } break

	switch(header.netID) {
		HANDLE_CASE(CN_ReadyToLoadCharacter);
		HANDLE_CASE(CA_SetGameGvt);
		HANDLE_CASE(CN_MapIsLoaded);
		HANDLE_CASE(CQ_GetCharacterInfo);
		HANDLE_CASE(CN_UpdatePosition);
		HANDLE_CASE(CN_ChannelChatMessage);
		HANDLE_CASE(CQ_SetLeaderCharacter);
		HANDLE_CASE(CN_GamePlayerSyncActionStateOnly);

		default: {
			LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void Channel::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_ReadyToLoadCharacter ::", clientID);
	replication.EventPlayerLoad(clientID);
}

void Channel::HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void Channel::HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_MapIsLoaded ::", clientID);

	replication.EventPlayerGameEnter(clientID);
}

void Channel::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, req.characterID);
	game.OnPlayerGetCharacterInfo(clientID, req.characterID);
}

void Channel::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);

	game.OnPlayerUpdatePosition(clientID, update.characterID, update.p3nPos, update.p3nDir, update.p3nEye, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void Channel::HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	LOG("[client%03d] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientID, chatType, msgLen, msg);

	game.OnPlayerChatMessage(clientID, chatType, msg, msgLen);
}

void Channel::HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientID, leader.characterID, leader.skinIndex);

	game.OnPlayerSetLeaderCharacter(clientID, leader.characterID, leader.skinIndex);
}

void Channel::HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_GamePlayerSyncActionStateOnly& sync = SafeCast<Cl::CN_GamePlayerSyncActionStateOnly>(packetData, packetSize);

	i32 state = sync.state;
	const char* stateStr = g_ActionStateInvalidString;
	if(state >= 0 && state < ARRAY_COUNT(g_ActionStateString)) {
		stateStr = g_ActionStateString[state];
	}

	LOG("[client%03d] Client :: CN_GamePlayerSyncActionStateOnly :: {", clientID);
	LOG("	characterID=%d", sync.characterID);
	LOG("	nState=%d (%s)", sync.state, stateStr);
	LOG("	bApply=%d", sync.bApply);
	LOG("	param1=%d", sync.param1);
	LOG("	param2=%d", sync.param2);
	LOG("	i4=%d", sync.i4);
	LOG("	rotate=%g", sync.rotate);
	LOG("	upperRotate=%g", sync.upperRotate);
	LOG("}");

	game.OnPlayerSyncActionState(clientID, sync);
}
