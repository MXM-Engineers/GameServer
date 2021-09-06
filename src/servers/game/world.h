#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>
#include "replication.h"
#include <mxm/core.h>
#include <mxm/physics.h>

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

			u8 tag: 1;
			u8 jump: 1;

			ActionStateID actionState;
			i32 actionParam1;
			i32 actionParam2;

			SkillID castSkill = SkillID::INVALID;
			vec3 castPos;
		};

		const UserID userID;
		const ClientHandle clientHd;
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
		PhysWorld::BodyHandle body;

		// book keeping
		struct {
			vec2 moveDir;
			RotationHumanoid rot;
			bool hasJumped = false;
		} movement;

		struct {
			SkillID skill;
			vec3 startPos;
			vec3 endPos;
			f32 moveDurationS;
		} cast;

		explicit Player(
				UserID playerID_,
				ClientHandle clientHd_,
				const WideString& name_,
				const WideString& guildTag_,
				ClassType mainClass_,
				SkinIndex mainSkin_,
				ClassType subClass_,
				SkinIndex subSkin_
				):
			userID(playerID_),
			clientHd(clientHd_),
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

	Player& CreatePlayer(ClientHandle clientHd, const wchar* name, const wchar* guildTag, ClassType mainClass, SkinIndex mainSkin, ClassType subClass, SkinIndex subSkin, const vec3& pos);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);

	Player* FindPlayer(UserID playerID);
	Player& GetPlayer(UserID playerID);
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();
};
