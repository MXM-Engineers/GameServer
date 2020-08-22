#include "game.h"
#include "coordinator.h"
#include <EAThread/eathread_thread.h>

intptr_t ThreadGame(void* pData)
{
	Game& game = *(Game*)pData;

	while(game.server->running) {
		game.Update();
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

void Game::Update()
{
	processPacketQueue.Clear();
	{
		LockGuard lock(mutexPacketDataQueue);
		processPacketQueue.Append(packetDataQueue.data, packetDataQueue.size);
		packetDataQueue.Clear();
	}

	ConstBuffer buff(processPacketQueue.data, processPacketQueue.size);
	while(buff.CanRead(4)) {
		const i32 clientID = buff.Read<i32>();
		const NetHeader& header = buff.Read<NetHeader>();
		const u8* packetData = buff.ReadRaw(header.size);
		ClientHandlePacket(clientID, header, packetData);
	}

	world.Update(1/60.0); // TODO: update delta
	replication.FrameEnd();

	Sleep(16); // TODO: do a time accumulator scheme to wait a precise time
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
	LOG("Game:: New player :: '%S'", accountData->nickname.data());
	playerAccountData[clientID] = accountData;
}

void Game::CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const LockGuard lock(mutexPacketDataQueue);
	packetDataQueue.Append(&clientID, sizeof(clientID));
	packetDataQueue.Append(&header, sizeof(header));
	packetDataQueue.Append(packetData, header.size);
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

		default: {
			LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void Game::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client ::CN_ReadyToLoadCharacter ::", clientID);

	ASSERT(playerAccountData[clientID]); // account data is not assigned

	const AccountData* account = playerAccountData[clientID];
	World::ActorCore& actor = world.SpawnPlayerActor(clientID, 35, account->nickname.data(), account->guildTag.data());
	actor.pos = Vec3(11959.4f, 6451.76f, 3012);
	actor.dir = Vec3(0, 0, 2.68874f);
	actor.eye = Vec3(0, 0, 0);
	playerActorUID[clientID] = actor.UID;

	replication.EventPlayerConnect(clientID, (u32)playerActorUID[clientID]);
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
	const World::ActorPlayer* actor = world.FindPlayerActor(clientID, playerActorUID[clientID]);
	replication.EventPlayerRequestCharacterInfo(clientID, (u32)actor->UID, (i32)actor->modelID, actor->classType, 100, 100);
}

void Game::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);

	ASSERT(playerActorUID[clientID] == (ActorUID)update.characterID); // TODO: soft cancel, kick client
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
	if(msg[0] == L'!') {
		msg++;

		if(wcsncmp(msg, L"lego", 4) == 0) {
			World::ActorCore* playerActor = world.FindActor(playerActorUID[clientID]);
			ASSERT(playerActor);

			ActorUID actorUID = world.NewNpcActorUID();
			World::ActorCore& actor = world.SpawnPlayerActor(-1, 18, L"legomage15", L"MEME");
			actor.pos = playerActor->pos;
			actor.dir = playerActor->dir;
			actor.eye = playerActor->eye;
		}
	}
}

void Game::SpawnNPC(i32 modelID, const Vec3& pos, const Vec3& dir)
{
	World::ActorCore& actor = world.SpawnNpcActor((ActorModelID)modelID);
	actor.pos = pos;
	actor.dir = dir;
}
