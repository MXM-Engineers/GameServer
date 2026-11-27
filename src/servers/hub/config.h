#pragma once
#include <common/base.h>

struct CConfigHub
{
	i32 ListenPort = 11900;
	i32 DevMode = false;
	i32 DevQuickConnect = false;
	i32 TraceNetwork = false;
	i32 LobbyMap = 160000042; // TODO: restore
	char AccountsDatabasePath[256] = "accounts.db";
	char GameDatabasePath[256] = "game.db";
	i32 RequireLoginSession = true;
	i32 DevSeedProfile = false;
	eastl::fixed_vector<i32,16> serviceRuleDefaultUnlockClass = { 35, 3 };
	i32 ServiceRuleDefaultMoney = 1000;
	i32 ServiceRuleDefaultMoneyBM = 30;
	i32 ServiceRuleDefaultGearIndex = 132199998;
	i32 ServiceRuleDefaultGearCount = 2;
	i32 ItemBasicDefaultGearOptionGroup = 723002202;
	eastl::fixed_vector<i32,16> serviceRuleDefaultLineCount = { 4, 4, 4 };
	i32 ServiceRuleWarehouseDefaultLineCount = 1;
	eastl::fixed_vector<i32,16> servicePriceInvenUnlockGold = { 1800, 3500 };
	i32 FixedDataJukeboxCoinIndex = 137120001;
	eastl::fixed_vector<i32,16> regionBanMaster = {};
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
