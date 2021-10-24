#include "config.h"
#include <EASTL/fixed_string.h>
#include <EAStdC/EASprintf.h>

bool CConfigGame::ParseLine(const char* line)
{
	if(EA::StdC::Sscanf(line, "ListenPort=%d", &ListenPort) == 1) return true;
	if(EA::StdC::Sscanf(line, "DevMode=%d", &DevMode) == 1) return true;
	if(EA::StdC::Sscanf(line, "DevQuickConnect=%d", &DevQuickConnect) == 1) return true;
	if(EA::StdC::Sscanf(line, "TraceNetwork=%d", &TraceNetwork) == 1) return true;
	if(EA::StdC::Sscanf(line, "WindowWidth=%d", &WindowWidth) == 1) return true;
	if(EA::StdC::Sscanf(line, "WindowHeight=%d", &WindowHeight) == 1) return true;

	if(EA::StdC::Sscanf(line, "DbgCamPosX=%f", &DbgCamPosX) == 1) return true;
	if(EA::StdC::Sscanf(line, "DbgCamPosY=%f", &DbgCamPosY) == 1) return true;
	if(EA::StdC::Sscanf(line, "DbgCamPosZ=%f", &DbgCamPosZ) == 1) return true;
	if(EA::StdC::Sscanf(line, "DbgCamDirX=%f", &DbgCamDirX) == 1) return true;
	if(EA::StdC::Sscanf(line, "DbgCamDirY=%f", &DbgCamDirY) == 1) return true;
	if(EA::StdC::Sscanf(line, "DbgCamDirZ=%f", &DbgCamDirZ) == 1) return true;
	return false;
}

// returns false on failing to open the config file
bool CConfigGame::LoadConfigFile()
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

bool CConfigGame::SaveConfigFile()
{
	eastl::fixed_string<char,4096,false> out;
	out.append_sprintf("ListenPort=%d\n", ListenPort);
	out.append_sprintf("DevMode=%d\n", DevMode);
	out.append_sprintf("DevQuickConnect=%d\n", DevQuickConnect);
	out.append_sprintf("TraceNetwork=%d\n", TraceNetwork);
	out.append_sprintf("WindowWidth=%d\n", WindowWidth);
	out.append_sprintf("WindowHeight=%d\n", WindowHeight);
	out.append_sprintf("DbgCamPosX=%f\n", DbgCamPosX);
	out.append_sprintf("DbgCamPosY=%f\n", DbgCamPosY);
	out.append_sprintf("DbgCamPosZ=%f\n", DbgCamPosZ);
	out.append_sprintf("DbgCamDirX=%f\n", DbgCamDirX);
	out.append_sprintf("DbgCamDirY=%f\n", DbgCamDirY);
	out.append_sprintf("DbgCamDirZ=%f\n", DbgCamDirZ);

	bool r = fileSaveBuff(CONFIG_PATH, out.data(), out.size());
	if(!r) {
		LOG("WARNING(SaveConfigFile): failed to save '%s'", CONFIG_PATH);
		return false;
	}
	return true;
}

void CConfigGame::Print() const
{
	LOG("Config = {");
	LOG("	ListenPort=%d", ListenPort);
	LOG("	DevMode=%d", DevMode);
	LOG("	DevQuickConnect=%d", DevQuickConnect);
	LOG("	TraceNetwork=%d", TraceNetwork);
	LOG("	WindowWidth=%d", WindowWidth);
	LOG("	WindowHeight=%d", WindowHeight);
	LOG("	DbgCamPosX=%f", DbgCamPosX);
	LOG("	DbgCamPosY=%f", DbgCamPosY);
	LOG("	DbgCamPosZ=%f", DbgCamPosZ);
	LOG("	DbgCamDirX=%f", DbgCamDirX);
	LOG("	DbgCamDirY=%f", DbgCamDirY);
	LOG("	DbgCamDirZ=%f", DbgCamDirZ);
	LOG("}");
}

static CConfigGame* g_Config = nullptr;

const CConfigGame& Config()
{
	return *g_Config;
}

CConfigGame& ConfigMutable()
{
	return *g_Config;
}

bool LoadConfig()
{
	static CConfigGame cfg;
	g_Config = &cfg;
	return g_Config->LoadConfigFile();
}

bool SaveConfig()
{
	return g_Config->SaveConfigFile();
}
