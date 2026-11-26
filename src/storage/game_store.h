#pragma once
#include <common/base.h>
#include <common/inner_protocol.h>
#include <common/protocol.h>
#include <common/utils.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread_semaphore.h>
#include <EASTL/functional.h>
#include <EASTL/deque.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>

struct ProfileMaster
{
	ClassType classType = ClassType::NONE;
	SkinIndex skinIndex = SkinIndex::DEFAULT;
};

struct ProfileSkin
{
	ClassType classType = ClassType::NONE;
	SkinIndex skinIndex = SkinIndex::DEFAULT;
	i64 expireAt = 0;
};

enum {
	PROFILE_SETTINGS_SLOTS = 4,
	PROFILE_SETTINGS_MAX_SIZE = 6144,
	STATUS_MESSAGE_MAX_LEN = 64,
	TUTORIAL_STATE_TOWN = -1
};

const i64 STATUS_MESSAGE_INTERVAL_SEC = 60;

enum class SettingsSaveCheck: u8
{
	OK = 0,
	BAD_SLOT,
	EMPTY,
	TOO_LARGE
};

struct ProfileSettings
{
	bool present = false;
	eastl::vector<u8> data;
};

struct PlayerProfile
{
	AccountUID accountUID = AccountUID::INVALID;
	i64 createdAt = 0;
	i64 lastLoginAt = 0;
	bool firstEntry = false;
	ClassType leaderClass = ClassType::NONE;
	SkinIndex leaderSkin = SkinIndex::DEFAULT;
	eastl::vector<ProfileMaster> masters;
	eastl::vector<ProfileSkin> skins;
	eastl::array<ProfileSettings,PROFILE_SETTINGS_SLOTS> settings;
	i32 tutorialState = TUTORIAL_STATE_TOWN;
	WideString statusMessage;
	i64 statusMessageAt = 0;
	i64 lastLogoutAt = 0;

	ProfileMaster* FindMaster(ClassType classType);
	const ProfileMaster* FindMaster(ClassType classType) const;
	bool OwnsSkin(ClassType classType, SkinIndex skinIndex) const;

	static SettingsSaveCheck CheckSettingsSave(i32 settingType, i32 size);
	void SetSettings(i32 settingType, const u8* data, i32 size);
	ErrorType CheckStatusMessage(i32 len, i64 now) const;
	void SetStatusMessage(const wchar* msg, i32 len, i64 now);
};

struct ProfileSeed
{
	struct Skin
	{
		ClassType classType;
		SkinIndex skinIndex;
	};

	bool dev = false;
	ClassType leaderClass = ClassType::NONE;
	eastl::vector<ClassType> masters;
	eastl::vector<Skin> skins;
};

void SeedProfileInMemory(const ProfileSeed& seed, PlayerProfile* profile);

struct GameStoreBackend
{
	virtual ~GameStoreBackend() {}

	virtual bool Open(const char* path) = 0;
	virtual void Close() = 0;

	virtual bool Begin() = 0;
	virtual bool Commit() = 0;
	virtual void Rollback() = 0;

	virtual bool LoadProfile(AccountUID accountUID, i64 now, const ProfileSeed& seed, PlayerProfile* out) = 0;
	virtual bool SetLeader(AccountUID accountUID, ClassType classType, SkinIndex skinIndex) = 0;
	virtual bool SaveSettings(AccountUID accountUID, i32 settingType, const u8* data, i32 size, i64 now) = 0;
	virtual bool SetStatusMessage(AccountUID accountUID, const wchar* msg, i32 len, i64 at) = 0;
	virtual bool SetLastLogout(AccountUID accountUID, i64 at) = 0;
};

struct GameStoreOptions
{
	ProfileSeed seed;
	bool assertOnFailure = true;
};

struct ProfileLoaded
{
	u32 tag;
	PlayerProfile profile;
};

typedef eastl::function<bool(GameStoreBackend&)> GameStoreWrite;

struct GameStore
{
	enum {
		MAX_LOADED_PER_TRANSFER = 128
	};

	typedef eastl::fixed_vector<ProfileLoaded,MAX_LOADED_PER_TRANSFER,false> LoadedList;

	bool Start(GameStoreBackend* backend_, const char* path, const GameStoreOptions& options_);
	void Shutdown();

	void QueueWrite(const char* name, GameStoreWrite write);
	void QueueLoadProfile(u32 tag, AccountUID accountUID);
	void TransferLoaded(LoadedList* out);

	i32 FailedJobCount();

private:
	enum class JobKind: u8
	{
		WRITE = 0,
		LOAD_PROFILE
	};

	struct Job
	{
		JobKind kind = JobKind::WRITE;
		const char* name = "";
		u32 tag = 0;
		AccountUID accountUID = AccountUID::INVALID;
		GameStoreWrite write;
	};

	GameStoreBackend* backend = nullptr;
	GameStoreOptions options;
	EA::Thread::Thread thread;
	EA::Thread::Semaphore wakeUp{0};
	bool running = false;

	Mutex mutexQueue;
	eastl::deque<Job> queue;
	bool stopping = false;

	Mutex mutexLoaded;
	eastl::deque<ProfileLoaded> loaded;
	i32 failedJobs = 0;

	static intptr_t ThreadStorage(void* pData);
	void Run();
	void RunJob(const Job& job);
	void Push(Job&& job);
};
