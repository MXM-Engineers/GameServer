#pragma once
#include <common/network.h>
#include <common/inner_protocol.h>

struct MatchmakerConnector
{
	struct Query
	{
		enum class Type: u8 {
			Invalid = 0,
			GameCreated,
			GameEnded,
		};

		const Type type;

		union {
			struct {
				SortieUID sortieUID;
			} GameCreated;
			struct {
				SortieUID sortieUID;
			} GameEnded;
		};

		explicit Query(Type type_): type(type_) {}
	};

	InnerConnection conn;
	GrowableBuffer packetQueue;

	ProfileMutex(Mutex, mutexQueries);
	eastl::fixed_vector<Query,2048> queries;

	MatchmakerConnector();

	bool Init();
	void Update();

	void QueryGameCreated(SortieUID sortieUID);
	void QueryGameEnded(SortieUID sortieUID);
};

MatchmakerConnector& Matchmaker();
