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
	struct ActorPlayer;
	struct ActorPlayerCharacter;

	typedef ListItT<ActorPlayer> PlayerHandle;
	typedef ListItT<ActorPlayerCharacter> ActorPlayerCharacterHandle;

	struct ActorPlayer
	{
		struct Input
		{
			vec3 moveTo;
			f32 speed;
			RotationHumanoid rot;

			ActionStateID actionState;
			i32 actionParam1;
			i32 actionParam2;
		};

		const ActorUID UID;
		const i32 clientID; // unique player identifier, also useful to send stuff
		const WideString name;
		const WideString guildTag;

		eastl::array<ActorPlayerCharacterHandle, 2> characters;
		PlayerCharaID::Enum currentCharaID = PlayerCharaID::Main;

		Input input;

		explicit ActorPlayer(ActorUID UID_, i32 clientID_, const WideString& name_, const WideString& guildTag_):
			UID(UID_),
			clientID(clientID_),
			name(name),
			guildTag(guildTag_)
		{}

		inline ActorPlayerCharacter& Current() const { return *characters[currentCharaID]; }
		inline ActorPlayerCharacter& Main() const { return *characters[PlayerCharaID::Main]; }
		inline ActorPlayerCharacter& Sub() const { return *characters[PlayerCharaID::Sub]; }
	};

	struct ActorPlayerCharacter
	{
		const ActorUID UID;
		PlayerHandle parent;
		ClassType classType;
		SkinIndex skinIndex;
		PhysWorld::BodyHandle body;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;

		explicit ActorPlayerCharacter(ActorUID UID_): UID(UID_) {}
	};

	struct ActorNpc
	{
		const ActorUID UID;
		CreatureIndex docID;
		i32 localID;
		i32 faction;
		vec3 pos;
		vec3 dir;

		explicit ActorNpc(ActorUID UID_): UID(UID_) {}
	};


	Replication* replication;

	eastl::fixed_list<ActorPlayer,512,true> actorPlayerList;
	eastl::fixed_list<ActorPlayerCharacter,512,true> actorPlayerCharacterList;
	eastl::fixed_list<ActorNpc,512,true> actorNpcList;

	typedef ListItT<ActorNpc> ActorNpcHandle;

	// TODO: make those fixed_hash_maps
	eastl::fixed_map<ActorUID, PlayerHandle, 2048, true> actorPlayerMap;
	eastl::fixed_map<ActorUID, ActorPlayerCharacterHandle, 2048, true> actorPlayerCharacterMap;
	eastl::fixed_map<ActorUID, ActorNpcHandle, 2048, true> actorNpcMap;

	u32 nextActorUID;
	Time localTime;

	PhysWorld physics;

	void Init(Replication* replication_);
	void Update(Time localTime_);
	void Replicate();

	ActorPlayer& SpawnPlayer(i32 clientID, const wchar* name, const wchar* guildTag, ClassType mainClass, SkinIndex mainSkin, ClassType subClass, SkinIndex subSkin, const vec3& pos);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);

	ActorPlayer* FindPlayerActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

	bool DestroyPlayerActor(ActorUID actorUID);

private:
	ActorUID NewActorUID();
};
