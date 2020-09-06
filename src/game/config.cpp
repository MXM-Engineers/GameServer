#include "config.h"
#include <EASTL/fixed_string.h>
#include <EAStdC/EASprintf.h>

bool CConfig::ParseLine(const char* line)
{
	if(EA::StdC::Sscanf(line, "ListenPort=%d", &listenPort) == 1) return true;
	if(EA::StdC::Sscanf(line, "DevMode=%d", &devMode) == 1) return true;
	if(EA::StdC::Sscanf(line, "TraceNetwork=%d", &traceNetwork) == 1) return true;
	return false;
}

// returns false on failing to open the config file
bool CConfig::LoadConfigFile()
{
	i32 fileSize;
	u8* fileData = fileOpenAndReadAll("game.cfg", &fileSize);
	if(!fileData) {
		LOG("WARNING(LoadConfigFile): failed to open 'game.cfg', using default config");
		return false;
	}
	defer(memFree(fileData));

	eastl::fixed_string<char,2048,true> contents;
	contents.assign((char*)fileData, fileSize);

	auto endLine = contents.find('\n');

	// parse at least one line
	if(endLine == -1) {
		bool r = ParseLine(contents.data());
		if(!r) {
			LOG("WARNING(LoadConfigFile): failed to parse line '%s'", contents.data());
			return true;
		}
	}

	do {
		auto line = contents.left(endLine);
		bool r = ParseLine(line.data());
		if(!r) {
			LOG("WARNING(LoadConfigFile): failed to parse line '%s'", line.data());
		}

		contents = contents.substr(endLine+1, contents.size());
		endLine = contents.find('\n');
	} while(endLine != -1);

	// last line
	if(contents.size() > 0) {
		bool r = ParseLine(contents.data());
		if(!r) {
			LOG("WARNING(LoadConfigFile): failed to parse line '%s'", contents.data());
		}
	}

	return true;
}

void CConfig::Print() const
{
	LOG("Config = {");
	LOG("	ListenPort=%d", listenPort);
	LOG("	DevMode=%d", devMode);
	LOG("	TraceNetwork=%d", traceNetwork);
	LOG("}");
}

static CConfig* g_Config = nullptr;

const CConfig& Config()
{
	return *g_Config;
}

bool LoadConfig()
{
	static CConfig cfg;
	g_Config = &cfg;
	return g_Config->LoadConfigFile();
}
