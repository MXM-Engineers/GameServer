#include "game.h"
#include "coordinator.h"
#include <EAThread/eathread_thread.h>

intptr_t ThreadGame(void* pData)
{
	Game& game = *(Game*)pData;

	thread_local timept t0 = TimeNow();
	thread_local f64 accumulatorMs = 0.0;
	const f64 UPDATE_RATE_MS = (1.0/60.0) * 1000.0;

	while(game.server->running)
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
				game.Update(UPDATE_RATE_MS);
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

void Game::Init(Server* server_)
{
	server = server_;
	packetDataQueue.Init(10 * (1024*1024)); // 10 MB
	processPacketQueue.Init(10 * (1024*1024)); // 10 MB

	replication.Init(server);
	world.Init(&replication);

	memset(&playerActorUID, 0, sizeof(playerActorUID));

	LoadMap();

	EA::Thread::Thread Thread;
	Thread.Begin(ThreadGame, this);
}

void Game::Update(f64 delta)
{
	{
		const LockGuard lock(mutexClientDisconnectedList);
		foreach(it, clientDisconnectedList) {
			OnClientDisconnect(*it);
		}
		clientDisconnectedList.clear();
	}

	{
		const LockGuard lock(mutexNewPlayerQueue);
		foreach(it, newPlayerQueue) {
			OnClientConnect(it->clientID, it->accountData);
		}
		newPlayerQueue.clear();
	}

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

	world.Update(delta); // TODO: update delta
	replication.FrameEnd();
}

bool Game::LoadMap()
{
	// TODO: pull this info from the game's xml files
	SpawnNPC(100036952, Vec3(12437, 4859.2, 2701.5), Vec3(0, 0, 1.02137));
	SpawnNPC(100036896, Vec3(11556.2, 13308.7, 3328.29), Vec3(-1.61652, -1.14546, -0.893085));
	SpawnNPC(100036891, Vec3(14819.3, 9705.18, 2604.1), Vec3(0, 0, 0.783478));
	SpawnNPC(100036895, Vec3(13522, 12980, 3313.52), Vec3(0, 0, 0.703193));
	SpawnNPC(100036897, Vec3(12263.3, 13262.3, 3328.29), Vec3(0, 0, 0.426558));
	SpawnNPC(100036894, Vec3(12005.8, 13952.3, 3529.39), Vec3(0, 0, 0));
	SpawnNPC(100036909, Vec3(11551.5, 5382.32, 2701.5), Vec3(-3.08504, -0.897274, 0.665145));
	SpawnNPC(100036842, Vec3(8511.02, 8348.46, 2604.1), Vec3(0, 0, -1.63747));
	SpawnNPC(100036902, Vec3(9042.14, 9732.58, 2604.1), Vec3(3.06654, 1.39138, -0.873886));
	SpawnNPC(100036843, Vec3(14809.8, 7021.74, 2604.1), Vec3(0, 0, 2.46842));
	SpawnNPC(100036899, Vec3(10309, 13149, 3313.52), Vec3(0.914029, 0.112225, -0.642456));
	SpawnNPC(100036904, Vec3(7922.89, 6310.55, 3016.64), Vec3(0, 0, -1.33937));
	SpawnNPC(100036905, Vec3(8617, 5617, 3016.64), Vec3(0, 0, 3.08347));
	SpawnNPC(100036903, Vec3(12949.5, 8886.19, 2604.1), Vec3(0.0986111, 0.642107, -1.29835));
	SpawnNPC(100036954, Vec3(9094, 7048, 2604.1), Vec3(0, 0, -2.31972));
	SpawnNPC(100036951, Vec3(11301, 12115, 3313.52), Vec3(0, 0, -1.01316));
	SpawnNPC(100036906, Vec3(10931, 7739, 2605.23), Vec3(0, 0, 1.83539));
	SpawnNPC(100036833, Vec3(15335.5, 8370.4, 2604.1), Vec3(0, 0, 1.53903));
	SpawnNPC(100036777, Vec3(11925, 6784, 3013), Vec3(0, 0, 0));
	SpawnNPC(110041382, Vec3(3667.41, 2759.76, 2601), Vec3(0, 0, -0.598997));
	return true;
}

void Game::CoordinatorRegisterNewPlayer(i32 clientID, const AccountData* accountData)
{
	LOG("[client%03d] Game:: New player :: '%S'", clientID, accountData->nickname.data());

	const LockGuard lock(mutexNewPlayerQueue);
	newPlayerQueue.push_back(NewPlayerEntry{ clientID, accountData });
}

void Game::CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const LockGuard lock(mutexPacketDataQueue);
	packetDataQueue.Append(&clientID, sizeof(clientID));
	packetDataQueue.Append(&header, sizeof(header));
	packetDataQueue.Append(packetData, header.size - sizeof(NetHeader));
}

void Game::CoordinatorHandleDisconnectedClients(i32* clientIDList, const i32 count)
{
	const LockGuard lock(mutexClientDisconnectedList);
	eastl::copy(clientIDList, clientIDList+count, eastl::back_inserter(clientDisconnectedList));
}

void Game::ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
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

void Game::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_ReadyToLoadCharacter ::", clientID);
	replication.EventPlayerLoad(clientID);
}

void Game::HandlePacket_CA_SetGameGvt(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
	LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
}

void Game::HandlePacket_CN_MapIsLoaded(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client :: CN_MapIsLoaded ::", clientID);

	replication.EventPlayerGameEnter(clientID);
}

void Game::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, req.characterID);

	// TODO: health
	const World::ActorPlayer* actor = world.FindPlayerActor(playerActorUID[clientID]);
	ASSERT(actor->clientID == clientID);
	replication.EventPlayerRequestCharacterInfo(clientID, (u32)actor->UID, (i32)actor->modelID, actor->classType, 100, 100);
}

void Game::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);

	ASSERT(playerActorUID[clientID] == replication.GetActorUID(clientID, update.characterID)); // TODO: soft cancel, kick client
	world.PlayerUpdatePosition(playerActorUID[clientID], update.p3nPos, update.p3nDir, update.p3nEye, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

void Game::HandlePacket_CN_ChannelChatMessage(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	ConstBuffer buff(packetData, packetSize);
	i32 chatType = buff.Read<i32>();
	const u16 msgLen = buff.Read<u16>();
	const wchar* msg = (wchar*)buff.ReadRaw(msgLen * 2);

	LOG("[client%03d] Client :: CN_ChannelChatMessage :: chatType=%d msg='%.*S'", clientID, chatType, msgLen, msg);

	// command
	if(ParseChatCommand(clientID, msg, msgLen)) {
		return; // we're done here
	}

	replication.EventChatMessage(playerAccountData[clientID]->nickname.data(), chatType, msg, msgLen);
}

void Game::HandlePacket_CQ_SetLeaderCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_SetLeaderCharacter& leader = SafeCast<Cl::CQ_SetLeaderCharacter>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_SetLeaderCharacter :: characterID=%d skinIndex=%d", clientID, leader.characterID, leader.skinIndex);

	const i32 leaderMasterID = (u32)leader.characterID - (u32)LocalActorID::FIRST_SELF_MASTER;

	Vec3 pos(11959.4f, 6451.76f, 3012);
	Vec3 dir(0, 0, 2.68874f);
	Vec3 eye(0, 0, 0);

	// TODO: check if already leader character
	if((playerActorUID[clientID] != ActorUID::INVALID)) {
		World::ActorCore* actor = world.FindPlayerActor(playerActorUID[clientID]);
		ASSERT(actor);

		pos = actor->pos;
		dir = actor->dir;
		eye = actor->eye;

		world.DestroyPlayerActor(playerActorUID[clientID]);
	}

	ASSERT(playerAccountData[clientID]); // account data is not assigned
	const AccountData* account = playerAccountData[clientID];

	// TODO: tie in account->leaderMasterID,skinIndex with class and model
	const eastl::array<i32,40> MasterClass = {
		35, // Lua
		3,	// Sizuka
		18, // Poharan
	};

	World::ActorPlayer& actor = world.SpawnPlayerActor(clientID, MasterClass[leaderMasterID], account->nickname.data(), account->guildTag.data());
	actor.pos = pos;
	actor.dir = dir;
	actor.eye = eye;
	actor.clientID = clientID; // TODO: this is not useful right now
	playerActorUID[clientID] = actor.UID;

	replication.EventPlayerSetLeaderMaster(clientID, playerActorUID[clientID], leaderMasterID);
}

void Game::HandlePacket_CN_GamePlayerSyncActionStateOnly(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
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

	DBG_ASSERT(replication.GetLocalActorID(clientID, playerActorUID[clientID]) == sync.characterID);

	// TODO: this should probably pass by the world in a form or another?
	// So we have actors that change action state
	replication.EventPlayerActionState(playerActorUID[clientID], sync); // TODO: temporarily directly pass the packet
}

void Game::OnClientConnect(i32 clientID, const AccountData* accountData)
{
	playerAccountData[clientID] = accountData;
	replication.EventPlayerConnect(clientID);
}

void Game::OnClientDisconnect(i32 clientID)
{
	LOG("[client%03d] Game :: OnClientDisconnect :: actorUID=%u", clientID, playerActorUID[clientID]);

	// we can disconnect before spawning, so test if we have an actor associated
	if(playerActorUID[clientID] != ActorUID::INVALID) {
		world.DestroyPlayerActor(playerActorUID[clientID]);
	}
	playerActorUID[clientID] = ActorUID::INVALID;

	replication.EventClientDisconnect(clientID);
}

bool Game::ParseChatCommand(i32 clientID, const wchar* msg, const i32 len)
{
	static ActorUID lastLegoActorUID = ActorUID::INVALID;

	if(msg[0] == L'!') {
		msg++;

		if(wcsncmp(msg, L"lego", 4) == 0) {
			World::ActorCore* playerActor = world.FindPlayerActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			World::ActorCore& actor = world.SpawnPlayerActor(-1, 18, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
			lastLegoActorUID = actor.UID;

			SendDbgMsg(clientID, LFMT(L"Actor spawned at (%g, %g, %g)", actor.pos.x, actor.pos.y, actor.pos.z));
			return true;
		}

		if(wcsncmp(msg, L"delete", 6) == 0) {
			world.DestroyPlayerActor(lastLegoActorUID);

			SendDbgMsg(clientID, LFMT(L"Actor destroyed (%u)", lastLegoActorUID));
			return true;
		}
	}

	return false;
}

void Game::SendDbgMsg(i32 clientID, const wchar* msg)
{
	replication.EventChatMessageToClient(clientID, L"System", 1, msg);
}

void Game::SpawnNPC(i32 modelID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor((ActorModelID)modelID);
	actor.pos = pos;
	actor.dir = dir;
}
