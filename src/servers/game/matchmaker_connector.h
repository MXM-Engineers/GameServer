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
		};

		const Type type;

		union {
			struct {
				SortieUID sortieUID;
			} GameCreated;
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
};

MatchmakerConnector& Matchmaker();
