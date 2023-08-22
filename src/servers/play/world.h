#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <mxm/core.h>

#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>

#include "replication.h"
#include "physics.h"

struct ColliderSize
{
	u16 radius;
	u16 height;
};

struct PlayerInputCastSkill
{
	SkillID skillID = SkillID::INVALID;
	vec3 pos;
	eastl::fixed_vector<ActorUID,10,false> targetList;
};

struct World
{
	struct Player;
	struct ActorMaster;

	typedef ListItT<ActorMaster> ActorMasterHandle;

	struct PlayerDescription
	{
		UserID userID;
		ClientHandle clientHd;
		WideString name;
		WideString guildTag;
		u8 team;

		eastl::array<ClassType,2> masters;
		eastl::array<SkinIndex,2> skins;
		eastl::array<ColliderSize,2> colliderSize;
		eastl::array<SkillID,4> skills;
	};

	struct Player
	{
		struct Input
		{
			vec3 moveTo;
			f32 speed;
			RotationHumanoid rot;

			u8 tag: 1;
			u8 jump: 1;

			ActionStateID action;
			i32 actionParam1; // TODO: investigate these
			i32 actionParam2;

			eastl::fixed_vector<PlayerInputCastSkill,4,false> cast;
		};

		const u32 index;
		const UserID userID;
		const ClientHandle clientHd;
		const WideString name;
		const WideString guildTag;
		const u8 team;

		const ClassType mainClass;
		const SkinIndex mainSkin;
		const ClassType subClass;
		const SkinIndex subSkin;

		f32 blockDuration = 0;

		const eastl::array<ColliderSize,2> colliderSize; // used when checking for gameplay collisions, not movement

		u8 level;
		u32 experience;

		eastl::array<ActorMasterHandle, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID = 0;

		Input input;
		PhysicsDynamicBody* body = nullptr;

		// book keeping
		struct {
			vec2 moveDir = vec2(0);
			f32 moveSpeed = 0;
			RotationHumanoid rot;
			bool hasJumped = false;
		} movement;

		explicit Player(u32 index_, const PlayerDescription& desc):
			index(index_),
			userID(desc.userID),
			clientHd(desc.clientHd),
			name(desc.name),
			guildTag(desc.guildTag),
			team(desc.team),
			mainClass(desc.masters[0]),
			mainSkin(desc.skins[0]),
			subClass(desc.masters[1]),
			subSkin(desc.skins[1]),
			colliderSize(desc.colliderSize)
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
		Faction faction;
		vec3 pos;
		vec3 rot;

		explicit ActorNpc(ActorUID UID_): UID(UID_) {}
	};

	struct ActorDynamic
	{
		const ActorUID UID;
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		ActionStateID action;
		Time tLastActionChange;
		vec3 pos;
		vec3 rot;

		explicit ActorDynamic(ActorUID UID_): UID(UID_) {}
	};


	Replication* replication;

	eastl::fixed_vector<Player,10,false> players;
	eastl::fixed_list<ActorMaster,512,true> actorMasterList;
	eastl::fixed_list<ActorNpc,512,true> actorNpcList;
	eastl::fixed_list<ActorDynamic,512,true> actorDynamicList;

	typedef ListItT<ActorNpc> ActorNpcHandle;
	typedef ListItT<ActorDynamic> ActorDynamicHandle;

	// TODO: make those fixed_hash_maps
	eastl::fixed_map<ActorUID, ActorMasterHandle, 2048, true> actorMasterMap;
	eastl::fixed_map<ActorUID, ActorNpcHandle, 2048, true> actorNpcMap;
	eastl::fixed_map<ActorUID, ActorDynamicHandle, 2048, true> actorDynamicMap;

	u32 nextActorUID;
	Time localTime = Time::ZERO;

	PhysicsScene physics;

	void Init(Replication* replication_);
	void Cleanup();

	void Update(Time localTime_);
	void Replicate();

	Player& CreatePlayer(const PlayerDescription& desc, const vec3& pos, const RotationHumanoid& rot);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);
	ActorDynamic& SpawnDynamic(CreatureIndex docID, i32 localID);

	Player& GetPlayer(u32 playerIndex);
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();

	void PlayerCastSkill(Player& player, SkillID skill, const vec3& castPos, Slice<const ActorUID> targets);
};
