#pragma once
#include <common/base.h>
#include <common/protocol.h>
#include <mxm/account_progress.h>
#include <mxm/game_content.h>
#include <mxm/item_writer.h>
#include <mxm/achievements.h>

struct Account;

static const u16 MEASURED_CQ_WEAPON_UNLOCK = 60058;
static const u16 MEASURED_CQ_WEAPON_EQUIP = 60059;
static const u16 MEASURED_CQ_SKILL_UNLOCK = 60070;
static const u16 MEASURED_CQ_SKILL_EQUIP = 60071;
static const u16 MEASURED_CQ_SKILL_UPGRADE = 60054;
static const u16 MEASURED_CQ_WEAPON_UPGRADE = 60104;
static const u8 BUY_TYPE_GOLD = 1;
static const u8 BUY_TYPE_CCOIN = 2;

struct ArsenalResult
{
	u16 answer = 0;
	u32 result = 1;
	u32 characterID = 0;
	u32 type = 0;
	i32 weaponIndex = -1;
	i32 grade = 0;
	u8 unlocked = 0;
	i32 skill = -1;
	u8 spentType = 0;
	i64 wallet = 0;
	i32 moneyReason = 0;
	u32 itemReason = 0;
	bool unlockedOnEquip = false;
	u32 skillSlot = 0;
	AchieveEvent achieve;
	eastl::fixed_vector<AccountItem,16,false> items;
};

bool IsArsenalPacket(u16 netID);
bool ArsenalHandle(Account& account, u16 netID, const u8* packetData, i32 packetSize, ArsenalResult* out);
i32 ArsenalWeaponIndex(const GameXmlContent::Master& master, u32 type, u32 grade);
i32 ArsenalEquippedWeapon(const AccountArsenal& arsenal, const GameXmlContent::Master& master, i32* grade);
void ArsenalSkills(const AccountArsenal& arsenal, const GameXmlContent::Master& master, SkillID* out);

template<typename Writer>
void WriteSkillProperties(Writer& packet, i32 skill, i32 level)
{
	const GameXmlContent::SkillUpgradeList* ups = GetGameXmlContent().FindSkillUpgrades(skill);
	const i32 count = ups ? MIN(level, (i32)ups->size()) : 0;
	packet.template Write<u16>((u16)count);
	for(i32 i = 0; i < count; i++) {
		packet.template Write<i32>((*ups)[i].propertyID);
		packet.template Write<u32>(1);
	}
}

template<typename Send>
void ArsenalSend(const ArsenalResult& r, Send send)
{
	if(r.spentType == BUY_TYPE_GOLD) {
		PacketWriter<Sv::SN_Money,32> money;
		money.Write<i64>(r.wallet);
		money.Write<i32>(r.moneyReason);
		send(money);
	}
	else if(r.spentType == BUY_TYPE_CCOIN) {
		PacketWriter<Sv::SN_CCoin,32> coin;
		coin.Write<i64>(r.wallet);
		coin.Write<u32>((u32)r.moneyReason);
		send(coin);
	}
	if(!r.items.empty()) {
		PacketWriter<Sv::SN_ItemUpdate,1024> update;
		update.Write<u16>((u16)r.items.size());
		foreach_const(it, r.items) WriteInventoryItem(update, *it);
		update.Write<u32>(r.itemReason);
		send(update);
	}
	if(r.answer == MEASURED_CQ_SKILL_UNLOCK || (r.answer == MEASURED_CQ_SKILL_EQUIP && r.unlockedOnEquip)) {
		PacketWriter<Sv::SA_SkillUnlock,64> packet;
		packet.Write<u32>(r.answer == MEASURED_CQ_SKILL_UNLOCK ? r.result : 0);
		packet.Write<u16>(1);
		packet.Write<u32>(r.characterID);
		packet.Write<i32>(r.skill);
		packet.Write<u8>(1);
		packet.Write<u8>(1);
		packet.Write<u16>(0);
		send(packet);
		if(r.answer == MEASURED_CQ_SKILL_UNLOCK) return;
	}
	if(r.answer == MEASURED_CQ_SKILL_UPGRADE) {
		PacketWriter<Sv::SA_SkillUpgrade,256> packet;
		packet.Write<u32>(r.result);
		packet.Write<u32>(r.characterID);
		packet.Write<u32>(r.characterID);
		packet.Write<i32>(r.skill);
		packet.Write<u8>(1);
		packet.Write<u8>(1);
		WriteSkillProperties(packet, r.skill, r.grade);
		send(packet);
		return;
	}
	if(r.answer == MEASURED_CQ_SKILL_EQUIP) {
		PacketWriter<Sv::SA_SkillSelect,64> packet;
		packet.Write<u32>(r.characterID);
		packet.Write<u32>(r.result);
		packet.Write<i32>(r.skill);
		packet.Write<u32>(r.skillSlot);
		send(packet);
		return;
	}
	auto weapon = [&r](auto& packet, u32 result) {
		packet.template Write<u32>(result);
		packet.template Write<u32>(r.characterID);
		packet.template Write<u32>(r.type);
		packet.template Write<i32>(r.weaponIndex);
		packet.template Write<i32>(r.grade);
		packet.template Write<u8>(r.unlocked);
		packet.template Write<u8>(1);
	};
	if(r.answer == MEASURED_CQ_WEAPON_EQUIP) {
		if(r.unlockedOnEquip) {
			PacketWriter<Sv::SA_WeaponUnlock,64> unlock;
			weapon(unlock, 0);
			send(unlock);
		}
		PacketWriter<Sv::SA_WeaponEquip,64> packet;
		packet.Write<u32>(r.result);
		packet.Write<u32>(r.characterID);
		packet.Write<u32>(r.type);
		packet.Write<i32>(r.weaponIndex);
		send(packet);
		return;
	}
	if(r.answer == MEASURED_CQ_WEAPON_UNLOCK) {
		PacketWriter<Sv::SA_WeaponUnlock,64> packet;
		weapon(packet, r.result);
		send(packet);
	}
	else if(r.answer == MEASURED_CQ_WEAPON_UPGRADE) {
		PacketWriter<Sv::SA_WeaponUpgrade,64> packet;
		weapon(packet, r.result);
		send(packet);
	}
}
