#include "game_store.h"
#include <time.h>

bool GameStore::Start(GameStoreBackend* backend_, const char* path, const GameStoreOptions& options_)
{
	ASSERT(!running);
	ASSERT(backend_);

	if(!backend_->Open(path)) return false;

	backend = backend_;
	options = options_;
	stopping = false;
	failedJobs = 0;
	running = true;
	thread.Begin(ThreadStorage, this);
	return true;
}

void GameStore::Shutdown()
{
	if(!running) return;

	{
		LOCK_MUTEX(mutexQueue);
		ASSERT(!stopping);
		stopping = true;
	}
	wakeUp.Post();
	thread.WaitForEnd();

	ASSERT(queue.empty());
	backend->Close();
	backend = nullptr;
	running = false;
}

void GameStore::QueueWrite(const char* name, GameStoreWrite write)
{
	ASSERT(write);
	Job job;
	job.kind = JobKind::WRITE;
	job.name = name;
	job.write = eastl::move(write);
	Push(eastl::move(job));
}

void GameStore::QueueLoadProfile(u32 tag, AccountUID accountUID)
{
	ASSERT(accountUID != AccountUID::INVALID);
	Job job;
	job.kind = JobKind::LOAD_PROFILE;
	job.name = "LoadProfile";
	job.tag = tag;
	job.accountUID = accountUID;
	Push(eastl::move(job));
}

void GameStore::TransferLoaded(LoadedList* out)
{
	out->clear();
	LOCK_MUTEX(mutexLoaded);
	while(!loaded.empty() && out->size() < out->capacity()) {
		out->push_back(loaded.front());
		loaded.pop_front();
	}
}

i32 GameStore::FailedJobCount()
{
	LOCK_MUTEX(mutexLoaded);
	return failedJobs;
}

void GameStore::Push(Job&& job)
{
	ASSERT(running);
	{
		LOCK_MUTEX(mutexQueue);
		ASSERT(!stopping);
		queue.push_back(eastl::move(job));
	}
	wakeUp.Post();
}

intptr_t GameStore::ThreadStorage(void* pData)
{
	ProfileSetThreadName("Storage");
	((GameStore*)pData)->Run();
	return 0;
}

void GameStore::Run()
{
	for(;;) {
		const i32 woken = wakeUp.Wait();
		ASSERT(woken >= 0);

		Job job;
		{
			LOCK_MUTEX(mutexQueue);
			if(queue.empty()) {
				if(stopping) break;
				continue;
			}
			job = eastl::move(queue.front());
			queue.pop_front();
		}

		RunJob(job);
	}

	LOG("GameStore :: storage thread drained its queue and stops");
}

void GameStore::RunJob(const Job& job)
{
	ProfileFunction();

	const i64 now = (i64)time(nullptr);

	bool ok = backend->Begin();
	PlayerProfile profile;
	if(ok) {
		switch(job.kind) {
			case JobKind::WRITE: ok = job.write(*backend); break;
			case JobKind::LOAD_PROFILE: ok = backend->LoadProfile(job.accountUID, now, options.seed, &profile); break;
		}

		if(ok) ok = backend->Commit();
		if(!ok) backend->Rollback();
	}

	if(!ok) {
		LOG("ERROR(GameStore): job '%s' failed and was rolled back (account=%u)", job.name, (u32)job.accountUID);
		{
			LOCK_MUTEX(mutexLoaded);
			failedJobs++;
		}
		if(options.assertOnFailure) ASSERT_MSG(0, "game store job failed");
		return;
	}

	if(job.kind == JobKind::LOAD_PROFILE) {
		LOCK_MUTEX(mutexLoaded);
		loaded.push_back(ProfileLoaded{ job.tag, profile });
	}
}
