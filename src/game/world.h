#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"
#include "core.h"


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
		CreatureIndex docID;
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
		ClassType classType;
		SkinIndex skinIndex;
		i32 clientID;
		WideString name;
		WideString guildTag;
	};

	struct ActorNpc: ActorCore
	{
		i32 localID;
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

	u32 nextActorUID;

	void Init(Replication* replication_);
	void Update(f64 delta);

	ActorUID NewActorUID();

	ActorPlayer& SpawnPlayerActor(i32 clientID, ClassType classType, SkinIndex skinIndex, const wchar* name, const wchar* guildTag);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);

	void PlayerUpdatePosition(ActorUID actorUID, const Vec3& pos, const Vec3& dir, const Vec3& eye, f32 rotate, f32 speed, i32 state, i32 actionID);

	ActorPlayer* FindPlayerActor(ActorUID actorUID);
	ActorNpc* FindNpcActor(ActorUID actorUID);

	bool DestroyPlayerActor(ActorUID actorUID);
};
