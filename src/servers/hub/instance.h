#pragma once
#include "channel.h"
#include "game.h"

struct HubInstance
{
	struct NewUser
	{
		ClientHandle clientHd;
		AccountUID accountUID;
	};

	const InstanceUID UID;
	ClientLocalMapping plidMap;
	HubPacketHandler packetHandler;
	HubGame game;

	HubInstance(InstanceUID UID_): UID(UID_) {}

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
	struct NewUser
	{
		ClientHandle clientHd;
		AccountUID accountUID;
		u8 team;
		u8 userID;
		u8 isBot;
	};

	struct User
	{
		const ClientHandle clientHd;
		const AccountUID accountUID;
		const u8 userID;
		const u8 isBot;

		// picking phase related stuff
		ClassType masterMain = ClassType::NONE;
		ClassType masterSub = ClassType::NONE;

		User(ClientHandle clientHd_, AccountUID accountUID_, u8 userID_, u8 isBot_):
			clientHd(clientHd_),
			accountUID(accountUID_),
			userID(userID_),
			isBot(isBot_)
		{

		}
	};

	enum Team {
		RED = 0,
		BLUE,
		SPECTATORS,
		COUNT
	};

	Server* server;
	const InstanceUID UID;
	const SortieUID sortieUID;
	ClientLocalMapping plidMap;

	eastl::fixed_list<User,16,false> userList;
	eastl::fixed_vector<decltype(userList)::iterator,5 ,false> teamRed;
	eastl::fixed_vector<decltype(userList)::iterator,5 ,false> teamBlue;
	eastl::fixed_vector<decltype(userList)::iterator,6 ,false> teamSpectator;
	eastl::fixed_vector<decltype(userList)::iterator,16,false> connectedUsers;

	RoomInstance(InstanceUID UID_, SortieUID sortieUID_):
		UID(UID_),
		sortieUID(sortieUID_)
	{

	}

	void Init(Server* server_, const NewUser* userlist, const i32 userCount);
	void Update(Time localTime_);

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
	void ReplicateMasterPick(const User& user);
};
