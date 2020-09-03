# server packets
from . import common
import zlib

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)

class ServerSerializer:

    

    def serialize_62002(netid, data):
        p = common.PacketReader(data)

        print('SA_FirstHello {')
        print('    dwProtocolCRC=%#x' % p.read_u32())
        print('    dwErrorCRC=%#x' % p.read_u32())
        print('    serverType=%d' % p.read_u8())
        print('    clientIp=(%d.%d.%d.%d)' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8()))
        print('    clientPort=%d' % p.read_u16())
        print('    tqosWorldId=%d' % p.read_u8())
        print('}')

    def serialize_62005(netid, data):
        p = common.PacketReader(data)

        print('SA_AuthResult {')
        print('    result=%d' % p.read_i32())
        print('}')

    def serialize_62006(netid, data):
        p = common.PacketReader(data)

        print('SN_RegionServicePolicy {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % (p.read_u8())
            count -= 1
        
        print('    newMasterRestrict[%s]' % s)

        count = p.read_u16()
        s = ''
        while count > 0:
            user_grade = p.read_u8()
            benefit_count = p.read_u16()
            benefits = ''
            while benefit_count > 0:
                benefits += '%d, ' % p.read_u8()
                benefit_count -= 1

            s += '(userGrade=%d, benefits=[%s]), ' % (user_grade, benefits)
            count -= 1
        
        print('    userGradePolicy[%s]' % s)
        print('    purchaseCCoinMethod=%d' % p.read_u8())
        print('    exchangeCCoinForGoldMethod=%d' % p.read_u8())
        print('    rewardCCoinMethod=%d' % p.read_u8())
        print('    pveRewardSlotOpenBuyChanceMethod=%d' % p.read_u8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % (p.read_i32())
            count -= 1
        print('    regionBanMaster=[%s]' % s)

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % (p.read_i32())
            count -= 1
        print('    eventBanMaster=[%s]' % s)

        print('    checkPeriodSec=%d' % p.read_i32())
        print('    maxTalkCount=%d' % p.read_i32())
        print('    blockPeriodSec=%d' % p.read_i32())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(%d %d), ' % (p.read_i32(), p.read_i32())
            count -= 1
        print('    regionBanSkinList=[%s]' % s)

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(%d %d), ' % (p.read_i32(), p.read_i32())
            count -= 1
        print('    pcCafeSkinList=[%s]' % s)
        print('    useFatigueSystem=%d' % p.read_u8())

        print('}')

    def serialize_62007(netid, data):
        p = common.PacketReader(data)

        print('SN_StationList {')

        count = p.read_u16()
        s = ''
        while count > 0:
            idc = p.read_i32()
            stations = ''
            station_count = p.read_u16()
            while station_count > 0:
                stations += 'gameServerIp=(%d.%d.%d.%d) pingServerIp=(%d.%d.%d.%d) port=%d' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u16)
                station_count = -1

            s += '(idc=%d stations=[%s])' % (idc, stations)
            count -= 1
        
        print('    stationList[%s]' % s)

        print('}')

    def serialize_62018(netid, data):
        p = common.PacketReader(data)

        print('SA_returnToCity {')
        print('    errCode=%d' % p.read_i32())
        print('}')

    def serialize_62025(netid, data):
        p = common.PacketReader(data)

        print('SN_GameCreateActor {')
        print('    objectID=%d' % p.read_i32())
        print('    nType=%d' % p.read_i32())
        print('    nIDX=%d' % p.read_i32())
        print('    dwLocalID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    spawnType=%d' % p.read_i32())
        print('    actionState=%d' % p.read_i32())
        print('    ownerID=%d' % p.read_i32())
        print('    bDirectionToNearPC=%d' % p.read_u8())
        print('    AiWanderDistOverride=%d' % p.read_i32())
        print('    tagID=%d' % p.read_i32())
        print('    faction=%d' % p.read_i32())
        print('    classType=%d' % p.read_i32())
        print('    skinIndex=%d' % p.read_i32())
        print('    seed=%d' % p.read_i32())

        print('    initStat={')
        
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(type=%d value=%g), ' % (p.read_u8(), p.read_f32())
            count -= 1

        print('      maxStats=[%s]' % s)

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(type=%d value=%g), ' % (p.read_u8(), p.read_f32())
            count -= 1

        print('      curStats=[%s]' % s)
        print('    }')
        print('    isInSight=%d' % p.read_u8())
        print('    isDead=%d' % p.read_u8())
        print('    serverTime=%#x' % p.read_i64())
        print('    meshChangeActionHistory_count=%d' % p.read_u16())

        print('}')

    def serialize_62026(netid, data):
        p = common.PacketReader(data)

        print('SN_SpawnPosForMinimap {')
        print('    objectID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('}')

    def serialize_62033(netid, data):
        p = common.PacketReader(data)

        print('SQ_CityLobbyJoinCity {}')

    def serialize_62048(netid, data):
        p = common.PacketReader(data)

        print('SN_PlayerSkillSlot {')
        print('    characterID=%d' % p.read_i32())
        print('    slots=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      skillIndex=%d' % p.read_i32())
            print('      coolTime=%d' % p.read_i32())
            print('      unlocked=%d' % p.read_u8())

            prop_count = p.read_u16()
            s = ''
            while prop_count > 0:
                s += '(skillPropertyIndex=%d level=%d), ' % (p.read_i32(), p.read_i32())
                prop_count -= 1

            print('      propList=[%s]' % s)
            print('      isUnlocked=%d' % p.read_u8())
            print('      isActivated=%d' % p.read_u8())
            print('    },')
            count -= 1

        print('    stageSkillIndex1=%d' % p.read_i32())
        print('    stageSkillIndex2=%d' % p.read_i32())
        print('    currentSkillSlot1=%d' % p.read_i32())
        print('    currentSkillSlot2=%d' % p.read_i32())
        print('    shirkSkillSlot=%d' % p.read_i32())
        print('}')

    def serialize_62050(netid, data):
        p = common.PacketReader(data)

        print('SN_LoadCharacterStart {}')

    def serialize_62051(netid, data):
        p = common.PacketReader(data)

        print('SN_ScanEnd {}')

    def serialize_62052(netid, data):
        p = common.PacketReader(data)

        print('SN_GamePlayerSyncByInt {')
        print('    playerID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDirection=%s' % read_Vec3(p))
        print('    p3nEye=%s' % read_Vec3(p))
        print('    nRotate=%g' % p.read_f32())
        print('    nSpeed=%g' % p.read_f32())
        print('    nState=%d' % p.read_i32())
        print('    nActionIDX=%d' % p.read_i32())
        print('}')

    def serialize_62057(netid, data):
        p = common.PacketReader(data)

        print('SN_Money {')
        print('    money=%d' % p.read_i64())
        print('    nReason=%d' % p.read_i32())
        print('}')

    def serialize_62059(netid, data):
        p = common.PacketReader(data)

        print('SN_DestroyEntity {')
        print('    objectID=%d' % p.read_i32())
        print('}')

    def serialize_62060(netid, data):
        p = common.PacketReader(data)

        print('SN_SetGameGvt {')
        print('    sendTime=%d' % p.read_i32())
        print('    virtualTime=%d' % p.read_i32())
        print('}')

    def serialize_62064(netid, data):
        p = common.PacketReader(data)

        print('SN_LoadClearedStages {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    clearedStageList=[%s]' % s)
        print('}')

    def serialize_62084(netid, data):
        p = common.PacketReader(data)

        print('SN_GamePlayerEquipWeapon {')
        print('    characterID=%d' % p.read_i32())
        print('    weaponDocIndex=%d' % p.read_i32())
        print('    additionnalOverHeatGauge=%d' % p.read_i32())
        print('    additionnalOverHeatGaugeRatio=%d' % p.read_i32())
        print('}')

    def serialize_62089(netid, data):
        p = common.PacketReader(data)

        print('SN_GamePlayerStock {')
        print('    playerID=%d' % p.read_i32())
        print('    name="%s"' % p.read_wstr())
        print('    class=%d' % p.read_i32())
        print('    displayTitleIDX=%d' % p.read_i32())
        print('    statTitleIDX=%d' % p.read_i32())
        print('    badgeType=%d' % p.read_u8())
        print('    badgeTierLevel=%d' % p.read_u8())
        print('    guildTag="%s"' % p.read_wstr())
        print('    vipLevel=%d' % p.read_u8())
        print('    staffType=%d' % p.read_u8())
        print('    isSubstituted=%d' % p.read_u8())
        print('}')

    def serialize_62090(netid, data):
        p = common.PacketReader(data)

        print('SN_PlayerStateInTown {')
        print('    playerID=%d' % p.read_i32())
        print('    playerStateInTown=%d' % p.read_i8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_u8()
            count -= 1

        print('    matchingGameModes=[%s]' % s)

        print('}')

    def serialize_62091(netid, data):
        p = common.PacketReader(data)

        print('SN_CityMapInfo {')
        print('    cityMapID=%d' % p.read_i32())
        print('}')

    def serialize_62097(netid, data):
        p = common.PacketReader(data)

        print('SN_SummaryInfoAll {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    listOfFinishedSummary=[%s]' % s)
        print('}')

    def serialize_62098(netid, data):
        p = common.PacketReader(data)

        print('SN_AvailableSullaryRewardCountList {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    rewardCountList=[%s]' % s)
        print('}')

    def serialize_62100(netid, data):
        p = common.PacketReader(data)

        print('SN_AchieveInfo {')
        print('    packetNum=%d' % p.read_u8())
        print('    achievementScore=%d' % p.read_i32())
        print('    achieveList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      index=%d' % p.read_i32())
            print('      type=%d' % p.read_i32())
            print('      isCleared=%d' % p.read_u8())

            list_count = p.read_u16()
            s = ''
            while list_count > 0:
                s += '%d, ' % p.read_i32()
                list_count -= 1

            print('      achievedList=[%s]' % s)
            print('      progressInt64=%d' % p.read_i64())
            print('      date=%d' % p.read_i64())
            print('    },')
            count -= 1
        
        print('    ]')
        print('}')

    def serialize_62101(netid, data):
        p = common.PacketReader(data)

        print('SN_AchieveLatest {')
        print('    achieveList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      index=%d' % p.read_i32())
            print('      type=%d' % p.read_i32())
            print('      isCleared=%d' % p.read_u8())

            list_count = p.read_u16()
            s = ''
            while list_count > 0:
                s += '%d, ' % p.read_i32()
                list_count -= 1

            print('      achievedList=[%s]' % s)
            print('      progressInt64=%d' % p.read_i64())
            print('      date=%d' % p.read_i64())
            print('    },')
            count -= 1
        
        print('    ]')
        print('}')
        

    def serialize_62102(netid, data):
        p = common.PacketReader(data)

        print('SN_AchieveUpdate {')
        print('    achievementScore=%d' % p.read_i32())
        print('    achieve={')
        print('        index=%d' % p.read_i32())
        print('        type=%d' % p.read_i32())
        print('        isCleared=%d' % p.read_u8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_u8()
            count -= 1

        print('        achievedList=[%s]' % s)
        print('      }')
        print('    progressInt64=%d' % p.read_i64())
        print('    date=%d' % p.read_i64())
        print('}')

    def serialize_62103(netid, data):
        p = common.PacketReader(data)

        print('SN_MissionList {')
        print('    missions=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      index=%d' % p.read_i32())
            print('      progessCount=%d' % p.read_i64())
            print('      changedCount=%d' % p.read_u16())
            print('      isCleared=%d' % p.read_u8())
            print('      isRewarded=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')
        print('    dailyMissionRemainTimeInMs=%d' % p.read_i32())
        print('    weeklyMissionRemainTimeInMs=%d' % p.read_i32())
        print('    monthlyMissionRemainTimeInMs=%d' % p.read_i32())
        print('}')

    def serialize_62106(netid, data):
        p = common.PacketReader(data)

        print('SN_AccountInfo {')
        print('    nick="%s"' % p.read_wstr())
        print('    inventoryLineCountTab0=%d' % p.read_i32())
        print('    inventoryLineCountTab1=%d' % p.read_i32())
        print('    inventoryLineCountTab2=%d' % p.read_i32())
        print('    displayTitlteIndex=%d' % p.read_i32())
        print('    statTitleIndex=%d' % p.read_i32())
        print('    warehouseLineCount=%d' % p.read_i32())
        print('    tutorialState=%d' % p.read_i32())
        print('    masterGearDurability=%d' % p.read_i32())
        print('    badgeType=%d' % p.read_u8())
        print('}')

    def serialize_62107(netid, data):
        p = common.PacketReader(data)

        print('SN_AccountExtraInfo {')
        print('    userGradeList=[')

        count = p.read_u16()
        s = ''
        while count > 0:
            print('    {')
            print('      userGrade=%d' % p.read_u8())
            print('      activated=%d' % p.read_u8())
            print('      expireDateTime64=%d' % p.read_i64())
            print('      level=%d' % p.read_u8())
            print('      point=%d' % p.read_u16())
            print('      nextPoint=%d' % p.read_u16())
            print('    },')
            count -= 1
        
        print('    ]')
        print('    activityPoint=%d' % p.read_i32())
        print('    activityRewaredState=%d' % p.read_u8())
        print('}')

    def serialize_62109(netid, data):
        p = common.PacketReader(data)

        print('SN_AllCharacterBaseData {')
        print('    characters=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('    docIndex=%d' % p.read_i32())

            base_count = p.read_u16()
            s = ''
            while base_count > 0:
                s += '(type=%d value=%g), ' % (p.read_u8(), p.read_f32())
                base_count -= 1

            print('    baseStats=[%s]' % s)

            skill_count = p.read_u16()
            s = ''
            while skill_count > 0:
                s += '(skillIndex=%d baseDamage=%g attMultiplier=%g healMultiplier=%g shieldMultiplier=%g dotMultiplier=%g), ' % (p.read_i32(), p.read_f32(), p.read_f32(), p.read_f32(), p.read_f32(), p.read_f32())
                skill_count -= 1

            print('    skillData=[%s]' % s)
            print('    }')
            count -= 1

        print('    cur=%d' % p.read_i32())
        print('    max=%d' % p.read_i32())
        print('}')

    def serialize_62113(netid, data):
        p = common.PacketReader(data)

        print('SA_GetCharacterInfo {')
        print('    characterID=%d' % p.read_i32())
        print('    docIndex=%d' % p.read_i32())
        print('    class=%d' % p.read_i32())
        print('    hp=%d' % p.read_i32())
        print('    maxHp=%d' % p.read_i32())
        print('}')

    def serialize_62123(netid, data):
        p = common.PacketReader(data)

        print('SN_LeaderCharacter {')
        print('    leaderID=%d' % p.read_i32())
        print('    skinIndex=%d' % p.read_i32())
        print('}')

    def serialize_62124(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileCharacters {')
        print('    characters=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      characterID=%d' % p.read_i32())
            print('      creatureIndex=%d' % p.read_i32())
            print('      skillShot1=%d' % p.read_i32())
            print('      skillShot2=%d' % p.read_i32())
            print('      class=%d' % p.read_i32())
            print('      x=%g' % p.read_f32())
            print('      y=%g' % p.read_f32())
            print('      z=%g' % p.read_f32())
            print('      characterType=%d' % p.read_i32())
            print('      skinIndex=%d' % p.read_i32())
            print('      weaponIndex=%d' % p.read_i32())
            print('      masterGearNo=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62125(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileItems {')
        print('    packetNum=%d' % p.read_u8())
        print('    items=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      itemID=%d' % p.read_i32())
            print('      invenType=%d' % p.read_u8())
            print('      slot=%d' % p.read_i32())
            print('      itemIndex=%d' % p.read_i32())
            print('      count=%d' % p.read_i32())
            print('      propertyGroupIndex=%d' % p.read_i32())
            print('      isLifeTimeAbsolute=%d' % p.read_u8())
            print('      lifeEndTimeUTC=%d' % p.read_i64())

            prop_count = p.read_u16()
            s = ''
            while prop_count > 0:
                s += '(type=%d typeDetail=%d valueType=%d value=%g fixed=%d), ' % (p.read_u8(), p.read_i32(), p.read_u8(), p.read_f32(), p.read_u8())
                prop_count -= 1

            print('      properties=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62126(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileWeapons {')
        print('    weapons=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      characterID=%d' % p.read_i32())
            print('      weaponType=%d' % p.read_i32())
            print('      weaponIndex=%d' % p.read_i32())
            print('      grade=%d' % p.read_i32())
            print('      isUnlocked=%d' % p.read_u8())
            print('      isActivated=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62127(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileSkills {')
        print('    packetNum=%d' % p.read_u8())
        print('    skills=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      characterID=%d' % p.read_i32())
            print('      skillIndex=%d' % p.read_i32())
            print('      isUnlocked=%d' % p.read_u8())
            print('      isActivated=%d' % p.read_u8())

            prop_count = p.read_u16()
            s = ''
            while prop_count > 0:
                s += '(skillIndex=%d level=%d), ' % (p.read_i32(), p.read_i32())
                prop_count -= 1

            print('      properties=[%s]' % s)

            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62128(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileTitles {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    titles=[%s]' % s)
        print('}')

    def serialize_62129(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileMasterGears {')
        print('    masterGears=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      masterGearNo=%d' % p.read_u8())
            print('      name=%s' % p.read_wstr())

            slot_count = p.read_u16()
            s = ''
            while slot_count > 0:
                s += '(gearType=%d gearItemID=%d), ' % (p.read_i32(), p.read_i32())
                slot_count -= 1
            
            print('      slots=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62158(netid, data):
        p = common.PacketReader(data)

        print('SA_EnqueueGame {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62173(netid, data):
        p = common.PacketReader(data)

        print('SA_AreaPopularity {')
        print('    errCode=%d' % p.read_i32())
        print('}')

    def serialize_62174(netid, data):
        p = common.PacketReader(data)

        print('SN_AreaPopularity {')
        print('    areaIndex=%d' % p.read_i32())
        print('    popularityDatas=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      stageIndex=%d' % p.read_i32())
            print('      gameType=%d' % p.read_i32())
            print('      popularityLevel=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62175(netid, data):
        p = common.PacketReader(data)

        print('SA_PartyCreate {')
        print('    retval=%d' % p.read_i32())
        print('    ownerUserId=%d' % p.read_i32())
        print('    stageType=%d' % p.read_i32())
        print('}')

    def serialize_62176(netid, data):
        p = common.PacketReader(data)

        print('SA_PartyModify {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62177(netid, data):
        p = common.PacketReader(data)

        print('SA_PartyOptionModify {')
        print('    retval=%d' % p.read_i32())
        print('    partyOptionType=%d' % p.read_u8())
        print('    enable=%d' % p.read_u8())
        print('}')

    def serialize_62191(netid, data):
        p = common.PacketReader(data)

        print('SA_partyAddBot {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62201(netid, data):
        p = common.PacketReader(data)

        print('SN_EnqueueMatchingQueue {')
        print('    stageIndex=%d' % p.read_i32())
        print('    currentMatchingTimeMS=%d' % p.read_i32())
        print('    avgMatchingTimeMS=%d' % p.read_i32())
        print('    disableMatchExpansion=%d' % p.read_u8())
        print('    isMatchingExpanded=%d' % p.read_u8())
        print('}')

    def serialize_62204(netid, data):
        p = common.PacketReader(data)

        print('SQ_MatchingPartyFound {')
        print('    sortieID=%d' % p.read_i64())
        print('    stageIndex=%d' % p.read_i32())
        print('    gameType=%d' % p.read_i32())
        print('    gameDefinitionType=%d' % p.read_i32())
        print('    stageRule=%d' % p.read_i32())

        print('    m_allies=[')
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=%d' % p.read_i32())
            print('      nickname=%s' % p.read_wstr())
            print('      isBot=%d' % p.read_u8())
            print('      tier=%d' % p.read_i32())
            print('      tierGroupRanking=%d' % p.read_i32())
            print('      tierSeriesFlag=%d' % p.read_i32())
            print('      pvpRate=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

        print('    m_enemies=[')
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=%d' % p.read_i32())
            print('      nickname=%s' % p.read_wstr())
            print('      isBot=%d' % p.read_u8())
            print('      tier=%d' % p.read_i32())
            print('      tierGroupRanking=%d' % p.read_i32())
            print('      tierSeriesFlag=%d' % p.read_i32())
            print('      pvpRate=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

        print('    m_spectator=[')
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=%d' % p.read_i32())
            print('      nickname=%s' % p.read_wstr())
            print('      isBot=%d' % p.read_u8())
            print('      tier=%d' % p.read_i32())
            print('      tierGroupRanking=%d' % p.read_i32())
            print('      tierSeriesFlag=%d' % p.read_i32())
            print('      pvpRate=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

        print('    timeToWaitInSec=%d' % p.read_i32())
        print('    elementMain=%d' % p.read_u8())
        print('    elementSub=%d' % p.read_u8())
        print('}')

    def serialize_62208(netid, data):
        p = common.PacketReader(data)

        print('SA_masterPick {')
        print('    retval=%d' % p.read_i32())
        print('    characterID=%d' % p.read_i32())
        print('}')

    def serialize_62214(netid, data):
        p = common.PacketReader(data)
        print('SN_ReadySortieRoom {')
        print('    userId=%d' % p.read_i32())
        print('    ready=%d' % p.read_u8())
        print('}')

    def serialize_62217(netid, data):
        p = common.PacketReader(data)
        print('SN_StartCountdownSortieRoom {')
        print('    stageType=%d' % p.read_i32())
        print('    timeToWaitSec=%d' % p.read_i32())
        print('}')

    def serialize_62227(netid, data):
        p = common.PacketReader(data)

        print('SN_SummaryInfoLatest {')
        print('    stateUpdatedTime=[')
          
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      summaryIndex=%d' % p.read_i32())
            print('      stageIndex=%d' % p.read_i32())
            print('      summaryType=%d' % p.read_i32())
            print('      summaryData=%d' % p.read_i32())
            print('      rewardReceived=%d' % p.read_u8())
            print('      completedDate=%d' % p.read_i64())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62242(netid, data):
        p = common.PacketReader(data)

        print('SN_ChatChannelMessage {')
        print('    chatType=%d' % p.read_i32())
        print('    senderNickname="%s"' % p.read_wstr())
        print('    senderStaffType=%d' % p.read_u8())
        print('    chatMsg="%s"' % p.read_wstr())
        print('}')

    def serialize_62251(netid, data):
        p = common.PacketReader(data)

        print('SN_FriendStateChanged {')
        print('    nickname="%s"' % p.read_wstr())
        print('    newState=%d' % p.read_u8())
        print('    isOnLeavePenalty=%d' % p.read_u8())
        print('    stateUpdatedTime=%d' % p.read_i64())
        print('}')

    def serialize_62257(netid, data):
        p = common.PacketReader(data)

        print('SN_FriendList {')
        print('    friends=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      name="%s"' % p.read_wstr())
            print('      leaderCreatureIndex=%d' % p.read_i32())
            print('      state=%d' % p.read_u8())
            print('      level=%d' % p.read_u16())
            print('      isFavorite=%d' % p.read_u8())
            print('      isComrade=%d' % p.read_u8())
            print('      isOnLeavePenalty=%d' % p.read_u8())
            print('      lastLogoutDate=%d' % p.read_i64())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62258(netid, data):
        p = common.PacketReader(data)

        print('SN_FriendRequestList {')
        print('    friendRequestList=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      name="%s"' % p.read_wstr())
            print('      leaderCreatureIndex=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62259(netid, data):
        p = common.PacketReader(data)

        print('SN_MutualFriendList {')
        print('    candidates=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      name="%s"' % p.read_wstr())
            print('      leaderCreatureIndex=%d' % p.read_i32())
            print('      mutualCount=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62260(netid, data):
        p = common.PacketReader(data)

        print('SN_RecommendedFriendList {')
        print('    candidates=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      nickname="%s"' % p.read_wstr())
            print('      leaderCreatureIndex=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62261(netid, data):
        p = common.PacketReader(data)

        print('SN_BlockList {')
        print('    blocks=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      name="%s"' % p.read_wstr())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62274(netid, data):
        p = common.PacketReader(data)

        print('SN_PVPAvailableReportCount {')
        print('    isTroll=%d' % p.read_u8())
        print('    remainingCount=%d' % p.read_u8())
        print('}')


    def serialize_62276(netid, data):
        p = common.PacketReader(data)

        print('SN_NotifyAasRestricted {')
        print('    isRestrictedByAAS=%d' % p.read_u8())
        print('}')

    def serialize_62278(netid, data):
        p = common.PacketReader(data)

        print('SN_Exp {')
        print('    leaderCsn=%d' % p.read_i32())
        print('    nickname="%s"' % p.read_wstr())
        print('    isLevelUp=%d' % p.read_u8())
        print('    level=%d' % p.read_u16())
        print('    currentLevelExp=%d' % p.read_i32())
        print('}')

    def serialize_62282(netid, data):
        p = common.PacketReader(data)

        print('SN_JukeboxEnqueuedList {')
        print('    trackList=[')

        track_count = p.read_u16()
        while track_count > 0:
            print('    {')
            print('      trackId=%d' % p.read_i32())
            print('      nickname="%s"' % p.read_wstr())
            print('    },')
            track_count -= 1
        print('    ]')
        print('}')

    def serialize_62283(netid, data):
        p = common.PacketReader(data)

        print('SN_JukeboxPlay {')
        print('    result=%d' % p.read_i32())
        print('    track={')
        print('      trackId=%d' % p.read_i32())
        print('      nickname="%s"' % p.read_wstr())
        print('    }')
        print('    playPositionSec=%d' % p.read_u16())
        print('}')

    def serialize_62284(netid, data):
        p = common.PacketReader(data)

        print('SN_JukeboxHotTrackList {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    trackList=[%s]' % s)
        print('}')

    def serialize_62285(netid, data):
        p = common.PacketReader(data)

        print('SN_TownHudStatistics {')
        print('    gameModeType=%d' % p.read_u8())
        print('    gameType=%d' % p.read_u8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    argList=[%s]' % s)
        print('}')
        
    def serialize_62299(netid, data):
        p = common.PacketReader(data)

        print('SA_GetGuildProfile {')
        print('    result=%d' % p.read_i32())
        print('    guildName="%s"' % p.read_wstr())
        print('    guildTag="%s"' % p.read_wstr())
        print('    emblemIndex=%d' % p.read_i32())
        print('    guildLvl=%d' % p.read_u8())
        print('    memberMax=%d' % p.read_u8())
        print('    ownerNickname="%s"' % p.read_wstr())
        print('    createdDate=%d' % p.read_i64())
        print('    dissolutionDate=%d' % p.read_i64())
        print('    joinType=%d' % p.read_u8())
        print('    guildInterest={')
        print('      likePveStage=%d' % p.read_u8())
        print('      likeDefence=%d' % p.read_u8())
        print('      likePvpNormal=%d' % p.read_u8())
        print('      likePvpOccupy=%d' % p.read_u8())
        print('      likePvpGot=%d' % p.read_u8())
        print('      likePvpRank=%d' % p.read_u8())
        print('      likeOlympic=%d' % p.read_u8())
        print('    }')
        print('    guildIntro="%s"' % p.read_wstr())
        print('    guildNotice="%s"' % p.read_wstr())
        print('    guildPoint=%d' % p.read_i32())
        print('    guildFund=%d' % p.read_i32())
        print('    guildPvpRecord={')
        print('      rp=%d' % p.read_i32())
        print('      win=%d' % p.read_u16())
        print('      draw=%d' % p.read_u16())
        print('      lose=%d' % p.read_u16())
        print('    }')
        print('    guildRankNo=%d' % p.read_i32())
        print('    guildMemberClassList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      id=%d' % p.read_i32())
            print('      type=%d' % p.read_u8())
            print('      iconIndex=%d' % p.read_u8())
            print('      name="%s"' % p.read_wstr())
            print('      rights={')
            print('        hasInviteRight=%d' % p.read_u8())
            print('        hasExpelRight=%d' % p.read_u8())
            print('        hasMembershipChgRight=%d' % p.read_u8())
            print('        hasClassAssignRight=%d' % p.read_u8())
            print('        hasNoticeChgRight=%d' % p.read_u8())
            print('        hasIntroChgRight=%d' % p.read_u8())
            print('        hasInterestChgRight=%d' % p.read_u8())
            print('        hasFundManageRight=%d' % p.read_u8())
            print('        hasJoinTypeRight=%d' % p.read_u8())
            print('        hasEmblemRight=%d' % p.read_u8())
            print('      }')
            print('    },')
            count -= 1
        
        print('    ]')

        print('    guildSkills=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      type=%d' % p.read_u8())
            print('      level=%d' % p.read_u8())
            print('      expiryDate=%d' % p.read_i64())
            print('      extensionCount=%d' % p.read_u16())
            print('    },')
            count -= 1
        
        print('    ]')

        print('    curDailyStageGuildPoint=%d' % p.read_i32())
        print('    maxDailyStageGuildPoint=%d' % p.read_i32())
        print('    curDailyArenaGuildPoint=%d' % p.read_i32())
        print('    maxDailyArenaGuildPoint=%d' % p.read_i32())
        print('    todayRollCallCount=%d' % p.read_u8())
        print('}')
    
    def serialize_62300(netid, data):
        p = common.PacketReader(data)

        print('SA_GetGuildMemberList {')
        print('    result=%d' % p.read_i32())
        print('    guildMemberProfileList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      nickname="%s"' % p.read_wstr())
            print('      membershipID=%d' % p.read_i32())
            print('      lvl=%d' % p.read_u16())
            print('      leaderClassType=%d' % p.read_u16())
            print('      masterCount=%d' % p.read_u16())
            print('      achievmentScore=%d' % p.read_i32())
            print('      topPvpTierGrade=%d' % p.read_u8())
            print('      topPvpTierPoint=%d' % p.read_u16())
            print('      contributedGuildPoint=%d' % p.read_i32())
            print('      contributedGuildFund=%d' % p.read_i32())
            print('      guildPvpWin=%d' % p.read_u16())
            print('      guildPvpPlay=%d' % p.read_u16())
            print('      lastLogoutDate=%d' % p.read_i64())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62302(netid, data):
        p = common.PacketReader(data)

        print('SA_GetGuildHistoryList {')
        print('    result=%d' % p.read_i32())
        print('    guildHistories=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      historyType=%d' % p.read_u8())
            print('      eventType=%d' % p.read_u8())
            print('      eventDate=%d' % p.read_i64())
            print('      strParam1="%s"' % p.read_wstr())
            print('      strParam2="%s"' % p.read_wstr())
            print('      lParam1=%d' % p.read_i32())
            print('      lParam2=%d' % p.read_i32())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62322(netid, data):
        p = common.PacketReader(data)

        print('SA_GetGuildRankingSeasonList {')
        print('    result=%d' % p.read_i32())
        print('    rankingType=%d' % p.read_u8())
        print('    rankingSeasonList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      seasonNo=%d' % p.read_u16())
            print('      beginDate=%d' % p.read_i64())
            print('      endDate=%d' % p.read_i64())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62330(netid, data):
        p = common.PacketReader(data)

        print('SN_MyGuild {')
        print('    guildTag="%s"' % p.read_wstr())
        print('    dissolutionDate=%d' % p.read_i64())
        print('    isFirstTodayRollCall=%d' % p.read_u8())
        print('}')

    def serialize_62338(netid, data):
        p = common.PacketReader(data)

        print('SN_GuildMemberStatus {')
        print('    guildMemberStatusList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      nickname="%s"' % p.read_wstr())
            print('      onlineStatus=%d' % p.read_u8())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62358(netid, data):
        p = common.PacketReader(data)

        print('SN_GuildChannelEnter {')
        print('    guildName="%s"' % p.read_wstr())
        print('    nick="%s"' % p.read_wstr())
        print('    onlineStatus=%d' % p.read_u8())
        print('}')

    def serialize_62364(netid, data):
        p = common.PacketReader(data)

        print('SN_PlayerSyncActionStateOnly {')
        print('    entityID=%d' % p.read_i32())
        print('    nState=%d' % p.read_i32())
        print('    nParam1=%d' % p.read_i32())
        print('    nParam2=%d' % p.read_i32())
        print('    rotate=%g' % p.read_f32())
        print('    upperRotate=%g' % p.read_f32())
        print('    graphMove={')
        print('      bApply=%d' % p.read_u8())
        print('      startPos=%s' % read_Vec3(p))
        print('      endPos=%s' % read_Vec3(p))
        print('      durationTimeS=%g' % p.read_f32())
        print('      originDistance=%g' % p.read_f32())
        print('    }')
        print('}')

    def serialize_62390(netid, data):
        p = common.PacketReader(data)

        print('SN_ProfileCharacterSkinList {')
        print('    skins=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      classType=%d' % p.read_i32())
            print('      skinIndex=%d' % p.read_i32())
            print('      bufCount=%d' % p.read_i32())
            print('      expireDateTime=%d' % p.read_i64())
            print('    },')
            count -= 1

        print('}')
        
    def serialize_62404(netid, data):
        p = common.PacketReader(data)

        print('SN_WarehouseItems {')
        print('    items=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      itemID=%d' % p.read_i32())
            print('      invenType=%d' % p.read_u8())
            print('      slot=%d' % p.read_i32())
            print('      itemIndex=%d' % p.read_i32())
            print('      count=%d' % p.read_i32())
            print('      propertyGroupIndex=%d' % p.read_i32())
            print('      isLifeTimeAbsolute=%d' % p.read_u8())
            print('      lifeEndTimeUTC=%d' % p.read_i64())

            prop_count = p.read_u16()
            s = ''
            while prop_count > 0:
                s += '(type=%d typeDetail=%d valueType=%d value=%g fixed=%d), ' % (p.read_u8(), p.read_i32(), p.read_u8(), p.read_f32(), p.read_u8())
                prop_count -= 1

            print('      properties=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')
    
    def serialize_62425(netid, data):
        p = common.PacketReader(data)

        print('SN_MailUnreadNotice {')
        print('    unreadInboxMailCount=%d' % p.read_u16())
        print('    unreadArchivedMailCount=%d' % p.read_u16())
        print('    unreadShopMailCount=%d' % p.read_u16())
        print('    inboxMailCount=%d' % p.read_u16())
        print('    archivedMailCount=%d' % p.read_u16())
        print('    shopMailCount=%d' % p.read_u16())

        print('    newAttachmentsPending=[')

        count = p.read_u16()
        while count > 0:
            print('    %d,' % p.read_i32())
            count -= 1
        print('    ]')

        print('}')

    def serialize_62426(netid, data):
        p = common.PacketReader(data)

        print('SN_UpdateEntrySystem {')
        print('    entrySystemList=[')

        entry_count = p.read_u16()
        while entry_count > 0:
            print('    {')

            print('      entrySystemIndex=%d' % p.read_i32())

            area_count = p.read_u16()
            s = ''
            while area_count > 0:
                s += '(areaKey=%d areaIndex=%d), ' % (p.read_u8(), p.read_i32())
                area_count -= 1

            print('      areaList=[%s]' % s)

            stage_count = p.read_u16()
            s = ''
            while stage_count > 0:
                s += '(areaKey=%d stageIndex=%d ' % (p.read_u8(), p.read_i32())

                gt_count = p.read_u16()
                gts = ''
                while gt_count > 0:
                    gts += '%d, ' % p.read_u8()
                    gt_count -= 1

                s += 'gametypes=[%s]), ' % gts
                stage_count -= 1
            
            print('      stageList=[%s]' % s)
            print('    },')
            entry_count -= 1

        print('    ]')
        print('}')

    def serialize_62427(netid, data):
        p = common.PacketReader(data)

        print('SN_GameEventActivated {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    eventIndexList=[%s]' %s)
        print('}')

    def serialize_62428(netid, data):
        p = common.PacketReader(data)

        print('SN_GameEventPending {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    eventIndexList=[%s]' %s)
        print('}')

    def serialize_62446(netid, data):
        p = common.PacketReader(data)

        print('SQ_Heartbeat {}')

    def serialize_62455(netid, data):
        p = common.PacketReader(data)

        print('SN_MasterRotationInfo {')
        print('    refreshCount=%d' % p.read_i32())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    freeRotation=[%s]' %s)

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    pccafeRotation=[%s]' %s)

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    vipRotation=[%s]' %s)
        print('}')

    def serialize_62466(netid, data):
        print('SN_SortieMasterPickPhaseEnd {')
        print('}')



    def serialize_62468(netid, data):
        p = common.PacketReader(data)

        print('SN_SortieMasterPickPhaseStep {')
        print('    isRandomPick=%d' % p.read_u8())
        print('}')

    def serialize_62469(netid, data):
        p = common.PacketReader(data)

        print('SA_TierRecord {')
        print('    seasonId=%d' % p.read_u8())
        print('    allTierWin=%d' % p.read_i32())
        print('    allTierDraw=%d' % p.read_i32())
        print('    allTierLose=%d' % p.read_i32())
        print('    allTierLeave=%d' % p.read_i32())
        print('    stageRecordList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      pvpSaveType=%d' % p.read_u8())
            print('      tierType=%d' % p.read_u8())
            print('      tierPoint=%d' % p.read_u16())
            print('      sectorId=%d' % p.read_u16())
            print('      sectorRanking=%d' % p.read_u8())
            print('      seasonalWin=%d' % p.read_i32())
            print('      seasonalDraw=%d' % p.read_i32())
            print('      seasonalLose=%d' % p.read_i32())
            print('      seasonalLeave=%d' % p.read_i32())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62472(netid, data):
        p = common.PacketReader(data)

        print('SN_Unknown_62472 {')
        print('    var=%d' % p.read_u8())
        print('}')

    def serialize_62483(netid, data):
        p = common.PacketReader(data)

        print('SN_PveTodayStageInfo {')
        print('    availablePlayCount=%d' % p.read_u16())
        print('    maxPlayCount=%d' % p.read_u16())
        print('}')

    def serialize_62484(netid, data):
        p = common.PacketReader(data)

        print('SN_FatiguePointInfo {')
        print('    todayFatiguePoint=%d' % p.read_u16())
        print('    maxFatiguePoint=%d' % p.read_u16())
        print('}')

    def serialize_62485(netid, data):
        p = common.PacketReader(data)

        print('SN_PveComradeInfo {')
        print('    availableComradeCount=%d' % p.read_i32())
        print('    maxComradeCount=%d' % p.read_i32())
        print('}')

    def serialize_62500(netid, data):
        p = common.PacketReader(data)

        print('SN_ClientSettings {')
        print('    settingType=%d' % p.read_u8())
        data_len = p.read_u16()
        data = p.read_raw(data_len)
        extracted = zlib.decompress(data)
        print('    decompressed=[%s]' % extracted.decode('utf-8'))
        print('    data="{}"'.format(p.read_raw(data_len)))
        print('}')

    def serialize_62525(netid, data):
        p = common.PacketReader(data)

        print('SN_AccountEquipmentList {')
        print('    supportKitDocIndex=%d' % p.read_i32())
        print('}')

    def serialize_62526(netid, data):
        p = common.PacketReader(data)

        print('SN_AddictionWarning {')
        print('    addictionLevel=%d' % p.read_u8())
        print('}')

    def serialize_62542(netid, data):
        p = common.PacketReader(data)

        print('SN_DateChanged {')
        print('}')


    def serialize_62544(netid, data):
        p = common.PacketReader(data)

        print('SN_Calendar {')
        print('    calendar=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      dateTime=%d' % p.read_i64())
            print('      calendarData=[')

            data_count = p.read_u16()
            while data_count > 0:
                print('      {')
                print('        dataType=%d' % p.read_u8())
                print('        index=%d' % p.read_i32())
                print('        startDateTime=%d' % p.read_i64())
                print('        endDateTime=%d' % p.read_i64())
                print('      }')
                data_count -= 1

            print('      ]')
            print('    },')
            count -= 1
        print('    ]')

        print('}')

    def serialize_62545(netid, data):
        p = common.PacketReader(data)

        print('SA_CalendarDetail {')
        print('    todayUTCDateTime=%d' % p.read_i64())
        print('    eventList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      dataType=%d' % p.read_u8())
            print('      index=%d' % p.read_i32())
            print('      startDateTime=%d' % p.read_i64())
            print('      endDateTime=%d' % p.read_i64())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62550(netid, data):
        p = common.PacketReader(data)

        print('SN_SortieMasterPickPhaseStep {')
        print('    isRandomPick=%d' % p.read_u8())
        print('}')

    def serialize_62551(netid, data):
        p = common.PacketReader(data)

        print('SN_ItemLifetimeExpired {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    itemDocIndexList=[%s]' % s)
        print('}')

    def serialize_62552(netid, data):
        p = common.PacketReader(data)

        print('SN_EffectLifetimeExpired {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    effectItemDocIndexList=[%s]' % s)
        print('}')

    def serialize_62554(netid, data):
        p = common.PacketReader(data)

        print('SN_EffectLifetime {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(effectItemDocIndex=%d expireDateTime=%d)' % (p.read_i32(), p.read_i64())
            count -= 1
        print('    effectItemDocIndexList=[%s]' % s)
        print('}')

    def serialize_62556(netid, data):
        p = common.PacketReader(data)

        print('SA_guideMissionAccept {')
        print('    result=%d' % p.read_i32())
        print('    step=%d' % p.read_u8())
        print('}')

    def serialize_62559(netid, data):
        p = common.PacketReader(data)

        print('SN_CurrentGuideMission {')
        print('    step=%d' % p.read_i8())
        print('    stage=%d' % p.read_u8())
        print('    isRewarded=%d' % p.read_u8())
        print('}')

    def serialize_62572(netid, data):
        p = common.PacketReader(data)

        print('SN_GMDisabledMasters {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    masterList=[%s]' % s)
        print('}')

    def serialize_62573(netid, data):
        p = common.PacketReader(data)

        print('SN_GMDisabledSkins {')
        
        
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '"%s", ' % p.read_wstr()
            count -= 1
        
        print('    skinKeyList=[%s]' % s)
        print('}')

    def serialize_62574(netid, data):
        p = common.PacketReader(data)

        print('SN_GMDisabledStage {')
        
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(stageIndex=%d gameType=%d), ' % (p.read_u8(), p.read_u8())
            count -= 1
        
        print('    stageList=[%s]' % s)
        print('}')

    def serialize_62591(netid, data):
        p = common.PacketReader(data)

        print('SN_HudEventList {')
        print('    eventList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      eventType=%d' % p.read_u8())
            print('      bannerUrl="%s"' % p.read_str())
            print('      parameter="%s"' % p.read_str())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62592(netid, data):
        p = common.PacketReader(data)

        print('SN_CShopEventList {')
        print('    eventList=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      eventType=%d' % p.read_u8())
            print('      positionType=%d' % p.read_u8())
            print('      goodsId=%d' % p.read_i32())
            print('      bannerUrl="%s"' % p.read_str())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62595(netid, data):
        p = common.PacketReader(data)

        print('SN_ServerUtcTime {')
        print('    effectItemDocIndexList=%d' % p.read_i64())
        print('}')
