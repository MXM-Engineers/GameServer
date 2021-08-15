#pragma once
#include <common/base.h>

struct CConfigHub
{
	i32 ListenPort = 11900;
	i32 DevMode = false;
	i32 TraceNetwork = false;
	i32 LobbyMap = 160000042; // TODO: restore

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
