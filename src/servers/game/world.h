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
#include "input_clock.h"

struct HorizontalMotionVariant;
struct DynamicCollisionProfile;

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
		eastl::array<WeaponIndex,2> weapons;
		eastl::array<i32,2> weaponGrades;
		eastl::array<u8,2> masterGearNo;
		eastl::array<i32,2> characterType;
	};

	struct Player
	{
		enum class JumpPhase: u8
		{
			None,
			Requested,
			Entered,
		};

		struct Input
		{
			vec3 moveTo;
			vec2 moveDir = vec2(0);
			f32 speed;
			f32 clientTime = 0.f;
			RotationHumanoid rot;

			u8 tag: 1;
			u8 jump: 1;
			vec2 jumpMoveDir = vec2(0);
			f32 jumpRotate = 0.0f;

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

		struct {
			vec2 moveDir = vec2(0);
			f32 moveSpeed = 0;
			RotationHumanoid rot;
			bool hasJumped = false;
			bool forcedMove = false;
			bool walkingBlocked = false;
			JumpPhase jumpPhase = JumpPhase::None;
			const JumpMotion* pendingJump = nullptr;
			f32 jumpRotate = 0.f;
			u32 jumpUpdate = 0;
		} movement;
		bool graphIgnoreCollision = false;
		bool graphIgnoreClearAfterStep = false;

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
		vec3 pos = vec3(0);
		vec3 rot = vec3(0);
		const DynamicCollisionProfile* collisionProfile = nullptr;
		eastl::fixed_vector<NxActor*,4,false> collisionActors;
		ActionStateID collisionAction = ActionStateID::INVALID;
		u32 collisionActionStart = 0;
		bool collisionActionDone = false;

		explicit ActorDynamic(ActorUID UID_): UID(UID_) {}
	};

	struct SkillProgram
	{
		SkillID skillID = SkillID::INVALID;
		ActionStateID actionID;
		vec3 castPos;
		f32 castAngle;
		ActorUID casterUID;
		eastl::fixed_vector<ActorUID,10,false> targetList;
		u16 commandMask = 0;
		u8 commandCount = 0;
		u8 clipIndex = 0;
		u8 clipStartedMask = 0;
		f32 clipElapsed = 0.0f;
		f32 clipClock = 0.0f;
		eastl::array<f32,8> clipStartedAt{};
		u32 clipUpdate = 0;
		vec3 moveStartPos = vec3(0);
		vec3 moveEndPos = vec3(0);
		f32 moveDuration = 0;
		const HorizontalMotionVariant* horizontalVariant = nullptr;
		f32 moveSampled = 0.0f;
		vec2 moveHorizDir = vec2(0);
		f32 completeAt = 0.0f;
		bool graphDone = false;
		bool graphActive = false;
		bool ownsWalking = false;
		i32 stateTimerRemainingMs = -1;
		u32 stateTimerUpdate = 0;
		u32 simTicks = 0;
		u32 bornUpdate = 0;
		f32 graphElapsed = 0.f;
		u32 graphApplyUpdate = 0;
		u32 inputSequence = 0;


		inline bool IsDoneExecuting() const { return skillID == SkillID::INVALID; }
		inline void Finish() { skillID = SkillID::INVALID; }
	};

	enum class InputKind: u8
	{
		Movement,
		Rotation,
		Action,
		Jump,
		Tag,
		Cast,
	};

	struct InputEvent
	{
		u32 sequence = 0;
		InputKind kind = InputKind::Movement;
		ActorUID actorUID = ActorUID::INVALID;
		Player::Input data{};
		f64 serverTime = 0.0;
		f64 receivedTime = 0.0;
		f32 clientTime = 0.0f;
		u32 clockGeneration = 0;
		u8 masterSlot = 0;
		bool timestamped = false;
		u32 notifications = 0;
		u32 commandNotifications = 0;
		f64 keepUntil = 0.0;
		const HorizontalMotionVariant* selectedVariant = nullptr;
	};

	static constexpr u32 InputHistoryFrames = 512;
	static constexpr u32 InputHistoryEvents = InputHistoryFrames * 4;

	struct PlayerFrame
	{
		u32 update = 0;
		Time localTime = Time::ZERO;
		f64 serverTime = 0.0;
		f64 previousServerTime = 0.0;
		PhysicsBodyState body;
		Player::Input input{};
		decltype(Player::movement) movement{};
		u8 mainCharaID = 0;
		ActionStateID actions[PLAYER_CHARACTER_COUNT]{};
		bool graphIgnoreCollision = false;
		bool graphIgnoreClearAfterStep = false;
		eastl::fixed_vector<SkillProgram,2,false> programs;
	};

	struct PlayerTimeline
	{
		eastl::array<InputClock,PLAYER_CHARACTER_COUNT> clocks;
		eastl::array<u32,PLAYER_CHARACTER_COUNT> clockGeneration{};
		eastl::array<f32,PLAYER_CHARACTER_COUNT> lastClientTime{};
		eastl::array<f32,PLAYER_CHARACTER_COUNT> minimumClientTime{};
		eastl::array<f64,PLAYER_CHARACTER_COUNT> clockStartTime{};
		eastl::array<f64,PLAYER_CHARACTER_COUNT> clockEndTime{ -1.0, -1.0 };
		eastl::fixed_vector<InputEvent,InputHistoryEvents,false> events;
		eastl::fixed_vector<PlayerFrame,InputHistoryFrames,false> frames;
		u32 oldestFrame = 0;
		u32 nextSequence = 1;
		u8 admittedMaster = 0;
		f64 replayFrom = -1.0;
		f64 lastStepTime = 0.0;
		bool enabled = false;
	};

	struct SceneActorState
	{
		NxActor* actor = nullptr;
		u32 flags = 0;
		bool raycastDisabled = false;
	};

	struct SceneFrame
	{
		u32 update = 0;
		eastl::fixed_vector<SceneActorState,128,false> actors;
	};

	struct SimulationHistory
	{
		eastl::array<PlayerTimeline,10> players;
		eastl::fixed_vector<SceneFrame,InputHistoryFrames,false> scenes;
		u32 oldestScene = 0;
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
	SimulationHistory* history = nullptr;
	Player* simulationPlayer = nullptr;
	u32 simulationUpdate = 0;
	f32 simulationStepDuration = f32(UPDATE_RATE);
	InputEvent* simulationEvent = nullptr;
	bool replayingInputs = false;

	u32 nextActorUID;
	Time localTime = Time::ZERO;
	Time physicsEpoch = Time::ZERO;
	bool physicsEpochSet = false;
	u32 updateIndex = 0;

	PhysicsScene physics;

	void Init(Replication* replication_);
	void Cleanup();

	void Update(Time localTime_, f64 serverTime);
	void Replicate();
	void BeginPlayerTimeline(ClientHandle clientHd, ActorUID actorUID, f64 serverTime);
	bool QueuePlayerInput(Player& player, ActorUID actorUID, InputKind kind, const Player::Input& input, f64 receivedTime, bool timestamped);
	bool EmitInputNotification(Player& player, u32 sequence, u32 notification, i32 commandIndex = -1);
	u32 SimulationUpdate() const { return simulationPlayer ? simulationUpdate : updateIndex; }
	f32 SimulationStepDuration() const { return simulationPlayer ? simulationStepDuration : physics.StepDuration(); }
	u8 AdmittedMaster(const Player& player) const {
		const auto& timeline = history->players[player.index];
		return timeline.enabled ? timeline.admittedMaster : player.mainCharaID;
	}

	Player& CreatePlayer(const PlayerDescription& desc, const vec3& pos, const RotationHumanoid& rot);
	ActorNpc& SpawnNpcActor(CreatureIndex docID, i32 localID);
	ActorDynamic& SpawnDynamic(CreatureIndex docID, i32 localID);

	Player& GetPlayer(u32 playerIndex);
	ActorMaster* FindMasterActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActor(ActorUID actorUID) const;
	ActorNpc* FindNpcActorByCreatureID(CreatureIndex docID); // Warning: slow!

private:
	ActorUID NewActorUID();
	ActorMasterHandle MasterInvalidHandle();

	void CancelPlayerPrograms(Player& player);
	void PlayerCastSkill(Player& player, SkillID skill, const vec3& castPos, Slice<const ActorUID> targets, f32 clientTime);
	void ExecuteSkillProgram(SkillProgram& prog);
	void SavePlayerFrame(Player& player, PlayerFrame& frame, u32 update, Time frameTime, f64 serverTime);
	void RestorePlayerFrame(Player& player, const PlayerFrame& frame);
	void ApplyPlayerAction(Player& player, ActionStateID observedAction);
	void SimulatePlayer(Player& player, u32 update, Time frameTime, f64 serverTime);
	void UpdatePlayerTimeline(Player& player, f64 serverTime);
	void ApplyPlayerControls(Player& player, InputEvent* event);
	void ApplyPlayerInput(Player& player, InputEvent& event);
	void CaptureSceneFrame(SceneFrame& frame);
	void RestoreSceneFrame(const SceneFrame& frame);
};
