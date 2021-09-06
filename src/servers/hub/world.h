#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"
#include <mxm/core.h>

struct WorldHub
{
	enum {
		MAX_PLAYERS = MAX_CLIENTS
	};

	struct ActorCore
	{
		const ActorUID UID;
		i32 type;
		CreatureIndex docID;
		vec3 pos;
		vec3 dir;
		vec3 eye;
		f32 rotate; // whole body rotate
		f32 upperRotate; // upper body rotate
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
		ClientHandle clientHd;
		WideString name;
		WideString guildTag;

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

	struct ActorJukebox: ActorNpc
	{
		struct Song
		{
			SongID songID = SongID::INVALID;
			i32 lengthInSec = 0;
			WideString requesterNick;
		};

		Song currentSong = { SongID::INVALID };
		Time playStartTime = Time::ZERO;
		eastl::fixed_list<Song,JUKEBOX_MAX_TRACKS,false> queue;

		explicit ActorJukebox(ActorUID UID_): ActorNpc(UID_) {}
	};

	ReplicationHub* replication;

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
	ActorJukebox jukebox = ActorJukebox(ActorUID::INVALID);

	u32 nextActorUID;
	Time localTime;

	void Init(ReplicationHub* replication_);
	void Update(Time localTime_);

	ActorUID NewActorUID();

	ActorPlayer& SpawnPlayerActor(i32 clientID, ClassType classType, SkinIndex skinIndex, const wchar* name, const wchar* guildTag);
	ActorPlayer& SpawnPlayerSubActor(i32 clientID, ActorUID parentActorUID, ClassType classType, SkinIndex skinIndex);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);
	ActorJukebox& SpawnJukeboxActor(CreatureIndex docID, i32 localID, const vec3& pos, const vec3& dir);

	ActorPlayer* FindPlayerActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

	bool DestroyPlayerActor(ActorUID actorUID);
};
