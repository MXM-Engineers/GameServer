#pragma once
#include <common/base.h>
#include <common/network.h>
#include <common/vector_math.h>
#include <mxm/core.h>
#include <mxm/game_content.h>

#include <EASTL/array.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_vector.h>

#include "replication.h"
#include "physics.h"
#include "world_master_rules.h"

struct StatusInfo;

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
	f32 clientTime = 0.f;
};

struct Remote;

struct World
{
	friend struct WorldRemoteSpawner;

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
		eastl::array<WeaponIndex,2> weapons;
		eastl::array<i32,2> weaponGrades;
		eastl::array<u8,2> masterGearNo;
		eastl::array<i32,2> characterType;
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

			PlayerInputCastSkill cast;
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

		const eastl::array<ColliderSize,2> colliderSize;
		const eastl::array<SkillID,4> skills;
		const WeaponIndex mainWeapon;
		const WeaponIndex subWeapon;
		const i32 mainWeaponGrade;
		const i32 subWeaponGrade;
		const u8 mainMasterGearNo;
		const u8 subMasterGearNo;
		const i32 mainCharacterType;
		const i32 subCharacterType;

		u8 level;
		u32 experience;

		eastl::array<ActorMasterHandle, PLAYER_CHARACTER_COUNT> characters;
		u8 mainCharaID = 0;

		Input input;
		PhysicsDynamicBody* body = nullptr;

		bool weaponFired = false;
		u8 firingCombo = 0;
		u8 chargeLevel = 0;
		i32 weaponState = 0;
		u32 resourceGainCast = 0;
		eastl::fixed_vector<u32,10,false> resourceGainTargets;
		eastl::array<i32,2> shotCount = { 0, 0 };
		bool passivesApplied = false;
		i32 damageTaken = 0;
		i32 hitsTaken = 0;
		i32 statusesTaken = 0;
		bool untargetable = false;
		i32 carriedBy = -1;
		u8 carrierCharaID = 0;
		Time nextCarrySync = Time::ZERO;
		struct Casting {
			SkillID skillID = SkillID::INVALID;
			vec3 castPos;
			ActorUID target = ActorUID::INVALID;
			vec2 startPos;
			Time executeAt = Time::ZERO;
			u8 type = 0;
			bool interruptByMove = false;
		} casting;
		SkillID toggleSkill = SkillID::INVALID;
		u32 toggleCastID = 0;
		Time toggleNext = Time::ZERO;
		i32 pendingChildSkill = 0;
		Time pendingChildSkillUntil = Time::ZERO;
		vec3 pendingChildPos = vec3(0);
		ActorUID pendingChildTarget = ActorUID::INVALID;
		eastl::array<Time,5> botSkillReady = { Time::ZERO, Time::ZERO, Time::ZERO, Time::ZERO, Time::ZERO };
		Time tNextBotSkill = Time::ZERO;
		f32 ultGauge = -1.0f;
		f32 stamina = -1.0f;
		f32 mana = -1.0f;
		Time tNextBotShot = Time::ZERO;
		f32 moveSpeedSent = -1.0f;
		Time tNextManaSend = Time::ZERO;
		eastl::array<f32,3> res = { -1, -1, -1 };
		f32 benchMana = -1.0f;
		eastl::array<f32,3> benchRes = { -1, -1, -1 };
		eastl::array<f32,3> resSent = { -1, -1, -1 };
		Time tNextResSend = Time::ZERO;
		Time tNextResRefresh = Time::ZERO;
		eastl::array<f32,2> heat = { 0, 0 };
		eastl::array<Time,2> heatLastShot = { Time::ZERO, Time::ZERO };
		eastl::array<Time,2> overheatUntil = { Time::ZERO, Time::ZERO };
		i32 heatSent = -1;
		Time tNextHeatSend = Time::ZERO;
		Time tNextStaminaSend = Time::ZERO;
		f32 ultGaugeSent = -1.0f;
		Time tNextUltSend = Time::ZERO;
		f32 shotLeadSec = 0;
		f32 weaponAimYaw = 0;
		vec3 lastClientPos = vec3(0);
		ActionStateID lastClientAction = ActionStateID::INVALID;
		i32 lastHitTargetIndex = -1;
		Time lastHitAt = Time::ZERO;
		Time lastClientActionSince = Time::ZERO;
		i32 comboSlot = -1;
		SkillID comboPending = SkillID::INVALID;
		SkillID comboSkill = SkillID::INVALID;
		Time comboSendAt = Time::ZERO;
		i32 comboResetCooltime = 0;
		SkillID comboBase = SkillID::INVALID;
		vec3 comboCastPos = vec3(0);
		Time comboUntil = Time::ZERO;
		Time comboAutoAt = Time::ZERO;
		vec3 placedAt = vec3(0);
		i32 clientPosCount = 0;
		i32 kills = 0;
		eastl::array<f32,2> weaponMaxShotDistance = { 0, 0 };
		i32 deaths = 0;
		Time tNextWeaponHit = Time::ZERO;
		bool dead = false;
		Time ccUntil = Time::ZERO;
		vec3 pushTarget = vec3(0);
		f32 pushSpeed = 0;
		Time pushUntil = Time::ZERO;
		vec2 dragCenter = vec2(0);
		f32 dragMin = 0;
		f32 dragMax = 0;
		f32 dragDuration = 0;
		f32 dragElapsed = 0;
		Time fearNextTurn = Time::ZERO;
		Time dashUntil = Time::ZERO;
		vec2 dashDir = vec2(1, 0);
		f32 dashSpeed = 0;
		Time dashStartAt = Time::ZERO;
		vec3 dashOrigin = vec3(0);
		Time clientPushUntil = Time::ZERO;
		f32 dashDist = 0;
		f32 dashDone = 0;
		const GameXmlContent::MoveCurve* dashCurve = nullptr;
		Time ccImmuneUntil = Time::ZERO;
		i32 cleanseCharges = -1;
		Time cleanseRefillAt = Time::ZERO;
		Time reviveReadyAt = Time::ZERO;
		Time reviveAt = Time::ZERO;
		f32 reviveRatio = 0;
		i32 weaponHitCounter = 0;
		vec2 fearDir = vec2(1, 0);
		Time tRespawn = Time::ZERO;
		ActorUID memorizedTarget = ActorUID::INVALID;
		ActorUID lastChildUID = ActorUID::INVALID;
		bool pendingShot = false;
		vec3 pendingShotOrigin = vec3(0);
		vec2 pendingShotDir = vec2(0);
		f32 pendingShotDistance = 0;
		eastl::array<u8,256> pendingShotRaw;
		i32 pendingShotRawSize = 0;
		Time pendingShotTime = Time::ZERO;
		bool pendingAim = false;
		f32 pendingAimYaw = 0;
		Time pendingAimTime = Time::ZERO;
		eastl::array<vec3,6> posHistory;
		u32 posHistoryCount = 0;
		u32 posHistoryHead = 0;
		SkillID lastComboSkill = SkillID::INVALID;
		Time lastComboTime = Time::ZERO;
		vec3 spawnPos = vec3(0);

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
			colliderSize(desc.colliderSize),
			skills(desc.skills),
			mainWeapon(desc.weapons[0]),
			subWeapon(desc.weapons[1]),
			mainWeaponGrade(desc.weaponGrades[0]),
			subWeaponGrade(desc.weaponGrades[1]),
			mainMasterGearNo(desc.masterGearNo[0]),
			subMasterGearNo(desc.masterGearNo[1]),
			mainCharacterType(desc.characterType[0]),
			subCharacterType(desc.characterType[1])
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
		i32 hp;
		i32 hpMax;
		i32 seed;
		i32 spawnAnim = -1;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;




		ActionStateID actionState;
		i32 actionParam1;
		i32 actionParam2;
		i8 stance = 0;
		Time stanceUntil = Time::ZERO;
		i32 stanceShotsLeft = -1;
		bool stanceCancelByDefeat = false;

		explicit ActorMaster(ActorUID UID_): UID(UID_) {}
	};

	struct ActorNpc
	{
		const ActorUID UID;
		CreatureIndex docID;
		i32 localID;
		Faction faction;
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		ActionStateID actionState = ActionStateID::INVALID;
		i32 seed = 0;
		i32 entityType = 1;
		ActorUID ownerUID = ActorUID::INVALID;
		Time despawnAt = Time::ZERO;
		u32 ownerIndex = 0;
		u8 ai = 0;
		ActorUID chaseTarget = ActorUID::INVALID;
		ActorUID skillTarget = ActorUID::INVALID;
		bool stuck = false;
		u8 dieMode = 0;
		Time nextMoveSync = Time::ZERO;
		ActionStateID pendingMotion = ActionStateID::INVALID;
		vec3 aimPos = vec3(0);
		Time motionAt = Time::ZERO;
		vec3 motionTarget = vec3(0);
		ActorUID motionTargetUID = ActorUID::INVALID;
		f32 motionYaw = 0;
		i32 relayMode = -1;
		i32 queuedState = -1;
		Time queuedStateAt = Time::ZERO;
		vec3 queuedGoal = vec3(0);
		void* wall = nullptr;
		i32 ownerClass = -1;
		Time nextPassive = Time::ZERO;
		Time passiveBlockedUntil = Time::ZERO;

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
		i32 spawnAnim = 0;
		i32 ownerID = 0;
		u8 dirToNearPC = 0;
		i32 wanderDist = -1;
		i32 tagID = -1;
		i32 seed = 0;
		i32 entityType = 3;

		Time tLastActionChange;
		vec3 pos;
		vec3 rot;

		explicit ActorDynamic(ActorUID UID_): UID(UID_) {}
	};

	struct SkillAnimPass { i32 index; f32 start; };
	struct SkillProgram
	{
		SkillID skillID = SkillID::INVALID;
		ActionStateID actionID;
		i8 stance = 0;
		vec3 castPos;
		f32 castAngle;
		ActorUID casterUID;
		eastl::fixed_vector<ActorUID,10,false> targetList;
		Time startTime;
		i32 commandID = 0;
		u64 done = 0;
		eastl::fixed_vector<SkillAnimPass,8,false> passes;

		inline bool IsDoneExecuting() const { return skillID == SkillID::INVALID; }
		inline void Finish() { skillID = SkillID::INVALID; }
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

	eastl::fixed_vector<SkillProgram,40,false> skillProgramList;

	struct Projectile
	{
		u32 ownerIndex;
		vec3 pos;
		vec2 dir;
		f32 travelled;
		f32 range;
		f32 aoeRadius;
		i32 damage;
		f32 speed;
		f32 halfWidth;
		u32 remoteID = 0;
		f32 pendingAdvance = 0;
		f32 forwardReach = 70.0f;
		RemoteIdx remoteDoc = RemoteIdx::INVALID;
		f32 closest = 999999.0f;
		vec3 start;
		i32 hitsLeft;
		bool graph;
		bool ignoreWalls = true;
		f32 wallZ = 0;
		f32 angularVel = 0;
		f32 homingLength = 0;
		f32 homingHalfCos = -1.0f;
		f32 homingTime = 0;
		f32 homingTimer = 0;
		bool homingLockOn = false;
		i32 homingTarget = -1;
		eastl::fixed_vector<u32,16,false> hitList;
	};

	eastl::fixed_vector<Projectile,256,false> projectileList;

	struct PendingRemote
	{
		bool remove = false;
		ActorUID sourceUID = ActorUID::INVALID;
		const GameXmlContent::CostLevel* costLevel = nullptr;
		Time time;
		RemoteIdx idx;
		i32 preset;
		f32 offsetSide;
		f32 offsetFront;
		u32 ownerIndex;
		SkillID skillID;
		u32 castID;
		i32 damage;
		vec3 castPos;
		f32 angle;
		ActorUID target;
		bool atTarget = false;
		bool stopAtTarget = false;
		u16 nodeName = 0;
	};

	struct RemoteInst
	{
		ActorUID sourceUID = ActorUID::INVALID;
		const GameXmlContent::CostLevel* costLevel = nullptr;
		f32 followDist = -1.0f;
		u32 instID = 0;
		bool killNow = false;
		const Remote* remote;
		u32 ownerIndex;
		SkillID skillID;
		u32 castID;
		i32 damage;
		vec3 pos;
		vec2 dir;
		f32 travelled;
		f32 range;
		f32 speed;
		Time spawnTime;
		ActorUID target;
		i32 hitsLeft;
		bool areaDone;
		u32 clientRemoteID = 0;
		bool destroyedByHit = false;
		Time nextTimeEvent = Time::ZERO;
		bool timeEventActive = false;
		i32 excludeIndex = -1;
		i32 lastHitIndex = -1;
		bool chainEnded = false;
		bool casterLocal = false;
		f32 landZ = 0;
		vec3 graphStart = vec3(0);
		bool stopAtFirstHit = false;
		vec2 graphDir = vec2(1, 0);
		bool hasLandZ = false;
		bool centeredBox = false;
		f32 wallZ = 0;
		bool actionChanged = false;
	};

	struct CastHit
	{
		u32 instID = 0;
		RemoteIdx remoteID;
		u32 castID;
		u32 playerIndex;
		Time until;
	};

	u32 nextRemoteInstID = 1;
	eastl::fixed_vector<PendingRemote,256,false> pendingRemotes;
	eastl::fixed_vector<RemoteInst,256,false> remoteInsts;
	eastl::fixed_vector<CastHit,512,false> castHits;
	struct RepeatHit { u32 castID; u32 targetIndex; i32 count; Time at; };
	eastl::fixed_vector<RepeatHit,64,true> repeatHits;

	struct ActiveStatus
	{
		i32 statusID;
		const StatusInfo* info;
		u32 targetIndex;
		u32 casterIndex;
		ActorUID targetUID;
		ActorUID casterUID;
		Time until;
		Time nextTick;
		i32 overlap;
		f32 shieldHP;
		Time nextHealTick;
		i32 healTicksDone;
		Time nextAuraPulse = Time::ZERO;
		Time appliedAt = Time::ZERO;
		f32 absorbed = 0;
		bool removeOnHit = false;
		i32 custom = 0;
		i32 damageAtStart = 0;
	};
	eastl::fixed_vector<ActiveStatus,512,false> activeStatuses;

	struct PendingStatus
	{
		Time time;
		i32 statusID;
		u32 casterIndex;
		ActorUID targetUID;
		f32 duration;
		f32 power;
		bool remove = false;
	};
	eastl::fixed_vector<PendingStatus,256,false> pendingStatuses;

	struct PendingSummon
	{
		Time time;
		i32 creature;
		u32 ownerIndex;
		vec3 pos;
		vec3 rot;
		ActorUID target = ActorUID::INVALID;
	};
	eastl::fixed_vector<PendingSummon,64,false> pendingSummons;

	struct PendingSummonCommand
	{
		Time time;
		u32 ownerIndex;
		i32 kind;
		i32 index;
		vec3 pos;
		f32 yaw;
		i32 skill;
		ActorUID target = ActorUID::INVALID;
		ActorUID source = ActorUID::INVALID;
		vec3 aim = vec3(0);
		Time until;
	};
	eastl::fixed_vector<PendingSummonCommand,128,false> pendingSummonCommands;

	struct GroundAura
	{
		u32 ownerIndex;
		i32 auraStatus;
		f32 range;
		vec3 pos;
		Time until;
		Time nextPulse;
	};
	eastl::fixed_vector<GroundAura,64,false> groundAuras;
	i32 nextSummonLocalID = 900000;
	u32 nextCastID = 1;
	u32 nextRemoteSeed = 0x40000000;
	eastl::fixed_vector<u32,64,false> comboTriggeredCasts;

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
	u32 PlayerCount() const { return (u32)players.size(); }
	bool PlayerPaySkillCost(Player& player, i32 skillID);
	void UpdateWeaponHeat(Player& player);
	void UpdateSummons();
	void UpdateBotCombat();
	void UpdateBotSkills();
	void UpdateSummonAi(f32 delta);
	bool applyingCasterStatus = false;
	void TeleportPlayer(Player& p, const vec3& pos, f32 yaw);
	void MovePlayer(Player& p, const vec3& pos, f32 yaw, bool snap);
	void RunSummonState(ActorNpc& npc, i32 state);
	void RelayPassive(ActorNpc& npc, const SummonInfo& info, Player& owner);
	void RelayWarp(ActorNpc& npc, const vec3& to);
	void ChildSkillOnExisting(Player& owner, i32 child, const vec3& pos);
	ActorUID statusCasterOverride = ActorUID::INVALID;
	bool statusFromBehind = false;
	bool statusForceCenterSet = false;
	vec3 statusForceCenter = vec3(0);
	const GameXmlContent::CostLevel* castCostLevel = nullptr;
	bool reflecting = false;
	bool frozen = false;
	ActorMaster* FindMasterActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

	void PlayerWeaponShot(Player& player, const vec3& origin, const vec2& dir, f32 distance);
	void SpawnPendingRemotes();
	void MoveCasterTargetRemote(RemoteInst& inst, Player& instOwner, f32 delta);
	void RunRemoteTimeEvent(RemoteInst& inst, f32 elapsed);
	bool UpdateChainRemote(RemoteInst& inst);
	void ApplyAttachedRemote(RemoteInst& inst, Player& instOwner);
	bool UpdateTargetRemote(RemoteInst& inst, f32 delta);
	bool UpdateGraphRemote(RemoteInst& inst, f32 delta);
	bool UpdateLinearRemote(RemoteInst& inst, f32 delta, f32 elapsed);
	void DestroyRemote(i32 index);
	void ExpireHitRecords();
	void GainResource(Player& player, i32 slot, f32 amount);
	void ApplyResourceGain(Player& player, ResourceEvent event);
	void ApplyRemoteHitResourceGain(Player& owner, RemoteIdx remote, u32 castID, u32 targetIndex);
	void GroupTeleport(Player& caster, SkillID skillID, const vec3& from);
	struct PendingInteraction { Time time; u32 ownerIndex; i32 interactionID; };
	struct OpenInteraction { ActorUID objectUID; u32 ownerIndex; i32 interactionID; Time until; Time announceAt; };
	eastl::fixed_vector<PendingInteraction,16,false> pendingInteractions;
	eastl::fixed_vector<OpenInteraction,16,false> openInteractions;
	i32 nextInteractionLocal = 0;
	void OpenInteractionObject(Player& owner, i32 interactionID);
	ActorUID InteractionObjectOf(const Player& owner) const;
	void RemoveDynamic(ActorUID uid);
	void PlayerInteract(Player& player, ActorUID targetUID);
	void BoardCarrier(Player& passenger, Player& carrier, i32 interactionID);
	void LeaveCarrier(Player& passenger);
	void UpdateInteractions();
	const ActiveStatus* FindCarrierPassive(const Player& carrier) const;
	const ActiveStatus* FindGroupTeleportPassive(const Player& caster, SkillID skillID) const;
	bool HasStatusFromSkill(const Player& target, u32 casterIndex, i32 skillID) const;
	void OnWeaponShotPacket(Player& player, const vec3& origin, const vec2& dir, f32 distance, const u8* rawPacket, i32 rawSize);
	void OnWeaponAimPacket(Player& player, f32 aimYaw);

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();

	void PlayerCastSkill(Player& player, SkillID skill, const vec3& castPos, Slice<const ActorUID> targets, f32 clientTime, ActionStateID forcedAction = ActionStateID::INVALID);
	SkillID ResolveComboRecast(Player& player, SkillID skillIn, ActionStateID forcedAction);
	vec3 ResolveCastPos(const Player& player, SkillID skillID, const vec3& castPosIn, ActionStateID forcedAction) const;
	bool SkillTargetsAllowed(const Player& player, SkillID skillID, Slice<const ActorUID> targets) const;
	const GameXmlContent::CostLevel* PickCostLevel(const Player& player, SkillID skillID) const;
	bool CheckAndPayCast(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets);
	void TargetPositionMove(const Player& player, const GameXmlContent::Action& action, const vec3& castPos, f32* distance, vec2* gotoDir) const;
	void StartCasting(Player& player, SkillID skillID, const GameXmlContent::SkillCast& cast, const vec3& castPos, Slice<const ActorUID> targets, const GameXmlContent::Action& action);
	void UpdateComboSlot(Player& player, SkillID skillID, SkillType skillType, const vec3& castPos);
	void QueueChildSkill(Player& player, ActionStateID actionState, const vec3& castPos, Slice<const ActorUID> targets);
	void ExecuteSkillProgram(SkillProgram& prog);
	void RunSkillProgramCommand(SkillProgram& prog, ActorMaster* caster, i32 cmdIndex, const vec2& dir);
	eastl::fixed_vector<SkillAnimPass,8,false> lastAnimPasses;

	void PlayerWeaponFire(Player& player);
	void UpdateProjectiles(f32 delta);
	void SteerProjectile(Projectile& proj, f32 delta);
	void Explode(Player& owner, const vec3& center, f32 radius, i32 damage);
	void RespawnPlayer(Player& player);
	bool ScheduleSkillRemotes(Player& player, SkillID skillID, const vec3& castPos, Slice<const ActorUID> targets, f32 angle, i32 forceAnim = -1, ActionStateID forcedAction = ActionStateID::INVALID);
	RemoteInst* SpawnRemote(u32 ownerIndex, RemoteIdx idx, const vec3& pos, const vec2& dir, ActorUID target, SkillID skillID, u32 castID, i32 damage, bool broadcast = false, const vec3* aimPos = nullptr, u16 nodeName = 0, bool casterCreatesLocally = false, bool stopAtTarget = false);
	void UpdateRemotes(f32 delta);
	void UpdateUltimateGauges(f32 delta);
	void ApplyStatus(Player& caster, Player& target, i32 statusID, f32 power = 0, f32 durationOverride = -1);
	ActiveStatus* FindActiveStatus(u32 targetIndex, i32 statusID);
	void ConsumeStatus(u32 targetIndex, i32 statusID);
public:
	void ApplyPassives(Player& player);
	void ApplyAllPassives();
	void ReleaseCast(Player& player);
	void ComboBaseCooldown(Player& player);
	void ToggleOn(Player& player, SkillID skillID, const vec3& castPos);
	void ToggleOff(Player& player);
	void UpdateToggles();
	void SpawnCreatureFromRemote(const RemoteInst& parent, i32 creature, const vec3& pos);
	f32 WeaponHitDamage(const Player& player, const Remote* remote) const;
	void PulseAura(u32 casterIndex, const StatusInfo& aura, const vec2& center, f32 range, i32 skipDebuffIndex);
private:
	void PassiveOnSkillCast(Player& player, i32 skillID);
	void PassiveOnWeaponHit(Player& attacker, Player& target, const Projectile& proj);
	void PassiveOnSkillHit(Player& attacker, Player& target);
	void ApplyPassiveStatusRule(const PassiveStatusRule& rule, Player& owner, Player* hitTarget);
	f32 PassiveWeaponDamageMul(Player& attacker, Player& target);
	const ActiveStatus* FindPassiveOfType(const Player& p, StatusType type) const;
	bool PassiveBlocksCC(Player& target, const StatusInfo& info, f32* duration);
	void PassiveOnDamaged(Player& target);
	bool PassiveOnLethal(Player& target);
	void UpdatePassives();
	void SetStance(ActorMaster& master, i8 stance, f32 duration, i32 weaponFireCount, bool cancelByDefeat);
	void StanceWeaponUse(Player& player);
	void UpdateStances();
	void CancelCast(Player& player, const char* reason);
	void UpdateCasts();
	bool HasStatusType(const Player& p, StatusType type) const;
	f32 StatusModSum(const Player& p, i32 stat) const;
	f32 StatusModMulSum(const Player& p, i32 stat) const;
	bool IsBlocked(const Player& p, bool attack, bool skill, bool shirk) const;
	void HealPlayer(Player& healer, Player& target, i32 amount);
	bool RemoteCanAffect(const RemoteInst& inst, const Player& p, bool includeOwner) const;
	f32 HitRadius(const Player& p) const;
	f32 BodyMidHeight(const Player& p) const;
	bool WeaponIsMelee(const Player& p) const;
	const WeaponFiring& ActiveWeaponFiring(const Player& p, i8 stance) const;
	RemoteIdx SelectWeaponRemote(Player& player, const WeaponFiring& wf);
	f32 WeaponRange(const Player& p) const;
	void RemoteApplyShape(RemoteInst& inst);
	void RemoteHitAlongPath(RemoteInst& inst);
	void UpdateStatuses();
	void RemoteApplyArea(RemoteInst& inst);
	f32 LagDistToSegment(const Player& p, const vec2& a, const vec2& b) const;
	bool LagSegmentHits(const Player& p, const vec2& a, const vec2& b, f32 lateral, f32 forwardReach) const;
	f32 LagDistToPoint(const Player& p, const vec2& point) const;
	vec2 LagClosestPos(const Player& p, const vec2& point) const;
	void RemoteHit(RemoteInst& inst, Player& target);
	void DamagePlayer(Player& attacker, Player& target, SkillID skillID, i32 damage, const Projectile* proj = nullptr, u32 remoteID = 0, RemoteIdx remoteDoc = RemoteIdx::INVALID, vec2 remoteDir = vec2(1, 0));
};
