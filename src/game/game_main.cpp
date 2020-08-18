#include <common/base.h>
#include <common/protocol.h>
#include <common/network.h>
#include <zlib.h>

// TODO:

#define LISTEN_PORT "11900"


DWORD ThreadGame(void* pData);

#ifdef CONF_DEBUG
static i32 packetCounter = 0;
#endif


Server* g_Server = nullptr;

struct Game
{
	Server* server;
	GrowableBuffer recvDataBuff;

	void Init(Server* server_)
	{
		server = server_;
		CreateThread(NULL, 0, ThreadGame, this, 0, NULL);

		recvDataBuff.Init(10 * (1024*1024)); // 10 MB
	}

	void Update()
	{
		server->TransferAllReceivedData(&recvDataBuff);

		ConstBuffer buff(recvDataBuff.data, recvDataBuff.size);
		while(buff.CanRead(sizeof(Server::ReceiveBufferHeader))) {
			const Server::ReceiveBufferHeader& header = buff.Read<Server::ReceiveBufferHeader>();
			const u8* data = buff.ReadRaw(header.len);
			ClientHandleReceivedChunk(header.clientID, data, header.len);
		}

		recvDataBuff.Clear();

		Sleep(16); // TODO: do a time accumulator scheme to wait a precise time
	}

	void ClientHandleReceivedChunk(i32 clientID, const u8* data, const i32 dataSize)
	{
		if(dataSize < sizeof(NetHeader)) {
			LOG("ERROR(ClientHandleReceivedChunk): received invalid data (%d < %d)", dataSize, (i32)sizeof(NetHeader));
			server->DisconnectClient(clientID);
			return;
		}

		ConstBuffer buff(data, dataSize);
		while(buff.CanRead(sizeof(NetHeader))) {
#ifdef CONF_DEBUG
			const u8* data = buff.cursor;
#endif
			const NetHeader& header = buff.Read<NetHeader>();
			const u8* packetData = buff.ReadRaw(header.size - sizeof(NetHeader));

#ifdef CONF_DEBUG
			fileSaveBuff(FMT("trace\\game_%d_cl_%d.raw", packetCounter, header.netID), data, header.size);
			packetCounter++;
#endif
			ClientHandlePacket(clientID, header, packetData);
		}
	}

	void ClientHandlePacket(i32 clientID, const NetHeader& header, const u8* packetData)
	{
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {
			case Cl::CQ_FirstHello::NET_ID: {
				LOG("[client%03d] Client :: Hello", clientID);

				const Server::ClientInfo& info = server->clientInfo[clientID];

				Sv::SA_FirstHello hello;
				hello.dwProtocolCRC = 0x28845199;
				hello.dwErrorCRC    = 0x93899e2c;
				hello.serverType    = 1;
				memmove(hello.clientIp, info.ip, sizeof(hello.clientIp));
				STATIC_ASSERT(sizeof(hello.clientIp) == sizeof(info.ip));
				hello.clientPort = info.port;
				hello.tqosWorldId = 1;

				LOG("[client%03d] Server :: SA_FirstHello :: protocolCrc=%x errorCrc=%x serverType=%d clientIp=(%s) clientPort=%d tqosWorldId=%d", clientID, hello.dwProtocolCRC, hello.dwErrorCRC, hello.serverType, IpToString(hello.clientIp), hello.clientPort, hello.tqosWorldId);
				SendPacket(clientID, hello);
			} break;

			case Cl::CQ_Authenticate::NET_ID: {
				ConstBuffer request(packetData, packetSize);
				u16 nickLen = request.Read<u16>();
				const wchar* nick = (wchar*)request.ReadRaw(nickLen * sizeof(wchar));
				i32 var = request.Read<i32>();
				LOG("[client%03d] Client :: RequestConnectGame :: %.*S var=%d", clientID, nickLen, nick, var);

				const Server::ClientInfo& info = server->clientInfo[clientID];

				Sv::SA_AuthResult auth;
				auth.result = 91;
				LOG("[client%03d] Server :: SA_AuthResult :: result=%d", clientID, auth.result);
				SendPacket(clientID, auth);

				// SN_RegionServicePolicy
				{
					u8 sendData[256];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // newMasterRestrict_count
					packet.Write<u8>(1); // newMasterRestrict[0]

					packet.Write<u16>(1); // userGradePolicy_count
					packet.Write<u8>(5); // userGradePolicy[0].userGrade
					packet.Write<u16>(1); // userGradePolicy[0].benefits_count
					packet.Write<u8>(9); // userGradePolicy[0].benefits[0]

					packet.Write<u8>(2); // purchaseCCoinMethod
					packet.Write<u8>(1); // exchangeCCoinForGoldMethod
					packet.Write<u8>(0); // rewardCCoinMethod
					packet.Write<u8>(1); // pveRewardSlotOpenBuyChanceMethod

					packet.Write<u16>(3); // regionBanMaster_count
					packet.Write<i32>(100000041); // regionBanMaster[0]
					packet.Write<i32>(100000042); // regionBanMaster[1]
					packet.Write<i32>(100000043); // regionBanMaster[2]

					packet.Write<u16>(1); // regionNewMaster_count
					packet.Write<i32>(100000038); // intList2[0]

					packet.Write<u16>(0); // eventBanMaster_count

					packet.Write<i32>(5);	// checkPeriodSec
					packet.Write<i32>(10);	// maxTalkCount
					packet.Write<i32>(120); // blockPeriodSec

					packet.Write<u16>(0); // regionBanSkinList_count
					packet.Write<u16>(0); // pcCafeSkinList_count

					packet.Write<u8>(1); // useFatigueSystem

					LOG("[client%03d] Server :: SN_RegionServicePolicy :: ", clientID);
					SendPacketData(clientID, Sv::SN_RegionServicePolicy::NET_ID, packet.size, packet.data);
				}

				// SN_AllCharacterBaseData
				{
					u8 sendData[4096];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // charaList_count

					packet.Write<i32>(100000001); // charaList[0].masterID
					packet.Write<u16>(22); // charaList[0].baseStats_count

					// charaList[0].baseStats
					typedef Sv::SN_AllCharacterBaseData::Character::Stat Stat;
					packet.Write(Stat{ 0, 2400.f });
					packet.Write(Stat{ 2, 200.f });
					packet.Write(Stat{ 37, 120.f });
					packet.Write(Stat{ 5, 5.f });
					packet.Write(Stat{ 42, 0.6f });
					packet.Write(Stat{ 7, 92.3077f });
					packet.Write(Stat{ 9, 3.f });
					packet.Write(Stat{ 10, 150.f });
					packet.Write(Stat{ 18, 100.f });
					packet.Write(Stat{ 13, 100.f });
					packet.Write(Stat{ 14, 100.f });
					packet.Write(Stat{ 15, 100.f });
					packet.Write(Stat{ 52, 100.f });
					packet.Write(Stat{ 16, 1.f });
					packet.Write(Stat{ 29, 20.f });
					packet.Write(Stat{ 23, 9.f });
					packet.Write(Stat{ 31, 14.f });
					packet.Write(Stat{ 22, 2.f });
					packet.Write(Stat{ 54, 15.f });
					packet.Write(Stat{ 63, 3.f });
					packet.Write(Stat{ 64, 150.f });
					packet.Write(Stat{ 55, 15.f });

					packet.Write<u16>(7); // charaList[0].skillData_count

					// charaList[0].skillData
					typedef Sv::SN_AllCharacterBaseData::Character::SkillRatio SkillR;
					packet.Write(SkillR{ 180010020, 355.f, 0.42f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010040, 995.f, 0.81f, 0.f, 0.f, 0.1f });
					packet.Write(SkillR{ 180010010, 550.f, 0.56f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010030, 0.f, 0.f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010050, 680.f, 0.37f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010000, 0.f, 1.0f, 0.f, 0.f, 0.f });
					packet.Write(SkillR{ 180010002, 0.f, 1.0f, 0.f, 0.f, 0.f });

					packet.Write<i32>(1); // cur
					packet.Write<i32>(1); // max


					LOG("[client%03d] Server :: SN_AllCharacterBaseData :: ", clientID);
					SendPacketData(clientID, Sv::SN_AllCharacterBaseData::NET_ID, packet.size, packet.data);
				}

				// SN_MyGuild
				{
					u8 sendData[256];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.WriteStringObj(L"XMX");
					packet.Write<i64>(0);
					packet.Write<u8>(0);

					LOG("[client%03d] Server :: SN_MyGuild :: ", clientID);
					SendPacketData(clientID, Sv::SN_MyGuild::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileCharacters
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // charaList_count

					Sv::SN_ProfileCharacters::Character chara;
					chara.characterID = 21013;
					chara.creatureIndex = 100000035;
					chara.skillShot1 = 180350010;
					chara.skillShot2 = 180350030;
					chara.class_ = 35;
					chara.x = 12029;
					chara.y = 12622;
					chara.z = 3328.29f;
					chara.characterType = 1;
					chara.skinIndex = 2;
					chara.weaponIndex = 131135012;
					chara.masterGearNo = 1;

					packet.Write(chara);

					LOG("[client%03d] Server :: SN_ProfileCharacters :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileCharacters::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileWeapons
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // weaponList_count

					Sv::SN_ProfileWeapons::Weapon weap;
					weap.characterID = 21013;
					weap.weaponType = 1;
					weap.weaponIndex = 131135012;
					weap.grade = 1;
					weap.isUnlocked = 1;
					weap.isActivated = 1;

					packet.Write(weap);

					LOG("[client%03d] Server :: SN_ProfileWeapons :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileWeapons::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileMasterGears
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // masterGears_count

					LOG("[client%03d] Server :: SN_ProfileMasterGears :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileMasterGears::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileItems
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // packetNum
					packet.Write<u16>(0); // items_count

					LOG("[client%03d] Server :: SN_ProfileItems :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileItems::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileSkills
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // packetNum
					packet.Write<u16>(0); // skills_count

					LOG("[client%03d] Server :: SN_ProfileSkills :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileSkills::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileTitles
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // titles_count
					packet.Write<i32>(320080004); // titles[0]

					LOG("[client%03d] Server :: SN_ProfileTitles :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileTitles::NET_ID, packet.size, packet.data);
				}

				// SN_ProfileCharacterSkinList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // skins_count

					LOG("[client%03d] Server :: SN_ProfileCharacterSkinList :: ", clientID);
					SendPacketData(clientID, Sv::SN_ProfileCharacterSkinList::NET_ID, packet.size, packet.data);
				}

				// SN_LeaderCharacter
				Sv::SN_LeaderCharacter leader;
				leader.leaderID = 21013;
				leader.skinIndex = 0;
				SendPacket(clientID, leader);

				// SN_AccountInfo
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.WriteStringObj(L"LordSk"); // nick
					packet.Write<i32>(4); // inventoryLineCountTab0
					packet.Write<i32>(4); // inventoryLineCountTab1
					packet.Write<i32>(4); // inventoryLineCountTab2
					packet.Write<i32>(320080005); // displayTitlteIndex
					packet.Write<i32>(320080005); // statTitleIndex
					packet.Write<i32>(1); // warehouseLineCount
					packet.Write<i32>(-1); // tutorialState
					packet.Write<i32>(3600); // masterGearDurability
					packet.Write<u8>(0); // badgeType

					LOG("[client%03d] Server :: SN_AccountInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_AccountInfo::NET_ID, packet.size, packet.data);
				}

				// SN_AccountExtraInfo
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // userGradeList_count
					packet.Write<i32>(0); // activityPoint
					packet.Write<u8>(0); // activityRewaredState

					LOG("[client%03d] Server :: SN_AccountExtraInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_AccountExtraInfo::NET_ID, packet.size, packet.data);
				}

				// SN_AccountEquipmentList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(-1); // supportKitDocIndex

					LOG("[client%03d] Server :: SN_AccountEquipmentList :: ", clientID);
					SendPacketData(clientID, Sv::SN_AccountEquipmentList::NET_ID, packet.size, packet.data);
				}

				// SN_Unknown_62472
				{
					u8 sendData[32];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u8>(1);

					LOG("[client%03d] Server :: SN_Unknown_62472 :: ", clientID);
					SendPacketData(clientID, 62472, packet.size, packet.data);
				}

				// SN_GuildChannelEnter
				{
					u8 sendData[32];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.WriteStringObj(L"XMX"); // guildName
					packet.WriteStringObj(L"LordSk"); // nick
					packet.Write<u8>(0); // onlineStatus

					LOG("[client%03d] Server :: SN_GuildChannelEnter :: ", clientID);
					SendPacketData(clientID, Sv::SN_GuildChannelEnter::NET_ID, packet.size, packet.data);
				}

				// SN_ClientSettings
				{
					u8 sendData[4096];
					PacketWriter packet(sendData, sizeof(sendData));

					const char* src = R"foo(
					<?xml version="1.0" encoding="utf-8"?>
					<KEY_DATA highDateTime="30633031" lowDateTime="3986680182" isCustom="0" keyboardLayoutName="00000813">
					  <data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_SHIRK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
					  <data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
					  <data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
					  <data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
					  <data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
					  <data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
					  <data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
					  <data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
					</KEY_DATA>
					)foo";

					u8 dest[2048];
					uLongf destLen = sizeof(dest);
					int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
					if(r != Z_OK) {
						if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
						else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
						ASSERT_MSG(0, "compress failed");
					}

					packet.Write<u8>(0); // settingType
					packet.Write<u16>(destLen);
					packet.WriteRaw(dest, destLen);

					LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
					SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
				}

				// SN_ClientSettings
				{
					u8 sendData[4096];
					PacketWriter packet(sendData, sizeof(sendData));

					const char* src =
						R"foo(
						<?xml version="1.0" encoding="utf-8"?>
						<userdata
						  version="2">
						  <useroption>
							  <displayUserName
								  version="0"
								  value="1" />
							  <displayNameOfUserTeam
								  version="0"
								  value="1" />
							  <displayNameOfOtherTeam
								  version="0"
								  value="1" />
							  <displayMonsterName
								  version="0"
								  value="1" />
							  <displayNpcName
								  version="0"
								  value="1" />
							  <displayUserTitle
								  version="0"
								  value="1" />
							  <displayOtherTitle
								  version="0"
								  value="1" />
							  <displayUserStatusBar
								  version="0"
								  value="1" />
							  <displayStatusBarOfOtherTeam
								  version="0"
								  value="1" />
							  <displayStatusBarOfUserTeam
								  version="0"
								  value="1" />
							  <displayMonsterStatusBar
								  version="0"
								  value="1" />
							  <displayDamage
								  version="0"
								  value="1" />
							  <displayStatus
								  version="0"
								  value="1" />
							  <displayMasterBigImageType
								  version="0"
								  value="0" />
							  <displayCursorSFX
								  version="0"
								  value="1" />
							  <displayTutorialInfos
								  version="0"
								  value="1" />
							  <displayUserStat
								  version="0"
								  value="0" />
							  <chatFiltering
								  version="0"
								  value="1" />
							  <chatTimstamp
								  version="0"
								  value="0" />
							  <useSmartCast
								  version="0"
								  value="0" />
							  <useMouseSight
								  version="0"
								  value="0" />
							  <alwaysActivateHUD
								  version="0"
								  value="1" />
						  </useroption>
						</userdata>
						)foo";

					u8 dest[2048];
					uLongf destLen = sizeof(dest);
					int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
					if(r != Z_OK) {
						if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
						else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
						ASSERT_MSG(0, "compress failed");
					}

					packet.Write<u8>(1); // settingType
					packet.Write<u16>(destLen);
					packet.WriteRaw(dest, destLen);

					LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
					SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
				}

				// SN_ClientSettings
				{
					u8 sendData[4096];
					PacketWriter packet(sendData, sizeof(sendData));

					const char* src =
							R"foo(
							<?xml version="1.0" encoding="utf-8"?>
							<KEY_DATA highDateTime="30633030" lowDateTime="3827081041" isCustom="0" keyboardLayoutName="00000813">
								<data input="INPUT_SHIRK" key="MKC_NOKEY" modifier="MKC_SHIFT" isaddkey="0" />
								<data input="INPUT_UIEDITMODE" key="MKC_NOKEY" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_PING" key="MKC_NOKEY" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_LATTACK" key="MKC_LBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_GAMEPING" key="MKC_LBUTTON" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_BACKPING" key="MKC_RBUTTON" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_DASHBOARD" key="MKC_TAB" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATON" key="MKC_RETURN" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHAT_ALLPLAYER_ONCE" key="MKC_RETURN" modifier="MKC_SHIFT" isaddkey="0" />
								<data input="INPUT_ESC" key="MKC_ESCAPE" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_JUMP_SAFEFALL" key="MKC_SPACE" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_REPLAY_GOTO_LIVE" key="MKC_0" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_0" key="MKC_0" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_WARFOGMODE_1" key="MKC_1" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_1" key="MKC_1" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_EMOTION_0" key="MKC_1" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_WARFOGMODE_2" key="MKC_2" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_2" key="MKC_2" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_EMOTION_1" key="MKC_2" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_WARFOGMODE_3" key="MKC_3" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_3" key="MKC_3" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_EMOTION_2" key="MKC_3" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_WARFOGMODE_4" key="MKC_4" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_4" key="MKC_4" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_CHATMACRO_5" key="MKC_5" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_TOGGLE_TIME_CONTROLLER" key="MKC_6" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_6" key="MKC_6" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_CHATMACRO_7" key="MKC_7" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_REPLAY_MOVEBACK" key="MKC_8" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_8" key="MKC_8" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_REPLAY_PAUSE_RESUME" key="MKC_9" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHATMACRO_9" key="MKC_9" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_1" key="MKC_A" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_SKILLUP_1" key="MKC_A" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_1_NOTIFY" key="MKC_A" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_OPT" key="MKC_B" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_3" key="MKC_C" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CSHOP" key="MKC_C" modifier="MKC_SHIFT" isaddkey="0" />
								<data input="INPUT_STAGE_SKILL_NOTIFY" key="MKC_C" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_RIGHT" key="MKC_D" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_2" key="MKC_E" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_SKILLUP_2" key="MKC_E" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_2_NOTIFY" key="MKC_E" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_ACTION" key="MKC_F" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_GUILD" key="MKC_G" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TUTORIAL" key="MKC_H" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_INVENTORY" key="MKC_I" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_MISSIONLIST" key="MKC_J" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_NAME_DECO" key="MKC_K" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLIST" key="MKC_L" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_SCHEDULE" key="MKC_M" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_NO" key="MKC_N" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_OPTIONWINDOW" key="MKC_O" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CHARACTER" key="MKC_P" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_POST" key="MKC_P" modifier="MKC_SHIFT" isaddkey="0" />
								<data input="INPUT_LEFT" key="MKC_Q" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_UG" key="MKC_R" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_SKILLUP_UG" key="MKC_R" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_QUICKSLOT_UG_NOTIFY" key="MKC_R" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_BACK" key="MKC_S" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ATTRIBUTE" key="MKC_T" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_AVATAR_NOTIFY" key="MKC_T" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_SUMMARY" key="MKC_U" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_PASSIVE_NOTIFY" key="MKC_V" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_TITAN_AVATAR" key="MKC_W" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_UI_TOGGLE" key="MKC_X" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_YES" key="MKC_Y" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRONT" key="MKC_Z" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_GUARDIAN_CAM" key="MKC_NUMPAD0" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_WIZARD_CAM" key="MKC_NUMPAD1" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ADAMAN_CAM" key="MKC_NUMPAD2" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_RUAK_CAM" key="MKC_NUMPAD3" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_BLUE_CAM_1" key="MKC_NUMPAD4" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_BLUE_CAM_2" key="MKC_NUMPAD5" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_BLUE_CAM_3" key="MKC_NUMPAD6" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_RED_CAM_1" key="MKC_NUMPAD7" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_RED_CAM_2" key="MKC_NUMPAD8" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TITAN_RED_CAM_3" key="MKC_NUMPAD9" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLY_CAM_1" key="MKC_F1" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLY_CAM_2" key="MKC_F2" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLY_CAM_3" key="MKC_F3" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLY_CAM_4" key="MKC_F4" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_FRIENDLY_CAM_5" key="MKC_F5" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ENEMY_CAM_1" key="MKC_F6" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ENEMY_CAM_2" key="MKC_F7" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ENEMY_CAM_3" key="MKC_F8" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ENEMY_CAM_4" key="MKC_F9" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ENEMY_CAM_5" key="MKC_F10" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CAM_MANUAL" key="MKC_F11" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_CAM_AUTO" key="MKC_F12" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_VIDEO_RECORDING" key="MKC_F12" modifier="MKC_CONTROL" isaddkey="0" />
								<data input="INPUT_CAM_ZOOM_TOGGLE" key="MKC_OEM_1" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TRADER_INGREDIENT" key="MKC_OEM_PLUS" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TRADER_SKIN" key="MKC_OEM_PERIOD" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TRADER_MEDAL" key="MKC_OEM_2" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TAG" key="MKC_WHEELDOWN" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TAG_NOTIFY" key="MKC_WHEELDOWN" modifier="MKC_MENU" isaddkey="0" />
								<data input="INPUT_BATTLELOG" key="MKC_OEM_6" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_TOGGLE_INGAME_INFORMATION" key="MKC_OEM_7" modifier="MKC_NOKEY" isaddkey="0" />
								<data input="INPUT_ATTRIBUTE" key="MKC_NOKEY" modifier="MKC_NOKEY" isaddkey="1" />
								<data input="INPUT_RATTACK" key="MKC_RBUTTON" modifier="MKC_CONTROL" isaddkey="1" />
								<data input="INPUT_TAG" key="MKC_WHEELUP" modifier="MKC_NOKEY" isaddkey="1" />
							</KEY_DATA>
							)foo";

					u8 dest[2048];
					uLongf destLen = sizeof(dest);
					int r = compress((Bytef*)dest, &destLen, (Bytef*)src, strlen(src));
					if(r != Z_OK) {
						if(r == Z_MEM_ERROR) LOG("ERROR(compress) not enough memory");
						else if(r == Z_BUF_ERROR) LOG("ERROR(compress) not enough room in the output buffer");
						ASSERT_MSG(0, "compress failed");
					}

					packet.Write<u8>(2); // settingType
					packet.Write<u16>(destLen);
					packet.WriteRaw(dest, destLen);

					LOG("[client%03d] Server :: SN_ClientSettings :: dataLen=%d", clientID, destLen);
					SendPacketData(clientID, Sv::SN_ClientSettings::NET_ID, packet.size, packet.data);
				}

				// SN_FriendList
				{
					u8 sendData[32];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // friendList_count

					LOG("[client%03d] Server :: SN_FriendList :: ", clientID);
					SendPacketData(clientID, Sv::SN_FriendList::NET_ID, packet.size, packet.data);
				}

				// SN_PveComradeInfo
				{
					u8 sendData[32];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(5); // availableComradeCount
					packet.Write<i32>(5); // maxComradeCount

					LOG("[client%03d] Server :: SN_PveComradeInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_PveComradeInfo::NET_ID, packet.size, packet.data);
				}

				// SN_AchieveUpdate
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(800); // achievementScore
					packet.Write<i32>(300190005); // index
					packet.Write<i32>(1); // type
					packet.Write<u8>(0); // isCleared
					packet.Write<u16>(0); // achievedList_count
					packet.Write<i64>(6); // progressInt64
					packet.Write<i64>(6); // date

					LOG("[client%03d] Server :: SN_AchieveUpdate :: ", clientID);
					SendPacketData(clientID, Sv::SN_AchieveUpdate::NET_ID, packet.size, packet.data);
				}

				// SN_FriendRequestList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // friendRequestList_count

					LOG("[client%03d] Server :: SN_FriendRequestList :: ", clientID);
					SendPacketData(clientID, Sv::SN_FriendRequestList::NET_ID, packet.size, packet.data);
				}

				// SN_BlockList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // blocks_count

					LOG("[client%03d] Server :: SN_BlockList :: ", clientID);
					SendPacketData(clientID, Sv::SN_BlockList::NET_ID, packet.size, packet.data);
				}

				// SN_MailUnreadNotice
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(1); // unreadInboxMailCount
					packet.Write<u16>(0); // unreadArchivedMailCount
					packet.Write<u16>(4); // unreadShopMailCount
					packet.Write<u16>(3); // inboxMailCount
					packet.Write<u16>(3); // archivedMailCount
					packet.Write<u16>(16); // shopMailCount
					packet.Write<u16>(0); // newAttachmentsPending_count

					LOG("[client%03d] Server :: SN_MailUnreadNotice :: ", clientID);
					SendPacketData(clientID, Sv::SN_MailUnreadNotice::NET_ID, packet.size, packet.data);
				}

				// SN_WarehouseItems
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // items_count

					LOG("[client%03d] Server :: SN_WarehouseItems :: ", clientID);
					SendPacketData(clientID, Sv::SN_WarehouseItems::NET_ID, packet.size, packet.data);
				}

				// SN_MutualFriendList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // candidates_count

					LOG("[client%03d] Server :: SN_MutualFriendList :: ", clientID);
					SendPacketData(clientID, Sv::SN_MutualFriendList::NET_ID, packet.size, packet.data);
				}

				// SN_GuildMemberStatus
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // guildMemberStatusList_count

					LOG("[client%03d] Server :: SN_GuildMemberStatus :: ", clientID);
					SendPacketData(clientID, Sv::SN_GuildMemberStatus::NET_ID, packet.size, packet.data);
				}

				// SN_Money
				Sv::SN_Money money;
				money.nMoney = 116472;
				money.nReason = 1;
				LOG("[client%03d] Server :: SN_Money :: ", clientID);
				SendPacket(clientID, money);
			} break;

			case Cl::CQ_GetGuildProfile::NET_ID: {
				LOG("[client%03d] Client :: CQ_GetGuildProfile ::", clientID);

				// SA_GetGuildProfile
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(0); // result;
					packet.WriteStringObj(L"The XMX dream"); // guildName
					packet.WriteStringObj(L"XMX"); // guildTag
					packet.Write<i32>(100203); // emblemIndex
					packet.Write<u8>(10); // guildLvl
					packet.Write<u8>(120); // memberMax
					packet.WriteStringObj(L"Malachi"); // ownerNickname
					packet.Write<i64>(131474874000000000); // createdDate
					packet.Write<i64>(0); // dissolutionDate
					packet.Write<u8>(0); // joinType

					Sv::SA_GetGuildProfile::ST_GuildInterest guildInterest;
					guildInterest.likePveStage = 1;
					guildInterest.likeDefence = 1;
					guildInterest.likePvpNormal = 1;
					guildInterest.likePvpOccupy = 1;
					guildInterest.likePvpGot = 1;
					guildInterest.likePvpRank = 1;
					guildInterest.likeOlympic = 1;
					packet.Write(guildInterest);

					packet.WriteStringObj(L"This is a great intro"); // guildIntro
					packet.WriteStringObj(L"Notice: this game is dead! (for now)"); // guildNotice
					packet.Write<i32>(460281); // guildPoint
					packet.Write<i32>(9999); // guildFund

					Sv::SA_GetGuildProfile::ST_GuildPvpRecord guildPvpRecord;
					guildPvpRecord.rp = 5;
					guildPvpRecord.win = 4;
					guildPvpRecord.draw = 3;
					guildPvpRecord.lose = 2;
					packet.Write(guildPvpRecord);

					packet.Write<i32>(-1); // guildRankNo

					packet.Write<u16>(1); // guildMemberClassList_count
					// guildMemberClassList[0]
					packet.Write<i32>(12456); // id
					packet.Write<u8>(3); // type
					packet.Write<u8>(2); // iconIndex
					packet.WriteStringObj(L"Malachi");

					Sv::SA_GetGuildProfile::ST_GuildMemberRights rights;
					rights.hasInviteRight = 1;
					rights.hasExpelRight = 1;
					rights.hasMembershipChgRight = 1;
					rights.hasClassAssignRight = 1;
					rights.hasNoticeChgRight = 1;
					rights.hasIntroChgRight = 1;
					rights.hasInterestChgRight = 1;
					rights.hasFundManageRight = 1;
					rights.hasJoinTypeRight = 1;
					rights.hasEmblemRight = 1;
					packet.Write(rights);

					packet.Write<u16>(1); // guildSkills_count
					// guildSkills[0]
					packet.Write<u8>(1); // type
					packet.Write<u8>(9); // level
					packet.Write<i64>(0); // expiryDate
					packet.Write<u16>(0); // extensionCount

					packet.Write<i32>(7); // curDailyStageGuildPoint
					packet.Write<i32>(500); // maxDailyStageGuildPoint
					packet.Write<i32>(2); // curDailyArenaGuildPoint
					packet.Write<i32>(450); // maxDailyArenaGuildPoint
					packet.Write<u8>(1); // todayRollCallCount

					LOG("[client%03d] Server :: SA_GetGuildProfile :: ", clientID);
					SendPacketData(clientID, Sv::SA_GetGuildProfile::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::CQ_GetGuildMemberList::NET_ID: {
				LOG("[client%03d] Client :: CQ_GetGuildMemberList ::", clientID);

				// SA_GetGuildMemberList
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(0); // result

					packet.Write<u16>(3); // guildMemberProfileList_count

					// guildMemberProfileList[0]
					packet.WriteStringObj(L"Malachi");
					packet.Write<i32>(0);  // membershipID
					packet.Write<u16>(99); // lvl
					packet.Write<u16>(10); // leaderClassType
					packet.Write<u16>(27); // masterCount
					packet.Write<i32>(12455); // achievementScore
					packet.Write<u8>(0); // topPvpTierGrade
					packet.Write<u16>(0); // topPvpTierPoint
					packet.Write<i32>(16965); // contributedGuildPoint
					packet.Write<i32>(60047); // contributedGuildFund
					packet.Write<u16>(0); // guildPvpWin
					packet.Write<u16>(0); // guildPvpPlay
					packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

					// guildMemberProfileList[1]
					packet.WriteStringObj(L"Delta-47");
					packet.Write<i32>(0);  // membershipID
					packet.Write<u16>(99); // lvl
					packet.Write<u16>(10); // leaderClassType
					packet.Write<u16>(27); // masterCount
					packet.Write<i32>(12455); // achievementScore
					packet.Write<u8>(0); // topPvpTierGrade
					packet.Write<u16>(0); // topPvpTierPoint
					packet.Write<i32>(16965); // contributedGuildPoint
					packet.Write<i32>(60047); // contributedGuildFund
					packet.Write<u16>(0); // guildPvpWin
					packet.Write<u16>(0); // guildPvpPlay
					packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

					// guildMemberProfileList[2]
					packet.WriteStringObj(L"LordSk");
					packet.Write<i32>(0);  // membershipID
					packet.Write<u16>(99); // lvl
					packet.Write<u16>(10); // leaderClassType
					packet.Write<u16>(27); // masterCount
					packet.Write<i32>(12455); // achievementScore
					packet.Write<u8>(0); // topPvpTierGrade
					packet.Write<u16>(0); // topPvpTierPoint
					packet.Write<i32>(16965); // contributedGuildPoint
					packet.Write<i32>(60047); // contributedGuildFund
					packet.Write<u16>(0); // guildPvpWin
					packet.Write<u16>(0); // guildPvpPlay
					packet.Write<i64>((i64)131568669600000000); // lastLogoutDate

					LOG("[client%03d] Server :: SA_GetGuildMemberList :: ", clientID);
					SendPacketData(clientID, Sv::SA_GetGuildMemberList::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::CQ_GetGuildHistoryList::NET_ID: {
				LOG("[client%03d] Client :: CQ_GetGuildHistoryList ::", clientID);

				// SA_GetGuildMemberList
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(0); // result

					packet.Write<u16>(0); // guildHistories_count


					LOG("[client%03d] Server :: SA_GetGuildHistoryList :: ", clientID);
					SendPacketData(clientID, Sv::SA_GetGuildHistoryList::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::CQ_GetGuildRankingSeasonList::NET_ID: {
				const Cl::CQ_GetGuildRankingSeasonList& rank = SafeCast<Cl::CQ_GetGuildRankingSeasonList>(packetData, packetSize);
				LOG("[client%03d] Client :: CQ_GetGuildRankingSeasonList :: rankingType=%d", clientID, rank.rankingType);

				// SA_GetGuildRankingSeasonList
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(0); // result
					packet.Write<u8>(rank.rankingType); // result

					packet.Write<u16>(0); // rankingSeasonList_count


					LOG("[client%03d] Server :: SA_GetGuildRankingSeasonList :: ", clientID);
					SendPacketData(clientID, Sv::SA_GetGuildRankingSeasonList::NET_ID, packet.size, packet.data);
				}
			} break;

			case Cl::CQ_TierRecord::NET_ID: {
				LOG("[client%03d] Client :: CQ_TierRecord ::", clientID);

				// SA_TierRecord
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u8>(1); // seasonId
					packet.Write<i32>(0); // allTierWin
					packet.Write<i32>(0); // allTierDraw
					packet.Write<i32>(0); // allTierLose
					packet.Write<i32>(0); // allTierLeave
					packet.Write<u16>(0); // stageRecordList_count

					LOG("[client%03d] Server :: SA_TierRecord :: ", clientID);
					SendPacketData(clientID, Sv::SA_TierRecord::NET_ID, packet.size, packet.data);
				}

				// TODO: send SN_AccountExtraInfo
				// TODO: send SN_AccountEquipmentList
				// TODO: send SN_GuildChannelEnter

				// TODO: send SN_AchieveUpdate x5
				// TODO: send SN_FriendRequestList
				// TODO: send SN_BlockList
				// TODO: send SN_MailUnreadNotice
				// TODO: send SN_WarehouseItems
				// TODO: send SN_MutualFriendList
				// TODO: send SN_GuildMemberStatus

			} break;

			case Cl::ReadyToLoadCharacter::NET_ID: {
				LOG("[client%03d] Client :: ReadyToLoadCharacter ::", clientID);

				// SN_LoadCharacterStart
				LOG("[client%03d] Server :: SN_LoadCharacterStart :: ", clientID);
				SendPacketData(clientID, Sv::SN_LoadCharacterStart::NET_ID, 0, nullptr);

				// SN_PlayerSkillSlot
				{
					u8 sendData[2048];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<i32>(21013);

					packet.Write<u16>(7); // slotList_count

					// slotList[0]
					packet.Write<i32>(180350010); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(1); // propList_count
					packet.Write<i32>(280351001); // skillPropertyIndex
					packet.Write<i32>(1); // level
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[1]
					packet.Write<i32>(180350030); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(1); // propList_count
					packet.Write<i32>(280353001); // skillPropertyIndex
					packet.Write<i32>(1); // level
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[2]
					packet.Write<i32>(180350040); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count

					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[3]
					packet.Write<i32>(180350020); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(0); // isUnlocked
					packet.Write<u8>(0); // isActivated

					// slotList[4]
					packet.Write<i32>(180350050); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[5]
					packet.Write<i32>(180350000); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[6]
					packet.Write<i32>(180350002); // skillIndex
					packet.Write<i32>(0); // coolTime
					packet.Write<u8>(1);  // unlocked
					packet.Write<u16>(0); // propList_count
					packet.Write<u8>(1); // isUnlocked
					packet.Write<u8>(1); // isActivated

					// slotList[7]
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);
					packet.Write<i32>(-1);

					ASSERT(packet.size == 133); // TODO: remove

					LOG("[client%03d] Server :: SN_PlayerSkillSlot :: ", clientID);
					SendPacketData(clientID, Sv::SN_PlayerSkillSlot::NET_ID, packet.size, packet.data);
				}

				// SN_SetGameGvt
				{
					Sv::SN_SetGameGvt gameGvt;
					i32 time = GetTime();
					gameGvt.sendTime = time;
					gameGvt.virtualTime = time;
					LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
					SendPacket(clientID, gameGvt);
				}

				// SN_SummaryInfoAll
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0);

					LOG("[client%03d] Server :: SN_SummaryInfoAll :: ", clientID);
					SendPacketData(clientID, Sv::SN_SummaryInfoAll::NET_ID, packet.size, packet.data);
				}

				// SN_AvailableSummaryRewardCountList
				{
					u8 sendData[128];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(8);

					const i32 rewardCount[8] = {
						220004100,
						2,
						220002200,
						3,
						220005100,
						3,
						220003300,
						2
					};

					packet.WriteRaw(rewardCount, sizeof(rewardCount));

					LOG("[client%03d] Server :: SN_AvailableSummaryRewardCountList :: ", clientID);
					SendPacketData(clientID, Sv::SN_AvailableSummaryRewardCountList::NET_ID, packet.size, packet.data);
				}


				// SN_SummaryInfoLatest
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(10);

					const Sv::SN_SummaryInfoLatest::Info infoList[10] = {
						{ 220002200, 200002201, 3, 220002201, 0, (i64)0xa6605c00 },
						{ 220002200, 200002201, 3, 220002204, 0, (i64)0xa6605c00 },
						{ 220002200, 200002201, 3, 220002205, 0, (i64)0xa6605c00 },
						{ 220005100, 200005101, 3, 220005101, 0, (i64)0x6a2bfc00 },
						{ 220005100, 200005101, 3, 220005102, 0, (i64)0x6a2bfc00 },
						{ 220005100, 200005101, 3, 220005105, 0, (i64)0x6a2bfc00 },
						{ 220003300, 200003301, 3, 220003301, 0, (i64)0xe0c7fa00 },
						{ 220003300, 200003301, 3, 220003305, 0, (i64)0xe0c7fa00 },
						{ 220004100, 200004101, 3, 220004101, 0, (i64)0x9eea8400 },
						{ 220004100, 200004101, 3, 220004105, 0, (i64)0x9eea8400 },
					};

					packet.WriteRaw(infoList, sizeof(infoList));

					LOG("[client%03d] Server :: SN_SummaryInfoLatest :: ", clientID);
					SendPacketData(clientID, Sv::SN_SummaryInfoLatest::NET_ID, packet.size, packet.data);
				}

				// SN_AchieveInfo
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u8>(1); // packetNum
					packet.Write<i32>(800); // achievementScore
					packet.Write<u16>(0); // achList_count

					LOG("[client%03d] Server :: SN_AchieveInfo :: ", clientID);
					SendPacketData(clientID, Sv::SN_AchieveInfo::NET_ID, packet.size, packet.data);
				}

				// SN_AchieveLatest
				{
					u8 sendData[1024];
					PacketWriter packet(sendData, sizeof(sendData));

					packet.Write<u16>(0); // achList_count

					LOG("[client%03d] Server :: SN_AchieveLatest :: ", clientID);
					SendPacketData(clientID, Sv::SN_AchieveLatest::NET_ID, packet.size, packet.data);
				}

				// SN_CityMapInfo
				Sv::SN_CityMapInfo cityMapInfo;
				cityMapInfo.cityMapID = 160000042;
				SendPacket(clientID, cityMapInfo);

				// SQ_CityLobbyJoinCity
				LOG("[client%03d] Server :: SQ_CityLobbyJoinCity :: ", clientID);
				SendPacketData(clientID, Sv::SQ_CityLobbyJoinCity::NET_ID, 0, nullptr);

				// SN_SetGameGvt
				{
					Sv::SN_SetGameGvt gameGvt;
					gameGvt.sendTime = 0;
					gameGvt.virtualTime = 0;
					LOG("[client%03d] Server :: SN_SetGameGvt :: ", clientID);
					SendPacket(clientID, gameGvt);
				}
			} break;

			case Cl::CA_SetGameGvt::NET_ID: {
				const Cl::CA_SetGameGvt& gvt = SafeCast<Cl::CA_SetGameGvt>(packetData, packetSize);
				LOG("[client%03d] Client :: CA_SetGameGvt :: sendTime=%d virtualTime=%d unk=%d", clientID, gvt.sendTime, gvt.virtualTime, gvt.unk);
			} break;

			case Cl::CN_MapIsLoaded::NET_ID: {
				LOG("[client%03d] Client :: CN_MapIsLoaded ::", clientID);

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

				// SN_ScanEnd
				{
					LOG("[client%03d] Server :: SN_ScanEnd ::", clientID);
					SendPacketData(clientID, Sv::SN_ScanEnd::NET_ID, 0, nullptr);
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
			} break;

			case Cl::CQ_GetCharacterInfo::NET_ID: {
				const Cl::CQ_GetCharacterInfo& req = SafeCast<Cl::CQ_GetCharacterInfo>(packetData, packetSize);
				LOG("[client%03d] Client :: CQ_GetCharacterInfo :: characterID=%d", clientID, req.characterID);

				// SA_GetCharacterInfo
				Sv::SA_GetCharacterInfo info;
				info.characterID = req.characterID;
				info.docIndex = 100000017;
				info.class_ = 17;
				info.hp = 100;
				info.maxHp = 100;
				LOG("[client%03d] Server :: SA_GetCharacterInfo :: ", clientID);
				SendPacket(clientID, info);
			} break;

			case Cl::CN_UpdatePosition::NET_ID: {
				const Cl::CN_UpdatePosition& update = SafeCast<Cl::CN_UpdatePosition>(packetData, packetSize);
				LOG("[client%03d] Client :: CN_UpdatePosition :: { characterID=%d p3nPos=(%g, %g, %g) p3nDir=(%g, %g, %g) p3nEye=(%g, %g, %g) nRotate=%g nSpeed=%g nState=%d nActionIDX=%d", clientID, update.characterID, update.p3nPos.x, update.p3nPos.y, update.p3nPos.z, update.p3nDir.x, update.p3nDir.y, update.p3nDir.z, update.p3nEye.x, update.p3nEye.y, update.p3nEye.z, update.nRotate, update.nSpeed, update.nState, update.nActionIDX);

				// SA_GetCharacterInfo
				Sv::SN_GamePlayerSyncByInt sync;
				sync.characterID = update.characterID;
				sync.p3nPos = update.p3nPos;
				sync.p3nDir = update.p3nDir;
				sync.p3nEye = update.p3nEye;
				sync.nRotate = update.nRotate;
				sync.nSpeed = update.nSpeed;
				sync.nState = update.nState;
				sync.nActionIDX = update.nActionIDX;
				LOG("[client%03d] Server :: SN_GamePlayerSyncByInt :: ", clientID);
				SendPacket(clientID, sync);
			} break;

			default: {
				LOG("[client%03d] Client :: Unknown packet :: size=%d netID=%d", clientID, header.size, header.netID);
			} break;
		}
	}

	template<typename Packet>
	void SendPacket(i32 clientID, const Packet& packet)
	{
		SendPacketData(clientID, Packet::NET_ID, sizeof(packet), &packet);
	}

	void SendPacketData(i32 clientID, u16 netID, u16 packetSize, const void* packetData)
	{
		const i32 packetTotalSize = packetSize+sizeof(NetHeader);
		u8 sendBuff[8192];
		ASSERT(packetTotalSize <= sizeof(sendBuff));

		NetHeader header;
		header.size = packetTotalSize;
		header.netID = netID;
		memmove(sendBuff, &header, sizeof(header));
		memmove(sendBuff+sizeof(NetHeader), packetData, packetSize);

		server->ClientSend(clientID, sendBuff, packetTotalSize);

#ifdef CONF_DEBUG
		fileSaveBuff(FMT("trace\\game_%d_sv_%d.raw", packetCounter, header.netID), sendBuff, header.size);
		packetCounter++;
#endif
	}

	void SendNPCSpawn(i32 clientID, i32 objectID, i32 nIDX, const Vec3& pos, const Vec3& dir)
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
};

DWORD ThreadGame(void* pData)
{
	Game& game = *(Game*)pData;

	while(game.server->running) {
		game.Update();
	}
	return 0;
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		LOG(">> Exit signal");
		g_Server->running = false;
	}

	return TRUE;
}

int main(int argc, char** argv)
{
	LogInit("game_server.log");
	LOG(".: Game server :.");

	if(!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		LOG("ERROR: Could not set control handler");
		return 1;
	}

	static Server server;
	bool r = server.Init(LISTEN_PORT);
	if(!r) {
		LOG("ERROR: failed to initialize server");
		return 1;
	}
	g_Server = &server;

	static Game game;
	game.Init(&server);

	// accept thread
	while(server.running) {
		// Accept a client socket
		LOG("Waiting for a connection...");
		struct sockaddr clientAddr;
		int addrLen = sizeof(sockaddr);
		SOCKET clientSocket = accept(server.serverSocket, &clientAddr, &addrLen);
		if(clientSocket == INVALID_SOCKET) {
			if(server.running) {
				LOG("ERROR(accept): failed: %d", WSAGetLastError());
				return 1;
			}
			else {
				break;
			}
		}

		LOG("New connection (%s)", GetIpString(clientAddr));
		server.AddClient(clientSocket, clientAddr);
	}

	LOG("Done.");
	return 0;
}
