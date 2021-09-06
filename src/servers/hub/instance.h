#pragma once
#include "channel.h"
#include "game.h"
#include "coordinator.h"

struct HubInstance: IInstance
{
	HubPacketHandler packetHandler;
	GameHub game;

	bool Init(Server* server_) override;
	void Update(Time localTime_) override;
	void OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count) override;
	void OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count) override;
	void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData) override;
};
