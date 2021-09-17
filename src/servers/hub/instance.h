#pragma once
#include "channel.h"
#include "game.h"
#include "coordinator.h"

struct HubInstance: IInstance
{
	const InstanceUID UID;
	ClientLocalMapping plidMap;
	HubPacketHandler packetHandler;
	HubGame game;

	HubInstance(InstanceUID UID_): UID(UID_) {}

	bool Init(Server* server_) override;
	void Update(Time localTime_) override;
	void OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count) override;
	void OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count) override;
	void OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData) override;
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData) override;
};
