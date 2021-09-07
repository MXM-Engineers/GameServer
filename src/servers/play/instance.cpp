#include "instance.h"

bool PvpInstance::Init(Server* server_)
{
	game.Init(server_, &plidMap);
	packetHandler.Init(&game);
	return true;
}

void PvpInstance::Update(Time localTime_)
{
	game.Update(localTime_);
}

void PvpInstance::OnNewClientsConnected(const eastl::pair<ClientHandle, const AccountData*>* clientList, const i32 count)
{
	for(int i = 0; i < count; i++) {
		plidMap.Push(clientList[i].first);
	}

	packetHandler.OnNewClientsConnected(clientList, count);
}

void PvpInstance::OnNewClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	packetHandler.OnNewClientsDisconnected(clientList, count);

	for(int i = 0; i < count; i++) {
		plidMap.Pop(clientList[i]);
	}
}

void PvpInstance::OnNewPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	packetHandler.OnNewPacket(clientHd, header, packetData);
}
