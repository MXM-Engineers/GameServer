#include "instance.h"

static const f64 HYP_LINK_WAIT_SEC = 10.0;

PvpInstance::PvpInstance(SortieUID sortieUID_, const In::MQ_CreateGame& gameInfo_, Server* server_):
	sortieUID(sortieUID_),
	gameInfo(gameInfo_),
	server(server_)
{
	clientAccountLink.fill(ClientHandle::INVALID);
	CountLinks();
	humanCount = remainingLinks;
}

void PvpInstance::CountLinks()
{
	remainingLinks = 0;

	for(int i = 0; i < gameInfo.playerCount; i++) {
		if(!gameInfo.players[i].isBot && clientAccountLink[i] == ClientHandle::INVALID) {
			remainingLinks++;
		}
	}
}

void PvpInstance::StartGame()
{
	phase = Phase::PlayingGame;

	game.Init(server, gameInfo, clientAccountLink);
	game.startTime = localTime;
	packetHandler.Init(&game);
}

void PvpInstance::Update(Time localTime_)
{
	localTime = localTime_;

	if(phase == Phase::PlayerConnecting) {
		if(remainingLinks >= humanCount) tFirstLink = Time::ZERO;
		else if(tFirstLink == Time::ZERO) tFirstLink = localTime;
		else if(TimeDiffSec(TimeDiff(tFirstLink, localTime)) >= HYP_LINK_WAIT_SEC) {
			LOG("[Inst_%llu] %d player(s) not connected after %.0f s, starting game without them...", sortieUID, remainingLinks, HYP_LINK_WAIT_SEC);
			StartGame();
		}
	}

	if(phase == Phase::PlayingGame) {
		game.Update(localTime);
	}
}

void PvpInstance::OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count)
{
	if(phase == Phase::PlayingGame) {
		for(int ci = 0; ci < count; ci++) {
			for(int p = 0; p < gameInfo.playerCount; p++) if(gameInfo.players[p].accountUID == clientList[ci].second) clientAccountLink[p] = clientList[ci].first;
			game.OnPlayerReconnect(clientList[ci].first, clientList[ci].second);
		}
		return;
	}
	for(int ci = 0; ci < count; ci++) {
		bool found = false;
		for(int p = 0; p < gameInfo.playerCount; p++) {
			if(gameInfo.players[p].accountUID == clientList[ci].second) {
				clientAccountLink[p] = clientList[ci].first;
				found = true;
			}
		}
		ASSERT(found);
	}
	CountLinks();

	// create game
	if(remainingLinks <= 0) {
		LOG("[Inst_%llu] All client connected, starting game...", sortieUID);
		StartGame();
	}
}

void PvpInstance::OnClientsDisconnected(const ClientHandle* clientList, const i32 count)
{
	if(phase == Phase::PlayingGame) {
		packetHandler.OnClientsDisconnected(clientList, count);
		return;
	}
	for(int ci = 0; ci < count; ci++) {
		for(int p = 0; p < gameInfo.playerCount; p++) {
			if(clientAccountLink[p] == clientList[ci]) clientAccountLink[p] = ClientHandle::INVALID;
		}
	}
	CountLinks();
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
