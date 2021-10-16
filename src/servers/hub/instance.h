#pragma once
#include "channel.h"
#include "game.h"

enum class HubInstanceUID: u32 {
	INVALID = 0
};

struct HubInstance
{
	struct NewUser
	{
		ClientHandle clientHd;
		AccountUID accountUID;
	};

	const HubInstanceUID UID;
	ClientLocalMapping plidMap;
	HubPacketHandler packetHandler;
	HubGame game;

	HubInstance(HubInstanceUID UID_): UID(UID_) {}

	bool Init(Server* server_);
	void Update(Time localTime_);
	void OnClientsConnected(const NewUser* clientList, const i32 count);
	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnClientsTransferOut(const ClientHandle* clientList, const i32 count);
	void OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData);
};

struct RoomInstance
{
	enum Team: u8 {
		RED = 0,
		BLUE,
		SPECTATORS,
		COUNT
	};

	enum {
		MASTER_MAIN = 0,
		MASTER_SUB,
		_MASTER_COUNT,
	};

	struct NewUser
	{
		ClientHandle clientHd;
		AccountUID accountUID;
		WideString name;
		u8 team;
		u8 userID;
		u8 isBot;
	};

	struct User
	{
		struct Master
		{
			ClassType classType = ClassType::NONE;
			SkinIndex skin = SkinIndex::DEFAULT;
			eastl::array<SkillID,2> skills = { SkillID::INVALID, SkillID::INVALID };
		};

		const ClientHandle clientHd;
		const AccountUID accountUID;
		const WideString name;
		const u8 userID;
		const u8 isBot;
		const Team team;

		// picking phase related stuff
		eastl::array<Master,2> masters;
		bool isReady = false;

		struct {
			u8 masterPick: 1;
			u8 ready: 1;
			// TODO: add other stuff
		} replicate = {0};

		User(ClientHandle clientHd_, AccountUID accountUID_, const WideString& name_, u8 userID_, u8 isBot_, Team team_):
			clientHd(clientHd_),
			accountUID(accountUID_),
			name(name_),
			userID(userID_),
			isBot(isBot_),
			team(team_)
		{
			if(isBot) isReady = true;
		}

		inline Master& Main() { return masters[MASTER_MAIN]; }
		inline Master& Sub() { return masters[MASTER_SUB]; }
	};

	Server* server;
	const HubInstanceUID UID;
	const SortieUID sortieUID;
	ClientLocalMapping plidMap;

	eastl::fixed_vector<User,16,false> userList;
	eastl::fixed_vector<User*,5 ,false> teamRed;
	eastl::fixed_vector<User*,5 ,false> teamBlue;
	eastl::fixed_vector<User*,6 ,false> teamSpectator;
	eastl::fixed_vector<User*,16,false> connectedUsers;

	eastl::fixed_set<ClassType,100,false> allowedMastersSet;
	eastl::array<eastl::array<u8,100>,2> teamMasterPickCount;

	Time startTime = Time::ZERO;
	Time localTime = Time::ZERO;

	bool hasInitiatedMatchStart = false;

	RoomInstance(HubInstanceUID UID_, SortieUID sortieUID_):
		UID(UID_),
		sortieUID(sortieUID_)
	{

	}

	void Init(Server* server_, const NewUser* userlist, const i32 userCount);
	void Update(Time localTime_);
	void Replicate();

	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData);

private:

	template<typename Packet>
	inline void SendPacket(ClientHandle clientHd, const Packet& packet)
	{
		SendPacketData<Packet>(clientHd, sizeof(packet), &packet);
	}

	template<typename Packet, u32 CAPACITY>
	inline void SendPacket(ClientHandle clientHd, const PacketWriter<Packet,CAPACITY>& writer)
	{
		SendPacketData<Packet>(clientHd, writer.size, writer.data);
	}

	template<typename Packet>
	inline void SendPacketData(ClientHandle clientHd, u16 packetSize, const void* packetData)
	{
		NT_LOG("[client%x] Room(%llx) :: %s", clientHd, sortieUID, PacketSerialize<Packet>(packetData, packetSize));
		server->SendPacketData(clientHd, Packet::NET_ID, packetSize, packetData);
	}

	User* FindUser(ClientHandle clientHd);
	bool TryPickMaster(User* user, ClassType classType);
	void UnpickMaster(User* user, ClassType classType);
	void ResetMasters(User* user);
	void SetReady(User* user);
	void SendDbgMsg(ClientHandle clientHd, const wchar* msg);
};
