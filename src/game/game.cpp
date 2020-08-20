#include "game.h"

DWORD ThreadGame(void* pData)
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

	CreateThread(NULL, 0, ThreadGame, this, 0, NULL);
}

void Game::Update()
{
	processPacketQueue.Clear();
	{
		const std::lock_guard<Mutex> lock(mutexPacketDataQueue);
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

void Game::CoordinatorClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
{
	const std::lock_guard<Mutex> lock(mutexPacketDataQueue);
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

		default: {
			LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
		} break;
	}

#undef HANDLE_CASE
}

void Game::HandlePacket_CN_ReadyToLoadCharacter(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	LOG("[client%03d] Client ::CN_ReadyToLoadCharacter ::", clientID);

	playerActorUID[clientID] = world.NewPlayerActorUID();
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

	World::ActorCore& actor = world.SpawnActor(playerActorUID[clientID]);
	actor.type = 1;
	actor.modelID = (ActorModelID)100000035;
	actor.classType = 35;
	actor.pos = Vec3(11959.4f, 6451.76f, 3012);
	actor.dir = Vec3(0, 0, 2.68874f);
	actor.eye = Vec3(0, 0, 0);
	actor.rotate = 0;
	actor.speed = 0;
	actor.state = 0;
	actor.actionID = 0;

	replication.EventPlayerGameEnter(clientID);

#if 0
	// SN_GameCreateActor
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013); // objectID
		packet.Write<i32>(1); // nType
		packet.Write<i32>(100000017); // nIDX
		packet.Write<i32>(-1); // dwLocalID
		packet.Write(Vec3(11959.4f, 6451.76f, 3012)); // p3nPos
		packet.Write(Vec3(0, 0, 2.68874f)); // p3nDir
		packet.Write<i32>(0); // spawnType
		packet.Write<i32>(-1); // actionState
		packet.Write<i32>(0); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<i32>(0); // faction
		packet.Write<i32>(17); // classType
		packet.Write<i32>(2); // skinIndex
		packet.Write<i32>(0); // seed

		typedef Sv::SN_GameCreateActor::BaseStat::Stat Stat;

		// initStat ------------------------
		packet.Write<u16>(53); // maxStats_count
		packet.Write(Stat{ 0, 2400 });
		packet.Write(Stat{ 35, 1000 });
		packet.Write(Stat{ 17, 0 });
		packet.Write(Stat{ 36, 0 });
		packet.Write(Stat{ 56, 0 });
		packet.Write(Stat{ 2, 200 });
		packet.Write(Stat{ 37, 120 });
		packet.Write(Stat{ 3, 0 });
		packet.Write(Stat{ 39, 5 });
		packet.Write(Stat{ 41, 0 });
		packet.Write(Stat{ 40, 0 });
		packet.Write(Stat{ 57, 0 });
		packet.Write(Stat{ 50, 0 });
		packet.Write(Stat{ 51, 0 });
		packet.Write(Stat{ 5, 5 });
		packet.Write(Stat{ 42, 0.6f });
		packet.Write(Stat{ 6, 0 });
		packet.Write(Stat{ 7, 93.75f });
		packet.Write(Stat{ 8, 0 });
		packet.Write(Stat{ 9, 3 });
		packet.Write(Stat{ 10, 150 });
		packet.Write(Stat{ 12, 0 });
		packet.Write(Stat{ 20, 0 });
		packet.Write(Stat{ 21, 0 });
		packet.Write(Stat{ 18, 100 });
		packet.Write(Stat{ 13, 100 });
		packet.Write(Stat{ 14, 98 });
		packet.Write(Stat{ 15, 100 });
		packet.Write(Stat{ 52, 100 });
		packet.Write(Stat{ 16, 1 });
		packet.Write(Stat{ 27, 0 });
		packet.Write(Stat{ 47, 0 });
		packet.Write(Stat{ 49, 0 });
		packet.Write(Stat{ 48, 0 });
		packet.Write(Stat{ 29, 20 });
		packet.Write(Stat{ 23, 9 });
		packet.Write(Stat{ 44, 15 });
		packet.Write(Stat{ 46, 0 });
		packet.Write(Stat{ 45, 0 });
		packet.Write(Stat{ 26, 0 });
		packet.Write(Stat{ 25, 0 });
		packet.Write(Stat{ 31, 14 });
		packet.Write(Stat{ 22, 2 });
		packet.Write(Stat{ 54, 15 });
		packet.Write(Stat{ 60, 0 });
		packet.Write(Stat{ 61, 0 });
		packet.Write(Stat{ 62, 0 });
		packet.Write(Stat{ 63, 3 });
		packet.Write(Stat{ 64, 150 });
		packet.Write(Stat{ 53, 0 });
		packet.Write(Stat{ 58, 0 });
		packet.Write(Stat{ 65, 0 });
		packet.Write(Stat{ 55, 15 });

		packet.Write<u16>(2); // curStats_count
		packet.Write(Stat{ 0, 2400 });
		packet.Write(Stat{ 2, 200 });
		// ------------------------------------

		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>(GetTime()); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		ASSERT(packet.size == 368); // TODO: remove

		LOG("[client%03d] Server :: SN_GameCreateActor :: ", clientID);
		SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
	}

	// SN_GamePlayerStock
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013); // playerID
		packet.WriteStringObj(L"LordSk"); // name
		packet.Write<i32>(35); // class_
		packet.Write<i32>(320080005); // displayTitleIDX
		packet.Write<i32>(320080005); // statTitleIDX
		packet.Write<u8>(0); // badgeType
		packet.Write<u8>(0); // badgeTierLevel
		packet.WriteStringObj(L"XMX"); // guildTag
		packet.Write<u8>(0); // vipLevel
		packet.Write<u8>(0); // staffType
		packet.Write<u8>(0); // isSubstituted

		LOG("[client%03d] Server :: SN_GamePlayerStock :: ", clientID);
		SendPacketData(clientID, Sv::SN_GamePlayerStock::NET_ID, packet.size, packet.data);
	}

	// SN_PlayerStateInTown
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013); // playerID
		packet.Write<u8>(-1); // playerStateInTown
		packet.Write<u16>(0); // matchingGameModes_count

		LOG("[client%03d] Server :: SN_PlayerStateInTown :: state=%d", clientID, -1);
		SendPacketData(clientID, Sv::SN_PlayerStateInTown::NET_ID, packet.size, packet.data);
	}

	// SN_GamePlayerEquipWeapon
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013); // characterID
		packet.Write<i32>(131135012); // weaponDocIndex
		packet.Write<i32>(0); // additionnalOverHeatGauge
		packet.Write<i32>(0); // additionnalOverHeatGaugeRatio

		LOG("[client%03d] Server :: SN_GamePlayerEquipWeapon :: ", clientID);
		SendPacketData(clientID, Sv::SN_GamePlayerEquipWeapon::NET_ID, packet.size, packet.data);
	}

	// SN_TownHudStatistics
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u8>(0); // gameModeType
		packet.Write<u8>(0); // gameType
		packet.Write<u16>(3); // argList_count

		// arglist
		packet.Write<i32>(479);
		packet.Write<i32>(0);
		packet.Write<i32>(16);

		LOG("[client%03d] Server :: SN_TownHudStatistics :: ", clientID);
		SendPacketData(clientID, Sv::SN_TownHudStatistics::NET_ID, packet.size, packet.data);
	}

	// NPCs
	SendNPCSpawn(clientID, 5000, 100036952, Vec3(12437, 4859.2, 2701.5), Vec3(0, 0, 1.02137));
	SendNPCSpawn(clientID, 5001, 100036896, Vec3(11556.2, 13308.7, 3328.29), Vec3(-1.61652, -1.14546, -0.893085));
	SendNPCSpawn(clientID, 5002, 100036891, Vec3(14819.3, 9705.18, 2604.1), Vec3(0, 0, 0.783478));
	SendNPCSpawn(clientID, 5003, 100036895, Vec3(13522, 12980, 3313.52), Vec3(0, 0, 0.703193));
	SendNPCSpawn(clientID, 5004, 100036897, Vec3(12263.3, 13262.3, 3328.29), Vec3(0, 0, 0.426558));
	SendNPCSpawn(clientID, 5005, 100036894, Vec3(12005.8, 13952.3, 3529.39), Vec3(0, 0, 0));
	SendNPCSpawn(clientID, 5006, 100036909, Vec3(11551.5, 5382.32, 2701.5), Vec3(-3.08504, -0.897274, 0.665145));
	SendNPCSpawn(clientID, 5007, 100036842, Vec3(8511.02, 8348.46, 2604.1), Vec3(0, 0, -1.63747));
	SendNPCSpawn(clientID, 5008, 100036902, Vec3(9042.14, 9732.58, 2604.1), Vec3(3.06654, 1.39138, -0.873886));
	SendNPCSpawn(clientID, 5009, 100036843, Vec3(14809.8, 7021.74, 2604.1), Vec3(0, 0, 2.46842));
	SendNPCSpawn(clientID, 5010, 100036899, Vec3(10309, 13149, 3313.52), Vec3(0.914029, 0.112225, -0.642456));
	SendNPCSpawn(clientID, 5011, 100036904, Vec3(7922.89, 6310.55, 3016.64), Vec3(0, 0, -1.33937));
	SendNPCSpawn(clientID, 5012, 100036905, Vec3(8617, 5617, 3016.64), Vec3(0, 0, 3.08347));
	SendNPCSpawn(clientID, 5013, 100036903, Vec3(12949.5, 8886.19, 2604.1), Vec3(0.0986111, 0.642107, -1.29835));
	SendNPCSpawn(clientID, 5014, 100036954, Vec3(9094, 7048, 2604.1), Vec3(0, 0, -2.31972));
	SendNPCSpawn(clientID, 5015, 100036951, Vec3(11301, 12115, 3313.52), Vec3(0, 0, -1.01316));
	SendNPCSpawn(clientID, 5016, 100036906, Vec3(10931, 7739, 2605.23), Vec3(0, 0, 1.83539));
	SendNPCSpawn(clientID, 5017, 100036833, Vec3(15335.5, 8370.4, 2604.1), Vec3(0, 0, 1.53903));
	SendNPCSpawn(clientID, 5018, 100036777, Vec3(11925, 6784, 3013), Vec3(0, 0, 0));
	SendNPCSpawn(clientID, 5019, 110041382, Vec3(3667.41, 2759.76, 2601), Vec3(0, 0, -0.598997));

	// SN_PlayerStateInTown
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(21013); // playerID
		packet.Write<u8>(0); // playerStateInTown
		packet.Write<u16>(0); // matchingGameModes_count

		LOG("[client%03d] Server :: SN_PlayerStateInTown :: state=%d", clientID, 0);
		SendPacketData(clientID, Sv::SN_PlayerStateInTown::NET_ID, packet.size, packet.data);
	}

	// SN_JukeboxEnqueuedList
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // trackList_count

		LOG("[client%03d] Server :: SN_JukeboxEnqueuedList ::", clientID);
		SendPacketData(clientID, Sv::SN_JukeboxEnqueuedList::NET_ID, packet.size, packet.data);
	}

	// SN_JukeboxHotTrackList
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<u16>(0); // trackList_count

		LOG("[client%03d] Server :: SN_JukeboxHotTrackList ::", clientID);
		SendPacketData(clientID, Sv::SN_JukeboxHotTrackList::NET_ID, packet.size, packet.data);
	}

	// SN_JukeboxPlay
	{
		u8 sendData[256];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(0); // result
		packet.Write<i32>(7770015); // trackID
		packet.WriteStringObj(L"Flashback"); // nickname
		packet.Write<u16>(0); // playPositionSec

		LOG("[client%03d] Server :: SN_JukeboxPlay ::", clientID);
		SendPacketData(clientID, Sv::SN_JukeboxPlay::NET_ID, packet.size, packet.data);
	}
#endif
}

void Game::HandlePacket_CQ_GetCharacterInfo(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
	LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, req.characterID);

	// TODO: health
	const World::ActorCore* actor = world.FindActor(playerActorUID[clientID]);
	replication.EventPlayerRequestCharacterInfo(clientID, (u32)actor->UID, (i32)actor->modelID, actor->classType, 100, 100);
}

void Game::HandlePacket_CN_UpdatePosition(i32 clientID, const NetHeader& header, const u8* packetData, const i32 packetSize)
{
	const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
	LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);

	ASSERT(playerActorUID[clientID] == (ActorUID)update.characterID); // TODO: soft cancel, kick client
	world.PlayerUpdatePosition(playerActorUID[clientID], update.p3nPos, update.p3nDir, update.p3nEye, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);
}

/*
void Game::SendNPCSpawn(i32 clientID, i32 objectID, i32 nIDX, const Vec3& pos, const Vec3& dir)
{
	// SN_GameCreateActor
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		static i32 localID = 1;

		packet.Write<i32>(objectID); // objectID
		packet.Write<i32>(1); // nType
		packet.Write<i32>(nIDX); // nIDX
		packet.Write<i32>(localID++); // dwLocalID
		packet.Write(pos); // p3nPos
		packet.Write(dir); // p3nDir
		packet.Write<i32>(0); // spawnType
		packet.Write<i32>(99); // actionState
		packet.Write<i32>(0); // ownerID
		packet.Write<u8>(0); // bDirectionToNearPC
		packet.Write<i32>(-1); // AiWanderDistOverride
		packet.Write<i32>(-1); // tagID
		packet.Write<i32>(-1); // faction
		packet.Write<i32>(-1); // classType
		packet.Write<i32>(0); // skinIndex
		packet.Write<i32>(0); // seed

		// initStat ------------------------
		packet.Write<u16>(0); // maxStats_count
		packet.Write<u16>(0); // curStats_count
		// ------------------------------------

		packet.Write<u8>(1); // isInSight
		packet.Write<u8>(0); // isDead
		packet.Write<i64>(GetTime()); // serverTime

		packet.Write<u16>(0); // meshChangeActionHistory_count

		LOG("[client%03d] Server :: SN_GameCreateActor :: NPC objectID=%d nIDX=%d", clientID, objectID, nIDX);
		SendPacketData(clientID, Sv::SN_GameCreateActor::NET_ID, packet.size, packet.data);
	}

	// SN_SpawnPosForMinimap
	{
		u8 sendData[1024];
		PacketWriter packet(sendData, sizeof(sendData));

		packet.Write<i32>(objectID); // objectID
		packet.Write(pos); // p3nPos

		LOG("[client%03d] Server :: SN_SpawnPosForMinimap :: objectID=%d", clientID, objectID);
		SendPacketData(clientID, Sv::SN_SpawnPosForMinimap::NET_ID, packet.size, packet.data);
	}
}
*/

