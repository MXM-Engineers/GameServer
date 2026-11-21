#pragma once
#include <common/base.h>
#include <common/vector_math.h>
#include <mxm/core.h>

#include <EASTL/fixed_vector.h>

struct ShowcaseState
{
	bool started = false;
	bool done = false;
	Time next = Time::ZERO;
	i32 casterSlot = 0;
	i32 dummySlot = -1;
	i32 masterPass = 0;
	i32 step = 0;
	i32 rangeIdx = 0;
	i32 testCount = 1;
	i32 lastDummyHp = 0;
	i32 lastDummyHits = 0;
	i32 lastDummyStatuses = 0;
	bool pendingResult = false;
	i32 prevExpect = -1;
	u32 prevDummy = 0;
	u32 prevCaster = 0;
	i32 phase = 0;
	vec3 mid = vec3(0);
	vec2 dir = vec2(1, 0);
	i32 probeDir = 0;
	f32 groundZ = 0;
	eastl::fixed_vector<vec2,24,false> candidates;
	i32 candIdx = 0;
	f32 bestLen = 0;
	vec2 bestDir = vec2(1, 0);
	vec2 bestCenter = vec2(0);
	f32 bestZ = 0;
	i32 comboSkill = 0;
	bool settled = false;
	i32 mirrorTurn = 0;
	bool castSeen = false;
	i32 mastersDone = 0;
	vec2 casterStart = vec2(0);
	vec2 dummyStart = vec2(0);
	Time castAt = Time::ZERO;
	i64 firstHitMs = -1;
	i32 firstHitDamage = 0;
	eastl::fixed_vector<i32,12,false> statusesSeen;
	f32 casterMana0 = 0;
	f32 casterStamina0 = 0;
	Time comboAt = Time::ZERO;
	i32 stanceShots = 0;
	Time stanceShotAt = Time::ZERO;
	i32 stanceShooter = -1;
	i32 evadeProbe = 0;
	Time evadeProbeAt = Time::ZERO;
	i32 evadeCaster = -1;
	vec3 comboPos = vec3(0);
	i32 comboCaster = -1;
	ActorUID comboTarget = ActorUID::INVALID;
	Time followShotAt = Time::ZERO;
	i32 comboDepth = 0;
	i32 followShooter = -1;
	f32 flatLen = 2050.0f;
	Time minNext = Time::ZERO;
	i32 gxPhase = 0;
	i32 gxStep = 0;
	i32 gxEnemy = -1;
	i32 gxRider = -1;
	Time quietSince = Time::ZERO;
	i32 posWaits = 0;
	vec3 anchor = vec3(0);
	f32 yaw = 0;
	i64 perfWindowStartUs = 0;
	i64 perfTotalUs = 0;
	i64 perfMaxUs = 0;
	i32 perfTicks = 0;
};

struct ShowcaseTickTimer
{
	ShowcaseState& state;
	const i64 startUs;
	explicit ShowcaseTickTimer(ShowcaseState& state_);
	~ShowcaseTickTimer();
};
