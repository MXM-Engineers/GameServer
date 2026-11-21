#pragma once
#include "channel.h"
#include "game.h"

struct PvpInstance
{
	enum class Phase: u8 {
		PlayerConnecting = 0,
		PlayingGame
	};

	struct AccountLink {
		AccountUID accountUID;
		ClientHandle clientHd;
	};

	const SortieUID sortieUID;
	const In::MQ_CreateGame gameInfo;
	Server* server;
	Time localTime;

	Phase phase = Phase::PlayerConnecting;
	bool endReported = false;
	eastl::array<ClientHandle, Game::MAX_PLAYERS> clientAccountLink;
	i32 remainingLinks;
	i32 humanCount;
	Time tFirstLink = Time::ZERO;
	Time emptySince = Time::ZERO;

	GamePacketHandler packetHandler;
	Game game;

	PvpInstance(SortieUID sortieUID_, const In::MQ_CreateGame& gameInfo_, Server* server_);

	void Update(Time localTime_);
	void OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count);
	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData);
	void CountLinks();
	void StartGame();
};
