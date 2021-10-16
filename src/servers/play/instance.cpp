#include "instance.h"

void PvpInstance::Update(Time localTime_)
{
	if(phase == Phase::PlayingGame) {
		game.Update(localTime_);
	}
}

void PvpInstance::OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count)
{
	if(phase == Phase::PlayingGame) {
		packetHandler.OnClientsConnected(clientList, count);
	}
}

void PvpInstance::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	if(phase == Phase::PlayingGame) {
		packetHandler.OnClientsDisconnected(clientList, count);
	}
}

void PvpInstance::OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData)
{
	if(phase == Phase::PlayerLoading) {
		const i32 packetSize = header.size - sizeof(NetHeader);

		switch(header.netID) {

			default: {
				NT_LOG("[client%x] Client :: Unknown packet :: size=%d netID=%d", clientHd, header.size, header.netID);
			} break;
		}
	}
	else if(phase == Phase::PlayingGame) {
		packetHandler.OnNewPacket(clientHd, header, packetData);
	}
}

void PvpInstance::OnMatchmakerPacket(const NetHeader& header, const u8* packetData)
{

}
