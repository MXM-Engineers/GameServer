#pragma once
#include <common/base.h>

struct CConfigGame
{
	i32 ListenPort = 13900;
	i32 TraceNetwork = false;

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

#define CONFIG_PATH "matchmaker.cfg"
