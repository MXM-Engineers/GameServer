#pragma once
#include "channel.h"
#include "game.h"

struct PvpInstance
{
	enum class Phase: u8 {
		PlayerLoading = 0,
		PlayingGame
	};

	const SortieUID sortieUID;
	const In::MQ_CreateGame gameInfo;
	Server* server;

	Phase phase = Phase::PlayerLoading;
	GamePacketHandler packetHandler;
	Game game;

	PvpInstance(SortieUID sortieUID_, const In::MQ_CreateGame& gameInfo_, Server* server_):
		sortieUID(sortieUID_),
		gameInfo(gameInfo_),
		server(server_)
	{}

	void Update(Time localTime_);
	void OnClientsConnected(const eastl::pair<ClientHandle,AccountUID>* clientList, const i32 count);
	void OnClientsDisconnected(const ClientHandle* clientList, const i32 count);
	void OnClientPacket(ClientHandle clientHd, const NetHeader& header, const u8* packetData);
	void OnMatchmakerPacket(const NetHeader& header, const u8* packetData);
};
