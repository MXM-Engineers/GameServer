#pragma once
#include <common/base.h>

struct CConfigGame
{
	i32 ListenPort = 12900;
	i32 DevMode = false;
	i32 DevQuickConnect = false;
	i32 DevForcedMap = 0;
	i32 DevBotStageSkill = 0;
	i32 ShotLeadMs = 50;
	i32 DevGameTimeSec = 0;
	i32 DevShowcase = 0;
	i32 DevShowcaseCount = 0;
	i32 DevShowcaseSteps = 0;
	i32 DevShowcaseFullRes = 0;
	i32 DevShowcaseUpgrade = 0;
	i32 DevShowcaseKeepSummons = 0;
	i32 DevHideDetectTest = 0;
	i32 DevValidate = 0;
	i32 DevGxTest = 0;
	i32 DevMirror = 0;
	i32 DevNoFireNode = 0;
	i32 DevAutoStage = 0;
	i32 DevTelemetry = 0;
	i32 DevTrail = 0;
	i32 DevTruth = 0;
	i32 DevScenario = 0;
	i32 DevObserver = 0;
	i32 DevPilotTour = 0;
	i32 DevPilotTourFirst = 0;
	i32 DevGodPlayers = 0;
	i32 DevStartLevel = 0;
	i32 DevPilotUpgrades = 0;
	i32 DevBotMonsterSkills = 0;
	i32 HostIP[4] = { 0, 0, 0, 0 };
	char DevPilotName[32] = {};
	i32 HubServerPublicIP[4] = { 0, 0, 0, 0 };
	i32 HubServerPort = 11900;
	i32 TraceNetwork = false;
	i32 TraceStream = false;
	i32 BotSyncCadence = 0;
	i32 WindowWidth = 1280;
	i32 WindowHeight = 720;
	f32 DbgCamPosX = 0;
	f32 DbgCamPosY = 0;
	f32 DbgCamPosZ = 6000;
	f32 DbgCamDirX = 0;
	f32 DbgCamDirY = 0;
	f32 DbgCamDirZ = -1;

	bool ParseLine(const char* line);
	// returns false on failing to open the config file
	bool LoadConfigFile();
	bool SaveConfigFile();
	void Print() const;
};

const CConfigGame& Config();
CConfigGame& ConfigMutable();
bool LoadConfig();
bool SaveConfig();

#define CONFIG_PATH "game.cfg"
