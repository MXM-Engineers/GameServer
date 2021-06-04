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
	struct Player;
	struct ActorMaster;

	typedef ListItT<ActorMaster> ActorMasterHandle;

	struct Player
	{
		struct Input
		{
			vec3 moveTo;
			f32 speed;
			RotationHumanoid rot;

			u8 tag = 0;
			ActionStateID actionState;
			i32 actionParam1;
			i32 actionParam2;
		};

		const PlayerID playerID;
		const i32 clientID;
		const WideString name;
		const WideString guildTag;

		const ClassType mainClass;
		const SkinIndex mainSkin;
		const ClassType subClass;
		const SkinIndex subSkin;

		u8 level;
		u32 experience;

		eastl::array<ActorMasterHandle, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID = 0;

		Input input;

		explicit Player(
				PlayerID playerID_,
				i32 clientID_,
				const WideString& name_,
				const WideString& guildTag_,
				ClassType mainClass_,
				SkinIndex mainSkin_,
				ClassType subClass_,
				SkinIndex subSkin_
				):
			playerID(playerID_),
			clientID(clientID_),
			name(name_),
			guildTag(guildTag_),
			mainClass(mainClass_),
			mainSkin(mainSkin_),
			subClass(subClass_),
			subSkin(subSkin_)
		{

		}

		inline ActorMaster& Main() const { return *characters[mainCharaID]; }
		inline ActorMaster& Sub() const { return *characters[mainCharaID ^ 1]; }
	};

	struct ActorMaster
	{
		const ActorUID UID;
		Player* parent;
		ClassType classType;
		SkinIndex skinIndex;
		PhysWorld::BodyHandle body;

		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;

		explicit ActorMaster(ActorUID UID_): UID(UID_) {}
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

	eastl::fixed_vector<Player,10,false> players;
	eastl::fixed_list<ActorMaster,512,true> actorMasterList;
	eastl::fixed_list<ActorNpc,512,true> actorNpcList;

	typedef ListItT<ActorNpc> ActorNpcHandle;

	// TODO: make those fixed_hash_maps
	eastl::fixed_map<ActorUID, ActorMasterHandle, 2048, true> actorMasterMap;
	eastl::fixed_map<ActorUID, ActorNpcHandle, 2048, true> actorNpcMap;

	u32 nextActorUID;
	Time localTime;

	PhysWorld physics;

	void Init(Replication* replication_);
	void Update(Time localTime_);
	void Replicate();

	Player& CreatePlayer(i32 clientID, const wchar* name, const wchar* guildTag, ClassType mainClass, SkinIndex mainSkin, ClassType subClass, SkinIndex subSkin);
	ActorMaster& SpawnPlayerMasters(Player& player, const vec3& pos);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);

	Player* FindPlayer(PlayerID playerID);
	Player& GetPlayer(PlayerID playerID);
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();
};
