#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"
#include <game/core.h>
#include <game/physics.h>


struct World
{
	enum {
		MAX_PLAYERS = Server::MAX_CLIENTS
	};

	struct ActorCore
	{
		const ActorUID UID;
		i32 type;
		CreatureIndex docID;
		vec3 pos;
		vec3 dir;
		RotationHumanoid rotation;
		f32 speed;
		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;
		i32 faction;

		explicit ActorCore(ActorUID UID_): UID(UID_) {}
	};

	struct ActorPlayer: ActorCore
	{
		const ActorUID parentActorUID;
		ClassType classType;
		SkinIndex skinIndex;
		i32 clientID;
		WideString name;
		WideString guildTag;
		PhysWorld::BodyHandle body;

		explicit ActorPlayer(ActorUID UID_, ActorUID parentActorUID_):
			ActorCore(UID_),
			parentActorUID(parentActorUID_)
		{

		}
	};

	struct ActorNpc: ActorCore
	{
		i32 localID;

		explicit ActorNpc(ActorUID UID_): ActorCore(UID_) {}
	};

	struct ActorMonster: ActorCore
	{
		explicit ActorMonster(ActorUID UID_): ActorCore(UID_) {}
	};


	Replication* replication;

	eastl::fixed_list<ActorPlayer,512,true> actorPlayerList;
	eastl::fixed_list<ActorNpc,512,true> actorNpcList;
	eastl::fixed_list<ActorMonster,2048,true> actorMonsterList;

	typedef decltype(actorPlayerList)::iterator ActorPlayerHandle;
	typedef decltype(actorNpcList)::iterator ActorNpcHandle;
	typedef decltype(actorMonsterList)::iterator ActorMonsterHandle;

	// TODO: make those fixed_hash_maps
	eastl::fixed_map<ActorUID, ActorPlayerHandle, 2048, true> actorPlayerMap;
	eastl::fixed_map<ActorUID, ActorNpcHandle, 2048, true> actorNpcMap;
	eastl::fixed_map<ActorUID, ActorMonsterHandle, 2048, true> actorMonsterMap;

	u32 nextActorUID;
	Time localTime;

	PhysWorld physics;

	void Init(Replication* replication_);
	void Update(Time localTime_);
	void Replicate();

	ActorUID NewActorUID();

	ActorPlayer& SpawnPlayerActor(i32 clientID, ClassType classType, SkinIndex skinIndex, const wchar* name, const wchar* guildTag);
	ActorPlayer& SpawnPlayerSubActor(i32 clientID, ActorUID parentActorUID, ClassType classType, SkinIndex skinIndex);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);

	ActorPlayer* FindPlayerActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

	bool DestroyPlayerActor(ActorUID actorUID);
};
