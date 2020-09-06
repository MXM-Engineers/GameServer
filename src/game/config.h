#pragma once
#include <common/base.h>

struct CConfig
{
	i32 listenPort = 11900;
	i32 devMode = false;
	i32 traceNetwork = false;

	bool ParseLine(const char* line);
	// returns false on failing to open the config file
	bool LoadConfigFile();
	void Print() const;
};

const CConfig& Config();
bool LoadConfig();
