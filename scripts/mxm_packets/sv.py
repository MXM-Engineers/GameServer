# server packets
from . import common

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
        print('    nRotate=%d' % p.read_f32())
        print('    nSpeed=%d' % p.read_f32())
        print('    nState=%d' % p.read_i32())
        print('    nActionIDX=%d' % p.read_i32())
        print('}')

    def serialize_62057(netid, data):
        p = common.PacketReader(data)

        print('SN_Money {')
        print('    money=%d' % p.read_i64())
        print('    nReason=%d' % p.read_i32())
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

    def serialize_62278(netid, data):
        p = common.PacketReader(data)

        print('SN_Exp {')
        print('    leaderCsn=%d' % p.read_i32())
        print('    nickname="%s"' % p.read_wstr())
        print('    isLevelUp=%d' % p.read_u8())
        print('    level=%d' % p.read_u16())
        print('    currentLevelExp=%d' % p.read_i32())
        print('}')
        
    def serialize_62330(netid, data):
        p = common.PacketReader(data)

        print('SN_MyGuild {')
        print('    guildTag="%s"' % p.read_wstr())
        print('    dissolutionDate=%d' % p.read_i64())
        print('    isFirstTodayRollCall=%d' % p.read_u8())
        print('}')

    def serialize_62358(netid, data):
        p = common.PacketReader(data)

        print('SN_GuildChannelEnter {')
        print('    guildName="%s"' % p.read_wstr())
        print('    nick="%s"' % p.read_wstr())
        print('    onlineStatus=%d' % p.read_u8())
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

    def serialize_62485(netid, data):
        p = common.PacketReader(data)

        print('SN_PveComradeInfo {')
        print('    availableComradeCount=%d' % p.read_i32())
        print('    maxComradeCount=%d' % p.read_i32())
        print('}')

    def serialize_62525(netid, data):
        p = common.PacketReader(data)

        print('SN_AccountEquipmentList {')
        print('    supportKitDocIndex=%d' % p.read_i32())
        print('}')

