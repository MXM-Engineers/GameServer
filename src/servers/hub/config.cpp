#include "config.h"
#include <EASTL/fixed_string.h>
#include <EAStdC/EASprintf.h>

bool CConfigHub::ParseLine(const char* line)
{
	if(EA::StdC::Sscanf(line, "ListenPort=%d", &ListenPort) == 1) return true;
	if(EA::StdC::Sscanf(line, "DevMode=%d", &DevMode) == 1) return true;
	if(EA::StdC::Sscanf(line, "TraceNetwork=%d", &TraceNetwork) == 1) return true;
	if(EA::StdC::Sscanf(line, "LobbyMap=%d", &LobbyMap) == 1) return true;
	return false;
}

// returns false on failing to open the config file
bool CConfigHub::LoadConfigFile()
{
	i32 fileSize;
	u8* fileData = fileOpenAndReadAll(CONFIG_PATH, &fileSize);
	if(!fileData) {
		LOG("WARNING(LoadConfigFile): failed to open '%s', using default config", CONFIG_PATH);
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

bool CConfigHub::SaveConfigFile()
{
	eastl::fixed_string<char,4096,false> out;
	out.append_sprintf("ListenPort=%d\n", ListenPort);
	out.append_sprintf("DevMode=%d\n", DevMode);
	out.append_sprintf("TraceNetwork=%d\n", TraceNetwork);
	out.append_sprintf("LobbyMap=%d\n", LobbyMap);

	bool r = fileSaveBuff(CONFIG_PATH, out.data(), out.size());
	if(!r) {
		LOG("WARNING(SaveConfigFile): failed to save '%s'", CONFIG_PATH);
		return false;
	}
	return true;
}

void CConfigHub::Print() const
{
	LOG("Config = {");
	LOG("	ListenPort=%d", ListenPort);
	LOG("	DevMode=%d", DevMode);
	LOG("	TraceNetwork=%d", TraceNetwork);
	LOG("	LobbyMap=%d", LobbyMap);
	LOG("}");
}

static CConfigHub* g_Config = nullptr;

const CConfigHub& Config()
{
	return *g_Config;
}

CConfigHub& ConfigMutable()
{
	return *g_Config;
}

bool LoadConfig()
{
	static CConfigHub cfg;
	g_Config = &cfg;
	return g_Config->LoadConfigFile();
}

bool SaveConfig()
{
	return g_Config->SaveConfigFile();
}
