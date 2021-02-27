#pragma once
#include <common/base.h>

struct CConfig
{
	i32 ListenLobbyPort = 11900;
	i32 ListenGamePort = 12900;
	i32 DevMode = false;
	i32 TraceNetwork = false;
	i32 LobbyMap = 160000042;
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

const CConfig& Config();
CConfig& ConfigMutable();
bool LoadConfig();
bool SaveConfig();
