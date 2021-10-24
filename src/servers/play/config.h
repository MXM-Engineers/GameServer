#pragma once
#include <common/base.h>

struct CConfigGame
{
	i32 ListenPort = 12900;
	i32 DevMode = false;
	i32 DevQuickConnect = false;
	i32 TraceNetwork = false;
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
