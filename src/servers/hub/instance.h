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
	};

	Server* server;
	const InstanceUID UID;
	const SortieUID sortieUID;
	ClientLocalMapping plidMap;

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
};
