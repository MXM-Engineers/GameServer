#include "instance.h"

bool PvpInstance::Init(Server* server_)
{
	game.Init(server_);
	packetHandler.Init(&game);
	return true;
}

void PvpInstance::Update(Time localTime_)
{
	game.Update(localTime_);
}

void PvpInstance::OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count)
{
	packetHandler.OnNewClientsConnected(clientList, count);
}

void PvpInstance::OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	packetHandler.OnNewClientsDisconnected(clientList, count);
}

void PvpInstance::OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	packetHandler.OnNewPacket(clientHd, header, packetData);
}
