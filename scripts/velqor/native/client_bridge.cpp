#include "os_input.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>

#include "third_party/Detours-4.0.1/src/detours.h"
#include "velqor.h"
#include "client_bridge.h"

namespace {

const uint32_t kStaticBase = 0x00400000u;

const uint32_t kGameSessionGlobalVa = 0x03308B00u;
const uint32_t kKeyManagerGlobalVa = 0x03308F18u;
const uint32_t kTranslateFallbackVa = 0x03312F70u;

const uint32_t kPhysControllerGetGameplayPosVa = 0x01B7B360u;
const uint32_t kCreatureGetTranslatePtrVa = 0x01A8F60Cu;
const uint32_t kPlayCreatePartyVa = 0x009D4A6Au;
const uint32_t kCorrectionDispatchVa = 0x006BB547u;
const uint32_t kCorrectionNetId = 0xF398u;
const uint32_t kNetMessageDataOffset = 0x0Cu;
const uint32_t kCorrectionEntityIdOffset = 0x04u;
const uint32_t kCorrectionMinPacketSize = 0x08u;
const uint32_t kApplicationGlobalVa = 0x03316044u;
const uint32_t kRuntimeCastVa = 0x01C736E4u;
const uint32_t kSceneTypeVa = 0x032DF404u;
const uint32_t kBattleSceneTypeVa = 0x032DF430u;
const uint32_t kInputLockVa = 0x01ADF149u;
const uint32_t kIsInputTypeLockedVa = 0x01ADF2ABu;
const uint32_t kGameStartDispatchVa = 0x008BCD77u;
const uint32_t kGameStartNetId = 0xF27Cu;

const uint32_t kSessionLocalEntityOffset = 0x3Cu;
const uint32_t kEntityNodeOffset = 0x30u;
const uint32_t kEntityActorIdOffset = 0xA88u;
const uint32_t kEntityMetadataOffset = 0x38u;
const uint32_t kMetadataClassTypeOffset = 0xB10u;
const uint32_t kEntityPhysControllerOffset = 0xD24u;
const uint32_t kPhysControllerNxOffset = 0x04u;
const uint32_t kManagerCursorXOffset = 0x894u;
const uint32_t kManagerCursorYOffset = 0x898u;

const int kMaxTrackedKeys = 64;

enum
{
	kPhaseBoot = 0,
	kPhaseFrontend = 1,
	kPhasePlay = 2,
	kPhaseBattle = 3,
	kPhaseError = 4
};

typedef float* (__thiscall* GetGameplayPosFn)(void*, float*);
typedef float* (__thiscall* GetTranslatePtrFn)(void*);
typedef uint32_t(__cdecl* PlayCreatePartyFn)(const int32_t*, int32_t);
typedef void(__fastcall* CorrectionDispatchFn)(void*, void*, uint32_t, void*, uint32_t);
typedef void*(__cdecl* RuntimeCastFn)(void*, long, void*, void*, int);
typedef void(__fastcall* InputLockFn)(void*, void*, int32_t, int32_t);
typedef uint8_t(__thiscall* IsInputTypeLockedFn)(void*, uint32_t);
typedef void(__stdcall* GameStartDispatchFn)(uint32_t, void*, uint32_t, uint16_t, const char*,
	int32_t);

uint8_t* g_base = 0;
volatile long g_initialized = 0;
volatile long g_fatal = 0;
volatile long g_play_issued = 0;
volatile long g_play_ok = 0;
volatile long g_phase = kPhaseBoot;
volatile long g_input_ready = 0;
volatile long g_hook_ready = 0;
volatile long g_match_started = 0;
void* volatile g_scene_seen = 0;
void* volatile g_session_seen = 0;
uint32_t g_pending_seq = 0;
int32_t g_cursor_x = 0;
int32_t g_cursor_y = 0;
uint32_t g_last_not_ready_hash = 0xFFFFFFFFu;
uint32_t g_tracked_keys[kMaxTrackedKeys];
int g_tracked_count = 0;
char g_last_error[192] = {0};

GetGameplayPosFn g_get_gameplay_pos = 0;
GetTranslatePtrFn g_get_translate = 0;
PlayCreatePartyFn g_play_create_party = 0;
CorrectionDispatchFn g_correction_dispatch = 0;
RuntimeCastFn g_runtime_cast = 0;
InputLockFn g_input_lock = 0;
IsInputTypeLockedFn g_is_input_type_locked = 0;
GameStartDispatchFn g_game_start_dispatch = 0;

void* StaticVa(uint32_t static_va)
{
	if (g_base == 0)
		return 0;
	return g_base + (static_va - kStaticBase);
}

bool IsFiniteF(float v)
{
	return v == v && v > -1.0e30f && v < 1.0e30f;
}

uint32_t HashText(const char* text)
{
	uint32_t h = 2166136261u;
	if (text == 0)
		return h;
	while (*text != '\0')
	{
		h ^= (uint8_t)(*text++);
		h *= 16777619u;
	}
	return h;
}

const char* PhaseText(long phase)
{
	switch (phase)
	{
	case kPhaseBoot:
		return "boot";
	case kPhaseFrontend:
		return "frontend";
	case kPhasePlay:
		return "play";
	case kPhaseBattle:
		return "battle";
	case kPhaseError:
		return "error";
	default:
		return "boot";
	}
}

void SetError(const char* message)
{
	if (message == 0)
		message = "";
	if (strncmp(g_last_error, message, sizeof(g_last_error) - 1) == 0)
		return;
	strncpy_s(g_last_error, sizeof(g_last_error), message, _TRUNCATE);
	char fields[256];
	snprintf(fields, sizeof(fields), "\"message\":\"%s\"", g_last_error);
	velqor::Emit("error", fields);
}

void SetFatal(const char* message)
{
	SetError(message);
	InterlockedExchange(&g_fatal, 1);
}

void SetPhase(long phase)
{
	long previous = g_phase;
	if (previous == phase)
		return;
	InterlockedExchange(&g_phase, phase);
	char fields[128];
	snprintf(fields, sizeof(fields), "\"to\":\"%s\",\"from\":\"%s\"", PhaseText(phase), PhaseText(previous));
	velqor::Emit("phase", fields);
}

void EmitNotReady(const char* reason)
{
	uint32_t hash = HashText(reason);
	if (hash == g_last_not_ready_hash)
		return;
	g_last_not_ready_hash = hash;
	char fields[160];
	snprintf(fields, sizeof(fields), "\"reason\":\"%s\"", reason);
	velqor::Emit("not_ready", fields);
}

void* LocalEntity()
{
	void** session_slot = (void**)StaticVa(kGameSessionGlobalVa);
	if (session_slot == 0)
		return 0;
	void* session = *session_slot;
	if (session == 0)
		return 0;
	return *(void**)((uint8_t*)session + kSessionLocalEntityOffset);
}

uint32_t LocalActorId(void* entity)
{
	if (entity == 0)
		return 0;
	return *(uint32_t*)((uint8_t*)entity + kEntityActorIdOffset);
}

int32_t LocalClassType(void* entity)
{
	void* metadata = entity ? *(void**)((uint8_t*)entity + kEntityMetadataOffset) : 0;
	return metadata ? *(int32_t*)((uint8_t*)metadata + kMetadataClassTypeOffset) : -1;
}

void* LocalPhysController(void* entity)
{
	if (entity == 0)
		return 0;
	void* phys = *(void**)((uint8_t*)entity + kEntityPhysControllerOffset);
	if (phys == 0)
		return 0;
	if (*(void**)((uint8_t*)phys + kPhysControllerNxOffset) == 0)
		return 0;
	return phys;
}

void* CurrentScene()
{
	void* app = *(void**)StaticVa(kApplicationGlobalVa);
	if (app == 0)
		return 0;
	void* manager = *(void**)((uint8_t*)app + 0x89Cu);
	if (manager == 0)
		return 0;
	return *(void**)((uint8_t*)manager + 0x10u);
}

bool InBattleScene()
{
	void* scene = CurrentScene();
	return scene != 0 && g_runtime_cast(scene, 0, StaticVa(kSceneTypeVa),
		StaticVa(kBattleSceneTypeVa), 0) != 0;
}

bool CurrentMatchStarted()
{
	void* scene = CurrentScene();
	void* session = *(void**)StaticVa(kGameSessionGlobalVa);
	if (InterlockedCompareExchangePointer(&g_scene_seen, 0, 0) != scene ||
		InterlockedCompareExchangePointer(&g_session_seen, 0, 0) != session)
	{
		InterlockedExchangePointer(&g_scene_seen, scene);
		InterlockedExchangePointer(&g_session_seen, session);
		InterlockedExchange(&g_match_started, 0);
		return false;
	}
	return InterlockedCompareExchange(&g_match_started, 0, 0) != 0;
}

bool InputUnlocked()
{
	void* manager = *(void**)StaticVa(kKeyManagerGlobalVa);
	return manager != 0 &&
		*((uint8_t*)manager + 0x890u) == 0 && *((uint8_t*)manager + 0x891u) == 0 &&
		(*((uint8_t*)manager + 0x8C8u) == 0 ||
			(g_is_input_type_locked(manager, 6) == 0 && g_is_input_type_locked(manager, 14) == 0));
}

bool LocalEntityReady()
{
	if (!InBattleScene())
		return false;
	void* entity = LocalEntity();
	if (entity == 0)
		return false;
	if (LocalActorId(entity) == 0)
		return false;
	if (LocalPhysController(entity) == 0)
		return false;
	if (LocalClassType(entity) < 0)
	{
		EmitNotReady("lobby_or_preview");
		return false;
	}
	return true;
}

bool SampleFeet(void* entity, float out[3])
{
	void* phys = LocalPhysController(entity);
	if (phys == 0 || g_get_gameplay_pos == 0)
		return false;
	g_get_gameplay_pos(phys, out);
	return IsFiniteF(out[0]) && IsFiniteF(out[1]) && IsFiniteF(out[2]);
}

bool SampleVisual(void* entity, float out[3])
{
	if (entity == 0 || g_get_translate == 0)
		return false;
	if (*(void**)((uint8_t*)entity + kEntityNodeOffset) == 0)
		return false;
	float* translate = g_get_translate(entity);
	if (translate == 0 || translate == (float*)StaticVa(kTranslateFallbackVa))
		return false;
	out[0] = translate[0];
	out[1] = translate[1];
	out[2] = translate[2];
	return IsFiniteF(out[0]) && IsFiniteF(out[1]) && IsFiniteF(out[2]);
}

void EmitSample(const char* phase)
{
	if (InterlockedCompareExchange(&g_initialized, 0, 0) == 0)
	{
		EmitNotReady("not_initialized");
		return;
	}
	if (!LocalEntityReady())
		return;
	void* entity = LocalEntity();
	if (entity == 0)
	{
		EmitNotReady("no_local_entity");
		return;
	}
	float feet[3];
	if (!SampleFeet(entity, feet))
	{
		EmitNotReady("no_controller");
		return;
	}
	float visual[3];
	if (!SampleVisual(entity, visual))
	{
		EmitNotReady("no_visual");
		return;
	}
	uint32_t actor_id = LocalActorId(entity);
	int32_t class_type = LocalClassType(entity);
	char fields[384];
	snprintf(fields, sizeof(fields),
		"\"phase\":\"%s\",\"feet\":[%.9g,%.9g,%.9g],\"visual\":[%.9g,%.9g,%.9g],\"local_actor_id\":%u,\"class_type\":%d,\"frame\":%llu",
		phase,
		(double)feet[0], (double)feet[1], (double)feet[2],
		(double)visual[0], (double)visual[1], (double)visual[2],
		(unsigned int)actor_id, (int)class_type,
		(unsigned long long)velqor::CurrentFrame());
	velqor::Emit("sample", fields);
	g_last_not_ready_hash = 0xFFFFFFFFu;
}

void EmitCorrection(uint32_t actor_id, const float before[3], const float after[3],
	const float visual[3])
{
	if (InterlockedCompareExchange(&g_initialized, 0, 0) == 0)
		return;
	char fields[384];
	snprintf(fields, sizeof(fields),
		"\"before_feet\":[%.9g,%.9g,%.9g],\"feet\":[%.9g,%.9g,%.9g],\"visual\":[%.9g,%.9g,%.9g],\"local_actor_id\":%u,\"frame\":%llu,\"observation\":\"post_handler\"",
		(double)before[0], (double)before[1], (double)before[2],
		(double)after[0], (double)after[1], (double)after[2],
		(double)visual[0], (double)visual[1], (double)visual[2],
		(unsigned int)actor_id,
		(unsigned long long)velqor::CurrentFrame());
	velqor::Emit("correction", fields);
}

void __fastcall BridgeCorrectionDispatch(void* self, void* reserved, uint32_t netid,
	void* message, uint32_t size)
{
	(void)reserved;
	CorrectionDispatchFn original = g_correction_dispatch;
	if (original == 0)
		return;
	void* entity = netid == kCorrectionNetId && message != 0 && size >= kCorrectionMinPacketSize
		? LocalEntity() : 0;
	uint32_t local_actor_id = LocalActorId(entity);
	const uint8_t* packet = local_actor_id != 0
		? *(const uint8_t**)((const uint8_t*)message + kNetMessageDataOffset) : 0;
	uint32_t packet_actor_id = packet != 0
		? *(const uint32_t*)(packet + kCorrectionEntityIdOffset) : 0;
	if (packet_actor_id == 0 || packet_actor_id != local_actor_id || !LocalEntityReady())
	{
		original(self, reserved, netid, message, size);
		return;
	}
	float before[3];
	bool before_ok = SampleFeet(entity, before);
	original(self, reserved, netid, message, size);
	if (!before_ok)
		return;
	void* after_entity = LocalEntity();
	if (after_entity == 0 || LocalActorId(after_entity) != packet_actor_id || !LocalEntityReady())
		return;
	float after[3];
	if (!SampleFeet(after_entity, after))
		return;
	float visual[3];
	if (!SampleVisual(after_entity, visual))
		return;
	EmitCorrection(packet_actor_id, before, after, visual);
}


void __fastcall BridgeInputLock(void* manager, void* reserved, int32_t category, int32_t locked)
{
	uint32_t caller = (uint32_t)((uint8_t*)_ReturnAddress() - g_base) + kStaticBase;
	g_input_lock(manager, reserved, category, locked);
	char fields[128];
	snprintf(fields, sizeof(fields), "\"category\":%d,\"locked\":%d,\"caller_va\":%u",
		(int)category, (int)locked, (unsigned int)caller);
	velqor::Emit("input_lock", fields);
}

void __stdcall BridgeGameStartDispatch(uint32_t context, void* payload, uint32_t size,
	uint16_t netid, const char* name, int32_t verbose)
{
	GameStartDispatchFn original = g_game_start_dispatch;
	if (original == 0)
		return;
	original(context, payload, size, netid, name, verbose);
	if (netid == kGameStartNetId)
	{
		CurrentMatchStarted();
		InterlockedExchange(&g_match_started, 1);
		velqor::Emit("match_start", "");
	}
}

bool InstallGameStartHook()
{
	void* target = StaticVa(kGameStartDispatchVa);
	if (target == 0)
	{
		SetError("game_start_hook_unavailable");
		return false;
	}
	if (!velqor::HookTransactionBegin())
	{
		SetError("hook_transaction_failed");
		return false;
	}
	g_game_start_dispatch = (GameStartDispatchFn)target;
	velqor::HookUpdateThread(GetCurrentThread());
	if (DetourAttach((PVOID*)&g_game_start_dispatch, (PVOID)&BridgeGameStartDispatch) != NO_ERROR)
	{
		DetourTransactionAbort();
		g_game_start_dispatch = 0;
		SetError("game_start_hook_failed");
		return false;
	}
	if (!velqor::HookTransactionCommit())
	{
		g_game_start_dispatch = 0;
		SetError("game_start_hook_commit_failed");
		return false;
	}
	if (g_game_start_dispatch == 0 ||
		g_game_start_dispatch == (GameStartDispatchFn)target)
	{
		g_game_start_dispatch = 0;
		SetError("game_start_hook_unverified");
		return false;
	}
	return true;
}

bool InstallCorrectionHook()
{
	void* target = StaticVa(kCorrectionDispatchVa);
	if (target == 0)
	{
		SetError("correction_hook_unavailable");
		return false;
	}
	if (!velqor::HookTransactionBegin())
	{
		SetError("hook_transaction_failed");
		return false;
	}
	g_correction_dispatch = (CorrectionDispatchFn)target;
	g_input_lock = (InputLockFn)StaticVa(kInputLockVa);
	velqor::HookUpdateThread(GetCurrentThread());
	if (DetourAttach((PVOID*)&g_correction_dispatch, (PVOID)&BridgeCorrectionDispatch) != NO_ERROR ||
		DetourAttach((PVOID*)&g_input_lock, (PVOID)&BridgeInputLock) != NO_ERROR)
	{
		DetourTransactionAbort();
		g_correction_dispatch = 0;
		SetError("correction_hook_failed");
		return false;
	}
	if (!velqor::HookTransactionCommit())
	{
		g_correction_dispatch = 0;
		SetError("correction_hook_commit_failed");
		return false;
	}
	if (g_correction_dispatch == 0 || g_correction_dispatch == (CorrectionDispatchFn)target ||
		g_input_lock == 0 || g_input_lock == (InputLockFn)StaticVa(kInputLockVa))
	{
		g_correction_dispatch = 0;
		SetError("correction_hook_unverified");
		return false;
	}
	InterlockedExchange(&g_hook_ready, 1);
	return true;
}

void RefreshPhase()
{
	long phase = kPhaseBoot;
	long input_ready = 0;
	bool match_started = InterlockedCompareExchange(&g_initialized, 0, 0) != 0 &&
		CurrentMatchStarted();
	if (InterlockedCompareExchange(&g_fatal, 0, 0) != 0)
	{
		phase = kPhaseError;
	}
	else if (InterlockedCompareExchange(&g_initialized, 0, 0) == 0)
	{
		phase = kPhaseBoot;
	}
	else if (LocalEntityReady())
	{
		phase = kPhaseBattle;
		if (match_started)
			input_ready = InputUnlocked() ? 1 : 0;
	}
	else if (InterlockedCompareExchange(&g_play_issued, 0, 0) != 0)
	{
		phase = kPhasePlay;
	}
	else
	{
		void* entity = LocalEntity();
		phase = entity != 0 && LocalActorId(entity) != 0 && LocalPhysController(entity) != 0
			? kPhaseFrontend : kPhaseBoot;
	}
	InterlockedExchange(&g_input_ready, input_ready);
	SetPhase(phase);
}

void TrackKey(uint32_t vk, int down)
{
	if (down != 0)
	{
		for (int i = 0; i < g_tracked_count; ++i)
		{
			if (g_tracked_keys[i] == vk)
				return;
		}
		if (g_tracked_count < kMaxTrackedKeys)
			g_tracked_keys[g_tracked_count++] = vk;
		return;
	}
	for (int i = 0; i < g_tracked_count; ++i)
	{
		if (g_tracked_keys[i] == vk)
		{
			for (int j = i; j < g_tracked_count - 1; ++j)
				g_tracked_keys[j] = g_tracked_keys[j + 1];
			--g_tracked_count;
			return;
		}
	}
}

void* KeyManager()
{
	void** slot = (void**)StaticVa(kKeyManagerGlobalVa);
	if (slot == 0)
		return 0;
	return *slot;
}

}

extern "C" bool BridgeOsSafetyInitialize()
{
	if (velqor::os::Ready())
		return true;
	if (!velqor::os::Initialize())
	{
		SetFatal("os_init_failed");
		return false;
	}
	return velqor::os::Ready();
}

extern "C" bool BridgeInitialize()
{
	if (InterlockedCompareExchange(&g_initialized, 0, 0) != 0)
		return true;
	if (!velqor::os::Ready())
	{
		SetError("os_not_ready");
		return false;
	}
	HMODULE module = GetModuleHandleW(0);
	if (module == 0)
	{
		SetError("no_module");
		return false;
	}
	g_base = (uint8_t*)module;
	if (KeyManager() == 0)
	{
		SetError("no_input_manager");
		return false;
	}
	if (StaticVa(kGameSessionGlobalVa) == 0 || *(void**)StaticVa(kGameSessionGlobalVa) == 0)
	{
		SetError("no_game_session");
		return false;
	}
	g_get_gameplay_pos = (GetGameplayPosFn)StaticVa(kPhysControllerGetGameplayPosVa);
	g_get_translate = (GetTranslatePtrFn)StaticVa(kCreatureGetTranslatePtrVa);
	g_play_create_party = (PlayCreatePartyFn)StaticVa(kPlayCreatePartyVa);
	g_runtime_cast = (RuntimeCastFn)StaticVa(kRuntimeCastVa);
	g_is_input_type_locked = (IsInputTypeLockedFn)StaticVa(kIsInputTypeLockedVa);
	bool hook_ok = InstallCorrectionHook();
	if (hook_ok)
		hook_ok = InstallGameStartHook();
	InterlockedExchange(&g_initialized, 1);
	if (!hook_ok)
		SetFatal(g_last_error[0] != 0 ? g_last_error : "correction_hook_failed");
	RefreshPhase();
	return true;
}

extern "C" void BridgeBeforeFrame()
{
	RefreshPhase();
	EmitSample("pre");
}

extern "C" void BridgeAfterFrame()
{
	RefreshPhase();
	EmitSample("post");
}

extern "C" bool BridgeSetCursor(int32_t x, int32_t y)
{
	velqor::os::SetVirtualCursor(x, y);
	g_cursor_x = x;
	g_cursor_y = y;
	void* manager = KeyManager();
	if (manager == 0)
		return false;
	*(int32_t*)((uint8_t*)manager + kManagerCursorXOffset) = x;
	*(int32_t*)((uint8_t*)manager + kManagerCursorYOffset) = y;
	return true;
}

extern "C" bool BridgePlay()
{
	if (InterlockedCompareExchange(&g_initialized, 0, 0) == 0)
	{
		SetError("not_initialized");
		return false;
	}
	if (!velqor::os::Ready())
	{
		SetError("os_not_ready");
		return false;
	}
	if (LocalEntityReady())
	{
		SetPhase(kPhaseBattle);
		return true;
	}
	if (InterlockedCompareExchange(&g_play_issued, 0, 0) != 0)
		return InterlockedCompareExchange(&g_play_ok, 0, 0) != 0;
	if (g_play_create_party == 0)
	{
		SetError("play_unavailable");
		return false;
	}
	const int32_t entry_system_id = 210036812;
	uint32_t queued = g_play_create_party(&entry_system_id, 2);
	if (queued == 0)
	{
		InterlockedExchange(&g_play_ok, 0);
		SetError("play_rejected");
		return false;
	}
	InterlockedExchange(&g_play_issued, 1);
	InterlockedExchange(&g_play_ok, 1);
	RefreshPhase();
	return true;
}

extern "C" const char* BridgeLastError()
{
	return g_last_error;
}

extern "C" bool BridgeReady()
{
	if (InterlockedCompareExchange(&g_fatal, 0, 0) != 0)
		return false;
	if (InterlockedCompareExchange(&g_initialized, 0, 0) == 0)
		return false;
	return InterlockedCompareExchange(&g_phase, 0, 0) == kPhaseBattle &&
		InterlockedCompareExchange(&g_input_ready, 0, 0) != 0;
}

extern "C" void BridgeNotifyKey(uint32_t vk, int down)
{
	TrackKey(vk, down);
}

extern "C" void BridgeNoteSetCursorPos(int32_t x, int32_t y)
{
	g_cursor_x = x;
	g_cursor_y = y;
}

extern "C" void BridgeSetPendingSeq(uint32_t seq)
{
	g_pending_seq = seq;
}

extern "C" void BridgeOnStop()
{
	for (int i = 0; i < g_tracked_count; ++i)
		g_tracked_keys[i] = 0;
	g_tracked_count = 0;
	g_last_not_ready_hash = 0xFFFFFFFFu;
	velqor::os::ReleaseAllVirtualKeys();
}

extern "C" bool BridgeUnfocused()
{
	return velqor::os::ClientUnfocused();
}

extern "C" const char* BridgePhaseName()
{
	if (InterlockedCompareExchange(&g_fatal, 0, 0) != 0)
		return "error";
	return PhaseText(g_phase);
}
