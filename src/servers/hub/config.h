#pragma once
#include <common/base.h>

struct CConfigHub
{
	i32 ListenPort = 11900;
	i32 DevMode = false;
	i32 DevQuickConnect = false;
	i32 DevAutoReturnToGame = false;
	i32 TraceNetwork = false;
	i32 DevShowcase = false;
	i32 DevAutoQueue = 0;
	i32 DevAutoQueueStage = 0;
	i32 DevAutoQueueGames = 0;
	i32 DevAutoParty = 0;
	i32 DevPickMasters[2] = { 0, 0 };
	i32 DevStageSweep = 0;
	i32 DevShowcaseStart = 0;
	i32 DevShowcaseSlices = 1;
	i32 LobbyMap = 160000042; // TODO: restore
	i32 DevDailyAreaEvent = 0;
	eastl::fixed_vector<i32,16> regionBanMaster = {};
	eastl::fixed_vector<i32,64> devShowcaseList = {};
	eastl::fixed_vector<i32,64> devHumanPick = {};
	eastl::fixed_vector<i32,64> devBotPool = {};
	i32 DevMirror = 0;
	i32 DevMirrorPass = 0;
	i32 DevLabSkills = 0;
	i32 DevLabSkillA = 0;
	i32 DevLabSkillB = 0;
	eastl::fixed_vector<i32,16> regionNewMaster = {};
	eastl::fixed_vector<i32,16> eventBanMaster = {};

	bool ParseLine(const char* line);
	// returns false on failing to open the config file
	bool LoadConfigFile();
	bool SaveConfigFile();
	void Print() const;
};

const CConfigHub& Config();
CConfigHub& ConfigMutable();
bool LoadConfig();
bool SaveConfig();


#define CONFIG_PATH "hub.cfg"
