#include "instance.h"

PvpInstance::PvpInstance(SortieUID sortieUID_, const In::MQ_CreateGame& gameInfo_, Server* server_):
	sortieUID(sortieUID_),
	gameInfo(gameInfo_),
	server(server_)
{
	clientAccountLink.fill(ClientHandle::INVALID);
	remainingLinks = 0;

	for(int i = 0; i < gameInfo.playerCount; i++) {
		if(!gameInfo.players[i].isBot) {
			remainingLinks++;
		}
	}
}

void PvpInstance::Update(Time localTime_)
{
	localTime = localTime_;

	if(phase == Phase::PlayingGame) {
		game.Update(localTime);
	}
}

void PvpInstance::OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count)
{
	for(int ci = 0; ci < count; ci++) {
		bool found = false;
		for(int p = 0; p < gameInfo.playerCount; p++) {
			if(gameInfo.players[p].accountUID == clientList[ci].second) {
				clientAccountLink[p] = clientList[ci].first;
				found = true;
			}
		}
		ASSERT(found);
		remainingLinks--;
	}

	// create game
	if(remainingLinks <= 0) {
		LOG("[Inst_%llu] All client connected, starting game...", sortieUID);
		phase = Phase::PlayingGame;

		game.Init(server, gameInfo, clientAccountLink);
		game.startTime = localTime;
		packetHandler.Init(&game);
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
	if(phase == Phase::PlayerConnecting) {
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
