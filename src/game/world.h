#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"

enum class ActorUID: u32 {
	INVALID = 0
};
enum class ActorModelID: i32 {
	INVALID = 0
};

struct World
{
	template<class T>
	using List = eastl::fixed_list<T,2048>;

	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct ActorCore
	{
		ActorUID UID;
		i32 type;
		ActorModelID modelID;
		Vec3 pos;
		Vec3 dir;
		Vec3 eye;
		f32 rotate;
		f32 speed;
		i32 state;
		i32 actionID;
	};

	struct ActorPlayer: ActorCore
	{
		i32 classType;
		i32 clientID;
		WideString name;
		WideString guildTag;
	};

	struct ActorNpc: ActorCore
	{

	};

	struct ActorMonster: ActorCore
	{

	};

	Replication* replication;

	// TODO: enable overflow for those
	List<ActorPlayer> actorPlayerList;
	List<ActorNpc> actorNpcList;
	List<ActorMonster> actorMonsterList;
	eastl::fixed_map<ActorUID,List<ActorPlayer>::iterator,2048> actorPlayerMap;
	eastl::fixed_map<ActorUID,List<ActorNpc>::iterator,2048> actorNpcMap;
	eastl::fixed_map<ActorUID,List<ActorMonster>::iterator,2048> actorMonsterMap;

	u32 nextPlayerActorUID;
	u32 nextNpcActorUID;

	void Init(Replication* replication_);
	void Update(f64 delta);

	ActorUID NewPlayerActorUID();
	ActorUID NewNpcActorUID();

	ActorPlayer& SpawnPlayerActor(i32 clientID, i32 classType, const wchar* name, const wchar* guildTag);
	ActorNpc& SpawnNpcActor(ActorModelID modelID);

	void PlayerUpdatePosition(ActorUID actorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID);

	ActorPlayer* FindPlayerActor(ActorUID actorUID);
	ActorNpc* FindNpcActor(ActorUID actorUID);
};
