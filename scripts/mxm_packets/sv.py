# server packets
from . import common
import zlib

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)

def read_Vec2(p):
    x, y = p.read_f32(), p.read_f32()
    return 'Vec2(%g, %g)' % (x, y)

class ServerSerializer:
    def serialize_62001(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62001 {')
        print('    field_0=%d' % p.read_i64())
        print('    field_1=%d' % p.read_u32())
        print('}')
    def serialize_62002(netid, p: common.PacketReader):
        print('SA_FirstHello {')
        print('    dwProtocolCRC=%#x' % p.read_u32())
        print('    dwErrorCRC=%#x' % p.read_u32())
        print('    serverType=%d' % p.read_u8())
        print('    clientIp=(%d.%d.%d.%d)' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8()))
        print('    clientPort=%d' % p.read_u16())
        print('    tqosWorldId=%d' % p.read_u8())
        print('}')

    def serialize_62008(netid, p: common.PacketReader):
        print('SN_PlayerTitleSelect {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    nDisplayTitleIndex=%d' % p.read_u32())
        print('    nStatTitleIndex=%d' % p.read_u32())
        print('}')
    def serialize_62012(netid, p: common.PacketReader):
        print('SN_DoConnectSpectateServer {')
        print('    dwPort=%d' % p.read_u16())
        print('    dwIP=%d' % p.read_u32())
        print('    idcHash=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    instantKey=%d' % p.read_u32())
        print('}')
    def serialize_62013(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62013 {')
        print('    field_0=%d' % p.read_u16())
        print('    field_1=%d' % p.read_u32())
        print('}')
    def serialize_62014(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62014 {')
        print('}')
    def serialize_62015(netid, p: common.PacketReader):
        print('SQ_PrepareHandoverToChannel {')
        print('}')
    def serialize_62016(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62016 {')
        print('}')
    def serialize_62017(netid, p: common.PacketReader):
        print('SN_SuspendedToCity {')
        print('    stageIdx=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u8())
        print('    gameDefType=%d' % p.read_u8())
        print('}')
    def serialize_62018(netid, p: common.PacketReader):
        print('SA_returnToCity {')
        print('    errCode=%d' % p.read_i32())
        print('}')
    def serialize_62019(netid, p: common.PacketReader):
        print('SN_EnterChannel {')
        print('    chID=%d' % p.read_u32())
        print('    channelType=%d' % p.read_u32())
        print('}')
    def serialize_62020(netid, p: common.PacketReader):
        print('SN_LeaveChannel {')
        print('    chID=%d' % p.read_u32())
        print('}')
    def serialize_62021(netid, p: common.PacketReader):
        print('SN_GameRestriction {')
        print('    timeToReleaseSec=%d' % p.read_u32())
        print('}')
    def serialize_62022(netid, p: common.PacketReader):
        print('SN_ShopItemList {')
        n = p.read_u16()
        print('    m_ShopItem_count=%d' % n)
        for _ in range(n):
            print('    m_ShopItem[] m_ItemKey=%d m_IDX=%d m_Count=%d m_Price=%d UpgradeIndex=%d' % (
                p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62023(netid, p: common.PacketReader):
        print('SA_SellShopItem {')
        print('    Result=%d' % p.read_u32())
        n = p.read_u16()
        print('    sellItemDocIndexes_count=%d' % n)
        for _ in range(n):
            print('    sellItemDocIndexes[] =%d' % p.read_u32())
        print('    sellAmount=%d' % p.read_i64())
        print('}')
    def serialize_62024(netid, p: common.PacketReader):
        print('SN_CreateGroundItem {')
        bits = p.read_u8()
        print('    excludedFieldBits=%d' % bits)
        if not (bits & 1):
            print('    startPos=%s' % read_Vec3(p))
        if not (bits & 2):
            print('    destPos=%s' % read_Vec3(p))
        print('    itemID=0x%08x' % p.read_u32())
        print('    dropType=%d' % p.read_u32())
        print('    nIndex=%d' % p.read_u32())
        print('    count=%d' % p.read_u32())
        print('    ownerID=0x%08x' % p.read_u32())
        print('    gettableType=%d' % p.read_i32())
        print('    factionType=%d' % p.read_i32())
        print('    isPrivate=%d' % p.read_u8())
        print('    positionType=%d' % p.read_u8())
        print('}')
    def serialize_62027(netid, p: common.PacketReader):
        print('SN_GameModifyActor {')
        print('    oldID=%d' % p.read_u32())
        print('    newID=%d' % p.read_u32())
        print('    nType=%d' % p.read_u32())
        print('    nIDX=%d' % p.read_u32())
        print('    dwLocalID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    p3nPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    p3nDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    SpawnType=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('    ownerID=0x%08x' % p.read_u32())
        print('    bDirectionToNearPC=%d' % p.read_u8())
        print('    nAIWanderDistOverride=%d' % p.read_u32())
        print('    tagID=%d' % p.read_u32())
        print('    faction=%d' % p.read_u32())
        print('    classType=%d' % p.read_u32())
        print('    skinIndex=%d' % p.read_u32())
        print('    seed=%d' % p.read_u32())
        n = p.read_u16()
        print('    maxStats_count=%d' % n)
        for _ in range(n):
            print('    maxStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        n = p.read_u16()
        print('    curStats_count=%d' % n)
        for _ in range(n):
            print('    curStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        print('}')
    def serialize_62032(netid, p: common.PacketReader):
        print('SA_ItemMerge {')
        print('    nEquipKey=%d' % p.read_u32())
        print('    nCount=%d' % p.read_u32())
        print('    nEquipKey2=%d' % p.read_u32())
        print('    nCount2=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62034(netid, p: common.PacketReader):
        print('SN_PrecastSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    bSyncMyPosition=%d' % p.read_u8())
        print('    posStruct {')
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('        pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('        destPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('        moveDir=(%.2f, %.2f)' % (p.read_f32(), p.read_f32()))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('        rotateStruct=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('        speed=%.2f' % p.read_f32())
        print('        clientTime=%.2f' % p.read_f32())
        print('    }')
        print('}')
    def serialize_62037(netid, p: common.PacketReader):
        print('SN_SwitchOnToggleSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        n = p.read_u16()
        print('    remoteList_count=%d' % n)
        for _ in range(n):
            print('    remoteList[] %d' % p.read_u32())
        print('    param1=%d' % p.read_u32())
        print('}')
    def serialize_62038(netid, p: common.PacketReader):
        print('SN_SwitchOffToggleSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    param1=%d' % p.read_u32())
        print('}')
    def serialize_62039(netid, p: common.PacketReader):
        print('SN_CancelSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62040(netid, p: common.PacketReader):
        print('SA_PrecastSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62042(netid, p: common.PacketReader):
        print('SA_ExecuteSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62043(netid, p: common.PacketReader):
        print('SA_SwitchOnToggleSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('}')
    def serialize_62044(netid, p: common.PacketReader):
        print('SA_SwitchOffToggleSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('}')
    def serialize_62045(netid, p: common.PacketReader):
        print('SA_CancelSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62046(netid, p: common.PacketReader):
        print('SN_ChangeSkillSlot {')
        print('    entity=%d' % p.read_u32())
        print('    slotIndex=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    resetBaseComboCooltimeID=%d' % p.read_u32())
        print('    param=%d' % p.read_u32())
        print('}')
    def serialize_62049(netid, p: common.PacketReader):
        print('SN_UpdateTargetGraphMove {')
        print('    characterID=0x%08x' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    curPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    targetID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    targetPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    curActionID=%d' % p.read_u32())
        print('}')
    def serialize_62053(netid, p: common.PacketReader):
        print('SA_ItemChange {')
        print('    nEquipKey=%d' % p.read_u32())
        print('    nESlot=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62054(netid, p: common.PacketReader):
        print('SA_ItemDelete {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62055(netid, p: common.PacketReader):
        print('SA_WarehouseItemDelete {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62056(netid, p: common.PacketReader):
        print('SN_UpdateStat {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    statType=%d' % p.read_i32())
        print('    cur=%d' % p.read_f32())
        print('    max=%d' % p.read_f32())
        print('    reasonCode=%d' % p.read_i32())
        print('}')

    def serialize_62057(netid, p: common.PacketReader):
        print('SN_Money {')
        print('    money=%d' % p.read_i64())
        print('    nReason=%d' % p.read_i32())
        print('}')

    def serialize_62058(netid, p: common.PacketReader):
        print('SN_CCoin {')
        print('    amount=%d' % p.read_i64())
        print('    nReason=%d' % p.read_u32())
        print('}')
    def serialize_62062(netid, p: common.PacketReader):
        print('SN_DeadAck {')
        print('    sourceID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    targetDocIndex=%d' % p.read_u32())
        print('    dwRemoteID=%d' % p.read_u32())
        print('    dwRemoteDocIndex=%d' % p.read_u32())
        print('    isPast=%d' % p.read_u8())
        print('}')
    def serialize_62063(netid, p: common.PacketReader):
        print('SN_DeadDamageInfo {')
        n = p.read_u16()
        print('    deadDamageInfo_count=%d' % n)
        for _ in range(n):
            print('    deadDamageInfo[] objectID=0x%08x skillDocIndex=%d statusDocIndex=%d damage=%d attackerIsMonster=%d damageType=%d' % (
                p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8()))
        print('    attackerKey=%d' % p.read_u32())
        m = p.read_u16()
        print('    others_count=%d' % m)
        for _ in range(m):
            k = p.read_u16()
            print('    others[].deadDamageInfo_count=%d' % k)
            for _ in range(k):
                print('    others[].deadDamageInfo[] objectID=0x%08x skillDocIndex=%d statusDocIndex=%d damage=%d attackerIsMonster=%d damageType=%d' % (
                    p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8()))
            print('    others[].attackerKey=%d' % p.read_u32())
        print('    durationTime=%d' % p.read_u32())
        print('}')
    def serialize_62065(netid, p: common.PacketReader):
        print('SA_BuyShopItem {')
        print('    Result=%d' % p.read_u32())
        print('    m_IDX=%d' % p.read_u32())
        print('}')
    def serialize_62066(netid, p: common.PacketReader):
        print('SA_BuyCshopItem {')
        print('    result=%d' % p.read_u32())
        print('    itemIndex=0x%08x' % p.read_u32())
        print('}')
    def serialize_62067(netid, p: common.PacketReader):
        print('SA_GiftFriendsAvailable {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    friendsList_count=%d' % n)
        for _ in range(n):
            ln = p.read_u16()
            s = p.read_raw(ln * 2).decode('utf-16-le', 'replace')
            print('    friendsList[] %s' % s)
        print('}')
    def serialize_62068(netid, p: common.PacketReader):
        print('SA_GiftFriendCharsAndSkins {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    characterList_count=%d' % n)
        for _ in range(n):
            print('    characterList[]=%d' % p.read_u32())
        n = p.read_u16()
        print('    skinList_count=%d' % n)
        for _ in range(n):
            print('    skinList[] classType=%d skinIndex=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62069(netid, p: common.PacketReader):
        print('SA_BuyGift {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62070(netid, p: common.PacketReader):
        print('SN_PlayerSkillUpdate {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    nSkillIndex=%d' % p.read_u32())
        print('    nCoolTime=%d' % p.read_u32())
        print('    bUnlocked=%d' % p.read_u8())
        n = p.read_u16()
        print('    properties_count=%d' % n)
        for _ in range(n):
            print('    properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('    isUnlocked=%d' % p.read_u8())
        print('    isActivated=%d' % p.read_u8())
        print('}')
    def serialize_62071(netid, p: common.PacketReader):
        print('SN_GameAreaMove {')
        print('    dwAreaIDX=%d' % p.read_u32())
        print('}')
    def serialize_62074(netid, p: common.PacketReader):
        print('SN_TrespassReady {')
        print('    isGameStarted=%d' % p.read_u8())
        print('    isPaused=%d' % p.read_u8())
        print('    readyElapsedMS=%d' % p.read_u32())
        print('}')
    def serialize_62075(netid, p: common.PacketReader):
        print('SA_GameReady {')
        print('    waitingTimeMS=%d' % p.read_i32())
        print('    serverTimestamp=%d' % p.read_i64())
        print('    readyElapsedMs=%d' % p.read_i32())
        print('}')

    def serialize_62077(netid, p: common.PacketReader):
        print('SN_TrespassGameStart {')
        print('    m_PlayTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62078(netid, p: common.PacketReader):
        print('SA_ItemUse {')
        print('    nErrorType=%d' % p.read_u32())
        print('    usedItemDocIndex=%d' % p.read_u32())
        print('    goldAmount=%d' % p.read_i64())
        n = p.read_u16()
        print('    addedItemList_count=%d' % n)
        for _ in range(n):
            print('    addedItemList[] itemId=0x%08x itemDocIndex=0x%08x itemCount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62079(netid, p: common.PacketReader):
        print('SN_PvpResult {')
        print('    gameEndReason=%d' % p.read_u32())
        print('    m_playTime=%d' % p.read_u32())
        n = p.read_u16()
        print('    pvpResults_count=%d' % n)
        for _ in range(n):
            print('    pvpResults[] teamType=%d pvpResultType=%d' % (p.read_u32(), p.read_u32()))
        print('    m_resultReward {')
        print('        m_playerID=%d' % p.read_u32())
        print('        m_baseRon=%d' % p.read_u32())
        n = p.read_u16()
        print('        m_bonusRon_count=%d' % n)
        for _ in range(n):
            print('        m_bonusRon[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        print('        m_baseExp=%d' % p.read_u32())
        n = p.read_u16()
        print('        m_bonusExp_count=%d' % n)
        for _ in range(n):
            print('        m_bonusExp[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('        rewardItems_count=%d' % n)
        for _ in range(n):
            print('        rewardItems[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('        m_guildPointReward m_baseGuildPoint=%d m_bonusGuildPoint=%d m_guildMemberNum=%d m_boostGoldRate=%d m_boostExpRate=%d m_bonusSlotOpen=%d' % (p.read_u32(), p.read_u32(), p.read_u8(), p.read_u16(), p.read_u16(), p.read_u16()))
        print('        m_pcCafeBonusItem m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('    }')
        print('    tierGameResult=%d' % p.read_u8())
        print('    tierTypeResult=%d' % p.read_u8())
        print('    curTierPoint=%d' % p.read_u16())
        print('    deltaTierPoint=%d' % p.read_u16())
        print('    deltaGuildRp=%d' % p.read_u16())
        n = p.read_u16()
        print('    pcCafeUserIds_count=%d' % n)
        for _ in range(n):
            print('    pcCafeUserIds[] %d' % p.read_u32())
        print('}')
    def serialize_62080(netid, p: common.PacketReader):
        print('SN_PvpResultScoreDeathmatch {')
        n = p.read_u16()
        print('    m_TeamScore_count=%d' % n)
        for _ in range(n):
            print('    m_TeamScore[] m_Team=%d m_TotalScore=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    m_PvPScore_count=%d' % n)
        for _ in range(n):
            print('    m_PvPScore[] m_playerID=%d' % p.read_u32())
            m = p.read_u16()
            print('    m_nickName_len=%d' % m)
            print('    m_nickName=%s' % p.read_raw(m * 2).decode('utf-16-le', errors='replace'))
            print('    m_teamType=%d' % p.read_u32())
            print('    m_Disconnected=%d' % p.read_u8())
            print('    m_TotalScore=%d' % p.read_u32())
            k = p.read_u16()
            print('    m_charScore_count=%d' % k)
            for _ in range(k):
                print('    m_charScore[] m_CreatureIndex=%d m_PlayTimeMS=%d m_KillCount=%d m_MaxContinuousKillCount=%d m_MaxMultiKillCount=%d m_DeadCount=%d m_AssistCount=%d m_TotalDamageDealt=%d m_MaxDamageDealt=%d m_TotalNormalDamageDealt=%d m_TotalSkillDamageDealt=%d m_TotalDamageTaken=%d m_MaxDamageTaken=%d m_TotalNormalDamageTaken=%d m_TotalSkillDamageTaken=%d' % (p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62081(netid, p: common.PacketReader):
        print('SN_PvpResultScoreOccupy {')
        n = p.read_u16()
        print('    m_TeamScore_count=%d' % n)
        for _ in range(n):
            print('    m_TeamScore[] m_Team=%d m_TotalScore=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    m_PvPScore_count=%d' % n)
        for _ in range(n):
            print('    m_PvPScore[] m_playerID=%d' % p.read_u32())
            nl = p.read_u16()
            print('    m_PvPScore[] m_nickName_len=%d' % nl)
            print('    m_PvPScore[] m_nickName=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
            print('    m_PvPScore[] m_teamType=%d' % p.read_u32())
            print('    m_PvPScore[] m_Disconnected=%d' % p.read_u8())
            nc = p.read_u16()
            print('    m_PvPScore[] m_charScore_count=%d' % nc)
            for _ in range(nc):
                print('    m_charScore[] m_CreatureIndex=%d m_PlayTimeMS=%d' % (p.read_u32(), p.read_u32()))
                print('    m_charScore[] m_KillCount=%d m_MaxContinuousKillCount=%d m_MaxMultiKillCount=%d m_DeadCount=%d m_AssistCount=%d' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8()))
                print('    m_charScore[] m_TotalDamageDealt=%d m_MaxDamageDealt=%d m_TotalNormalDamageDealt=%d m_TotalSkillDamageDealt=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
                print('    m_charScore[] m_TotalDamageTaken=%d m_MaxDamageTaken=%d m_TotalNormalDamageTaken=%d m_TotalSkillDamageTaken=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
                print('    m_charScore[] m_OccupyCount=%d m_OccupyScore=%d' % (p.read_u8(), p.read_u16()))
        print('}')
    def serialize_62082(netid, p: common.PacketReader):
        print('SN_PvpResultScoreGot {')
        n = p.read_u16()
        print('    m_TeamScore_count=%d' % n)
        for _ in range(n):
            print('    m_TeamScore[] m_Team=%d' % p.read_u32())
            print('    m_TeamScore[] m_TotalScore=%d' % p.read_u32())
            m = p.read_u16()
            print('    m_TeamScore[] m_TitanSummonList_count=%d' % m)
            for _ in range(m):
                print('    m_TeamScore[] m_TitanSummonList[]=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_PvPScore_count=%d' % n)
        for _ in range(n):
            print('    m_PvPScore[] m_playerID=%d' % p.read_u32())
            sl = p.read_u16()
            print('    m_PvPScore[] m_nickName_len=%d' % sl)
            print('    m_PvPScore[] m_nickName=%s' % p.read_raw(sl * 2).decode('utf-16-le', errors='replace'))
            print('    m_PvPScore[] m_teamType=%d' % p.read_u32())
            print('    m_PvPScore[] m_Disconnected=%d' % p.read_u8())
            print('    m_PvPScore[] m_IngameLevel=%d' % p.read_u8())
            print('    m_PvPScore[] m_TotalScore=%d' % p.read_u32())
            c = p.read_u16()
            print('    m_PvPScore[] m_charPropertyInfo1_count=%d' % c)
            for _ in range(c):
                print('    m_PvPScore[] m_charPropertyInfo1[] propertyIndex=%d propertyLevel=%d' % (p.read_u8(), p.read_u8()))
            c = p.read_u16()
            print('    m_PvPScore[] m_charPropertyInfo2_count=%d' % c)
            for _ in range(c):
                print('    m_PvPScore[] m_charPropertyInfo2[] propertyIndex=%d propertyLevel=%d' % (p.read_u8(), p.read_u8()))
            c = p.read_u16()
            print('    m_PvPScore[] m_charScore_count=%d' % c)
            for _ in range(c):
                print('    m_PvPScore[] m_charScore[] m_CreatureIndex=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_PlayTimeMS=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_KillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_MaxContinuousKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_MaxMultiKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_DeadCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_AssistCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_TotalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_MaxDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalNormalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalSkillDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalBuildingDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_MaxDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalNormalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalSkillDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_TotalHeal=%d' % p.read_u32())
                print('    m_PvPScore[] m_charScore[] m_DestroyTowerCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_KillBossCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_OccupyStatueCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_SavePieceCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_OccupySightCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_KillMinionCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_charScore[] m_KillNeutralMobCount=%d' % p.read_u8())
            c = p.read_u16()
            print('    m_PvPScore[] m_activeTransformCharScore_count=%d' % c)
            for _ in range(c):
                print('    m_PvPScore[] m_activeTransformCharScore[] m_CreatureIndex=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_PlayTimeMS=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_KillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_MaxContinuousKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_MaxMultiKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_DeadCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_AssistCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_MaxDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalNormalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalSkillDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalBuildingDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_MaxDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalNormalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalSkillDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_TotalHeal=%d' % p.read_u32())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_DestroyTowerCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_KillBossCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_OccupyStatueCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_SavePieceCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_OccupySightCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_KillMinionCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_activeTransformCharScore[] m_KillNeutralMobCount=%d' % p.read_u8())
            c = p.read_u16()
            print('    m_PvPScore[] m_inActiveTransformCharScore_count=%d' % c)
            for _ in range(c):
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_CreatureIndex=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_PlayTimeMS=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_KillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_MaxContinuousKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_MaxMultiKillCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_DeadCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_AssistCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_MaxDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalNormalDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalSkillDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalBuildingDamageDealt=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_MaxDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalNormalDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalSkillDamageTaken=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_TotalHeal=%d' % p.read_u32())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_DestroyTowerCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_KillBossCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_OccupyStatueCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_SavePieceCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_OccupySightCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_KillMinionCount=%d' % p.read_u8())
                print('    m_PvPScore[] m_inActiveTransformCharScore[] m_KillNeutralMobCount=%d' % p.read_u8())
        print('}')
    def serialize_62083(netid, p: common.PacketReader):
        print('SN_PvpResultScoreSport {')
        n = p.read_u16()
        print('    m_TeamScore_count=%d' % n)
        for _ in range(n):
            print('    m_TeamScore[] m_Team=%d m_TotalScore=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    m_PvPScore_count=%d' % n)
        for _ in range(n):
            playerID = p.read_u32()
            nickLen = p.read_u16()
            nick = p.read_raw(nickLen * 2).decode('utf-16-le', errors='replace')
            print('    m_PvPScore[] playerID=0x%08x nickName=%s teamType=%d activeCreatureIndex=%d inactiveCreatureIndex=%d Disconnected=%d OlympicRecord=%d Ranking=%d IsFinished=%d TotalScore=%d' % (
                playerID, nick, p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_u32(), p.read_u8(), p.read_u8(), p.read_u32()))
        print('}')
    def serialize_62085(netid, p: common.PacketReader):
        print('SA_GetGroundItem {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    itemID=0x%08x' % p.read_u32())
        print('    itemISN=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62086(netid, p: common.PacketReader):
        print('SN_StageClearResult {')
        print('    m_playerID=%d' % p.read_u32())
        n = p.read_u16()
        print('    member_count=%d' % n)
        for _ in range(n):
            print('    member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
            name_len = p.read_u16()
            print('    member[] nickname=%s' % p.read_raw(name_len * 2).decode('utf-16-le', errors='replace'))
        print('    isRandomRestart=%d' % p.read_u8())
        print('    m_StageIndex=%d' % p.read_u32())
        print('    m_KillCount=%d' % p.read_u32())
        print('    m_DeathCount=%d' % p.read_u32())
        print('    m_PlayTime=%d' % p.read_u32())
        print('    m_ClearRank=%d' % p.read_u8())
        print('    m_CombatScore=%d' % p.read_u32())
        print('    m_baseRon=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_bonusRon_count=%d' % n)
        for _ in range(n):
            print('    m_bonusRon[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        print('    m_baseExp=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_bonusExp_count=%d' % n)
        for _ in range(n):
            print('    m_bonusExp[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        print('    m_attackScore=%d' % p.read_u32())
        print('    m_defenseScore=%d' % p.read_u32())
        print('    m_timeScore=%d' % p.read_u32())
        print('    m_deathTimeScore=%d' % p.read_u32())
        print('    m_totalScore=%d' % p.read_u32())
        print('    m_ranking=%d' % p.read_u32())
        print('    m_technicGrade=%d' % p.read_u8())
        print('    m_guildPointReward base=%d bonus=%d members=%d goldRate=%d expRate=%d slots=%d' % (p.read_u32(), p.read_u32(), p.read_u8(), p.read_u16(), p.read_u16(), p.read_u16()))
        print('}')
    def serialize_62087(netid, p: common.PacketReader):
        print('SN_DefenceClearResult {')
        print('    m_playerID=%d' % p.read_u32())
        n = p.read_u16()
        print('    member_count=%d' % n)
        for _ in range(n):
            print('    member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
            print('    member[] nickname=%s' % p.read_wstr())
        print('    m_StageIndex=%d' % p.read_u32())
        print('    m_ClearStepIndex=%d' % p.read_u32())
        print('    m_ClearTime=%d' % p.read_u32())
        print('    m_pstResultReward {')
        print('        m_playerID=%d' % p.read_u32())
        print('        m_baseRon=%d' % p.read_u32())
        n = p.read_u16()
        print('        m_bonusRon_count=%d' % n)
        for _ in range(n):
            print('        m_bonusRon[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        print('        m_baseExp=%d' % p.read_u32())
        n = p.read_u16()
        print('        m_bonusExp_count=%d' % n)
        for _ in range(n):
            print('        m_bonusExp[] bonusType=%d amount=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('        rewardItems_count=%d' % n)
        for _ in range(n):
            print('        rewardItems[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('        m_guildPointReward baseGuildPoint=%d bonusGuildPoint=%d guildMemberNum=%d' % (p.read_u32(), p.read_u32(), p.read_u8()))
        print('        m_guildPointReward boostGoldRate=%d boostExpRate=%d bonusSlotOpen=%d' % (p.read_u16(), p.read_u16(), p.read_u16()))
        print('        m_pcCafeBonusItem m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('    }')
        n = p.read_u16()
        print('    pcCafeUserIds_count=%d' % n)
        for _ in range(n):
            print('    pcCafeUserIds[] id=%d' % p.read_u32())
        print('}')
    def serialize_62088(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62088 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('}')
    def serialize_62092(netid, p: common.PacketReader):
        print('SN_CommunityChat {')
        print('    m_ChatType=%d' % p.read_u32())
        print('    m_SendPlayerID=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_SendName_len=%d' % n)
        print('    m_SendName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    m_SendStaffType=%d' % p.read_u8())
        n = p.read_u16()
        print('    m_Msg_len=%d' % n)
        print('    m_Msg=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    m_DurationMS=%d' % p.read_u16())
        print('}')
    def serialize_62093(netid, p: common.PacketReader):
        print('SN_GetGroundItem {')
        print('    dwPlayerID=%d' % p.read_u32())
        print('    dwItemIndex=%d' % p.read_u32())
        print('    oid=%d' % p.read_u32())
        print('}')
    def serialize_62094(netid, p: common.PacketReader):
        print('SN_DestroyGroundItem {')
        print('    itemID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62095(netid, p: common.PacketReader):
        print('SN_GroundItemsSnapshot {')
        n = p.read_u16()
        print('    items_count=%d' % n)
        for _ in range(n):
            print('    items[] docIndex=0x%08x itemID=0x%08x dropType=%d count=%d ownerID=0x%08x gettableType=%d factionType=%d isPrivate=%d positionType=%d bFiltered=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8(), p.read_u8()))
            x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
            print('    items[] pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62096(netid, p: common.PacketReader):
        print('SA_WorldSetInfoList {')
        n = p.read_u16()
        print('    m_ServerInfo_count=%d' % n)
        for _ in range(n):
            print('    m_ServerInfo[] {')
            print('        m_nWorldID=%d' % p.read_u32())
            print('        m_nChannelID=%d' % p.read_u32())
            print('        m_nServerID=%d' % p.read_u32())
            print('        m_nServerRealID=%d' % p.read_u32())
            print('        m_nServerType=%d' % p.read_u32())
            print('        m_nIP=%d' % p.read_u32())
            print('        m_nPort=%d' % p.read_u32())
            print('        m_nState=%d' % p.read_u32())
            print('        m_nMaxSessionCount=%d' % p.read_u32())
            print('        m_nCurrentSessionCount=%d' % p.read_u32())
            print('    }')
        print('}')
    def serialize_62099(netid, p: common.PacketReader):
        print('SN_AntihackAuth {')
        n = p.read_u16()
        print('    authKey_count=%d' % n)
        for _ in range(n):
            print('    authKey[]=%d' % p.read_u8())
        print('}')
    def serialize_62103(netid, p: common.PacketReader):
        print('SN_MissionList {')
        n = p.read_u16()
        print('    missions_count=%d' % n)
        for _ in range(n):
            print('    missions[] index=%d' % p.read_u32())
            print('    missions[] progressCount=%d' % p.read_i64())
            print('    missions[] changedCount=%d' % p.read_u16())
            print('    missions[] isCleared=%d' % p.read_u8())
            print('    missions[] isRewarded=%d' % p.read_u8())
        print('    dailyMissionRemainTimeMS=%d' % p.read_u32())
        print('    weeklyMissionRemainTimeMS=%d' % p.read_u32())
        print('    monthlyMissionRemainTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62104(netid, p: common.PacketReader):
        print('SN_MissionUpdate {')
        print('    mission {')
        print('        index=%d' % p.read_u32())
        print('        progressCount=%d' % p.read_i64())
        print('        changedCount=%d' % p.read_u16())
        print('        isCleared=%d' % p.read_u8())
        print('        isRewarded=%d' % p.read_u8())
        print('    }')
        print('}')
    def serialize_62105(netid, p: common.PacketReader):
        print('SA_DailyMissionChange {')
        print('    result=%d' % p.read_u32())
        print('    step=%d' % p.read_u32())
        print('    mission.index=%d' % p.read_u32())
        print('    mission.progressCount=%d' % p.read_i64())
        print('    mission.changedCount=%d' % p.read_u16())
        print('    mission.isCleared=%d' % p.read_u8())
        print('    mission.isRewarded=%d' % p.read_u8())
        print('}')
    def serialize_62108(netid, p: common.PacketReader):
        print('SN_ChangedUserGradeInfo {')
        print('    userGrade=%d' % p.read_u8())
        print('    activated=%d' % p.read_u8())
        print('    expireDateTime64=%d' % p.read_i64())
        print('    level=%d' % p.read_u8())
        print('    point=%d' % p.read_u16())
        print('    nextPoint=%d' % p.read_u16())
        print('    activityPoint=%d' % p.read_u32())
        print('    activityRewardedState=%d' % p.read_u8())
        print('}')
    def serialize_62110(netid, p: common.PacketReader):
        print('SN_StageRank {')
        print('    rank=%d' % p.read_i32())
        print('    timeScoreRank=%d' % p.read_i32())
        print('}')

    def serialize_62111(netid, p: common.PacketReader):
        print('SN_GetPublicGroundItem {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    itemID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62114(netid, p: common.PacketReader):
        print('SN_UpdateGamePlayerTagCooltime {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    tagCooltimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62115(netid, p: common.PacketReader):
        print('SN_UpdateCanCastSkillSlotUg {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    canCastSkillSlotUG=%d' % p.read_u8())
        print('}')
    def serialize_62116(netid, p: common.PacketReader):
        print('SN_ItemAcquisition {')
        print('    nReason=%d' % p.read_u32())
        itemId, itemDocIndex, itemCount = p.read_u32(), p.read_u32(), p.read_u32()
        print('    itemInfo itemId=0x%08x itemDocIndex=0x%08x itemCount=%d' % (itemId, itemDocIndex, itemCount))
        print('}')
    def serialize_62117(netid, p: common.PacketReader):
        print('SN_ItemUpdate {')
        n = p.read_u16()
        print('    m_updatedItems_count=%d' % n)
        for _ in range(n):
            print('    m_itemID=0x%08x' % p.read_u32())
            print('    m_invenType=%d' % p.read_u8())
            print('    m_slot=%d' % p.read_u32())
            print('    m_itemIndex=0x%08x' % p.read_u32())
            print('    m_count=%d' % p.read_u32())
            print('    m_propertyGroupIndex=%d' % p.read_u32())
            print('    m_isLifeTimeAbsolute=%d' % p.read_u8())
            print('    m_lifeEndTimeUTC=%d' % p.read_i64())
            m = p.read_u16()
            print('    m_properties_count=%d' % m)
            for _ in range(m):
                print('    m_properties[] Type=%d TypeDetail=%d ValueType=%d Value=%.2f Fixed=%d' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_f32(), p.read_u8()))
        print('    context=%d' % p.read_u32())
        print('}')
    def serialize_62118(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62118 {')
        n = p.read_u16()
        print('    str_len=%d' % n)
        print('    str=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62119(netid, p: common.PacketReader):
        print('SN_NicknameNeeded {')
        print('}')
    def serialize_62120(netid, p: common.PacketReader):
        print('SA_NicknameSet {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62130(netid, p: common.PacketReader):
        print('SN_ExtraCharacters {')
        print('    isSelected=%d' % p.read_u8())
        n = p.read_u16()
        print('    profiles_count=%d' % n)
        for _ in range(n):
            print('    profiles[] leaderID=0x%08x' % p.read_u32())
            nl = p.read_u16()
            print('    profiles[] nickname_len=%d' % nl)
            print('    profiles[] nickname=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
            print('    profiles[] displayTitleIndex=%d' % p.read_u32())
            print('    profiles[] tier=%d' % p.read_u32())
            print('    profiles[] available=%d' % p.read_u8())
        n = p.read_u16()
        print('    characters_count=%d' % n)
        for _ in range(n):
            print('    characters[] m_characterID=%d' % p.read_u32())
            print('    characters[] m_creatureIndex=%d' % p.read_u32())
            print('    characters[] m_skillSlot1=%d' % p.read_u32())
            print('    characters[] m_skillSlot2=%d' % p.read_u32())
            print('    characters[] m_class=%d' % p.read_u32())
            x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
            print('    characters[] m_x=%.2f m_y=%.2f m_z=%.2f' % (x, y, z))
            print('    characters[] m_characterType=%d' % p.read_u32())
            print('    characters[] m_skinIndex=%d' % p.read_u32())
            print('    characters[] m_weaponIndex=%d' % p.read_u32())
            print('    characters[] m_masterGearNo=%d' % p.read_u8())
        n = p.read_u16()
        print('    items_count=%d' % n)
        for _ in range(n):
            print('    items[] m_itemID=0x%08x' % p.read_u32())
            print('    items[] m_invenType=%d' % p.read_u8())
            print('    items[] m_slot=%d' % p.read_u32())
            print('    items[] m_itemIndex=0x%08x' % p.read_u32())
            print('    items[] m_count=%d' % p.read_u32())
            print('    items[] m_propertyGroupIndex=%d' % p.read_u32())
            print('    items[] m_isLifeTimeAbsolute=%d' % p.read_u8())
            print('    items[] m_lifeEndTimeUTC=%d' % p.read_i64())
            np = p.read_u16()
            print('    items[] m_properties_count=%d' % np)
            for _ in range(np):
                print('    items[] m_properties[] Type=%d TypeDetail=%d ValueType=%d Value=%.2f Fixed=%d' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_f32(), p.read_u8()))
        n = p.read_u16()
        print('    weapons_count=%d' % n)
        for _ in range(n):
            print('    weapons[] characterID=0x%08x' % p.read_u32())
            print('    weapons[] weaponType=%d' % p.read_u32())
            print('    weapons[] weaponIndex=%d' % p.read_u32())
            print('    weapons[] grade=%d' % p.read_u32())
            print('    weapons[] isUnlocked=%d' % p.read_u8())
            print('    weapons[] isActivated=%d' % p.read_u8())
        n = p.read_u16()
        print('    skills_count=%d' % n)
        for _ in range(n):
            print('    skills[] m_characterID=%d' % p.read_u32())
            print('    skills[] m_skillIndex=%d' % p.read_u32())
            print('    skills[] isUnlocked=%d' % p.read_u8())
            print('    skills[] isActivated=%d' % p.read_u8())
            np = p.read_u16()
            print('    skills[] m_properties_count=%d' % np)
            for _ in range(np):
                print('    skills[] m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    masterGears_count=%d' % n)
        for _ in range(n):
            print('    masterGears[] masterGearNo=%d' % p.read_u8())
            nl = p.read_u16()
            print('    masterGears[] name_len=%d' % nl)
            print('    masterGears[] name=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
            ns = p.read_u16()
            print('    masterGears[] slots_count=%d' % ns)
            for _ in range(ns):
                print('    masterGears[] slots[] gearType=%d gearItemID=0x%08x' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62131(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62131 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u8())
        print('    field_7=%d' % p.read_i64())
        print('    field_8=%d' % p.read_u16())
        print('    field_9=%d' % p.read_u8())
        print('    field_10=%d' % p.read_u32())
        print('    field_11=%d' % p.read_u8())
        print('    field_12=%d' % p.read_u32())
        print('    field_13=%d' % p.read_u8())
        print('    field_14=%d' % p.read_u32())
        print('    field_15=%d' % p.read_u32())
        print('    field_16=%d' % p.read_u32())
        print('    field_17=%d' % p.read_u32())
        print('    field_18=%d' % p.read_u8())
        print('    field_19=%d' % p.read_u8())
        print('    field_20=%d' % p.read_u32())
        print('    field_21=%d' % p.read_u32())
        print('    field_22=%d' % p.read_u8())
        print('    field_23=%d' % p.read_u8())
        print('    field_24=%d' % p.read_u16())
        print('    field_25=%d' % p.read_u32())
        print('    field_26=%d' % p.read_u32())
        print('    field_27=%d' % p.read_u8())
        print('    field_28=%d' % p.read_u16())
        print('    field_29=%d' % p.read_u16())
        print('    field_30=%d' % p.read_u32())
        print('    field_31=%d' % p.read_u32())
        print('}')
    def serialize_62132(netid, p: common.PacketReader):
        print('SA_SkillUpgrade {')
        print('    result=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    skill:')
        print('        m_characterID=%d' % p.read_u32())
        print('        m_skillIndex=%d' % p.read_u32())
        print('        isUnlocked=%d' % p.read_u8())
        print('        isActivated=%d' % p.read_u8())
        n = p.read_u16()
        print('        m_properties_count=%d' % n)
        for _ in range(n):
            print('        m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62133(netid, p: common.PacketReader):
        print('SA_SkillUpgradeIngame {')
        print('    result=%d' % p.read_u32())
        print('    mainID=0x%08x' % p.read_u32())
        print('    mainSkill {')
        print('        m_characterID=%d' % p.read_u32())
        print('        m_skillIndex=%d' % p.read_u32())
        print('        isUnlocked=%d' % p.read_u8())
        print('        isActivated=%d' % p.read_u8())
        n = p.read_u16()
        print('        m_properties_count=%d' % n)
        for _ in range(n):
            print('        m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('    }')
        print('    subID=0x%08x' % p.read_u32())
        print('    subSkill {')
        print('        m_characterID=%d' % p.read_u32())
        print('        m_skillIndex=%d' % p.read_u32())
        print('        isUnlocked=%d' % p.read_u8())
        print('        isActivated=%d' % p.read_u8())
        n = p.read_u16()
        print('        m_properties_count=%d' % n)
        for _ in range(n):
            print('        m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('    }')
        print('}')
    def serialize_62134(netid, p: common.PacketReader):
        print('SA_TitleSelect {')
        print('    Result=%d' % p.read_u32())
        print('    DisplayTitleIndex=%d' % p.read_u32())
        print('    StatTitleIndex=%d' % p.read_u32())
        print('}')
    def serialize_62135(netid, p: common.PacketReader):
        print('SN_TitleAdd {')
        print('    NewTitleAdd=%d' % p.read_i32())
        print('}')

    def serialize_62136(netid, p: common.PacketReader):
        print('SA_ItemEquip {')
        print('    result=%d' % p.read_u32())
        print('    itemID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62137(netid, p: common.PacketReader):
        print('SA_ItemUnequip {')
        print('    result=%d' % p.read_u32())
        print('    itemID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62138(netid, p: common.PacketReader):
        print('SA_SelectAccountEquipment {')
        print('    result=%d' % p.read_u32())
        print('    etcType=%d' % p.read_u8())
        print('    itemDocIndex=0x%08x' % p.read_u32())
        print('}')
    def serialize_62139(netid, p: common.PacketReader):
        print('SA_WeaponUnlock {')
        print('    result=%d' % p.read_u32())
        print('    newWeapon characterID=0x%08x' % p.read_u32())
        print('    newWeapon weaponType=%d' % p.read_u32())
        print('    newWeapon weaponIndex=%d' % p.read_u32())
        print('    newWeapon grade=%d' % p.read_u32())
        print('    newWeapon isUnlocked=%d' % p.read_u8())
        print('    newWeapon isActivated=%d' % p.read_u8())
        print('}')
    def serialize_62140(netid, p: common.PacketReader):
        print('SA_WeaponEquip {')
        print('    result=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    weaponType=%d' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('}')
    def serialize_62141(netid, p: common.PacketReader):
        print('SA_GearEquip {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('    gearItemID=0x%08x' % p.read_u32())
        print('    slot=%d' % p.read_u32())
        print('}')
    def serialize_62142(netid, p: common.PacketReader):
        print('SA_GearUnequip {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('    slot=%d' % p.read_u32())
        print('}')
    def serialize_62143(netid, p: common.PacketReader):
        print('SA_GearSwap {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('    fromSlot=%d' % p.read_u8())
        print('    fromGearItemID=%d' % p.read_u32())
        print('    toSlot=%d' % p.read_u8())
        print('    toGearItemID=%d' % p.read_u32())
        print('}')
    def serialize_62144(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62144 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u8())
        print('    field_3=%d' % p.read_u32())
        print('}')
    def serialize_62145(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62145 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u8())
        print('}')
    def serialize_62146(netid, p: common.PacketReader):
        print('SA_MastergearSelect {')
        print('    result=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('}')
    def serialize_62147(netid, p: common.PacketReader):
        print('SA_MastergearRename {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62148(netid, p: common.PacketReader):
        print('SA_MastergearRepair {')
        print('    result=%d' % p.read_u32())
        print('    durabillity=%d' % p.read_u32())
        print('}')
    def serialize_62149(netid, p: common.PacketReader):
        print('SA_MastergearAdd {')
        print('    result=%d' % p.read_u32())
        print('    newMasterGear {')
        print('        masterGearNo=%d' % p.read_u8())
        n = p.read_u16()
        print('        name_len=%d' % n)
        print('        name=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('        slots_count=%d' % n)
        for _ in range(n):
            print('        slots[] gearType=%d gearItemID=0x%08x' % (p.read_u32(), p.read_u32()))
        print('        }')
        print('}')
    def serialize_62150(netid, p: common.PacketReader):
        print('SA_CharacterUnlock {')
        print('    result=%d' % p.read_u32())
        print('    characterDocIndex=%d' % p.read_u32())
        print('}')
    def serialize_62151(netid, p: common.PacketReader):
        print('SA_SkillUnlock {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    newSkills_count=%d' % n)
        for _ in range(n):
            print('    newSkills[] m_characterID=%d m_skillIndex=%d isUnlocked=%d isActivated=%d' % (p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8()))
            m = p.read_u16()
            print('    newSkills[].m_properties_count=%d' % m)
            for _ in range(m):
                print('    newSkills[].m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62152(netid, p: common.PacketReader):
        print('SA_SkillSelect {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    result=%d' % p.read_u32())
        print('    dwSkillIndex=%d' % p.read_u32())
        print('    dwSkillSlot=%d' % p.read_u32())
        print('}')
    def serialize_62153(netid, p: common.PacketReader):
        print('SN_SkillSelect {')
        print('    userId=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    dwSkillIndex=%d' % p.read_u32())
        print('    dwSkillSlot=%d' % p.read_u32())
        print('}')
    def serialize_62154(netid, p: common.PacketReader):
        print('SA_SkillSwap {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    result=%d' % p.read_u32())
        print('    dwSlot1SkillIndex=%d' % p.read_u32())
        print('    dwSlot2SkillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62155(netid, p: common.PacketReader):
        print('SN_SkillSwap {')
        print('    userId=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    dwSlot1SkillIndex=%d' % p.read_u32())
        print('    dwSlot2SkillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62156(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62156 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62157(netid, p: common.PacketReader):
        print('SA_StartGame {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62159(netid, p: common.PacketReader):
        print('SN_RestartVoteStart {')
        print('    stageIndex=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefinitionType=%d' % p.read_u32())
        print('    stageRule=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_allies_count=%d' % n)
        for _ in range(n):
            print('    m_allies[] userId=%d' % p.read_u32())
            nlen = p.read_u16()
            print('    m_allies[] nickname_len=%d' % nlen)
            print('    m_allies[] nickname=%s' % p.read_raw(nlen * 2).decode('utf-16-le', errors='replace'))
            print('    m_allies[] isBot=%d' % p.read_u8())
            print('    m_allies[] tier=%d' % p.read_u32())
            print('    m_allies[] tierGroupRanking=%d' % p.read_u32())
            print('    m_allies[] tierSeriesFlag=%d' % p.read_u32())
            print('    m_allies[] pvpRate=%.2f' % p.read_f32())
        print('    voteTimeMS=%d' % p.read_u16())
        print('}')
    def serialize_62160(netid, p: common.PacketReader):
        print('SN_RestartVoteFinish {')
        print('    voteResult=%d' % p.read_u8())
        print('    voteFailReason=%d' % p.read_u8())
        print('}')
    def serialize_62161(netid, p: common.PacketReader):
        print('SN_RestartVoteUpdate {')
        n = p.read_u16()
        print('    voteList_count=%d' % n)
        for _ in range(n):
            print('    voteList[] %d' % p.read_u8())
        print('}')
    def serialize_62162(netid, p: common.PacketReader):
        print('SA_RestartGame {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62163(netid, p: common.PacketReader):
        print('SA_RestartVote {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62164(netid, p: common.PacketReader):
        print('SN_RestartPvpEntryStart {')
        print('    voteTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62165(netid, p: common.PacketReader):
        print('SN_RestartPvpEntryFinish {')
        print('}')
    def serialize_62166(netid, p: common.PacketReader):
        print('SA_RestartPvpEntryJoin {')
        print('    errcode=%d' % p.read_u32())
        print('}')
    def serialize_62167(netid, p: common.PacketReader):
        print('SA_RestartPvpEntryLeave {')
        print('    errcode=%d' % p.read_u32())
        print('}')
    def serialize_62168(netid, p: common.PacketReader):
        print('SN_RestartPvpEntryUpdate {')
        n = p.read_u16()
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    isJoined=%d' % p.read_u8())
        print('}')
    def serialize_62169(netid, p: common.PacketReader):
        print('SN_RestartPvpGameInfo {')
        print('    stageIndex=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u8())
        print('    gameDefType=%d' % p.read_u8())
        print('}')
    def serialize_62170(netid, p: common.PacketReader):
        print('SN_RestartPvpWaitingOthers {')
        print('}')
    def serialize_62171(netid, p: common.PacketReader):
        print('SN_RestartPvpCanceled {')
        print('    reasonCode=%d' % p.read_u32())
        print('}')
    def serialize_62172(netid, p: common.PacketReader):
        print('SN_MatchingPenalty {')
        print('    isMyPenalty=%d' % p.read_u8())
        print('    remainPenaltyTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62177(netid, p: common.PacketReader):
        print('SA_PartyOptionModify {')
        print('    retval=%d' % p.read_i32())
        print('    partyOptionType=%d' % p.read_u8())
        print('    enable=%d' % p.read_u8())
        print('}')

    def serialize_62178(netid, p: common.PacketReader):
        print('SN_PartySwapTeam {')
        print('    originTeam=%d' % p.read_u32())
        print('    originSlotIndex=%d' % p.read_u8())
        print('    targetTeam=%d' % p.read_u32())
        print('    targetSlotIndex=%d' % p.read_u8())
        print('}')
    def serialize_62179(netid, p: common.PacketReader):
        print('SA_PartySwapTeam {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62180(netid, p: common.PacketReader):
        print('SA_PartyInvite {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62181(netid, p: common.PacketReader):
        print('SA_PartyInviteRecommend {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62182(netid, p: common.PacketReader):
        print('SN_PartyInviteResponse {')
        n = p.read_u16()
        print('    account_len=%d' % n)
        account = p.read_raw(n * 2).decode('utf-16-le', errors='replace') if hasattr(p, 'read_wstr') else p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    account=%s' % account)
        print('    accept=%d' % p.read_u8())
        print('}')
    def serialize_62183(netid, p: common.PacketReader):
        print('SN_PartyInvite {')
        print('    channelID=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    stageIndexes_count=%d' % n)
        for _ in range(n):
            print('    stageIndexes[] %d' % p.read_u32())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefinitionType=%d' % p.read_u32())
        print('    stageRule=%d' % p.read_u32())
        print('    partyID=%d' % p.read_u32())
        print('    teamType=%d' % p.read_u32())
        print('}')
    def serialize_62184(netid, p: common.PacketReader):
        print('SN_PartyInviteRecommend {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62185(netid, p: common.PacketReader):
        print('SA_PartyInviteRecommendResponse {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62186(netid, p: common.PacketReader):
        print('SN_PartyInviteRecommendResponse {')
        print('    inviteeNickname=%s' % p.read_wstr())
        print('    accept=%d' % p.read_u8())
        print('}')
    def serialize_62187(netid, p: common.PacketReader):
        print('SA_PartyJoin {')
        print('    retval=%d' % p.read_u32())
        print('    partyID=%d' % p.read_u32())
        print('    stageType=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefinitionType=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_allies_count=%d' % n)
        for _ in range(n):
            print('    m_allies[] userId=%d' % p.read_u32())
            ln = p.read_u16()
            nick = p.buff[:ln*2].decode('utf-16-le', errors='replace')
            p.buff = p.buff[ln*2:]
            print('    m_allies[] nickname=%s' % nick)
            print('    m_allies[] isBot=%d' % p.read_u8())
            print('    m_allies[] tier=%d' % p.read_u32())
            print('    m_allies[] tierGroupRanking=%d' % p.read_u32())
            print('    m_allies[] tierSeriesFlag=%d' % p.read_u32())
            print('    m_allies[] pvpRate=%d' % p.read_u32())
        print('    alliesTeamType=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_enemies_count=%d' % n)
        for _ in range(n):
            print('    m_enemies[] userId=%d' % p.read_u32())
            ln = p.read_u16()
            nick = p.buff[:ln*2].decode('utf-16-le', errors='replace')
            p.buff = p.buff[ln*2:]
            print('    m_enemies[] nickname=%s' % nick)
            print('    m_enemies[] isBot=%d' % p.read_u8())
            print('    m_enemies[] tier=%d' % p.read_u32())
            print('    m_enemies[] tierGroupRanking=%d' % p.read_u32())
            print('    m_enemies[] tierSeriesFlag=%d' % p.read_u32())
            print('    m_enemies[] pvpRate=%d' % p.read_u32())
        print('    enemiesTeamType=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_spectators_count=%d' % n)
        for _ in range(n):
            print('    m_spectators[] userId=%d' % p.read_u32())
            ln = p.read_u16()
            nick = p.buff[:ln*2].decode('utf-16-le', errors='replace')
            p.buff = p.buff[ln*2:]
            print('    m_spectators[] nickname=%s' % nick)
            print('    m_spectators[] isBot=%d' % p.read_u8())
            print('    m_spectators[] tier=%d' % p.read_u32())
            print('    m_spectators[] tierGroupRanking=%d' % p.read_u32())
            print('    m_spectators[] tierSeriesFlag=%d' % p.read_u32())
            print('    m_spectators[] pvpRate=%d' % p.read_u32())
        print('    isSpectator=%d' % p.read_u8())
        print('}')
    def serialize_62188(netid, p: common.PacketReader):
        print('SN_PartyJoin {')
        print('    userId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    isBot=%d' % p.read_u8())
        print('    creatureIndex=%d' % p.read_u32())
        print('    isOwner=%d' % p.read_u8())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefinitionType=%d' % p.read_u32())
        print('    teamType=%d' % p.read_u32())
        print('    teamSlotIndex=%d' % p.read_u8())
        print('}')
    def serialize_62189(netid, p: common.PacketReader):
        print('SA_PartyLeave {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62190(netid, p: common.PacketReader):
        print('SN_PartyLeave {')
        print('    userId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    isOwner=%d' % p.read_u8())
        print('}')
    def serialize_62191(netid, p: common.PacketReader):
        print('SA_PartyAddBot {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62192(netid, p: common.PacketReader):
        print('SA_PartyRemoveBot {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62193(netid, p: common.PacketReader):
        print('SA_PartyBreakup {')
        print('    retval=%d' % p.read_u32())
        print('    remainMemberCount=%d' % p.read_u32())
        print('}')
    def serialize_62194(netid, p: common.PacketReader):
        print('SN_PartyBreakup {')
        print('}')
    def serialize_62195(netid, p: common.PacketReader):
        print('SN_PartyKicked {')
        print('    partyID=%d' % p.read_u32())
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62196(netid, p: common.PacketReader):
        print('SA_PartyKickAll {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62197(netid, p: common.PacketReader):
        print('SA_QuickRunArena {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    partyInfo.stageIndexes_count=%d' % n)
        for _ in range(n):
            print('    partyInfo.stageIndexes[]=%d' % p.read_u32())
        print('    partyInfo.gameType=%d' % p.read_u32())
        print('    partyInfo.gameDefinitionMode=%d' % p.read_u32())
        print('    partyInfo.selectSortieMasterType=%d' % p.read_u8())
        print('}')
    def serialize_62198(netid, p: common.PacketReader):
        print('SN_EnqueueTrollPenalty {')
        print('    retval=%d' % p.read_u32())
        print('    trollReducingGameCount=%d' % p.read_u16())
        print('    maxPenaltyMinutes=%d' % p.read_f32())
        print('}')
    def serialize_62199(netid, p: common.PacketReader):
        print('SA_EnqueueTrollPenaltyCancel {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62200(netid, p: common.PacketReader):
        print('SN_EnqueueTrollPenaltyCancel {')
        print('}')
    def serialize_62201(netid, p: common.PacketReader):
        print('SN_EnqueueMatchingQueue {')
        print('    stageIndex=%d' % p.read_i32())
        print('    currentMatchingTimeMS=%d' % p.read_i32())
        print('    avgMatchingTimeMS=%d' % p.read_i32())
        print('    disableMatchExpansion=%d' % p.read_u8())
        print('    isMatchingExpanded=%d' % p.read_u8())
        print('}')

    def serialize_62202(netid, p: common.PacketReader):
        print('SA_CancelMatchingQueue {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62203(netid, p: common.PacketReader):
        print('SN_CancelMatchingQueue {')
        print('    byRemoveMember=%d' % p.read_u8())
        print('}')
    def serialize_62206(netid, p: common.PacketReader):
        print('SN_MatchingPartyConfirm {')
        print('    userId=%d' % p.read_u32())
        print('    teamType=%d' % p.read_u32())
        print('    comfirmed=%d' % p.read_u8())
        print('}')
    def serialize_62207(netid, p: common.PacketReader):
        print('SN_ChannelChattingChannel {')
        print('    channelType=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62209(netid, p: common.PacketReader):
        print('SN_MasterPick {')
        print('    userID=0x%08x' % p.read_u32())
        count = p.read_u16()
        print('    characterSelectInfos(%d)=[' % count)
        while count > 0:
            print('    {')
            print('      characterID=0x%08x' % p.read_u32())
            print('      creatureIndex=%d' % p.read_i32())
            print('      skillSlot1=%d' % p.read_i32())
            print('      skillSlot2=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

    def serialize_62210(netid, p: common.PacketReader):
        print('SA_MasterUnpick {')
        print('    retval=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62211(netid, p: common.PacketReader):
        print('SN_MasterUnpick {')
        print('    userId=%d' % p.read_u32())
        n = p.read_u16()
        print('    m_characterSelectInfos_count=%d' % n)
        for _ in range(n):
            print('    m_characterSelectInfos[] characterID=0x%08x creatureIndex=%d skillSlot1=%d skillSlot2=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62212(netid, p: common.PacketReader):
        print('SA_MasterUnpickAll {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62213(netid, p: common.PacketReader):
        print('SN_MasterUnpickAll {')
        print('    userId=%d' % p.read_u32())
        n = p.read_u16()
        print('    masters_count=%d' % n)
        for _ in range(n):
            print('    masters[] characterID=0x%08x creatureIndex=%d skillSlot1=%d skillSlot2=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62214(netid, p: common.PacketReader):
        print('SN_ReadySortieRoom{')
        print('    userId=%d' % p.read_i32())
        print('    ready=%d' % p.read_u8())
        print('}')
    def serialize_62215(netid, p: common.PacketReader):
        print('SA_SortieRoomBreakup {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62216(netid, p: common.PacketReader):
        print('SN_SortieRoomBreakup {')
        print('    stageType=%d' % p.read_u32())
        print('    nReason=%d' % p.read_u32())
        print('}')
    def serialize_62220(netid, p: common.PacketReader):
        print('SA_WeaponUpgrade {')
        print('    result=%d' % p.read_u32())
        print('    weaponInfo characterID=0x%08x' % p.read_u32())
        print('    weaponInfo weaponType=%d' % p.read_u32())
        print('    weaponInfo weaponIndex=%d' % p.read_u32())
        print('    weaponInfo grade=%d' % p.read_u32())
        print('    weaponInfo isUnlocked=%d' % p.read_u8())
        print('    weaponInfo isActivated=%d' % p.read_u8())
        print('}')
    def serialize_62221(netid, p: common.PacketReader):
        print('SA_InventoryExpand {')
        print('    result=%d' % p.read_u32())
        print('    invenTab=%d' % p.read_u32())
        print('    lineCount=%d' % p.read_u32())
        print('}')
    def serialize_62222(netid, p: common.PacketReader):
        print('SN_DespawnMonsters {')
        n = p.read_u16()
        print('    clearIDs_count=%d' % n)
        for _ in range(n):
            print('    clearIDs[] %d' % p.read_u32())
        print('}')
    def serialize_62223(netid, p: common.PacketReader):
        print('SN_DespawnMonster {')
        print('    objectID=0x%08x' % p.read_u32())
        print('}')
    def serialize_62225(netid, p: common.PacketReader):
        print('SN_UpdateAihost {')
        print('    serverBot=%d' % p.read_u8())
        print('    userId=%d' % p.read_u32())
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62226(netid, p: common.PacketReader):
        print('SA_RequestSummaryInfoEach {')
        print('    summaryIndex=%d' % p.read_u32())
        n = p.read_u16()
        print('    summaryInfoList_count=%d' % n)
        for _ in range(n):
            print('    summaryInfoList[] stageIndex=%d summaryType=%d summaryData=%d rewardReceived=%d completedDate=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_i64()))
        print('}')
    def serialize_62228(netid, p: common.PacketReader):
        print('SN_SummaryUpdate {')
        print('    summaryInfo {')
        print('        summaryIndex=%d' % p.read_u32())
        print('        stageIndex=%d' % p.read_u32())
        print('        summaryType=%d' % p.read_u32())
        print('        summaryData=%d' % p.read_u32())
        print('        rewardReceived=%d' % p.read_u8())
        print('        completedDate=%d' % p.read_i64())
        print('    }')
        print('}')
    def serialize_62230(netid, p: common.PacketReader):
        print('SN_AddStatus {')
        bits = p.read_u8()
        print('    excludedFieldBits=%d' % bits)
        if not (bits & 1):
            print('    extraGraphInfo.totalRatio=%.3f' % p.read_f32())
            print('    extraGraphInfo.defeatFrom=%s' % read_Vec3(p))
            print('    extraGraphInfo.defeatTo=%s' % read_Vec3(p))
            print('    extraGraphInfo.defeatDist=%.2f' % p.read_f32())
            print('    extraGraphInfo.defeatOriginDist=%.2f' % p.read_f32())
            print('    extraGraphInfo.defeatDurationTime=%.2f' % p.read_f32())
        print('    statusID=%d' % p.read_u32())
        print('    bEnabled=%d' % p.read_u8())
        print('    targetID=%d' % p.read_u32())
        print('    casterID=%d' % p.read_u32())
        print('    isOverlap=%d' % p.read_u8())
        print('    overlapCount=%d' % p.read_u8())
        print('    customValue=%d' % p.read_u8())
        print('    durationTime=%.2f' % p.read_f32())
        print('    elapsedTime=%.2f' % p.read_f32())
        print('}')
    def serialize_62231(netid, p: common.PacketReader):
        print('SN_ModifyStatus {')
        print('    statusID=%d' % p.read_u32())
        print('    bEnabled=%d' % p.read_u8())
        print('    targetID=%d' % p.read_u32())
        print('    casterID=%d' % p.read_u32())
        print('    replaceCasterID=%d' % p.read_u32())
        print('    overlapCount=%d' % p.read_u8())
        print('    customValue=%d' % p.read_u8())
        print('    extraStatusProcEvent=%d' % p.read_u8())
        print('}')
    def serialize_62232(netid, p: common.PacketReader):
        print('SN_RemoveStatus {')
        print('    status=%d' % p.read_i32())
        print('    targetID=%d' % p.read_i32())
        print('    casterID=%d' % p.read_i32())
        print('}')

    def serialize_62233(netid, p: common.PacketReader):
        print('SN_EnterUserByTrespass {')
        print('    usn=%d' % p.read_u32())
        print('}')
    def serialize_62234(netid, p: common.PacketReader):
        print('SN_LeaveUser {')
        print('    usn=%d' % p.read_u32())
        print('    canReturn=%d' % p.read_u8())
        print('}')
    def serialize_62235(netid, p: common.PacketReader):
        print('SN_BroadcastDamage {')
        print('    damageSeqNum=%d' % p.read_u32())
        print('    remoteID=%d' % p.read_u32())
        print('    defID=%d' % p.read_u32())
        print('    attID=%d' % p.read_u32())
        print('    remoteDocIndex=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    remotePos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    remoteDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    remoteForceDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    hitPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    hitDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    damageType=%d' % p.read_u32())
        print('    skillDocID=%d' % p.read_u32())
        print('    nRagePoint=%d' % p.read_u32())
        print('    nRelativeElement=%d' % p.read_u32())
        n = p.read_u16()
        print('    hitNodeName_len=%d' % n)
        print('    hitNodeName=%r' % p.read_raw(n).decode('utf-8', 'replace'))
        print('    damage=%d' % p.read_u32())
        n = p.read_u16()
        print('    partName_len=%d' % n)
        print('    partName=%r' % p.read_raw(n * 2).decode('utf-16-le', 'replace'))
        print('    partDamage=%d' % p.read_u32())
        print('    masterGroupingDamage=%d' % p.read_u32())
        print('    optionalResultOfHit=%d' % p.read_u32())
        print('}')
    def serialize_62236(netid, p: common.PacketReader):
        print('SN_BroadcastNonRemoteDamage {')
        print('    defID=%d' % p.read_u32())
        print('    attID=%d' % p.read_u32())
        print('    statusDocID=%d' % p.read_u32())
        print('    skillDocID=%d' % p.read_u32())
        print('    damageType=%d' % p.read_u32())
        print('    damage=%d' % p.read_u32())
        print('    masterGroupingDamage=%d' % p.read_u32())
        print('}')
    def serialize_62237(netid, p: common.PacketReader):
        print('SN_RemoteActivated {')
        print('    remoteID=%d' % p.read_u32())
        print('    defID=%d' % p.read_u32())
        print('    penetrationCount=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    remotePos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62239(netid, p: common.PacketReader):
        print('SA_ChannelChat {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62240(netid, p: common.PacketReader):
        print('SN_ChatChannelJoin {')
        print('    channelType=%d' % p.read_u32())
        n = p.read_u16()
        s = ''.join(chr(p.read_u16()) for _ in range(n))
        print('    chatChannelName_len=%d' % n)
        print('    chatChannelName=%s' % s)
        n = p.read_u16()
        s = ''.join(chr(p.read_u16()) for _ in range(n))
        print('    password_len=%d' % n)
        print('    password=%s' % s)
        print('}')
    def serialize_62241(netid, p: common.PacketReader):
        print('SN_ChatChannelLeave {')
        print('    channelType=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62243(netid, p: common.PacketReader):
        print('SN_ChatChannelUserJoin {')
        n = p.read_u16()
        print('    channelName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    nickname=%s' % p.read_wstr())
        print('}')
    def serialize_62244(netid, p: common.PacketReader):
        print('SN_ChatChannelUserLeave {')
        n = p.read_u16()
        print('    channelName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    nickname=%s' % p.read_wstr())
        print('}')
    def serialize_62245(netid, p: common.PacketReader):
        print('SN_FriendAdded {')
        print('    newFriendInfo {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    leaderCreatureIndex=%d' % p.read_u32())
        print('    state=%d' % p.read_u8())
        print('    level=%d' % p.read_u16())
        print('    isFavorite=%d' % p.read_u8())
        print('    isComrade=%d' % p.read_u8())
        print('    isOnLeavePenalty=%d' % p.read_u8())
        print('    lastLogoutDate=%d' % p.read_i64())
        print('    }')
        print('}')
    def serialize_62246(netid, p: common.PacketReader):
        print('SA_FriendRemove {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    targetNickname_len=%d' % n)
        s = ''
        for _ in range(n):
            s += chr(p.read_u16())
        print('    targetNickname=%s' % s)
        print('}')
    def serialize_62247(netid, p: common.PacketReader):
        print('SA_CommunityBlockAdd {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    userinfo[] nickname_len=%d' % n)
        print('    userinfo[] nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62248(netid, p: common.PacketReader):
        print('SA_CommunityBlockRemove {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62249(netid, p: common.PacketReader):
        print('SN_FriendRemoved {')
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        raw = p.read_raw(n * 2)
        print('    nickname=%s' % raw.decode('utf-16-le', errors='replace'))
        print('}')

    def serialize_62250(netid, p: common.PacketReader):
        print('SA_FriendStateChange {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62251(netid, p: common.PacketReader):
        print('SN_FriendStateChanged {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    newState=%d' % p.read_u8())
        print('    isOnLeavePenalty=%d' % p.read_u8())
        print('    stateUpdatedTime=%d' % p.read_i64())
        print('}')
    def serialize_62252(netid, p: common.PacketReader):
        print('SN_FriendNicknameChanged {')
        n = p.read_u16()
        print('    oldNickname_len=%d' % n)
        print('    oldNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    newNickname_len=%d' % n)
        print('    newNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62253(netid, p: common.PacketReader):
        print('SA_CommunityStatusMessage {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62254(netid, p: common.PacketReader):
        print('SA_CommunityChannelMove {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62255(netid, p: common.PacketReader):
        print('SN_CommunityStatusMessage {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62256(netid, p: common.PacketReader):
        print('SN_FriendLeaderChanged {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    creatureIndex=%d' % p.read_u32())
        print('}')
    def serialize_62260(netid, p: common.PacketReader):
        print('SN_RecommendedfriendList {')
        n = p.read_u16()
        print('    candidates_count=%d' % n)
        for _ in range(n):
            slen = p.read_u16()
            s = p.read_raw(slen * 2).decode('utf-16-le', errors='replace')
            print('    candidates[] nickname=%s' % s)
            print('    candidates[] leaderCreatureIndex=%d' % p.read_u32())
        print('}')
    def serialize_62262(netid, p: common.PacketReader):
        print('SA_FriendRequest {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62263(netid, p: common.PacketReader):
        print('SN_FriendRequested {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    leaderCreatureIndex=%d' % p.read_u32())
        print('}')
    def serialize_62264(netid, p: common.PacketReader):
        print('SA_FriendRespond {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        chars = ''.join(chr(p.read_u16()) for _ in range(n))
        print('    nickname=%s' % chars)
        print('    bAccept=%d' % p.read_u8())
        print('}')
    def serialize_62265(netid, p: common.PacketReader):
        print('SN_FriendResponded {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    accept=%d' % p.read_u8())
        print('}')
    def serialize_62266(netid, p: common.PacketReader):
        print('SA_FriendRequestLeaderInfo {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    statusMessage=%s' % p.read_wstr())
        print('    level=%d' % p.read_u16())
        print('    creatureIndex=%d' % p.read_u32())
        print('    power=%d' % p.read_u32())
        print('    viability=%d' % p.read_u32())
        print('    tierType=%d' % p.read_u32())
        n = p.read_u16()
        print('    skills_count=%d' % n)
        for _ in range(n):
            print('    skills[] m_characterID=%d m_skillIndex=%d isUnlocked=%d isActivated=%d' % (
                p.read_u32(), p.read_u32(), p.read_u8(), p.read_u8()))
            m = p.read_u16()
            print('        m_properties_count=%d' % m)
            for _ in range(m):
                print('        m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62267(netid, p: common.PacketReader):
        print('SA_RequestLeaderInfo {')
        print('    retval=%d' % p.read_u32())
        print('    charinfo {')
        n = p.read_u16()
        print('        nickname_len=%d' % n)
        print('        nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('        statusMessage_len=%d' % n)
        print('        statusMessage=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('        level=%d' % p.read_u16())
        print('        creatureIndex=%d' % p.read_u32())
        print('        power=%d' % p.read_u32())
        print('        viability=%d' % p.read_u32())
        print('        tierType=%d' % p.read_u32())
        cnt = p.read_u16()
        print('        skills_count=%d' % cnt)
        for _ in range(cnt):
            print('        skills[] {')
            print('            m_characterID=%d' % p.read_u32())
            print('            m_skillIndex=%d' % p.read_u32())
            print('            isUnlocked=%d' % p.read_u8())
            print('            isActivated=%d' % p.read_u8())
            pcnt = p.read_u16()
            print('            m_properties_count=%d' % pcnt)
            for _ in range(pcnt):
                print('            m_properties[] m_skillPropertyIndex=%d m_level=%d' % (p.read_u32(), p.read_u32()))
            print('            }')
        print('        }')
        print('}')
    def serialize_62268(netid, p: common.PacketReader):
        print('SA_FriendSetComrade {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62269(netid, p: common.PacketReader):
        print('SA_FriendUnsetComrade {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    friendNickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le')
        print('    friendNickname=%s' % s)
        print('}')
    def serialize_62270(netid, p: common.PacketReader):
        print('SA_FriendSetFavorite {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    friendNickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    friendNickname=%s' % s)
        print('}')
    def serialize_62271(netid, p: common.PacketReader):
        print('SA_FriendUnsetFavorite {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62272(netid, p: common.PacketReader):
        print('SN_FriendPartycreation {')
        n = p.read_u16()
        print('    friendNickname_len=%d' % n)
        print('    friendNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    stageIndex=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u8())
        print('    gameDefType=%d' % p.read_u8())
        print('}')
    def serialize_62273(netid, p: common.PacketReader):
        print('SA_RequestMissionReward {')
        print('    result=%d' % p.read_u32())
        print('    mission.index=%d' % p.read_u32())
        print('    mission.progressCount=%d' % p.read_i64())
        print('    mission.changedCount=%d' % p.read_u16())
        print('    mission.isCleared=%d' % p.read_u8())
        print('    mission.isRewarded=%d' % p.read_u8())
        print('}')
    def serialize_62274(netid, p: common.PacketReader):
        print('SN_PVPAvailableReportCount {')
        print('    isTroll=%d' % p.read_u8())
        print('    remainingCount=%d' % p.read_u8())
        print('}')

    def serialize_62275(netid, p: common.PacketReader):
        print('SN_ReturnRoomExpirationCountdown {')
        print('}')
    def serialize_62277(netid, p: common.PacketReader):
        print('SN_GlobalNotice {')
        print('    chatType=%d' % p.read_u32())
        print('    durationMS=%d' % p.read_u16())
        n = p.read_u16()
        print('    noticeList_count=%d' % n)
        for _ in range(n):
            print('    noticeList[] locale=%d' % p.read_u8())
            m = p.read_u16()
            s = p.read_raw(m * 2).decode('utf-16-le', errors='replace')
            print('    noticeList[] message=%s' % s)
        print('}')
    def serialize_62279(netid, p: common.PacketReader):
        print('SN_ExpGain {')
        print('    amount=%d' % p.read_u32())
        print('}')
    def serialize_62280(netid, p: common.PacketReader):
        print('SA_GetUserinfo {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    level=%d' % p.read_u16())
        print('    achievementScore=%d' % p.read_u32())
        print('    tierGrade=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    creatureIndex=%d' % p.read_u32())
        print('    power=%d' % p.read_u32())
        print('    viability=%d' % p.read_u32())
        print('    skinId=%d' % p.read_u8())
        n = p.read_u16()
        print('    skills_count=%d' % n)
        for _ in range(n):
            print('    skills[] index=%d level=%d isUnlocked=%d' % (p.read_u32(), p.read_u8(), p.read_u8()))
        print('    weapon type=%d index=%d grade=%d isUnlocked=%d' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_u8()))
        n = p.read_u16()
        print('    masterGears_count=%d' % n)
        for _ in range(n):
            print('    masterGears[] itemID=0x%08x invenType=%d slot=%d itemIndex=0x%08x count=%d propertyGroupIndex=%d isLifeTimeAbsolute=%d lifeEndTimeUTC=%d' % (p.read_u32(), p.read_u8(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u8(), p.read_i64()))
            m = p.read_u16()
            print('    properties_count=%d' % m)
            for _ in range(m):
                print('    properties[] %d %d %d %d %d' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_u32(), p.read_u8()))
        print('}')
    def serialize_62281(netid, p: common.PacketReader):
        print('SA_JukeboxEnqueue {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62286(netid, p: common.PacketReader):
        print('SN_OpenEventMasters {')
        n = p.read_u16()
        print('    openedEventMasters_count=%d' % n)
        for _ in range(n):
            print('    openedEventMasters[]=%d' % p.read_u32())
        print('}')
    def serialize_62287(netid, p: common.PacketReader):
        print('SA_CreateGuild {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildName_len=%d' % n)
        print('    guildName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    guildTag_len=%d' % n)
        print('    guildTag=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    masterTopPvpTierGrade=%d' % p.read_u8())
        print('    masterTopPvpTierPoint=%d' % p.read_u16())
        print('    masterAchievementScore=%d' % p.read_u32())
        print('}')
    def serialize_62288(netid, p: common.PacketReader):
        print('SA_DissolveGuild {')
        print('    result=%d' % p.read_u32())
        print('    dissolutionDate=%d' % p.read_i64())
        print('}')
    def serialize_62289(netid, p: common.PacketReader):
        print('SA_CancelGuilddissolution {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62290(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62290 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62291(netid, p: common.PacketReader):
        print('SA_QuitGuild {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62292(netid, p: common.PacketReader):
        print('SA_KickGuildmember {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62293(netid, p: common.PacketReader):
        print('SA_JoinGuild {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildName_len=%d' % n)
        guildName = p.read_raw(n * 2).decode('utf-16-le', errors='replace') if hasattr(p, 'read_wstr') else p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    guildName=%s' % guildName)
        print('    guildJoinType=%d' % p.read_u8())
        print('}')
    def serialize_62294(netid, p: common.PacketReader):
        print('SA_GetGuildjoinrequestlist {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildJoinRequestList_count=%d' % n)
        for _ in range(n):
            print('    guildJoinRequestList[] {')
            print('        candidateNickname_len=%d' % (clen := p.read_u16()))
            print('        candidateNickname=%s' % p.read_raw(clen * 2).decode('utf-16-le', errors='replace'))
            print('        lvl=%d' % p.read_u16())
            print('        leaderClassType=%d' % p.read_u16())
            print('        masterCount=%d' % p.read_u16())
            print('        achievementScore=%d' % p.read_u32())
            print('        topPvpTierGrade=%d' % p.read_u8())
            print('        topPvpTierPoint=%d' % p.read_u16())
            print('    }')
        print('}')
    def serialize_62295(netid, p: common.PacketReader):
        print('SA_RespondGuildjoinrequest {')
        print('    result=%d' % p.read_u32())
        print('    isApproval=%d' % p.read_u8())
        print('    candidate {')
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    membershipId=%d' % p.read_u32())
        print('    lvl=%d' % p.read_u16())
        print('    leaderClassType=%d' % p.read_u16())
        print('    masterCount=%d' % p.read_u16())
        print('    achievementScore=%d' % p.read_u32())
        print('    topPvpTierGrade=%d' % p.read_u8())
        print('    topPvpTierPoint=%d' % p.read_u16())
        print('    contributedGuildPoint=%d' % p.read_u32())
        print('    contributedGuildFund=%d' % p.read_u32())
        print('    guildPvpWin=%d' % p.read_u16())
        print('    guildPvpPlay=%d' % p.read_u16())
        print('    lastLogoutDate=%d' % p.read_i64())
        print('    }')
        print('}')
    def serialize_62296(netid, p: common.PacketReader):
        print('SA_InviteGuildmember {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    inviteeNickname_len=%d' % n)
        s = ''.join(chr(p.read_u16()) for _ in range(n))
        print('    inviteeNickname=%s' % s)
        print('}')
    def serialize_62297(netid, p: common.PacketReader):
        print('SA_GetGuildinvitationlist {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildInvitationList_count=%d' % n)
        for _ in range(n):
            print('    guildInvitationList[] {')
            print('        guildName=%s' % p.read_wstr())
            print('        inviterName=%s' % p.read_wstr())
            print('    }')
        print('}')
    def serialize_62298(netid, p: common.PacketReader):
        print('SA_RespondGuildinvitation {')
        print('    result=%d' % p.read_u32())
        print('    accept=%d' % p.read_u8())
        n = p.read_u16()
        s = p.read_raw(n * 2).decode('utf-16-le')
        print('    guildName_len=%d' % n)
        print('    guildName=%s' % s)
        n = p.read_u16()
        s = p.read_raw(n * 2).decode('utf-16-le')
        print('    guildTag_len=%d' % n)
        print('    guildTag=%s' % s)
        print('}')
    def serialize_62301(netid, p: common.PacketReader):
        print('SA_GetGuildemblemlist {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildEmblemList_count=%d' % n)
        for _ in range(n):
            print('    guildEmblemList[] %d' % p.read_u32())
        print('}')
    def serialize_62303(netid, p: common.PacketReader):
        print('SA_GetGuildpublicprofile {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    emblemIndex=%d' % p.read_u32())
        print('    guildLvl=%d' % p.read_u8())
        print('    memberMax=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    createdDate=%d' % p.read_i64())
        print('    dissolutionDate=%d' % p.read_i64())
        print('    joinType=%d' % p.read_u8())
        print('    guildInterest likePveStage=%d' % p.read_u8())
        print('    guildInterest likeDefence=%d' % p.read_u8())
        print('    guildInterest likePvpNormal=%d' % p.read_u8())
        print('    guildInterest likePvpOccupy=%d' % p.read_u8())
        print('    guildInterest likePvpGot=%d' % p.read_u8())
        print('    guildInterest likePvpRank=%d' % p.read_u8())
        print('    guildInterest likeOlympic=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    memberNum=%d' % p.read_u8())
        print('}')
    def serialize_62304(netid, p: common.PacketReader):
        print('SA_GetGuildpublicprofileFromUser {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    emblemIndex=%d' % p.read_u32())
        print('    guildLvl=%d' % p.read_u8())
        print('    memberMax=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    createdDate=%d' % p.read_i64())
        print('    dissolutionDate=%d' % p.read_i64())
        print('    joinType=%d' % p.read_u8())
        print('    guildInterest {')
        print('        likePveStage=%d' % p.read_u8())
        print('        likeDefence=%d' % p.read_u8())
        print('        likePvpNormal=%d' % p.read_u8())
        print('        likePvpOccupy=%d' % p.read_u8())
        print('        likePvpGot=%d' % p.read_u8())
        print('        likePvpRank=%d' % p.read_u8())
        print('        likeOlympic=%d' % p.read_u8())
        print('    }')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    memberNum=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62305(netid, p: common.PacketReader):
        print('SA_EditGuildnotice {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildNotice_len=%d' % n)
        if hasattr(p, 'read_wstr'):
            s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        else:
            s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    guildNotice=%s' % s)
        print('}')
    def serialize_62306(netid, p: common.PacketReader):
        print('SA_EditGuildintro {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62307(netid, p: common.PacketReader):
        print('SA_EditGuildtag {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    guildTag_len=%d' % n)
        print('    guildTag=%s' % s)
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62308(netid, p: common.PacketReader):
        print('SA_SetGuildinterest {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62309(netid, p: common.PacketReader):
        print('SA_SetGuildemblem {')
        print('    result=%d' % p.read_u32())
        print('    emblemIndex=%d' % p.read_u32())
        print('}')
    def serialize_62310(netid, p: common.PacketReader):
        print('SA_SetGuildjointype {')
        print('    result=%d' % p.read_u32())
        print('    joinType=%d' % p.read_u8())
        print('}')
    def serialize_62311(netid, p: common.PacketReader):
        print('SA_SetGuildmemberclass {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    oldMembershipId=%d' % p.read_u32())
        print('    newMembershipId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62312(netid, p: common.PacketReader):
        print('SA_SetGuildmaster {')
        print('    result=%d' % p.read_u32())
        print('    myMembershipId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62313(netid, p: common.PacketReader):
        print('SA_CreateGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    hasInviteRight=%d' % p.read_u8())
        print('    hasExpelRight=%d' % p.read_u8())
        print('    hasMembershipChgRight=%d' % p.read_u8())
        print('    hasClassAssignRight=%d' % p.read_u8())
        print('    hasNoticeChgRight=%d' % p.read_u8())
        print('    hasIntroChgRight=%d' % p.read_u8())
        print('    hasInterestChgRight=%d' % p.read_u8())
        print('    hasFundManageRight=%d' % p.read_u8())
        print('    hasJoinTypeChgRight=%d' % p.read_u8())
        print('    hasEmblemChgRight=%d' % p.read_u8())
        print('}')
    def serialize_62314(netid, p: common.PacketReader):
        print('SA_DeleteGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    regularMembershipId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    reassignedMemberNicknames_count=%d' % n)
        for _ in range(n):
            print('    reassignedMemberNicknames[] %s' % p.read_wstr())
        print('}')
    def serialize_62315(netid, p: common.PacketReader):
        print('SA_EditGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    rights[] hasInviteRight=%d hasExpelRight=%d hasMembershipChgRight=%d hasClassAssignRight=%d hasNoticeChgRight=%d hasIntroChgRight=%d hasInterestChgRight=%d hasFundManageRight=%d hasJoinTypeChgRight=%d hasEmblemChgRight=%d' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8()))
        print('}')
    def serialize_62316(netid, p: common.PacketReader):
        print('SA_DonateToGuild {')
        print('    result=%d' % p.read_u32())
        print('    donatedGold=%d' % p.read_u32())
        print('    guildFundGain=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62317(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62317 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_u32())
        print('}')
    def serialize_62318(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62318 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_u32())
        print('}')
    def serialize_62319(netid, p: common.PacketReader):
        print('SA_GetGuildskilllist {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guildSkills_count=%d' % n)
        for _ in range(n):
            print('    guildSkills[] type=%d lvl=%d expiryDate=%d extensionCount=%d' % (p.read_u8(), p.read_u8(), p.read_i64(), p.read_u16()))
        print('}')
    def serialize_62320(netid, p: common.PacketReader):
        print('SA_BuyGuildskill {')
        print('    result=%d' % p.read_u32())
        print('    guildSkillType=%d' % p.read_u8())
        print('    guildSkillLvl=%d' % p.read_u8())
        print('    expiryDate=%d' % p.read_i64())
        print('    timeLimitHour=%d' % p.read_u16())
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62321(netid, p: common.PacketReader):
        print('SA_ExtendGuildlimitedskill {')
        print('    result=%d' % p.read_u32())
        print('    guildSkillType=%d' % p.read_u8())
        print('    guildSkillLvl=%d' % p.read_u8())
        print('    extensionCount=%d' % p.read_u16())
        print('    expiryDate=%d' % p.read_i64())
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62323(netid, p: common.PacketReader):
        print('SA_GetGuildrank {')
        print('    result=%d' % p.read_u32())
        print('    rankingType=%d' % p.read_u8())
        print('    seasonNo=%d' % p.read_u16())
        print('    rankInfo:')
        print('    rankNo=%d' % p.read_u32())
        print('    prevRankNo=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    rankingValue=%d' % p.read_u32())
        print('}')
    def serialize_62324(netid, p: common.PacketReader):
        print('SA_GetGuildranklist {')
        print('    result=%d' % p.read_u32())
        print('    rankingType=%d' % p.read_u8())
        print('    seasonNo=%d' % p.read_u16())
        print('    pageNo=%d' % p.read_u32())
        print('    rowsPerPage=%d' % p.read_u8())
        print('    maxPage=%d' % p.read_u32())
        print('    latestUpdateTime=%d' % p.read_i64())
        n = p.read_u16()
        print('    guildRankList_count=%d' % n)
        for _ in range(n):
            rankNo = p.read_u32()
            prevRankNo = p.read_u32()
            guildTag = p.read_wstr()
            guildName = p.read_wstr()
            guildOwnerNickname = p.read_wstr()
            rankingValue = p.read_u32()
            print('    guildRankList[] rankNo=%d prevRankNo=%d guildTag=%s guildName=%s guildOwnerNickname=%s rankingValue=%d' % (rankNo, prevRankNo, guildTag, guildName, guildOwnerNickname, rankingValue))
        print('}')
    def serialize_62325(netid, p: common.PacketReader):
        print('SA_GetGuildrankrewardinfo {')
        print('    result=%d' % p.read_u32())
        print('    rankingType=%d' % p.read_u8())
        n = p.read_u16()
        print('    rewards_count=%d' % n)
        for _ in range(n):
            print('    rewards[] firstRankNo=%d lastRankNo=%d' % (p.read_u32(), p.read_u32()))
            m = p.read_u16()
            print('    rewards[].items_count=%d' % m)
            for _ in range(m):
                print('    rewards[].items[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62326(netid, p: common.PacketReader):
        print('SA_GetRecommendedguildlist {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    guilds_count=%d' % n)
        for _ in range(n):
            print('    guilds[] guildName=%s' % p.read_wstr())
            print('    guilds[] guildTag=%s' % p.read_wstr())
            print('    guilds[] emblemIndex=%d' % p.read_u32())
            print('    guilds[] guildLvl=%d' % p.read_u8())
            print('    guilds[] joinType=%d' % p.read_u8())
            print('    guilds[] memberNum=%d' % p.read_u8())
            print('    guilds[] memberMax=%d' % p.read_u8())
            likePveStage = p.read_u8()
            likeDefence = p.read_u8()
            likePvpNormal = p.read_u8()
            likePvpOccupy = p.read_u8()
            likePvpGot = p.read_u8()
            likePvpRank = p.read_u8()
            likeOlympic = p.read_u8()
            print('    guilds[] interest=(pve=%d,def=%d,pvpN=%d,pvpO=%d,pvpG=%d,pvpR=%d,oly=%d)' % (likePveStage, likeDefence, likePvpNormal, likePvpOccupy, likePvpGot, likePvpRank, likeOlympic))
        print('}')
    def serialize_62327(netid, p: common.PacketReader):
        print('SA_SearchGuild {')
        print('    result=%d' % p.read_u32())
        print('    pageNo=%d' % p.read_u32())
        print('    rowsPerPage=%d' % p.read_u8())
        print('    maxPage=%d' % p.read_u32())
        n = p.read_u16()
        print('    guilds_count=%d' % n)
        for _ in range(n):
            print('    guilds[] guildName=%s' % p.read_wstr())
            print('    guilds[] guildTag=%s' % p.read_wstr())
            print('    guilds[] emblemIndex=%d' % p.read_u32())
            print('    guilds[] guildLvl=%d' % p.read_u8())
            print('    guilds[] joinType=%d' % p.read_u8())
            print('    guilds[] memberNum=%d' % p.read_u8())
            print('    guilds[] memberMax=%d' % p.read_u8())
            print('    guilds[] interest likePveStage=%d' % p.read_u8())
            print('    guilds[] interest likeDefence=%d' % p.read_u8())
            print('    guilds[] interest likePvpNormal=%d' % p.read_u8())
            print('    guilds[] interest likePvpOccupy=%d' % p.read_u8())
        print('}')
    def serialize_62328(netid, p: common.PacketReader):
        print('SA_GetPlayingGuildpvplist {')
        print('    result=%d' % p.read_u32())
        print('    pageNo=%d' % p.read_u16())
        print('    rowsPerPage=%d' % p.read_u8())
        print('    maxPageNo=%d' % p.read_u16())
        n = p.read_u16()
        print('    guildPvpList_count=%d' % n)
        for _ in range(n):
            print('    guildPvpList[] {')
            print('        instanceId=%d' % p.read_u32())
            print('        createdDate=%d' % p.read_i64())
            for team_name in ('redTeam', 'blueTeam'):
                print('        %s.guildId=%d' % (team_name, p.read_u32()))
                print('        %s.guildName=%s' % (team_name, p.read_wstr()))
                print('        %s.guildEmblemIndex=%d' % (team_name, p.read_u32()))
                m = p.read_u16()
                print('        %s.nicknameList_count=%d' % (team_name, m))
                for _ in range(m):
                    print('        %s.nicknameList[] %s' % (team_name, p.read_wstr()))
            print('        }')
        print('}')
    def serialize_62329(netid, p: common.PacketReader):
        print('SA_WatchGuildpvp {')
        print('    result=%d' % p.read_u32())
        print('    guildPvpInstanceId=%d' % p.read_u32())
        print('}')
    def serialize_62331(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62331 {')
        print('    field_0=%d' % p.read_u8())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_i64())
        print('    field_3=%d' % p.read_u16())
        print('}')
    def serialize_62332(netid, p: common.PacketReader):
        print('SN_GuildInvitation {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62333(netid, p: common.PacketReader):
        print('SN_GuildJoinapproval {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62334(netid, p: common.PacketReader):
        print('SN_GuildtagChanged {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62335(netid, p: common.PacketReader):
        print('SN_GuildNotice {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62336(netid, p: common.PacketReader):
        print('SN_GuildmemberJoin {')
        n = p.read_u16()
        print('    newMember nickname_len=%d' % n)
        print('    newMember nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    newMember membershipId=%d' % p.read_u32())
        print('    newMember lvl=%d' % p.read_u16())
        print('    newMember leaderClassType=%d' % p.read_u16())
        print('    newMember masterCount=%d' % p.read_u16())
        print('    newMember achievementScore=%d' % p.read_u32())
        print('    newMember topPvpTierGrade=%d' % p.read_u8())
        print('    newMember topPvpTierPoint=%d' % p.read_u16())
        print('    newMember contributedGuildPoint=%d' % p.read_u32())
        print('    newMember contributedGuildFund=%d' % p.read_u32())
        print('    newMember guildPvpWin=%d' % p.read_u16())
        print('    newMember guildPvpPlay=%d' % p.read_u16())
        print('    newMember lastLogoutDate=%d' % p.read_i64())
        print('}')
    def serialize_62337(netid, p: common.PacketReader):
        print('SN_GuildmemberQuit {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    isKickedOut=%d' % p.read_u8())
        print('}')
    def serialize_62339(netid, p: common.PacketReader):
        print('SN_GuildmemberChgnickname {')
        n = p.read_u16()
        print('    oldNickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    oldNickname=%s' % s)
        n = p.read_u16()
        print('    newNickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    newNickname=%s' % s)
        print('}')
    def serialize_62340(netid, p: common.PacketReader):
        print('SN_GuildmemberChgleaderclass {')
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    nickname=%s' % s)
        print('    newLeaderClass=%d' % p.read_u16())
        print('}')
    def serialize_62341(netid, p: common.PacketReader):
        print('SN_GuildmembershipAdded {')
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    hasInviteRight=%d' % p.read_u8())
        print('    hasExpelRight=%d' % p.read_u8())
        print('    hasMembershipChgRight=%d' % p.read_u8())
        print('    hasClassAssignRight=%d' % p.read_u8())
        print('    hasNoticeChgRight=%d' % p.read_u8())
        print('    hasIntroChgRight=%d' % p.read_u8())
        print('    hasInterestChgRight=%d' % p.read_u8())
        print('    hasFundManageRight=%d' % p.read_u8())
        print('    hasJoinTypeChgRight=%d' % p.read_u8())
        print('    hasEmblemChgRight=%d' % p.read_u8())
        print('}')
    def serialize_62342(netid, p: common.PacketReader):
        print('SN_GuildmembershipRemoved {')
        print('    membershipId=%d' % p.read_u32())
        print('}')
    def serialize_62343(netid, p: common.PacketReader):
        print('SN_GuildmembershipModified {')
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    rights hasInviteRight=%d' % p.read_u8())
        print('    rights hasExpelRight=%d' % p.read_u8())
        print('    rights hasMembershipChgRight=%d' % p.read_u8())
        print('    rights hasClassAssignRight=%d' % p.read_u8())
        print('    rights hasNoticeChgRight=%d' % p.read_u8())
        print('    rights hasIntroChgRight=%d' % p.read_u8())
        print('    rights hasInterestChgRight=%d' % p.read_u8())
        print('    rights hasFundManageRight=%d' % p.read_u8())
        print('    rights hasJoinTypeChgRight=%d' % p.read_u8())
        print('    rights hasEmblemChgRight=%d' % p.read_u8())
        print('}')
    def serialize_62344(netid, p: common.PacketReader):
        print('SN_GuildmemberChgclass {')
        print('    oldMembershipId=%d' % p.read_u32())
        print('    newMembershipId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62345(netid, p: common.PacketReader):
        print('SN_GuildmasterDelegation {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    formerMasterMembershipId=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62346(netid, p: common.PacketReader):
        print('SN_GuildfundAdded {')
        n = p.read_u16()
        print('    memberNickname_len=%d' % n)
        print('    memberNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    guildFundGain=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62347(netid, p: common.PacketReader):
        print('SN_GuildemblemChange {')
        print('    emblemIndex=%d' % p.read_u32())
        print('}')
    def serialize_62348(netid, p: common.PacketReader):
        print('SN_GuildemblemAdded {')
        print('    emblemIndex=%d' % p.read_u32())
        print('    assetChangeReason=%d' % p.read_u8())
        print('}')
    def serialize_62349(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62349 {')
        print('    field_0=%d' % p.read_u8())
        print('}')
    def serialize_62350(netid, p: common.PacketReader):
        print('SN_GuildskillAcquired {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    skillType=%d' % p.read_u8())
        print('    skillLvl=%d' % p.read_u8())
        print('    timeLimitHour=%d' % p.read_u16())
        print('    expiryDate=%d' % p.read_i64())
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62351(netid, p: common.PacketReader):
        print('SN_GuildskillUpgraded {')
        n = p.read_u16()
        print('    memberNickname_len=%d' % n)
        print('    memberNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    skillType=%d' % p.read_u8())
        print('    skillLvl=%d' % p.read_u8())
        print('    timeLimitHour=%d' % p.read_u16())
        print('    expiryDate=%d' % p.read_i64())
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62352(netid, p: common.PacketReader):
        print('SN_GuildskillExtended {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    skillType=%d' % p.read_u8())
        print('    skillLvl=%d' % p.read_u8())
        print('    skillExtensionCount=%d' % p.read_u16())
        print('    extensionTimeHour=%d' % p.read_u16())
        print('    skillExpiryDate=%d' % p.read_i64())
        print('    guildFundCost=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62353(netid, p: common.PacketReader):
        print('SN_GuildconstraintChange {')
        print('    changeDate=%d' % p.read_i64())
        print('    constraint maxMember=%d' % p.read_u8())
        print('    constraint maxEmblemLevel=%d' % p.read_u8())
        print('    constraint maxGuildMissionLevel=%d' % p.read_u8())
        print('    constraint maxDailyStagePlayGuildPoint=%d' % p.read_u16())
        print('    constraint maxDailyArenaPlayGuildPoint=%d' % p.read_u16())
        print('}')
    def serialize_62354(netid, p: common.PacketReader):
        print('SN_GuildmissionCompleted {')
        print('    missionIndex=%d' % p.read_u8())
        print('    rewardedGuildFund=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u16())
        print('    contributedGuildFund=%d' % p.read_u16())
        n = p.read_u16()
        print('    memberNicknames_count=%d' % n)
        for _ in range(n):
            print('    memberNicknames[]=%s' % p.read_wstr())
        print('    memberNickname=%s' % p.read_wstr())
        print('}')
    def serialize_62355(netid, p: common.PacketReader):
        print('SN_GuildpvpResult {')
        print('    guildPvpResultType=%d' % p.read_u8())
        print('    rp=%d' % p.read_u32())
        print('    win=%d' % p.read_u16())
        print('    draw=%d' % p.read_u16())
        print('    lose=%d' % p.read_u16())
        n = p.read_u16()
        print('    members_count=%d' % n)
        for _ in range(n):
            slen = p.read_u16()
            print('    members[] len=%d %s' % (slen, p.read_raw(slen * 2).decode('utf-16-le', errors='replace')))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62356(netid, p: common.PacketReader):
        print('SN_GuildpointandfundChange {')
        print('    changeDate=%d' % p.read_i64())
        print('    totalGuildPoint=%d' % p.read_u32())
        print('    seasonalGuildPoint=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u32())
        print('}')
    def serialize_62357(netid, p: common.PacketReader):
        print('SN_GuildLevelup {')
        print('    prevGuildLvl=%d' % p.read_u8())
        print('    currGuildLvl=%d' % p.read_u8())
        print('}')
    def serialize_62359(netid, p: common.PacketReader):
        print('SN_GuildchannelLeave {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62360(netid, p: common.PacketReader):
        print('SN_PlayerSyncMove {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    destPos=%s' % read_Vec3(p))
        print('    moveDir=(%f, %f)' % (p.read_f32(), p.read_f32()))
        print('    upperDir=(%f, %f)' % (p.read_f32(), p.read_f32()))
        print('    nRotate=%f' % p.read_f32())
        print('    nSpeed=%f' % p.read_f32())
        print('    flags=%x' % p.read_u8())
        print('    acionStateID=%d' % p.read_i32())
        print('}')

    def serialize_62361(netid, p: common.PacketReader):
        print('SN_PlayerSyncTurn {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    upperDir_x=%g' % p.read_f32())
        print('    upperDir_y=%g' % p.read_f32())
        print('    nRotate=%g' % p.read_f32())
        print('}')

    def serialize_62362(netid, p: common.PacketReader):
        print('SN_PlayerBlink {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    CurPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    NewPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y = p.read_f32(), p.read_f32()
        print('    NewDir=(%.2f, %.2f)' % (x, y))
        print('    nSpeed=%.2f' % p.read_f32())
        print('    nState=%d' % p.read_u32())
        print('    bGhostBlink=%d' % p.read_u8())
        print('}')
    def serialize_62363(netid, p: common.PacketReader):
        print('SN_InvalidRemoteLog {')
        n = p.read_u16()
        log = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    log_len=%d' % n)
        print('    log=%s' % log)
        print('}')
    def serialize_62364(netid, p: common.PacketReader):
        print('SN_PlayerSyncActionStateOnly {')
        print('    entityID=0x%08x' % p.read_u32())
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

    def serialize_62365(netid, p: common.PacketReader):
        print('SN_WeaponState {')
        print('    ownerID=0x%08x' % p.read_u32())
        print('    weaponID=%d' % p.read_i32())
        print('    state=%d' % p.read_i32())
        print('    chargeLevel=%d' % p.read_u8())
        print('    firingCombo=%g' % p.read_u8())
        print('    result=%g' % p.read_i32())
        print('}')

    def serialize_62366(netid, p: common.PacketReader):
        print('SN_SyncActionMove {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    TargetPos=%s' % read_Vec3(p))
        print('}')
    def serialize_62367(netid, p: common.PacketReader):
        print('SN_SyncStanceType {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    StanceType=%d' % p.read_u32())
        print('    durationtimeSec=%d' % p.read_f32())
        print('}')
    def serialize_62368(netid, p: common.PacketReader):
        print('SN_AiSyncMoveMotion {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    dwMotionType=%d' % p.read_u32())
        print('    dwMotionTypeKey=%d' % p.read_u32())
        print('    dwAIMoveType=%d' % p.read_u32())
        print('    dwAIMoveVariationType=%d' % p.read_u32())
        print('    vPos=%s' % (read_Vec3(p),))
        print('    isAttack=%d' % p.read_u32())
        print('    targetId=%d' % p.read_u32())
        print('}')
    def serialize_62369(netid, p: common.PacketReader):
        print('SN_AiSyncBehaviorMotion {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    dwMotionType=%d' % p.read_u32())
        print('    stance=%d' % p.read_u8())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vUpperDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    p3nPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    isAttack=%d' % p.read_u32())
        print('    targetId=%d' % p.read_u32())
        print('}')
    def serialize_62370(netid, p: common.PacketReader):
        print('SN_AiSyncCooltime {')
        print('    entityID=0x%08x' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    fCooltime=%.2f' % p.read_f32())
        print('    nodeID=%d' % p.read_u32())
        print('}')
    def serialize_62371(netid, p: common.PacketReader):
        print('SN_AiSyncSpeedRatio {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    fRatio=%d' % p.read_f32())
        print('}')
    def serialize_62372(netid, p: common.PacketReader):
        print('SN_AiSyncRotateRatio {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    fRatio=%d' % p.read_f32())
        print('}')
    def serialize_62373(netid, p: common.PacketReader):
        print('SN_AiSyncSceneroot {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    wantedYaw=%d' % p.read_f32())
        print('}')
    def serialize_62374(netid, p: common.PacketReader):
        print('SN_AiSyncTarpos {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    dwSentTime=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vSPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vUpperDir=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    fSpeed=%.2f' % p.read_f32())
        print('    fFrameRate=%.2f' % p.read_f32())
        print('    fUpForce=%.2f' % p.read_f32())
        print('}')
    def serialize_62375(netid, p: common.PacketReader):
        print('SN_AiSyncTarget {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('}')
    def serialize_62376(netid, p: common.PacketReader):
        print('SN_AiAddCom {')
        print('    entityID=0x%08x' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62377(netid, p: common.PacketReader):
        print('SN_AiSyncBodyYaw {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    fradian=%d' % p.read_f32())
        print('}')
    def serialize_62378(netid, p: common.PacketReader):
        print('SN_AiSyncBodyPitch {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    fradian=%d' % p.read_f32())
        print('}')
    def serialize_62379(netid, p: common.PacketReader):
        print('SN_AiSyncPhy {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    fradian=%.2f' % p.read_f32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    vPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62380(netid, p: common.PacketReader):
        print('SN_AiSetTarget {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    targetEntityID=%d' % p.read_u32())
        print('}')
    def serialize_62381(netid, p: common.PacketReader):
        print('SN_AiSyncMonsterSkillTarget {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    targetPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62382(netid, p: common.PacketReader):
        print('SN_AiSetActionState {')
        print('    excludedFieldBits=%d' % p.read_u8())
        print('    entityID=0x%08x' % p.read_u32())
        print('    motionType=%d' % p.read_i16())
        print('    lowerBodyYaw=%d' % p.read_i16())
        print('    moveSpeed=%d' % p.read_u16())
        print('    pos=%s' % read_Vec3(p))
        print('}')
    def serialize_62383(netid, p: common.PacketReader):
        print('SN_AiSetMovetargetpos {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    movePresetType=%d' % p.read_u32())
        print('    moveTargetPos=%s' % read_Vec3(p))
        print('}')
    def serialize_62384(netid, p: common.PacketReader):
        print('SN_AiChangeMoveType {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    moveType=%d' % p.read_u32())
        print('}')
    def serialize_62385(netid, p: common.PacketReader):
        print('SN_RemoteSyncCreateFromCreatorId {')
        print('    ParentEntity=%d' % p.read_u32())
        print('    OwnerEntity=%d' % p.read_u32())
        print('    RemoteSeedID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    FirePosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    FireRotationX=%d' % p.read_u16())
        print('    FireRotationY=%d' % p.read_u16())
        print('    FireRotationZ=%d' % p.read_u16())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    TargetPosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    Scale=%d' % p.read_u16())
        print('    LifeTime=%.2f' % p.read_f32())
        print('    HitInvalidTarget=%d' % p.read_u32())
        print('    RemoteCreatorID=%d' % p.read_u16())
        print('    ErrorAngleX=%d' % p.read_u16())
        print('    ErrorAngleY=%d' % p.read_u16())
        print('    ChargingLevel=%d' % p.read_u8())
        print('    GaugeLevel=%d' % p.read_u8())
        print('    FiringCount=%d' % p.read_u16())
        print('    ComboCount=%d' % p.read_u8())
        print('    TargetEntity=%d' % p.read_u32())
        print('    RemoteCreateFlags=%d' % p.read_u8())
        print('}')
    def serialize_62386(netid, p: common.PacketReader):
        print('SN_RemoteSyncCreateFromRemoteDoc {')
        print('    ParentEntity=%d' % p.read_u32())
        print('    OwnerEntity=%d' % p.read_u32())
        print('    RemoteSeedID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    FirePosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    FireRotationX=%d' % p.read_u16())
        print('    FireRotationY=%d' % p.read_u16())
        print('    FireRotationZ=%d' % p.read_u16())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    TargetPosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        n = p.read_u16()
        print('    FireObject_len=%d' % n)
        print('    FireObject=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    Scale=%d' % p.read_u16())
        print('    LifeTime=%.2f' % p.read_f32())
        print('    HitInvalidTarget=%d' % p.read_u32())
        print('    RemoteDocIndex=%d' % p.read_u32())
        print('    TargetEntity=%d' % p.read_u32())
        print('    RemoteCreateFlags=%d' % p.read_u8())
        print('}')
    def serialize_62387(netid, p: common.PacketReader):
        print('SN_RemoteSnapshotFromRemoteDoc {')
        print('    ParentEntity=%d' % p.read_u32())
        print('    OwnerEntity=%d' % p.read_u32())
        print('    RemoteSeedID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    FirePosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    FireRotationX=%d' % p.read_u16())
        print('    FireRotationY=%d' % p.read_u16())
        print('    FireRotationZ=%d' % p.read_u16())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    TargetPosition=(%.2f, %.2f, %.2f)' % (x, y, z))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    Scale=%d' % p.read_u16())
        print('    LifeTime=%d' % p.read_u32())
        print('    DurationTime=%d' % p.read_u32())
        print('    HitInvalidTarget=%d' % p.read_u32())
        print('    RemoteDocIndex=%d' % p.read_u32())
        print('    TargetEntity=%d' % p.read_u32())
        print('    RemoteCreateFlags=%d' % p.read_u8())
        print('}')
    def serialize_62388(netid, p: common.PacketReader):
        print('SN_RemoteSyncTarget {')
        bits = p.read_u8()
        print('    excludedFieldBits=%d' % bits)
        print('    remoteID=%d' % p.read_u32())
        print('    targetEntity=%d' % p.read_u32())
        print('    DestPos=%s' % read_Vec3(p))
        if not (bits & 8):
            print('    MoveDir=%s' % read_Vec3(p))
        print('}')

    def serialize_62389(netid, p: common.PacketReader):
        print('SN_RemoteSyncForecastCollision {')
        n = p.read_u16()
        print('    m_CollisionInfo_count=%d' % n)
        for _ in range(n):
            print('    m_CollisionInfo[] remoteID=%d' % p.read_u32())
            print('    m_CollisionInfo[] targetEntity=%d' % p.read_u32())
            x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
            print('    m_CollisionInfo[] DestPos=(%.2f, %.2f, %.2f)' % (x, y, z))
            print('    m_CollisionInfo[] TimeOfImpact=%.2f' % p.read_f32())
            print('    m_CollisionInfo[] Destroy=%d' % p.read_u8())
        print('}')
    def serialize_62391(netid, p: common.PacketReader):
        print('SA_CharacterSkinSelect {')
        print('    result=%d' % p.read_u32())
        print('    classType=%d' % p.read_u32())
        print('    skinIndex=%d' % p.read_u32())
        print('}')
    def serialize_62392(netid, p: common.PacketReader):
        print('SN_CharacterSkinUnlock {')
        print('    result=%d' % p.read_u32())
        print('    classType=%d' % p.read_u32())
        print('    skinIndex=%d' % p.read_u32())
        print('    bufCount=%d' % p.read_u32())
        print('}')
    def serialize_62393(netid, p: common.PacketReader):
        print('SN_CallMonsterByRemoteControl {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    docIndex=0x%08x' % p.read_u32())
        print('    bAIUse=%d' % p.read_u8())
        print('}')
    def serialize_62394(netid, p: common.PacketReader):
        print('SQ_UseLiferecover {')
        print('    usn=%d' % p.read_u32())
        print('    liferecoverCount=%d' % p.read_u32())
        print('}')
    def serialize_62395(netid, p: common.PacketReader):
        print('SQ_ReviveByLifeCount {')
        print('    countdownPeriod=%d' % p.read_u32())
        print('}')
    def serialize_62396(netid, p: common.PacketReader):
        print('SQ_ReviveByCoin {')
        print('    usn=%d' % p.read_u32())
        print('}')
    def serialize_62397(netid, p: common.PacketReader):
        print('SN_RevivePlayer {')
        print('    usn=%d' % p.read_u32())
        print('    activeID=%d' % p.read_u32())
        print('    inactiveID=%d' % p.read_u32())
        print('    followID=%d' % p.read_u32())
        print('    reviveBySelf=%d' % p.read_u8())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    position=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62398(netid, p: common.PacketReader):
        print('SN_RevivePlayerAtStartingPoint {')
        print('    usn=%d' % p.read_u32())
        print('    activeID=%d' % p.read_u32())
        print('    inactiveID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    position=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62399(netid, p: common.PacketReader):
        print('SN_RespawnDelaytime {')
        print('    usn=%d' % p.read_u32())
        print('    respawnDelayTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62401(netid, p: common.PacketReader):
        print('SN_WarnPveGameend {')
        print('    remainTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62402(netid, p: common.PacketReader):
        print('SN_StartGameNotQualified {')
        print('    reason=%d' % p.read_u32())
        n = p.read_u16()
        print('    members_count=%d' % n)
        for _ in range(n):
            ln = p.read_u16()
            s = p.read_raw(ln * 2).decode('utf-16-le', 'replace')
            print('    members[] %s' % s)
        print('}')
    def serialize_62403(netid, p: common.PacketReader):
        print('SQ_KickFromGameserver {')
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62405(netid, p: common.PacketReader):
        print('SN_WarehouseItemUpdate {')
        n = p.read_u16()
        print('    updatedItemList_count=%d' % n)
        for _ in range(n):
            print('    updatedItemList[] PST_PROFILE_ITEM {')
            print('        m_itemID=0x%08x' % p.read_u32())
            print('        m_invenType=%d' % p.read_u8())
            print('        m_slot=%d' % p.read_u32())
            print('        m_itemIndex=0x%08x' % p.read_u32())
            print('        m_count=%d' % p.read_u32())
            print('        m_propertyGroupIndex=%d' % p.read_u32())
            print('        m_isLifeTimeAbsolute=%d' % p.read_u8())
            print('        m_lifeEndTimeUTC=%d' % p.read_i64())
            m = p.read_u16()
            print('        m_properties_count=%d' % m)
            for _ in range(m):
                print('        m_properties[] _Type=%d _TypeDetail=%d _ValueType=%d _Value=%.2f _Fixed=%d' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_f32(), p.read_u8()))
            print('    }')
        print('}')
    def serialize_62408(netid, p: common.PacketReader):
        print('SA_WarehouseExpand {')
        print('    result=%d' % p.read_u32())
        print('    lineCount=%d' % p.read_u32())
        print('}')
    def serialize_62409(netid, p: common.PacketReader):
        print('SA_UserAfk {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62410(netid, p: common.PacketReader):
        print('SN_RewardItemList {')
        n = p.read_u16()
        print('    rewardItemList_count=%d' % n)
        for _ in range(n):
            print('    rewardItemList[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('    spSlotIndex=%d' % p.read_u32())
        print('    vipSlotCount=%d' % p.read_u8())
        print('    guildSkillSlotCount=%d' % p.read_u8())
        print('    pcCafeSlotCount=%d' % p.read_u8())
        n = p.read_u16()
        print('    pcCafeUserIds_count=%d' % n)
        for _ in range(n):
            print('    pcCafeUserIds[] id=%d' % p.read_u32())
        print('}')
    def serialize_62411(netid, p: common.PacketReader):
        print('SN_RewardStageStart {')
        print('    timeToWaitSec=%d' % p.read_u32())
        print('    nextAction=%d' % p.read_u8())
        print('    nextActionCost=%d' % p.read_u32())
        print('    slotCount=%d' % p.read_u8())
        print('    slotRewardPenaltyReason=%d' % p.read_u32())
        print('}')
    def serialize_62412(netid, p: common.PacketReader):
        print('SN_RewardStageStartNp {')
        print('    timeToWaitSec=%d' % p.read_u32())
        print('    nextAction=%d' % p.read_u8())
        print('    slotCount=%d' % p.read_u8())
        print('    slotRewardPenaltyReason=%d' % p.read_u32())
        print('}')
    def serialize_62413(netid, p: common.PacketReader):
        print('SA_RewardSelectSlot {')
        print('    retval=%d' % p.read_u32())
        print('    slotIndex=%d' % p.read_u32())
        print('    rewardItem m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('    isSpecialSlot=%d' % p.read_u8())
        print('}')
    def serialize_62414(netid, p: common.PacketReader):
        print('SA_RewardSelectSlotAllRandom {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    slotIndexes_count=%d' % n)
        for _ in range(n):
            print('    slotIndexes[]=%d' % p.read_u32())
        n = p.read_u16()
        print('    rewardItems_count=%d' % n)
        for _ in range(n):
            print('    rewardItems[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('    spSlotIndex=%d' % p.read_u32())
        print('}')
    def serialize_62415(netid, p: common.PacketReader):
        print('SN_RewardStageEnd {')
        n = p.read_u16()
        print('    rewardItemList_count=%d' % n)
        for _ in range(n):
            print('    rewardItemList[] m_ItemType=%d m_ItemIndex=%d m_ItemAmount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    otherSelectList_count=%d' % n)
        for _ in range(n):
            print('    otherSelectList[] usn=%d' % p.read_u32())
            m = p.read_u16()
            print('    otherSelectList[] nickname_len=%d' % m)
            print('    otherSelectList[] nickname=%s' % p.read_raw(m * 2).decode('utf-16-le', errors='replace'))
            k = p.read_u16()
            print('    otherSelectList[] pickedItemIndexes_count=%d' % k)
            for _ in range(k):
                print('    otherSelectList[] pickedItemIndexes[]=%d' % p.read_u32())
        print('    spSlotIndex=%d' % p.read_u32())
        print('}')
    def serialize_62416(netid, p: common.PacketReader):
        print('SA_RewardBuyChance {')
        print('    retval=%d' % p.read_u32())
        print('    newSlotCount=%d' % p.read_u32())
        print('    nextAction=%d' % p.read_u8())
        print('    nextActionCost=%d' % p.read_u32())
        print('}')
    def serialize_62417(netid, p: common.PacketReader):
        print('SA_RewardRequestFinish {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62418(netid, p: common.PacketReader):
        print('SA_WarehouseItemChange {')
        print('    orgItemID=0x%08x' % p.read_u32())
        print('    orgInvenType=%d' % p.read_u32())
        print('    targetInvenType=%d' % p.read_u32())
        print('    targetSlot=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62419(netid, p: common.PacketReader):
        print('SN_MailList {')
        print('    packetNum=%d' % p.read_u8())
        print('    mailboxType=%d' % p.read_u8())
        n = p.read_u16()
        print('    mailList_count=%d' % n)
        for _ in range(n):
            print('    mailList[] {')
            print('        mailId=%d' % p.read_i64())
            print('        sendUTCDate=%d' % p.read_i64())
            print('        expireUTCDate=%d' % p.read_i64())
            print('        isRead=%d' % p.read_u8())
            print('        isGMMail=%d' % p.read_u8())
            print('        hasAttachment=%d' % p.read_u8())
            m = p.read_u16()
            print('        fromNickname_len=%d' % m)
            print('        fromNickname=%s' % p.read_raw(m * 2).decode('utf-16-le', errors='replace'))
            s = p.read_u16()
            print('        subject_len=%d' % s)
            print('        subject=%s' % p.read_raw(s * 2).decode('utf-16-le', errors='replace'))
            k = p.read_u16()
            print('        items_count=%d' % k)
            for _ in range(k):
                print('        items[]=%d' % p.read_u32())
            print('    }')
        print('}')
    def serialize_62420(netid, p: common.PacketReader):
        print('SN_MailBlocklist {')
        print('    packetNum=%d' % p.read_u8())
        n = p.read_u16()
        print('    blockNicknameList_count=%d' % n)
        for _ in range(n):
            print('    blockNicknameList[] %s' % p.read_wstr())
        print('}')
    def serialize_62421(netid, p: common.PacketReader):
        print('SN_MailWriteResult {')
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62422(netid, p: common.PacketReader):
        print('SN_MailRead {')
        print('    mailId=%d' % p.read_i64())
        print('    isRead=%d' % p.read_u8())
        print('    isGMMail=%d' % p.read_u8())
        n = p.read_u16()
        print('    fromNickname_len=%d' % n)
        print('    fromNickname=%s' % ''.join(chr(p.read_u16()) for _ in range(n)))
        n = p.read_u16()
        print('    subject_len=%d' % n)
        print('    subject=%s' % ''.join(chr(p.read_u16()) for _ in range(n)))
        n = p.read_u16()
        print('    content_len=%d' % n)
        print('    content=%s' % ''.join(chr(p.read_u16()) for _ in range(n)))
        print('    sendUTCDate=%d' % p.read_i64())
        print('    expireUTCDate=%d' % p.read_i64())
        print('    money=%d' % p.read_i64())
        print('    exp=%d' % p.read_i64())
        print('    ccoin=%d' % p.read_i64())
        print('    guildFund=%d' % p.read_u32())
        n = p.read_u16()
        print('    itemList_count=%d' % n)
        for _ in range(n):
            print('    itemList[] itemIndex=0x%08x itemCount=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62423(netid, p: common.PacketReader):
        print('SN_MailGetAttachmentResult {')
        print('    nErrorType=%d' % p.read_u32())
        print('    mailId=%d' % p.read_i64())
        print('    expireUTCDate=%d' % p.read_i64())
        count = p.read_u16()
        print('    takenItems(%d)=[' % count)
        while count > 0:
            print('        (itemIndex=0x%08x itemCount=%d),' % (p.read_u32(), p.read_u32()))
            count -= 1
        print('    ]')
        count = p.read_u16()
        print('    remainItems(%d)=[' % count)
        while count > 0:
            print('        (itemIndex=0x%08x itemCount=%d),' % (p.read_u32(), p.read_u32()))
            count -= 1
        print('    ]')
        print('}')
    def serialize_62424(netid, p: common.PacketReader):
        print('SN_MailMoveResult {')
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62427(netid, p: common.PacketReader):
        print('SN_GameEventActivated {')
        count = p.read_u16()
        print('    eventIndexList_count=%d' % count)
        for _ in range(count):
            print('    eventIndexList[] %d' % p.read_u32())
        print('}')
    def serialize_62428(netid, p: common.PacketReader):
        print('SN_GameEventPending {')
        n = p.read_u16()
        print('    eventIndexList_count=%d' % n)
        for _ in range(n):
            print('    eventIndexList[] %d' % p.read_u32())
        print('}')
    def serialize_62429(netid, p: common.PacketReader):
        print('SN_GameEventStart {')
        print('    eventIndex=%d' % p.read_u32())
        print('}')
    def serialize_62430(netid, p: common.PacketReader):
        print('SN_GameEventEnd {')
        print('    eventIndex=%d' % p.read_u32())
        print('}')
    def serialize_62431(netid, p: common.PacketReader):
        print('SN_PartyCancelInvite {')
        print('    partyId=%d' % p.read_u32())
        print('}')
    def serialize_62432(netid, p: common.PacketReader):
        print('SN_ActivatedSupportkit {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    supportKitItemIndex=%d' % p.read_u32())
        print('    cooltimeId=%d' % p.read_u32())
        print('    cooltimeSec=%d' % p.read_f32())
        print('}')
    def serialize_62433(netid, p: common.PacketReader):
        print('SN_NotifyCooltime {')
        print('    characterID=0x%08x' % p.read_u32())
        count = p.read_u16()
        print('    coolTimeList(%d)=[' % count)
        while count > 0:
            print('        (coolTimeType=%d coolTimeID=%d leftCoolTime=%d totalCoolTime=%d),' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
            count -= 1
        print('    ]')
        print('}')
    def serialize_62434(netid, p: common.PacketReader):
        print('SN_MailListNewInbox {')
        print('    mailboxType=%d' % p.read_u8())
        n = p.read_u16()
        print('    mailList_count=%d' % n)
        for _ in range(n):
            print('    mailList[] mailId=%d' % p.read_i64())
            print('    mailList[] sendUTCDate=%d' % p.read_i64())
            print('    mailList[] expireUTCDate=%d' % p.read_i64())
            print('    mailList[] isRead=%d' % p.read_u8())
            print('    mailList[] isGMMail=%d' % p.read_u8())
            print('    mailList[] hasAttachment=%d' % p.read_u8())
            print('    mailList[] fromNickname=%s' % p.read_wstr())
            print('    mailList[] subject=%s' % p.read_wstr())
            ic = p.read_u16()
            print('    mailList[] items_count=%d' % ic)
            for _ in range(ic):
                print('    mailList[] items[]=%d' % p.read_u32())
        print('}')
    def serialize_62435(netid, p: common.PacketReader):
        print('SN_PlayerServerPosition {')
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    Pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y = p.read_f32(), p.read_f32()
        print('    MoveDir=(%.2f, %.2f)' % (x, y))
        print('}')
    def serialize_62436(netid, p: common.PacketReader):
        print('SN_RemoteServerPosition {')
        print('    remoteServerID=%d' % p.read_i64())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    dir=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    boundType=%d' % p.read_u8())
        print('    durationType=%d' % p.read_u8())
        print('    colorType=%d' % p.read_u8())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    dim=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    flag=%d' % p.read_u8())
        print('}')
    def serialize_62437(netid, p: common.PacketReader):
        print('SN_MonsterServerPosition {')
        print('    monsterID=%d' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    Pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        fx, fy = p.read_f32(), p.read_f32()
        print('    MoveDir_fX=%.2f MoveDir_fY=%.2f' % (fx, fy))
        print('}')
    def serialize_62438(netid, p: common.PacketReader):
        print('SN_MonsterServerChase {')
        print('    monsterID=%d' % p.read_u32())
        print('    chaseNearRadius=%d' % p.read_f32())
        print('    chaseFarRadius=%d' % p.read_f32())
        print('}')
    def serialize_62439(netid, p: common.PacketReader):
        print('SN_RegameData {')
        print('    ownerUserId=%d' % p.read_u32())
        print('    areaIndex=%d' % p.read_u32())
        print('}')
    def serialize_62440(netid, p: common.PacketReader):
        print('SN_RegameAvailable {')
        print('    gameType=%d' % p.read_u32())
        print('    stageIndex=%d' % p.read_u32())
        print('    bRetry=%d' % p.read_u8())
        print('    reasonCode=%d' % p.read_u8())
        print('}')
    def serialize_62441(netid, p: common.PacketReader):
        print('SA_PvpRecord {')
        n = p.read_u16()
        print('    stPvpRecords_count=%d' % n)
        for _ in range(n):
            print('    stPvpRecords[] gameType=%d pvpSaveType=%d winCount=%d loseCount=%d drawCount=%d leaveCount=%d killCount=%d deathCount=%d assistCount=%d score=%d data1=%d' % (p.read_u8(), p.read_u8(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        n = p.read_u16()
        print('    stPvpOlympicRecords_count=%d' % n)
        for _ in range(n):
            print('    stPvpOlympicRecords[] pvpSaveType=%d firstCount=%d secondCount=%d thirdCount=%d timeRecord=%d' % (p.read_u8(), p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62442(netid, p: common.PacketReader):
        print('SA_PvpDetailRecord {')
        print('    gameType=%d' % p.read_u32())
        print('    pvpSaveType=%d' % p.read_u8())
        print('    stPvpMasterRecord {')
        print('        mostPlayClass=%d' % p.read_u32())
        print('        mostPlayCount=%d' % p.read_u32())
        print('        mostWinClass=%d' % p.read_u32())
        print('        mostWinCount=%d' % p.read_u32())
        print('    }')
        n = p.read_u16()
        print('    recentGames_count=%d' % n)
        for _ in range(n):
            print('    recentGames[] resultType=%d fstClass=%d scndClass=%d myScore=%d myTeamScore=%d enemyTeamScore=%d myTeamPlayerCount=%d enemyTeamPlayerCount=%d killCount=%d deathCount=%d assistCount=%d occupyCount=%d pvpSpecificRecord=%d beginTimeStamp=%d' % (
                p.read_u8(), p.read_u32(), p.read_u32(), p.read_u16(), p.read_u16(), p.read_u16(),
                p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(),
                p.read_u32(), p.read_i64()))
        print('    olympicTotalPlayCount=%d' % p.read_u32())
        print('    olympicWorldRecord=%d' % p.read_u32())
        n = p.read_u16()
        print('    olympicWorldRecordNickname_len=%d' % n)
        print('    olympicWorldRecordNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62443(netid, p: common.PacketReader):
        print('SN_SummaryRewardResult {')
        print('    result=%d' % p.read_u32())
        print('    summaryInfo {')
        print('        summaryIndex=%d' % p.read_u32())
        print('        stageIndex=%d' % p.read_u32())
        print('        summaryType=%d' % p.read_u32())
        print('        summaryData=%d' % p.read_u32())
        print('        rewardReceived=%d' % p.read_u8())
        print('        completedDate=%d' % p.read_i64())
        print('    }')
        print('}')
    def serialize_62444(netid, p: common.PacketReader):
        print('SN_ReduceCooltime {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    cooltimeID=%d' % p.read_u32())
        print('    valueType=%d' % p.read_u32())
        print('    value=%d' % p.read_f32())
        print('}')
    def serialize_62445(netid, p: common.PacketReader):
        print('SN_BroadcastGamePingData {')
        print('    characterID=0x%08x' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    pingPos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    targetID=%d' % p.read_u32())
        print('    pingData=%d' % p.read_u32())
        print('}')
    def serialize_62447(netid, p: common.PacketReader):
        print('SN_BroadcastEvade {')
        print('    attID=%d' % p.read_u32())
        print('    defID=%d' % p.read_u32())
        print('    remoteID=%d' % p.read_u32())
        print('    remoteDocIndex=%d' % p.read_u32())
        print('}')
    def serialize_62449(netid, p: common.PacketReader):
        print('SN_RunClientLevelEventSeq {')
        print('    needCompleteTriggerAckID=%d' % p.read_i32())
        print('    rootEventID=%d' % p.read_i32())
        print('    caller=%d' % p.read_i32())
        print('    serverTime=%d' % p.read_i64())
        print('}')

    def serialize_62451(netid, p: common.PacketReader):
        print('SN_ErrorMessage {')
        print('    type=%d' % p.read_u8())
        print('    duration=%d' % p.read_u32())
        print('    errCode=%d' % p.read_u32())
        n = p.read_u16()
        print('    param1_len=%d' % n)
        print('    param1=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    param2_len=%d' % n)
        print('    param2=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62452(netid, p: common.PacketReader):
        print('SN_BushObjectState {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    bushID=%d' % p.read_u32())
        print('}')
    def serialize_62453(netid, p: common.PacketReader):
        print('SN_BushState {')
        print('    bushID=%d' % p.read_u32())
        print('    curState=%d' % p.read_u32())
        print('}')
    def serialize_62454(netid, p: common.PacketReader):
        print('SN_HideDetect {')
        print('    hideDetectorOwner=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    flag=%d' % p.read_u8())
        print('}')
    def serialize_62456(netid, p: common.PacketReader):
        print('SN_SortieMasterBanPhaseStart {')
        print('    alliesLeaderUserId=%d' % p.read_u32())
        print('    enemiesLeaderUserId=%d' % p.read_u32())
        print('    timeSec=%d' % p.read_u32())
        print('    banCount=%d' % p.read_u32())
        n = p.read_u16()
        print('    enemiesSlotInfos_count=%d' % n)
        for _ in range(n):
            print('    enemiesSlotInfos[] creatureIndex=%d' % p.read_u32())
            m = p.read_u16()
            print('    enemiesSlotInfos[] slotStates_count=%d' % m)
            for _ in range(m):
                print('    enemiesSlotInfos[] slotStates[]=%d' % p.read_u32())
        print('}')
    def serialize_62457(netid, p: common.PacketReader):
        print('SN_SortieMasterAssignPhaseStart {')
        print('    alliesLeaderUserId=%d' % p.read_u32())
        print('    enemiesLeaderUserId=%d' % p.read_u32())
        print('    timeSec=%d' % p.read_u32())
        print('    assignCount=%d' % p.read_u32())
        n = p.read_u16()
        print('    enemiesSlotInfos_count=%d' % n)
        for _ in range(n):
            print('    enemiesSlotInfos[] creatureIndex=%d' % p.read_u32())
            m = p.read_u16()
            print('    enemiesSlotInfos[] slotStates_count=%d' % m)
            for _ in range(m):
                print('    enemiesSlotInfos[] slotStates[]=%d' % p.read_u32())
        print('}')
    def serialize_62458(netid, p: common.PacketReader):
        print('SA_SortieMasterBan {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62459(netid, p: common.PacketReader):
        print('SN_SortieMasterBan {')
        print('    teamType=%d' % p.read_u32())
        print('    creatureIndex=%d' % p.read_u32())
        print('}')
    def serialize_62460(netid, p: common.PacketReader):
        print('SA_SortieMasterAssign {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62461(netid, p: common.PacketReader):
        print('SN_SortieMasterAssign {')
        print('    teamType=%d' % p.read_u32())
        print('    creatureIndex=%d' % p.read_u32())
        print('}')
    def serialize_62463(netid, p: common.PacketReader):
        print('SN_SortieMasterBanPhaseEnd {')
        n = p.read_u16()
        print('    alliesBanInfo_count=%d' % n)
        for _ in range(n):
            print('    alliesBanInfo[] banCreatureIndexes=%d' % p.read_u32())
        n = p.read_u16()
        print('    enemiesBanInfo_count=%d' % n)
        for _ in range(n):
            print('    enemiesBanInfo[] banCreatureIndexes=%d' % p.read_u32())
        print('}')
    def serialize_62464(netid, p: common.PacketReader):
        print('SN_SortieMasterAssignPhaseEnd {')
        for name in ('alliesAssignInfo', 'enemiesAssignInfo'):
            print('    %s {' % name)
            print('        assignedUserId=%d' % p.read_u32())
            n = p.read_u16()
            print('        candidateUserIds_count=%d' % n)
            for _ in range(n):
                print('        candidateUserIds[]=%d' % p.read_u32())
            n = p.read_u16()
            print('        assignCreatureIndexes_count=%d' % n)
            for _ in range(n):
                print('        assignCreatureIndexes[]=%d' % p.read_u32())
            print('    }')
        print('}')
    def serialize_62470(netid, p: common.PacketReader):
        print('SA_TierStageRecord {')
        print('    seasonId=%d' % p.read_u8())
        print('    stageRecord {')
        print('        pvpSaveType=%d' % p.read_u8())
        print('        tierType=%d' % p.read_u8())
        print('        tierPoint=%d' % p.read_u16())
        print('        sectorId=%d' % p.read_u16())
        print('        sectorRanking=%d' % p.read_u8())
        print('        seasonalWin=%d' % p.read_u32())
        print('        seasonalDraw=%d' % p.read_u32())
        print('        seasonalLose=%d' % p.read_u32())
        print('        seasonalLeave=%d' % p.read_u32())
        print('    }')
        print('    sectorId=%d' % p.read_u32())
        n = p.read_u16()
        print('    sectorRankingList_count=%d' % n)
        for _ in range(n):
            print('    sectorRankingList[] sectorRanking=%d' % p.read_u8())
            nl = p.read_u16()
            print('    sectorRankingList[] nickname_len=%d' % nl)
            print('    sectorRankingList[] nickname=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
            print('    sectorRankingList[] tierPoint=%d' % p.read_u16())
            print('    sectorRankingList[] totalWin=%d' % p.read_u32())
            print('    sectorRankingList[] recentWin=%d' % p.read_u32())
            print('    sectorRankingList[] recentLose=%d' % p.read_u32())
            print('    sectorRankingList[] recentDraw=%d' % p.read_u32())
            print('    sectorRankingList[] recentLeave=%d' % p.read_u32())
        print('}')
    def serialize_62471(netid, p: common.PacketReader):
        print('SA_PvpRanking {')
        print('    seasonId=%d' % p.read_u8())
        print('    pvpSaveType=%d' % p.read_u8())
        n = p.read_u16()
        print('    rankingList_count=%d' % n)
        for _ in range(n):
            print('    rankingList[] ranking=%d' % p.read_u8())
            nl = p.read_u16()
            print('    rankingList[] nickname=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
            print('    rankingList[] tierType=%d' % p.read_u8())
            print('    rankingList[] tierPoint=%d' % p.read_u16())
            print('    rankingList[] totalWin=%d' % p.read_u32())
            print('    rankingList[] recentWin=%d' % p.read_u32())
            print('    rankingList[] recentLose=%d' % p.read_u32())
            print('    rankingList[] recentDraw=%d' % p.read_u32())
            print('    rankingList[] recentLeave=%d' % p.read_u32())
        print('}')
    def serialize_62473(netid, p: common.PacketReader):
        print('SN_NotifyIsInSafeZone {')
        print('    inSafeZone=%d' % p.read_u8())
        print('}')
    def serialize_62475(netid, p: common.PacketReader):
        print('SA_PveRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u8())
        n = p.read_u16()
        print('    highScore.member_count=%d' % n)
        for _ in range(n):
            print('    highScore.member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
            print('    highScore.member[].nickname=%s' % p.read_wstr())
        print('    highScoreClearRankType=%d' % p.read_u8())
        m = p.read_u16()
        print('    rankingList_count=%d' % m)
        for _ in range(m):
            print('    rankingList[] ranking=%d totalScore=%d playTime=%d deathCount=%d' % (p.read_u32(), p.read_u32(), p.read_u32(), p.read_u32()))
            k = p.read_u16()
            print('    rankingList[].member_count=%d' % k)
            for _ in range(k):
                print('    rankingList[].member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
                print('    rankingList[].member[].nickname=%s' % p.read_wstr())
        print('}')
    def serialize_62476(netid, p: common.PacketReader):
        print('SA_MyPveRanking {')
        print('    stageIndex=%d' % p.read_u32())
        print('    ranking=%d' % p.read_u32())
        print('    totalScore=%d' % p.read_u32())
        print('    playTime=%d' % p.read_u32())
        print('    deathCount=%d' % p.read_u32())
        n = p.read_u16()
        print('    member_count=%d' % n)
        for _ in range(n):
            print('    member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
            nl = p.read_u16()
            print('    member[] nickname=%s' % p.read_raw(nl * 2).decode('utf-16-le', errors='replace'))
        print('    highScoreClearRankType=%d' % p.read_u8())
        print('}')
    def serialize_62477(netid, p: common.PacketReader):
        print('SN_PveLastgameRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u8())
        n = p.read_u16()
        print('    rankingList_count=%d' % n)
        for _ in range(n):
            print('    rankingList[] ranking=%d' % p.read_u32())
            print('    rankingList[] totalScore=%d' % p.read_u32())
            print('    rankingList[] playTime=%d' % p.read_u32())
            print('    rankingList[] deathCount=%d' % p.read_u32())
            m = p.read_u16()
            print('    rankingList[] member_count=%d' % m)
            for _ in range(m):
                print('    rankingList[] member[] mainClassType=%d' % p.read_u32())
                print('    rankingList[] member[] subClassType=%d' % p.read_u32())
                s = p.read_u16()
                print('    rankingList[] member[] nickname_len=%d' % s)
                print('    rankingList[] member[] nickname=%s' % p.read_raw(s * 2).decode('utf-16-le', errors='replace'))
        print('    ranking=%d' % p.read_u32())
        print('}')
    def serialize_62478(netid, p: common.PacketReader):
        print('SN_DefenceLastgameRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u8())
        n = p.read_u16()
        print('    rankingList_count=%d' % n)
        for _ in range(n):
            print('    rankingList[] ranking=%d clearStepIndex=%d clearTime=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
            m = p.read_u16()
            print('    member_count=%d' % m)
            for _ in range(m):
                print('    member[] mainClassType=%d subClassType=%d' % (p.read_u32(), p.read_u32()))
                s = p.read_u16()
                print('    member[].nickname=%s' % p.read_raw(s * 2).decode('utf-16-le', errors='replace'))
        print('    ranking=%d' % p.read_u32())
        print('}')
    def serialize_62479(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62479 {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u8())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u16())
        print('    field_7=%d' % p.read_u32())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u16())
        print('    field_10=%d' % p.read_u32())
        print('    field_11=%d' % p.read_u32())
        print('    field_12=%d' % p.read_u32())
        print('    field_13=%d' % p.read_u16())
        print('    field_14=%d' % p.read_u32())
        print('    field_15=%d' % p.read_u32())
        print('    field_16=%d' % p.read_u16())
        print('}')
    def serialize_62480(netid, p: common.PacketReader):
        print('SN_ActiveIngameEventList {')
        n = p.read_u16()
        print('    eventIndexList_count=%d' % n)
        for _ in range(n):
            print('    eventIndexList[]=%d' % p.read_u32())
        print('}')
    def serialize_62481(netid, p: common.PacketReader):
        print('SN_NotifyTimestamp {')
        print('    serverTimeStamp=%d' % p.read_i64())
        print('    curCount=%d' % p.read_i32())
        print('    maxCount=%d' % p.read_i32())
        print('}')

    def serialize_62482(netid, p: common.PacketReader):
        print('SA_RTT_Time {')
        print('    clientTimeStamp=%u' % p.read_u32())
        print('    serverTimeStamp=%d' % p.read_i64())
        print('}')

    def serialize_62483(netid, p: common.PacketReader):
        print('SN_PveTodayStageInfo {')
        print('    availablePlayCount=%d' % p.read_u16())
        print('    maxPlayCount=%d' % p.read_u16())
        print('}')

    def serialize_62484(netid, p: common.PacketReader):
        print('SN_FatiguePointInfo {')
        print('    todayFatiguePoint=%d' % p.read_u16())
        print('    maxFatiguePoint=%d' % p.read_u16())
        print('}')

    def serialize_62486(netid, p: common.PacketReader):
        print('SN_PlaytimePause {')
        print('    playTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62487(netid, p: common.PacketReader):
        print('SN_PlaytimeResume {')
        print('}')
    def serialize_62488(netid, p: common.PacketReader):
        print('SN_PvpEventAnnouncement {')
        print('    stAnnounceMent {')
        print('        announcementType=%d' % p.read_u32())
        print('        announcementData=%d' % p.read_u32())
        print('        score=%d' % p.read_u32())
        print('        attackerID=0x%08x' % p.read_u32())
        print('        attackeeID=%d' % p.read_u32())
        print('        isAllDead=%d' % p.read_u8())
        print('    }')
        print('}')
    def serialize_62489(netid, p: common.PacketReader):
        print('SA_ChatChannelInvite {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    inviteeNickname_len=%d' % n)
        print('    inviteeNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62490(netid, p: common.PacketReader):
        print('SN_ChatChannelInvite {')
        n = p.read_u16()
        print('    inviterName_len=%d' % n)
        print('    inviterName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    channelID_len=%d' % n)
        print('    channelID=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    channelCenter=%d' % p.read_u32())
        print('    channelType=%d' % p.read_u32())
        n = p.read_u16()
        print('    channelName_len=%d' % n)
        print('    channelName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    channelPassword_len=%d' % n)
        print('    channelPassword=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    bInvite=%d' % p.read_u8())
        print('}')
    def serialize_62491(netid, p: common.PacketReader):
        print('SA_ChatChannelInviteFeedback {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62492(netid, p: common.PacketReader):
        print('SN_ChatChannelInviteFeedback {')
        print('    errorCode=%d' % p.read_u32())
        n = p.read_u16()
        print('    inviteeNickname_len=%d' % n)
        print('    inviteeNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    channelName_len=%d' % n)
        print('    channelName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62493(netid, p: common.PacketReader):
        print('SN_InteractionStatus {')
        print('    state=%d' % p.read_i32())
        print('    objectID=0x%08x' % p.read_u32())
        print('    interactionID=%d' % p.read_i32())
        print('    occupiedTeam=%d' % p.read_i32())
        print('    coolTime=%d' % p.read_f32())
        print('}')

    def serialize_62494(netid, p: common.PacketReader):
        print('SN_InteractionCancel {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('}')
    def serialize_62495(netid, p: common.PacketReader):
        print('SN_InteractionCasting {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('    castingTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62496(netid, p: common.PacketReader):
        print('SN_InteractionExecute {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('}')
    def serialize_62497(netid, p: common.PacketReader):
        print('SQ_TeleportObject {')
        print('    objectID=0x%08x' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62498(netid, p: common.PacketReader):
        print('SN_PlayerSyncTeleport {')
        print('    objectID=0x%08x' % p.read_u32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('}')
    def serialize_62499(netid, p: common.PacketReader):
        print('SN_ChangeBattleState {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    isInBattle=%d' % p.read_u8())
        print('    baseMoveSpeed=%d' % p.read_f32())
        print('}')
    def serialize_62501(netid, p: common.PacketReader):
        print('QueueStatus {')
        print('    unk=%d' % p.read_u8())
        print('}')
    def serialize_62502(netid, p: common.PacketReader):
        print('SA_RefreshWaitingQueue {')
        print('    waitingNumber=%d' % p.read_u32())
        print('    estimatedSec=%d' % p.read_u32())
        print('    channelCount=%d' % p.read_u16())
        print('}')
    def serialize_62503(netid, p: common.PacketReader):
        print('SN_GamePartyBroken {')
        print('}')
    def serialize_62504(netid, p: common.PacketReader):
        print('SN_ScoreUpdate {')
        print('    usn=%d' % p.read_u32())
        print('    teamType=%d' % p.read_u32())
        n = p.read_u16()
        print('    records_count=%d' % n)
        for _ in range(n):
            print('    records[] type=%d value=%d' % (p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62505(netid, p: common.PacketReader):
        print('SN_AIMonsterSyncMove {')
        print('    excludedFieldBits=%d' % p.read_u8())
        print('    monsterID=%d' % p.read_u32())
        print('    upperBodyPitch=%d' % p.read_i16())
        print('    upperBodyYaw=%d' % p.read_i16())
        print('    lowerBodyYaw=%d' % p.read_i16())
        print('    angularSpeed=%d' % p.read_i16())
        print('    linearSpeed=%d' % p.read_u16())
        print('    currPos=%s' % read_Vec3(p))
        print('    goalPos=%s' % read_Vec3(p))
        print('    movingFlags=%d' % p.read_u8())
        print('}')
    def serialize_62506(netid, p: common.PacketReader):
        print('SN_AIMonsterSyncTurn {')
        print('    excludedFieldBits=%d' % p.read_u8())
        print('    monsterID=%d' % p.read_u32())
        print('    upperBodyPitch=%d' % p.read_i16())
        print('    upperBodyYaw=%d' % p.read_i16())
        print('    lowerBodyYaw=%d' % p.read_i16())
        print('    angularSpeed=%d' % p.read_i16())
        print('}')
    def serialize_62507(netid, p: common.PacketReader):
        print('SA_ItemTrade {')
        print('    result=%d' % p.read_u32())
        print('    goldAmount=%d' % p.read_i64())
        n = p.read_u16()
        print('    tradedItemList_count=%d' % n)
        for _ in range(n):
            print('    tradedItemList[] itemId=0x%08x itemDocIndex=0x%08x itemCount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62508(netid, p: common.PacketReader):
        print('SA_ItemCraft {')
        print('    craftingItemType=%d' % p.read_u8())
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    craftedItemList_count=%d' % n)
        for _ in range(n):
            print('    craftedItemList[] itemId=0x%08x itemDocIndex=0x%08x itemCount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62509(netid, p: common.PacketReader):
        print('SA_ItemDisassemble {')
        print('    craftingItemType=%d' % p.read_u8())
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    dissolvedItemList_count=%d' % n)
        for _ in range(n):
            print('    dissolvedItemList[] itemId=0x%08x itemDocIndex=0x%08x itemCount=%d' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    def serialize_62510(netid, p: common.PacketReader):
        print('SA_OlympicBestRecord {')
        print('    personalRecord=%d' % p.read_u32())
        print('    worldRecord=%d' % p.read_u32())
        n = p.read_u16()
        print('    worldRecordNickname_len=%d' % n)
        print('    worldRecordNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62511(netid, p: common.PacketReader):
        print('SA_DefenceBestRecord {')
        print('    personalClearStepIndex=%d' % p.read_u32())
        print('    personalClearTime=%d' % p.read_u32())
        print('    worldClearStepIndex=%d' % p.read_u32())
        print('    worldClearTime=%d' % p.read_u32())
        print('}')
    def serialize_62512(netid, p: common.PacketReader):
        print('SN_OlympicIngameRecords {')
        n = p.read_u16()
        print('    rankingList_count=%d' % n)
        for _ in range(n):
            print('    rankingList[] usn=%d ranking=%d record=%d isFinished=%d isDisconnected=%d' % (p.read_u32(), p.read_u8(), p.read_u32(), p.read_u8(), p.read_u8()))
        print('}')
    def serialize_62513(netid, p: common.PacketReader):
        print('SA_PveRefillBattery {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62514(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62514 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62515(netid, p: common.PacketReader):
        print('SA_SelectBadge {')
        print('    badgeType=%d' % p.read_u8())
        print('}')
    def serialize_62516(netid, p: common.PacketReader):
        print('SN_ResistStatus {')
        print('    target=%d' % p.read_u32())
        print('    caster=%d' % p.read_u32())
        print('    event=%d' % p.read_u32())
        print('}')
    def serialize_62517(netid, p: common.PacketReader):
        print('SA_TransformCharacter {')
        print('    result=%d' % p.read_u32())
        print('    characterID=0x%08x' % p.read_u32())
        print('    docIndex=0x%08x' % p.read_u32())
        print('    coolTime=%d' % p.read_u32())
        print('}')
    def serialize_62518(netid, p: common.PacketReader):
        print('SN_TransformCharacter {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    docIndex=0x%08x' % p.read_u32())
        print('    durationTime=%d' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('    SkillSlot1=%d' % p.read_u32())
        print('    SkillSlot2=%d' % p.read_u32())
        print('    ugSkill=%d' % p.read_u32())
        n = p.read_u16()
        print('    maxStats_count=%d' % n)
        for _ in range(n):
            print('    maxStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        n = p.read_u16()
        print('    curStats_count=%d' % n)
        for _ in range(n):
            print('    curStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        print('    baseMoveSpeed=%.2f' % p.read_f32())
        print('    action=%d' % p.read_u8())
        print('}')
    def serialize_62519(netid, p: common.PacketReader):
        print('SN_ChangeCharacterMode {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    docIndex=0x%08x' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('    skillSlot1=%d' % p.read_u32())
        print('    skillSlot2=%d' % p.read_u32())
        print('    ugSkill=%d' % p.read_u32())
        n = p.read_u16()
        print('    maxStats_count=%d' % n)
        for _ in range(n):
            print('    maxStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        n = p.read_u16()
        print('    curStats_count=%d' % n)
        for _ in range(n):
            print('    curStats[] type=%d value=%.2f' % (p.read_u8(), p.read_f32()))
        print('    baseMoveSpeed=%.2f' % p.read_f32())
        print('    action=%d' % p.read_u8())
        print('}')
    def serialize_62520(netid, p: common.PacketReader):
        print('SN_ItemOptiongroupList {')
        n = p.read_u16()
        print('    optionGroupList_count=%d' % n)
        for _ in range(n):
            print('    optionGroupList[] optionGroupIndex=%d' % p.read_u32())
            m = p.read_u16()
            print('    fixed_count=%d' % m)
            for _ in range(m):
                print('    fixed[] propType=%d propTypeDetail=%d valueType=%d minValue=%.2f maxValue=%.2f' % (p.read_u8(), p.read_u32(), p.read_u8(), p.read_f32(), p.read_f32()))
        print('}')
    def serialize_62521(netid, p: common.PacketReader):
        print('SA_Honor {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62522(netid, p: common.PacketReader):
        print('SA_Blame {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62523(netid, p: common.PacketReader):
        print('SN_DynamicSyncAniTrack {')
        print('    oid=%d' % p.read_u32())
        print('    aniTrackType=%d' % p.read_u32())
        print('    pathIndex=%d' % p.read_u32())
        print('    isPlaying=%d' % p.read_u8())
        print('    aniMoveSpeedTimeSec=%.2f' % p.read_f32())
        print('    progress=%.2f' % p.read_f32())
        x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
        print('    pos=(%.2f, %.2f, %.2f)' % (x, y, z))
        print('    curLoopCount=%d' % p.read_u32())
        print('    maxLoopCount=%d' % p.read_u32())
        print('    serverTime=%d' % p.read_i64())
        print('    startTime=%d' % p.read_i64())
        print('}')
    def serialize_62524(netid, p: common.PacketReader):
        print('SN_DisconnectReason {')
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62526(netid, p: common.PacketReader):
        print('SN_AddictionWarning {')
        print('    addictionLevel=%d' % p.read_u8())
        print('}')

    def serialize_62527(netid, p: common.PacketReader):
        print('SA_WatchGame {')
        print('    errcode=%d' % p.read_u32())
        print('}')
    def serialize_62528(netid, p: common.PacketReader):
        print('SN_UserReturnToCity {')
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')

    def serialize_62529(netid, p: common.PacketReader):
        print('SN_NcguardMsg {')
        n = p.read_u16()
        print('    msg_count=%d' % n)
        print('    msg=%s' % p.read_raw(n).decode('utf-8', errors='replace'))
        print('}')
    def serialize_62530(netid, p: common.PacketReader):
        print('SN_TenprotectPunishment {')
        print('    punishMode=%d' % p.read_u8())
        n = p.read_u16()
        print('    punishReason_len=%d' % n)
        print('    punishReason=%s' % p.read_raw(n).decode('utf-8', errors='replace'))
        print('}')
    def serialize_62531(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62531 {')
        print('    field_0=%d' % p.read_u16())
        print('    field_1=%d' % p.read_u16())
        print('    field_2=%d' % p.read_u16())
        print('}')
    def serialize_62532(netid, p: common.PacketReader):
        print('SN_LightOnArea {')
        print('    cellArrayIndex=%d' % p.read_u32())
        print('    team=%d' % p.read_u8())
        print('    radius=%d' % p.read_u8())
        print('}')
    def serialize_62533(netid, p: common.PacketReader):
        print('SN_LightOffArea {')
        print('    cellArrayIndex=%d' % p.read_u32())
        print('    team=%d' % p.read_u8())
        print('    radius=%d' % p.read_u8())
        print('}')
    def serialize_62534(netid, p: common.PacketReader):
        print('SA_RequestVoting {')
        print('    usn=%d' % p.read_u32())
        print('    votingType=%d' % p.read_u32())
        print('    errorType=%d' % p.read_u32())
        print('    coolTime=%d' % p.read_u32())
        print('}')
    def serialize_62535(netid, p: common.PacketReader):
        print('SN_RequestVoting {')
        print('    requestorUsn=%d' % p.read_u32())
        print('    votingType=%d' % p.read_u32())
        print('    period=%d' % p.read_u32())
        print('    voterCount=%d' % p.read_u32())
        print('    isModify=%d' % p.read_u8())
        print('}')
    def serialize_62536(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62536 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62537(netid, p: common.PacketReader):
        print('SA_Vote {')
        print('    usn=%d' % p.read_u32())
        print('    votingType=%d' % p.read_u32())
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62538(netid, p: common.PacketReader):
        print('SN_Vote {')
        print('    votingType=%d' % p.read_u32())
        print('    voteType=%d' % p.read_u32())
        print('}')
    def serialize_62539(netid, p: common.PacketReader):
        print('SN_VotingResult {')
        print('    candidateUsn=%d' % p.read_u32())
        print('    votingType=%d' % p.read_u32())
        print('    votingStateType=%d' % p.read_u32())
        print('    votingResultReasonType=%d' % p.read_u32())
        print('    playerCoolTime=%d' % p.read_u32())
        print('    teamCoolTime=%d' % p.read_u32())
        print('}')
    def serialize_62540(netid, p: common.PacketReader):
        print('SN_PenaltyBadMouther {')
        print('    remainPenaltyTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62541(netid, p: common.PacketReader):
        print('SN_PvpAiModeEventAnnouncement {')
        print('    announcementType=%d' % p.read_u32())
        print('    announcementData=%d' % p.read_u32())
        print('    score=%d' % p.read_u32())
        print('    attackerID=0x%08x' % p.read_u32())
        print('    attackeeID=%d' % p.read_u32())
        print('    isAllDead=%d' % p.read_u8())
        print('}')
    def serialize_62542(netid, p: common.PacketReader):
        print('SN_DateChanged {')
        print('}')


    def serialize_62543(netid, p: common.PacketReader):
        print('SN_CalendarUpdated {')
        print('}')
    def serialize_62544(netid, p: common.PacketReader):
        print('SN_Calendar {')
        n = p.read_u16()
        print('    calendar_count=%d' % n)
        for _ in range(n):
            print('    calendar[] {')
            print('        dateTime=%d' % p.read_i64())
            m = p.read_u16()
            print('        calendarData_count=%d' % m)
            for _ in range(m):
                print('        calendarData[] dataType=%d index=%d startDateTime=%d endDateTime=%d' % (p.read_u8(), p.read_u32(), p.read_i64(), p.read_i64()))
            print('    }')
        print('}')
    def serialize_62546(netid, p: common.PacketReader):
        print('SN_StageSkillAvailable {')
        n = p.read_u16()
        print('    stageSkillList_count=%d' % n)
        for _ in range(n):
            print('    stageSkillList[]=%d' % p.read_u32())
        print('    currentSkillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62547(netid, p: common.PacketReader):
        print('SA_StageSkillSelect {')
        print('    errcode=%d' % p.read_u32())
        print('    currentSkillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62548(netid, p: common.PacketReader):
        print('SN_StageSkillSelect {')
        print('    userId=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62549(netid, p: common.PacketReader):
        print('SN_SinglemodeSelectCharacter {')
        print('}')
    def serialize_62550(netid, p: common.PacketReader):
        print('SN_AvailableAccountEquipmentList {')
        n = p.read_u16()
        print('    itemList_count=%d' % n)
        for _ in range(n):
            print('    itemList[] etcType=%d docIndex=0x%08x count=%d' % (p.read_u8(), p.read_u32(), p.read_u16()))
        print('}')
    def serialize_62551(netid, p: common.PacketReader):
        print('SN_ItemLifetimeExpired {')
        n = p.read_u16()
        print('    itemDocIndexList_count=%d' % n)
        for _ in range(n):
            print('    itemDocIndexList[] itemDocIndex=0x%08x' % p.read_u32())
        print('}')
    def serialize_62552(netid, p: common.PacketReader):
        print('SN_EffectLifetimeExpired {')
        n = p.read_u16()
        print('    effectItemDocIndexList_count=%d' % n)
        for _ in range(n):
            print('    effectItemDocIndexList[] PST_CHARACTER_SKIN_ITEM=%d' % p.read_u32())
        print('}')
    def serialize_62553(netid, p: common.PacketReader):
        print('SN_Gauge {')
        print('    userId=%d' % p.read_u32())
        print('    updateType=%d' % p.read_u8())
        print('    flag=%d' % p.read_u8())
        print('    overheatTime=%d' % p.read_f32())
        print('    gauge=%d' % p.read_u16())
        print('    maxGauge=%d' % p.read_u16())
        print('}')
    def serialize_62554(netid, p: common.PacketReader):
        print('SN_EffectLifetimeList {')
        n = p.read_u16()
        print('    effectItemDocIndexList_count=%d' % n)
        for _ in range(n):
            print('    effectItemDocIndexList[] effectItemDocIndex=%d' % p.read_u32())
            print('    effectItemDocIndexList[] expireDateTime=%d' % p.read_i64())
        print('}')
    def serialize_62555(netid, p: common.PacketReader):
        print('SN_MonsterRespawnTime {')
        print('    stringKey=%d' % p.read_u32())
        print('    localId=%d' % p.read_u32())
        print('    remainTime=%d' % p.read_u32())
        print('}')
    def serialize_62556(netid, p: common.PacketReader):
        print('SA_guideMissionAccept {')
        print('    result=%d' % p.read_i32())
        print('    step=%d' % p.read_u8())
        print('}')
    def serialize_62557(netid, p: common.PacketReader):
        print('SA_GuideMissionReward {')
        print('    result=%d' % p.read_u32())
        print('    step=%d' % p.read_u8())
        print('}')
    def serialize_62558(netid, p: common.PacketReader):
        print('SA_GuideMissionQuit {')
        print('    result=%d' % p.read_u32())
        print('    step=%d' % p.read_u8())
        print('}')
    def serialize_62559(netid, p: common.PacketReader):
        print('SN_CurrentGuideMission {')
        print('    step=%d' % p.read_i8())
        print('    stage=%d' % p.read_u8())
        print('    isRewarded=%d' % p.read_u8())
        print('}')

    def serialize_62560(netid, p: common.PacketReader):
        print('SN_MonsterMineralLossTiming {')
        print('    tagID=%d' % p.read_u32())
        n = p.read_u16()
        print('    lossTiming_count=%d' % n)
        for _ in range(n):
            print('    lossTiming[]=%d' % p.read_u32())
        print('}')
    def serialize_62561(netid, p: common.PacketReader):
        print('SN_PveTimeOverWarning {')
        print('    remainTime=%d' % p.read_i64())
        print('}')
    def serialize_62562(netid, p: common.PacketReader):
        print('SA_ActivityReward {')
        print('    result=%d' % p.read_u32())
        print('    activityRewardedState=%d' % p.read_u8())
        print('}')
    def serialize_62563(netid, p: common.PacketReader):
        print('SA_NpBuyCshopItem {')
        print('    result=%d' % p.read_u32())
        print('    goodsId=%d' % p.read_u32())
        print('    quantity=%d' % p.read_u32())
        print('}')
    def serialize_62564(netid, p: common.PacketReader):
        print('SN_ExpIngame {')
        print('    usn=%d' % p.read_u32())
        print('    acquisitionExp=%d' % p.read_u16())
        print('    currentExp=%d' % p.read_u16())
        print('    target=%d' % p.read_u32())
        print('}')
    def serialize_62565(netid, p: common.PacketReader):
        print('SN_LevelIngame {')
        print('    usn=%d' % p.read_u32())
        print('    expForLevelUp=%d' % p.read_u16())
        print('    currentLevel=%d' % p.read_u8())
        print('}')
    def serialize_62566(netid, p: common.PacketReader):
        print('SN_SpectatorDelay {')
        print('    delayMs=%d' % p.read_u32())
        print('}')
    def serialize_62567(netid, p: common.PacketReader):
        print('SN_SpectatorStart {')
        print('    readyElapsedMS=%d' % p.read_u32())
        print('    playtimeMs=%d' % p.read_u32())
        print('}')
    def serialize_62568(netid, p: common.PacketReader):
        print('SN_SpectatorTrespassReady {')
        print('    isPaused=%d' % p.read_u8())
        print('    readyElapsedMS=%d' % p.read_u32())
        print('}')
    def serialize_62569(netid, p: common.PacketReader):
        print('SN_PhaseStart {')
        print('    step=%d' % p.read_u32())
        print('}')
    def serialize_62570(netid, p: common.PacketReader):
        print('SN_PhaseEnd {')
        print('    step=%d' % p.read_u32())
        print('    endReason=%d' % p.read_u32())
        print('    clearTime=%d' % p.read_u32())
        print('}')
    def serialize_62571(netid, p: common.PacketReader):
        print('SA_MasterTrainingEvent {')
        print('    eventType=%d' % p.read_u8())
        print('    ret=%d' % p.read_u32())
        print('    entityID=0x%08x' % p.read_u32())
        print('    param1=%d' % p.read_u32())
        print('    param2=%d' % p.read_u32())
        print('}')
    def serialize_62572(netid, p: common.PacketReader):
        print('SN_GmDisabledMasters {')
        n = p.read_u16()
        print('    masterList_count=%d' % n)
        for _ in range(n):
            print('    masterList[]=%d' % p.read_u32())
        print('}')
    def serialize_62573(netid, p: common.PacketReader):
        print('SN_GmDisabledSkins {')
        n = p.read_u16()
        print('    skinKeyList_count=%d' % n)
        for _ in range(n):
            print('    skinKeyList[]=%s' % p.read_wstr())
        print('}')
    def serialize_62574(netid, p: common.PacketReader):
        print('SN_GmDisabledStages {')
        n = p.read_u16()
        print('    stageList_count=%d' % n)
        for _ in range(n):
            print('    stageList[] stageIndex=%d gameType=%d' % (p.read_u32(), p.read_u8()))
        print('}')
    def serialize_62578(netid, p: common.PacketReader):
        print('SN_CharacterPropertyPoint {')
        print('    characterId=%d' % p.read_u32())
        print('    propertyPoint=%d' % p.read_u8())
        print('}')
    def serialize_62579(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62579 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62580(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62580 {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62581(netid, p: common.PacketReader):
        print('SA_CharacterPropertyUpgrade {')
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62582(netid, p: common.PacketReader):
        print('SA_CharacterPropertyReset {')
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62583(netid, p: common.PacketReader):
        print('SN_CharacterPropertyReset {')
        print('    characterId=%d' % p.read_u32())
        print('}')
    def serialize_62584(netid, p: common.PacketReader):
        print('SN_CharacterPropertyInfo {')
        print('    characterId=%d' % p.read_u32())
        n = p.read_u16()
        print('    propertyInfos_count=%d' % n)
        for _ in range(n):
            print('    propertyInfos[] propertyIndex=%d propertyLevel=%d' % (p.read_u8(), p.read_u8()))
        print('}')
    def serialize_62585(netid, p: common.PacketReader):
        print('SN_GameRecordInfoList {')
        n = p.read_u16()
        print('    packetRecInfos_count=%d' % n)
        for _ in range(n):
            print('    packetRecInfos[] elapsedTime=%d' % p.read_u32())
            m = p.read_u16()
            print('    packetRecInfos[] packetData_len=%d' % m)
            print('    packetRecInfos[] packetData=%s' % p.read_raw(m).hex())
        print('}')
    def serialize_62586(netid, p: common.PacketReader):
        print('SA_ChatGetInviteeGameaccountid {')
        print('    result=%d' % p.read_u32())
        print('    inviteeCenterId=%d' % p.read_u32())
        print('    inviteeNickname=%s' % p.read_wstr())
        print('    inviteeGameAccountId=%s' % p.read_str())
        print('}')
    def serialize_62587(netid, p: common.PacketReader):
        print('SA_UseCoupon {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    couponKey_len=%d' % n)
        print('    couponKey=%s' % p.read_raw(n).decode('utf-8', errors='replace'))
        print('}')
    def serialize_62588(netid, p: common.PacketReader):
        print('SN_NotifyChattingChannelStatus {')
        print('    channelId=%s' % p.read_wstr())
        print('    channelCenter=%s' % p.read_wstr())
        print('    password=%s' % p.read_wstr())
        print('    token=%s' % p.read_wstr())
        print('    argument=%s' % p.read_wstr())
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62589(netid, p: common.PacketReader):
        print('SN_DefenceModeIngameInfo {')
        print('    currentStep=%d' % p.read_u32())
        print('    totalClearTime=%d' % p.read_u32())
        print('}')
    def serialize_62590(netid, p: common.PacketReader):
        print('SA_RequestToken {')
        n = p.read_u16()
        print('    authnToken_len=%d' % n)
        print('    authnToken=%s' % p.read_raw(n).decode('utf-8', errors='replace'))
        print('}')
    def serialize_62591(netid, p: common.PacketReader):
        print('SN_HudEventList {')
        n = p.read_u16()
        print('    eventList_count=%d' % n)
        for _ in range(n):
            print('    eventList[] eventType=%d' % p.read_u8())
            print('    eventList[] bannerUrl=%s' % p.read_str())
            print('    eventList[] parameter=%s' % p.read_str())
        print('}')
    def serialize_62592(netid, p: common.PacketReader):
        print('SN_CshopEventList {')
        n = p.read_u16()
        print('    eventList_count=%d' % n)
        for _ in range(n):
            print('    eventList[] eventType=%d positionType=%d goodsId=%d bannerUrl=%s' % (
                p.read_u8(), p.read_u8(), p.read_u32(), p.read_str()))
        print('}')
    def serialize_62593(netid, p: common.PacketReader):
        print('SA_UserReport {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62594(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62594 {')
        print('    field_0=%d' % p.read_u8())
        print('    field_1=%d' % p.read_u8())
        print('}')
    def serialize_62595(netid, p: common.PacketReader):
        print('SN_ServerUtcTime {')
        print('    effectItemDocIndexList=%d' % p.read_i64())
        print('}')

    def serialize_62596(netid, p: common.PacketReader):
        print('SN_NotifyPccafeTagbuff {')
        print('}')
    def serialize_62597(netid, p: common.PacketReader):
        print('SN_PraiseInfo {')
        print('    durationMs=%d' % p.read_u32())
        n = p.read_u16()
        print('    praiseTargetInfos_count=%d' % n)
        for _ in range(n):
            print('    praiseTargetInfos[] usn=%d' % p.read_u32())
            nick_len = p.read_u16()
            print('    praiseTargetInfos[] nickName=%s' % p.read_raw(nick_len * 2).decode('utf-16-le', errors='replace'))
            print('    praiseTargetInfos[] teamType=%d' % p.read_u32())
            print('    praiseTargetInfos[] activeCreatureIndex=%d' % p.read_u32())
            print('    praiseTargetInfos[] inActiveCreatureIndex=%d' % p.read_u32())
            print('    praiseTargetInfos[] praiseType=%d' % p.read_u32())
            print('    praiseTargetInfos[] value=%d' % p.read_u32())
            print('    praiseTargetInfos[] praiseRelationType=%d' % p.read_u32())
            print('    praiseTargetInfos[] relationValue=%d' % p.read_u32())
        print('}')
    def serialize_62598(netid, p: common.PacketReader):
        print('SN_PraiseStart {')
        print('    remainDurationMs=%d' % p.read_u32())
        print('}')
    def serialize_62599(netid, p: common.PacketReader):
        print('SA_Praise {')
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62600(netid, p: common.PacketReader):
        print('SN_Praise {')
        n = p.read_u16()
        print('    nickName_len=%d' % n)
        print('    nickName=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    praiseCount=%d' % p.read_u32())
        print('}')
    def serialize_62601(netid, p: common.PacketReader):
        print('SN_UpdateMasterGroupingEffect {')
        print('    instanceId=%d' % p.read_i32())
        print('    index=%d' % p.read_i32())
        print('    classType=%d' % p.read_i32())
        print('    activate=%d' % p.read_u8())
        print('}')
    def serialize_62602(netid, p: common.PacketReader):
        print('SA_PauseGame {')
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62603(netid, p: common.PacketReader):
        print('SN_PausedGame {')
        print('    remainDurationMs=%d' % p.read_u32())
        print('    playTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62604(netid, p: common.PacketReader):
        print('SA_ResumeGame {')
        print('    errorType=%d' % p.read_u32())
        print('}')
    def serialize_62605(netid, p: common.PacketReader):
        print('SN_ResumeGameDelay {')
        print('    resumeDelayMs=%d' % p.read_u32())
        print('}')
    def serialize_62606(netid, p: common.PacketReader):
        print('SN_ResumedGame {')
        print('}')
    def serialize_62607(netid, p: common.PacketReader):
        print('SA_NpReportChat {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62608(netid, p: common.PacketReader):
        print('SA_NpReportMail {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62005(netid, p: common.PacketReader):
        print('SA_AuthResult {')
        print('    result=%d' % p.read_i32())
        print('}')

    def serialize_62006(netid, p: common.PacketReader):
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
        print('    regionNewMaster=[%s]' % s)

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

    def serialize_62007(netid, p: common.PacketReader):
        print('SN_StationList {')

        count = p.read_u16()
        s = ''
        while count > 0:
            idc = p.read_i32()
            stations = ''
            station_count = p.read_u16()
            while station_count > 0:
                stations += 'gameServerIp=(%d.%d.%d.%d) pingServerIp=(%d.%d.%d.%d) port=%d' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u16())
                station_count = -1

            s += '(idc=%d stations=[%s])' % (idc, stations)
            count -= 1
        
        print('    stationList[%s]' % s)

        print('}')

    def serialize_62010(netid, p: common.PacketReader):
        print('SN_DoConnectGameServer {')
        print('    dwPort=%d' % p.read_u16())
        print('    dwIP=%d' % p.read_u32())
        print('    dwGameID=%d' % p.read_u32())
        print('    idcHash=%d' % p.read_u32())
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    nickname=%s' % s)
        print('    instantKey=%d' % p.read_u32())
        print('}')
    def serialize_62025(netid, p: common.PacketReader):
        print('SN_GameCreateActor {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    nType=%d' % p.read_i32())
        print('    nIDX=%d' % p.read_i32())
        print('    dwLocalID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    spawnType=%d' % p.read_i32())
        print('    actionState=%d' % p.read_i32())
        print('    ownerID=0x%08x' % p.read_u32())
        print('    bDirectionToNearPC=%d' % p.read_u8())
        print('    AiWanderDistOverride=%d' % p.read_i32())
        print('    tagID=%d' % p.read_i32())
        print('    faction=%d' % p.read_i32())
        print('    classType=%d' % p.read_i32())
        print('    skinIndex=%d' % p.read_i32())
        print('    seed=%d' % p.read_i32())

        print('    initStat={')

        count = p.read_u16()
        print('        maxStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('      ]')

        count = p.read_u16()
        print('        curStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('      ]')

        print('    }')
        print('    isInSight=%d' % p.read_u8())
        print('    isDead=%d' % p.read_u8())
        print('    serverTime=%d' % p.read_i64())
        print('    meshChangeActionHistory_count=%d' % p.read_u16())

        print('}')

    def serialize_62026(netid, p: common.PacketReader):
        print('SN_SpawnPosForMinimap {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('}')

    def serialize_62028(netid, p: common.PacketReader):
        print('SN_GameCreateSubActor {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    mainEntityID=0x%08x' % p.read_u32())
        print('    nType=%d' % p.read_i32())
        print('    nIDX=%d' % p.read_i32())
        print('    dwLocalID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    spawnType=%d' % p.read_i32())
        print('    actionState=%d' % p.read_i32())
        print('    ownerID=0x%08x' % p.read_u32())
        print('    tagID=%d' % p.read_i32())
        print('    faction=%d' % p.read_i32())
        print('    classType=%d' % p.read_i32())
        print('    skinIndex=%d' % p.read_i32())
        print('    seed=%d' % p.read_i32())

        print('    initStat={')
        
        count = p.read_u16()
        print('        maxStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('      ]')

        count = p.read_u16()
        print('        curStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('      ]')

        print('    }')
        print('    meshChangeActionHistory_count=%d' % p.read_u16())
        print('}')

    def serialize_62029(netid, p: common.PacketReader):
        print('SN_GameEnterActor {')
        excludedBits = p.read_u8()
        print('    excludedBits=%x' % excludedBits)

        # graph move
        if (excludedBits & 1) == 0:
            print('    extraGraphMove={')
            print('        hasGraphMove=%d' % p.read_u8())
            print('        distance=%f' % p.read_f32())
            print('        totalTimeS=%f' % p.read_f32())
            print('        curTimeS=%f' % p.read_f32())
            print('        startPos=%s' % read_Vec3(p))
            print('        endPos=%s' % read_Vec3(p))
            print('        originDistance=%f' % p.read_f32())
            print('        hasExtraMove=%d' % p.read_u8())
            print('        vExtraPointMoveTarget=%s' % read_Vec2(p))
            print('        vExtraPointMoveRemainTime=%f' % p.read_f32())
            print('        vExtraDirMove=%s' % read_Vec2(p))
            print('        vExtraDirMoveRemainTime=%f' % p.read_f32())
            print('    }')

        print('    objectID=0x%08x' % p.read_u32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    p2nMoveDir=%s' % read_Vec2(p))
        print('    p2nUpperDir=%s' % read_Vec2(p))
        print('    p3nMoveTargetPos=%s' % read_Vec3(p))
        print('    isBattleState=%d' % p.read_u8())
        print('    baseMoveSpeed=%f' % p.read_f32())
        print('    actionState=%d' % p.read_i32())
        print('    aiTargetID=%d' % p.read_i32())

        print('    statSnapshot={')

        count = p.read_u16()
        print('        curStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('        ]')
        
        count = p.read_u16()
        print('        maxStats(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('        ]')

        count = p.read_u16()
        print('        addPrivate(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('        ]')

        count = p.read_u16()
        print('        mulPrivate(%d)=[' % count)
        while count > 0:
            print('            (type=%d value=%g),' % (p.read_u8(), p.read_f32()))
            count -= 1
        print('        ]')

        print('    }')
        print('}')

    def serialize_62030(netid, p: common.PacketReader):
        print('SN_GameLeaveActor {')
        print('    objectID=0x%08x' % p.read_u32())
        print('}')

    def serialize_62031(netid, p: common.PacketReader):
        print('SN_StatusSnapshot {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    statusArray=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      statusIndex=%d' % p.read_i32())
            print('      bEnabled=%d' % p.read_u8())
            print('      caster=%d' % p.read_i32())
            print('      overlapCount=%d' % p.read_u8())
            print('      customValue=%d' % p.read_u8())
            print('      durationTimeMs=%d' % p.read_i32())
            print('      remainTimeMs=%d' % p.read_i32())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62033(netid, p: common.PacketReader):
        print('SQ_CityLobbyJoinCity {}')

    def serialize_62035(netid, p: common.PacketReader):
        print('SN_CastSkill {')
        print('	entityID=0x%08x' % p.read_u32())
        print('	ret=%d' % p.read_i32())
        print('	skillIndex=%d' % p.read_i32())
        print('	costLevel=%d' % p.read_u8())
        print('	actionstate=%d' % p.read_i32())
        print('	targetPos=%s' % read_Vec3(p))
        
        count = p.read_u16()
        print('	targetIDs(%d)=[' % count)
        while count > 0:
            print('		%d,' % p.read_i32())
            count -= 1
        print('	]')

        print('	bSyncMyPosition=%d' % p.read_u8())
        print('	posStruct={')
        print('		pos=%s' % read_Vec3(p))
        print('		destPos=%s' % read_Vec3(p))
        print('		moveDir=(%g, %g)' % (p.read_f32(), p.read_f32()))
        print('		rotateStruct=%s' % read_Vec3(p))
        print('		speed=%g' % p.read_f32())
        print('		clientTime=%d' % p.read_i32())
        print('	}')
        print('}')

    def serialize_62036(netid, p: common.PacketReader):
        print('SN_ExecuteSkill {')
        print('	entityID=0x%08x' % p.read_u32())
        print('	ret=%d' % p.read_i32())
        print('	skillIndex=%d' % p.read_i32())
        print('	costLevel=%d' % p.read_u8())
        print('	actionstate=%d' % p.read_i32())
        print('	targetPos=%s' % read_Vec3(p))
        
        count = p.read_u16()
        print('	targetIDs(%d)=[' % count)
        while count > 0:
            print('		%d,' % p.read_i32())
            count -= 1
        print('	]')

        print('	bSyncMyPosition=%d' % p.read_u8())
        print('	posStruct={')
        print('		pos=%s' % read_Vec3(p))
        print('		destPos=%s' % read_Vec3(p))
        print('		moveDir=(%g, %g)' % (p.read_f32(), p.read_f32()))
        print('		rotateStruct=%s' % read_Vec3(p))
        print('		speed=%g' % p.read_f32())
        print('		clientTime=%d' % p.read_i32())
        print('	}')
        print('	fSkillChargeDamageMultiplier=%g' % p.read_f32())
        print('	graphMove={')
        print('		bApply=%d' % p.read_u8())
        print('		startPos=%s' % read_Vec3(p))
        print('		endPos=%s' % read_Vec3(p))
        print('		durationTimeS=%g' % p.read_f32())
        print('		originDistance=%g' % p.read_f32())
        print('	}')

        print('}')

    def serialize_62041(netid, p: common.PacketReader):
        print('SA_CastSkill {')
        print('	characterID=0x%08x' % p.read_u32())
        print('	ret=%d' % p.read_i32())
        print('	skillIndex=%d' % p.read_i32())
        print('}')

    def serialize_62048(netid, p: common.PacketReader):
        print('SN_PlayerSkillSlot {')
        print('    characterID=0x%08x' % p.read_u32())

        count = p.read_u16()
        print('    slots(%d)=[' % count)
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

            print('      propList(%d)=[%s]' % (prop_count, s))
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

    def serialize_62050(netid, p: common.PacketReader):
        print('SN_LoadCharacterStart {}')

    def serialize_62051(netid, p: common.PacketReader):
        print('SN_ScanEnd {}')

    def serialize_62052(netid, p: common.PacketReader):
        print('SN_GamePlayerSyncByInt {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDirection=%s' % read_Vec3(p))
        print('    p3nEye=%s' % read_Vec3(p))
        print('    nRotate=%g' % p.read_f32())
        print('    nSpeed=%g' % p.read_f32())
        print('    nState=%d' % p.read_i32())
        print('    nActionIDX=%d' % p.read_i32())
        print('}')

    def serialize_62059(netid, p: common.PacketReader):
        print('SN_DestroyEntity {')
        print('    objectID=0x%08x' % p.read_u32())
        print('}')

    def serialize_62060(netid, p: common.PacketReader):
        print('SN_SetGameGvt {')
        print('    sendTime=%d' % p.read_i32())
        print('    virtualTime=%d' % p.read_i32())
        print('}')

    def serialize_62061(netid, p: common.PacketReader):
        print('SN_LobbyStartGame {')
        print('    stageType=%d' % p.read_i32())
        print('}')

    def serialize_62064(netid, p: common.PacketReader):
        print('SN_LoadClearedStages {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    clearedStageList=[%s]' % s)
        print('}')

    def serialize_62072(netid, p: common.PacketReader):
        print('SN_GameFieldReady {')
        print('    InGameID=%d' % p.read_i32())
        print('    GameType=%d' % p.read_i32())
        print('    AreaIndex=%d' % p.read_i32())
        print('    StageIndex=%d' % p.read_i32())
        print('    GameDefinitionType=%d' % p.read_i32())
        print('    initPlayerCount=%d' % p.read_u8())
        print('    CanEscape=%d' % p.read_u8())
        print('    IsTrespass=%d' % p.read_u8())
        print('    IsSpectator=%d' % p.read_u8())
        print('    InGameUsers=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=0x%08x' % p.read_u32())
            print('      nickname="%s"' % p.read_wstr())
            print('      team=%d' % p.read_u8())
            print('      isBot=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')

        print('    IngamePlayers=[')
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=0x%08x' % p.read_u32())
            print('      mainCreatureIndex=%d' % p.read_i32())
            print('      mainSkinIndex=%d' % p.read_i32())
            print('      mainSkillindex1=%d' % p.read_i32())
            print('      mainSkillIndex2=%d' % p.read_i32())
            print('      subCreatureIndex=%d' % p.read_i32())
            print('      subSkinIndex=%d' % p.read_i32())
            print('      subSkillIndex1=%d' % p.read_i32())
            print('      subSkillIndex2=%d' % p.read_i32())
            print('      stageSkillIndex1=%d' % p.read_i32())
            print('      stageSkillIndex2=%d' % p.read_i32())
            print('      supportKitIndex=%d' % p.read_i32())
            print('      isBot=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')

        print('    IngameGuilds=[')
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      teamType=%d' % p.read_u8())
            print('      guildName="%s"' % p.read_wstr())
            print('      guildTag="%s"' % p.read_wstr())
            print('      guildEmblemIndex=%d' % p.read_i32())
            print('      guildPvpRankNo=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

        print('    surrenderAbleTime=%d' % p.read_i32())
        print('}')

    def serialize_62084(netid, p: common.PacketReader):
        print('SN_GamePlayerEquipWeapon {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    weaponDocIndex=%d' % p.read_i32())
        print('    additionnalOverHeatGauge=%d' % p.read_i32())
        print('    additionnalOverHeatGaugeRatio=%d' % p.read_i32())
        print('}')

    def serialize_62089(netid, p: common.PacketReader):
        print('SN_GamePlayerStock {')
        print('    playerID=0x%08x' % p.read_u32())
        n = p.read_u16()
        print('    wStrPlayerName_len=%d' % n)
        print('    wStrPlayerName=%r' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    m_Class=%d' % p.read_u32())
        print('    m_DisplayTitleIDX=%d' % p.read_u32())
        print('    m_StatTitleIDX=%d' % p.read_u32())
        print('    m_badgeType=%d' % p.read_u8())
        print('    m_badgeTierLevel=%d' % p.read_u8())
        n = p.read_u16()
        print('    m_guildTag_len=%d' % n)
        print('    m_guildTag=%r' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    m_vipLevel=%d' % p.read_u8())
        print('    m_staffType=%d' % p.read_u8())
        print('    m_isSubstituted=%d' % p.read_u8())
        print('}')
    def serialize_62090(netid, p: common.PacketReader):
        print('SN_PlayerStateInTown {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    playerStateInTown=%d' % p.read_i8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_u8()
            count -= 1

        print('    matchingGameModes=[%s]' % s)

        print('}')

    def serialize_62091(netid, p: common.PacketReader):
        print('SN_CityMapInfo {')
        print('    cityMapID=%d' % p.read_i32())
        print('}')

    def serialize_62097(netid, p: common.PacketReader):
        print('SN_SummaryInfoAll {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    listOfFinishedSummary=[%s]' % s)
        print('}')

    def serialize_62098(netid, p: common.PacketReader):
        print('SN_AvailableSullaryRewardCountList {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    rewardCountList=[%s]' % s)
        print('}')

    def serialize_62100(netid, p: common.PacketReader):
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

    def serialize_62101(netid, p: common.PacketReader):
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
        

    def serialize_62102(netid, p: common.PacketReader):
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

    def serialize_62106(netid, p: common.PacketReader):
        print('SN_AccountInfo {')
        n = p.read_u16()
        print('    Nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    inventoryLineCountTab0=%d' % p.read_u32())
        print('    inventoryLineCountTab1=%d' % p.read_u32())
        print('    inventoryLineCountTab2=%d' % p.read_u32())
        print('    displayTitleIndex=%d' % p.read_u32())
        print('    statTitleIndex=%d' % p.read_u32())
        print('    warehouseLineCount=%d' % p.read_u32())
        print('    tutorialState=%d' % p.read_u32())
        print('    masterGearDurability=%d' % p.read_u32())
        print('    badgeType=%d' % p.read_u8())
        print('}')
    def serialize_62107(netid, p: common.PacketReader):
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

    def serialize_62109(netid, p: common.PacketReader):
        print('SN_AllCharacterBaseData {')
        print('    characters=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('    docIndex=0x%08x' % p.read_i32())

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

    def serialize_62112(netid, p: common.PacketReader):
        print('SN_GamePlayerTag {')
        print('    result=%d' % p.read_i32())
        print('    mainID=0x%08x' % p.read_u32())
        print('    subID=0x%08x' % p.read_u32())
        print('    attackerID=0x%08x' % p.read_u32())
        print('}')

    def serialize_62113(netid, p: common.PacketReader):
        print('SA_GetCharacterInfo {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    docIndex=0x%08x' % p.read_i32())
        print('    class=%d' % p.read_i32())
        print('    hp=%d' % p.read_i32())
        print('    maxHp=%d' % p.read_i32())
        print('}')

    def serialize_62123(netid, p: common.PacketReader):
        print('SN_LeaderCharacter {')
        print('    leaderID=0x%08x' % p.read_u32())
        print('    skinIndex=%d' % p.read_i32())
        print('}')

    def serialize_62124(netid, p: common.PacketReader):
        print('SN_ProfileCharacters {')
        print('    characters=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      characterID=0x%08x' % p.read_u32())
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

    def serialize_62125(netid, p: common.PacketReader):
        print('SN_ProfileItems {')
        print('    packetNum=%d' % p.read_u8())
        print('    items=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      itemID=0x%08x' % p.read_u32())
            print('      invenType=%d' % p.read_u8())
            print('      slot=%d' % p.read_i32())
            print('      itemIndex=0x%08x' % p.read_i32())
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

    def serialize_62126(netid, p: common.PacketReader):
        print('SN_ProfileWeapons {')
        print('    weapons=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      characterID=0x%08x' % p.read_u32())
            print('      weaponType=%d' % p.read_i32())
            print('      weaponIndex=%d' % p.read_i32())
            print('      grade=%d' % p.read_i32())
            print('      isUnlocked=%d' % p.read_u8())
            print('      isActivated=%d' % p.read_u8())
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62127(netid, p: common.PacketReader):
        print('SN_ProfileSkills {')
        print('    packetNum=%d' % p.read_u8())
        
        count = p.read_u16()
        print('    skills(%d)=[' % count)
        while count > 0:
            print('    {')
            print('      characterID=0x%08x' % p.read_u32())
            print('      skillIndex=%d' % p.read_i32())
            print('      isUnlocked=%d' % p.read_u8())
            print('      isActivated=%d' % p.read_u8())

            prop_count = p.read_u16()
            s = ''
            while prop_count > 0:
                s += '(skillIndex=%d level=%d), ' % (p.read_i32(), p.read_i32())
                prop_count -= 1

            print('      properties(%d)=[%s]' % (prop_count, s))

            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62128(netid, p: common.PacketReader):
        print('SN_ProfileTitles {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    titles=[%s]' % s)
        print('}')

    def serialize_62129(netid, p: common.PacketReader):
        print('SN_ProfileMasterGears {')
        print('    masterGears=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      masterGearNo=%d' % p.read_u8())
            print('      name="%s"' % p.read_wstr())

            slot_count = p.read_u16()
            s = ''
            while slot_count > 0:
                s += '(gearType=%d gearItemID=0x%08x), ' % (p.read_i32(), p.read_i32())
                slot_count -= 1
            
            print('      slots=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62158(netid, p: common.PacketReader):
        print('SA_EnqueueGame {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62173(netid, p: common.PacketReader):
        print('SA_AreaPopularity {')
        print('    errCode=%d' % p.read_i32())
        print('}')

    def serialize_62174(netid, p: common.PacketReader):
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

    def serialize_62175(netid, p: common.PacketReader):
        print('SA_PartyCreate {')
        print('    retval=%d' % p.read_i32())
        print('    ownerUserId=%d' % p.read_i32())
        print('    stageType=%d' % p.read_i32())
        print('}')

    def serialize_62176(netid, p: common.PacketReader):
        print('SA_PartyModify {')
        print('    retval=%d' % p.read_i32())
        print('}')

    def serialize_62204(netid, p: common.PacketReader):
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
            print('      userID=0x%08x' % p.read_u32())
            print('      nickname="%s"' % p.read_wstr())
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
            print('      userID=0x%08x' % p.read_u32())
            print('      nickname="%s"' % p.read_wstr())
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
            print('      userID=0x%08x' % p.read_u32())
            print('      nickname="%s"' % p.read_wstr())
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

    def serialize_62205(netid, p: common.PacketReader):
        print('SN_MatchingPartyGathered {')
        print('    allConfirmed=%u' % p.read_u8())
        print('}')

    def serialize_62208(netid, p: common.PacketReader):
        print('SA_MasterPick {')
        print('    retval=%d' % p.read_i32())
        print('    characterID=0x%08x' % p.read_u32())
        print('}')

    def serialize_62217(netid, p: common.PacketReader):
        print('SN_StartCountdownSortieRoom {')
        print('    stageType=%d' % p.read_i32())
        print('    timeToWaitSec=%d' % p.read_i32())
        print('}')

    def serialize_62218(netid, p: common.PacketReader):
        print('SN_SortiePrepare {')
        count = p.read_u16()
        print('    skinList(%d)=[' % count)
        while count > 0:
            print('    {')
            print('      classType=%d' % p.read_i32())
            print('      skinIndex=%d' % p.read_i32())
            print('      bufCount=%d' % p.read_i32())
            print('      expireTime=%d' % p.read_i64())
            print('    },')
            count -= 1
        print('    ]')
        count = p.read_u16()
        print('    skillList(%d)=[' % count)
        while count > 0:
            print('     %d,' % p.read_i32())
            count -= 1
        print('    ]')
        print('}')

    def serialize_62219(netid, p: common.PacketReader):
        print('SN_SortiePrepareBotInfo {')
        count = p.read_u16()
        print('    botIndexes(%d)=[' % count)
        while count > 0:
            print('     %d,' % p.read_i32())
            count -= 1
        print('    ]')
        print('}')

    def serialize_62224(netid, p: common.PacketReader):
        print('SN_UpdateGameOwner {')
        print('    userID=0x%08x' % p.read_u32())
        print('}')

    def serialize_62227(netid, p: common.PacketReader):
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

    def serialize_62229(netid, p: common.PacketReader):
        print('SN_NotifyPcDetailInfos {')
        print('    pcList=[')
          
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      userID=0x%08x' % p.read_u32())

            s = 'characterID=0x%08x, docIndex=0x%08x, class=%d, hp=%d, maxHp=%d' % (p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32())
            print('      mainPc=( %s )' % s)
            s = 'characterID=0x%08x, docIndex=0x%08x, class=%d, hp=%d, maxHp=%d' % (p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32())
            print('      subPc=( %s )' % s)
            print('      remainTagCooltimeMs=%d' % p.read_i32())
            print('      canCastSkillSlotUG=%d' % p.read_u8())
            print('    },')
            count -= 1

        print('    ]')
        print('}')

    def serialize_62238(netid, p: common.PacketReader):
        print('SA_ResultSpAction {')
        excludedFieldBits = p.read_u8()
        print('    excludedFieldBits=%u' % excludedFieldBits)
        print('    actionID=%d' % p.read_i32())
        print('    objectID=0x%08x' % p.read_u32())
        print('    rotate=%g' % p.read_f32())
        print('    moveDir=(%g, %g)' % (p.read_f32(), p.read_f32()))
        print('    errorType=%d' % p.read_i32())

        if excludedFieldBits & 0x20 == 0:
            print('    startPos=%s' % read_Vec3(p))
        print('}')

    def serialize_62242(netid, p: common.PacketReader):
        print('SN_ChatChannelMessage {')
        print('    chatType=%d' % p.read_u32())
        n = p.read_u16()
        print('    senderNickname_len=%d' % n)
        print('    senderNickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    senderStaffType=%d' % p.read_u8())
        n = p.read_u16()
        print('    chatMsg_len=%d' % n)
        print('    chatMsg=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62257(netid, p: common.PacketReader):
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

    def serialize_62258(netid, p: common.PacketReader):
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

    def serialize_62259(netid, p: common.PacketReader):
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

    def serialize_62261(netid, p: common.PacketReader):
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

    def serialize_62276(netid, p: common.PacketReader):
        print('SN_NotifyAasRestricted {')
        print('    isRestrictedByAAS=%d' % p.read_u8())
        print('}')

    def serialize_62278(netid, p: common.PacketReader):
        print('SN_Exp {')
        print('    leaderCSN=%d' % p.read_u32())
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    isLevelUp=%d' % p.read_u8())
        print('    level=%d' % p.read_u16())
        print('    currentLevelExp=%d' % p.read_u32())
        print('}')
    def serialize_62282(netid, p: common.PacketReader):
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

    def serialize_62283(netid, p: common.PacketReader):
        print('SN_JukeboxPlay {')
        print('    result=%d' % p.read_i32())
        print('    track={')
        print('      trackId=%d' % p.read_i32())
        print('      nickname="%s"' % p.read_wstr())
        print('    }')
        print('    playPositionSec=%d' % p.read_u16())
        print('}')

    def serialize_62284(netid, p: common.PacketReader):
        print('SN_JukeboxHotTrackList {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    trackList=[%s]' % s)
        print('}')

    def serialize_62285(netid, p: common.PacketReader):
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
        
    def serialize_62299(netid, p: common.PacketReader):
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
    
    def serialize_62300(netid, p: common.PacketReader):
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

    def serialize_62302(netid, p: common.PacketReader):
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

    def serialize_62322(netid, p: common.PacketReader):
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

    def serialize_62330(netid, p: common.PacketReader):
        print('SN_MyGuild {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    dissolutionDate=%d' % p.read_i64())
        print('    isFirstTodayRollCall=%d' % p.read_u8())
        print('}')
    def serialize_62338(netid, p: common.PacketReader):
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

    def serialize_62358(netid, p: common.PacketReader):
        print('SN_GuildChannelEnter {')
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    payload=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    onlineStatus=%d' % p.read_u8())
        print('}')
    def serialize_62390(netid, p: common.PacketReader):
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

    def serialize_62400(netid, p: common.PacketReader):
        print('SN_NotifyUserLifeInfo {')
        print('    usn=%d' % p.read_i32())
        print('    lifeCount=%d' % p.read_i32())
        print('    maxLifeCount=%d' % p.read_i32())
        print('    remainLifeCount=%d' % p.read_i32())
        print('}')
        
    def serialize_62404(netid, p: common.PacketReader):
        print('SN_WarehouseItems {')
        print('    items=[')
        
        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      itemID=0x%08x' % p.read_u32())
            print('      invenType=%d' % p.read_u8())
            print('      slot=%d' % p.read_i32())
            print('      itemIndex=0x%08x' % p.read_i32())
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
    
    def serialize_62425(netid, p: common.PacketReader):
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

    def serialize_62426(netid, p: common.PacketReader):
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

    def serialize_62446(netid, p: common.PacketReader):
        print('SQ_Heartbeat {}')

    def serialize_62448(netid, p: common.PacketReader):
        print('SN_RunClientLevelEvent {')
        print('    eventID=%d' % p.read_i32())
        print('    caller=%d' % p.read_i32())
        print('    serverTime=%d' % p.read_i64())
        print('}')

    def serialize_62450(netid, p: common.PacketReader):
        print('SN_LoadingProgressData {')
        print('    usn=%d' % p.read_u32())
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le'))
        print('    progressData=%d' % p.read_u8())
        print('    activeCreatureIndex=%d' % p.read_u32())
        print('    inactiveCreatureIndex=%d' % p.read_u32())
        print('    isSpectator=%d' % p.read_u8())
        print('}')
    def serialize_62455(netid, p: common.PacketReader):
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

    def serialize_62462(netid, p: common.PacketReader):
        print('SN_SortieCharacterSlotInfo {')
        count = p.read_u16()
        print('    slotInfos(%d)=[' % count)
        while count > 0:
            print('    {')
            print('     creatureIndex=%d' % p.read_i32())
            slotStatesCount = p.read_u16()
            s = ''
            while slotStatesCount > 0:
                s += '%d, ' % p.read_i32() 
                slotStatesCount -= 1
            print('     slotStates=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62465(netid, p: common.PacketReader):
        print('SN_SortieMasterPickPhaseStart {')
        print('    isRandomPick=%d' % p.read_u8())

        count = p.read_u16()
        print('    alliesSlotInfos(%d)=[' % count)
        while count > 0:
            print('    {')
            print('     creatureIndex=%d' % p.read_i32())
            slotStatesCount = p.read_u16()
            s = ''
            while slotStatesCount > 0:
                s += '%d, ' % p.read_i32() 
                slotStatesCount -= 1
            print('     slotStates=[%s]' % s)
            print('    },')
            count -= 1
        print('    ]')
        print('}')

    def serialize_62466(netid, p: common.PacketReader):
        print('SN_SortieMasterPickPhaseEnd {')
        print('}')

    def serialize_62467(netid, p: common.PacketReader):
        print('SN_SortieMasterPickPhaseStepStart {')
        print('    timeSec=%d' % p.read_i32())

        print('    alliesTeamUserIds=[')
        count = p.read_u16()
        while count > 0:
            print('     %d,' % p.read_i32())
            count -= 1
        print('    ]')

        print('    enemiesTeamUserIds=[')
        count = p.read_u16()
        while count > 0:
            print('     %d,' % p.read_i32())
            count -= 1
        print('    ]')

        print('}')

    def serialize_62468(netid, p: common.PacketReader):
        print('SN_SortieMasterPickPhaseStep {')
        print('    isRandomPick=%d' % p.read_u8())
        print('}')

    def serialize_62469(netid, p: common.PacketReader):
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

    def serialize_62472(netid, p: common.PacketReader):
        print('SN_Unknown_62472 {')
        print('    var=%d' % p.read_u8())
        print('}')

    def serialize_62474(netid, p: common.PacketReader):
        print('SN_NotifyIngameSkillPoint {')
        print('    userID=0x%08x' % p.read_u32())
        print('    skillPoint=%d' % p.read_i32())
        print('}')

    def serialize_62485(netid, p: common.PacketReader):
        print('SN_PveComradeInfo {')
        print('    availableComradeCount=%d' % p.read_i32())
        print('    maxComradeCount=%d' % p.read_i32())
        print('}')

    def serialize_62500(netid, p: common.PacketReader):
        print('SN_ClientSettings {')
        print('    settingType=%d' % p.read_u8())
        data_len = p.read_u16()
        print('    dataLen=%d' % data_len)
        data = p.read_raw(data_len)
        if data_len > 4:
            data = zlib.decompress(data)
        print('    decompressed=[%s]' % data.decode('utf-8'))
        print('}')

    def serialize_62525(netid, p: common.PacketReader):
        print('SN_AccountEquipmentList {')
        print('    supportKitDocIndex=%d' % p.read_i32())
        print('}')

    def serialize_62545(netid, p: common.PacketReader):
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

    def serialize_62575(netid, p: common.PacketReader):
        print('SN_InitScoreBoard {')
        
        count = p.read_u16()
        print('    userInfos(%d)=[' % count)
        while count > 0:
            print('    {')
            print('      usn=%d' % p.read_i32())
            print('      nickname="%s"' % p.read_wstr())
            print('      teamType=%d' % p.read_i32())
            print('      mainCreatureIndex=%d' % p.read_i32())
            print('      subCreatureIndex=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('}')

    def serialize_62576(netid, p: common.PacketReader):
        print('SN_InitIngameModeInfo {')
        print('    transformationVotingPlayerCoolTimeByVotingFail=%d' % p.read_i32())
        print('    transformationVotingTeamCoolTimeByTransformationEnd=%d' % p.read_i32())
        print('    playerCoolTimeByTransformationEnd=%d' % p.read_i32())
        print('    currentTransformationVotingPlayerCoolTimeByVotingFail=%d' % p.read_i32())
        print('    currentTransformationVotingTeamCoolTimeByTransformationEnd=%d' % p.read_i32())
        print('    currentPlayerCoolTimeByTransformationEnd=%d' % p.read_i32())
        print('    chPropertyResetCoolTime=%d' % p.read_i32())
        print('    transformationPieceCount=%d' % p.read_u8())

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % (p.read_i32())
            count -= 1

        print('    titanDocIndexes=[%s]' % s)
        print('    nextTitanIndex=%d' % p.read_u8())
        print('    listExceptionStat=[')

        count = p.read_u16()
        while count > 0:
            print('    {')
            print('      statType=%d' % p.read_i32())
            print('      min=%g' % p.read_f32())
            print('      max=%g' % p.read_f32())
            print('      incRatioMin=%g' % p.read_f32())
            print('      incRatioMax=%g' % p.read_f32())
            print('    },')
            count -= 1

        print('    ]')

        print('}')

    def serialize_62577(netid, p: common.PacketReader):
        print('SN_ActionChangeLevelEvent {')

        count = p.read_u16()
        print('    targetIDs(%d)=[' % count)
        while count > 0:
            print('        %d' % p.read_i32())
            count -= 1
        print('    ]')
        print('    actionID=%d' % p.read_i32())
        print('    serverTime=%d' % p.read_i64())
        print('}')

    def serialize_62047(netid, p: common.PacketReader):
        print('SA_VersionInfo {')
        n = p.read_u16()
        print('    m_VersionName_len=%d' % n)
        m_VersionName = p.read_raw(n * 2).decode('utf-16-le', errors='replace')
        print('    m_VersionName=%s' % m_VersionName)
        print('}')

    def serialize_62121(netid, p: common.PacketReader):
        print('SA_CheckDupNickname {')
        print('    result=%d' % p.read_u32())
        n = p.read_u16()
        print('    reqNickname_len=%d' % n)
        s = p.read_raw(n * 2).decode('utf-16-le') if n else ''
        print('    reqNickname=%s' % s)
        print('}')

    def serialize_62406(netid, p: common.PacketReader):
        print('SA_WhisperSend {')
        print('    retval=%d' % p.read_u32())
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        n = p.read_u16()
        print('    message_len=%d' % n)
        print('    message=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')

    def serialize_62407(netid, p: common.PacketReader):
        print('SN_WhisperReceive {')
        n = p.read_u16()
        print('    nickname_len=%d' % n)
        print('    nickname=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('    staffType=%d' % p.read_u8())
        n = p.read_u16()
        print('    message_len=%d' % n)
        print('    message=%s' % p.read_raw(n * 2).decode('utf-16-le', errors='replace'))
        print('}')
    def serialize_62003(netid, p: common.PacketReader):
        print('SA_UserloginResult {')
        print('    result=%d' % p.read_i32())
        print('}')
    def serialize_62004(netid, p: common.PacketReader):
        print('SA_UserloginResult2 {')
        print('    result=%d' % p.read_i32())
        print('}')
    def serialize_62073(netid, p: common.PacketReader):
        print('SA_LoadingComplete {}')
    def serialize_62076(netid, p: common.PacketReader):
        print('SN_GameStart {}')
    def serialize_62122(netid, p: common.PacketReader):
        print('SA_SetLeader {')
        print('    result=%d' % p.read_i32())
        print('    leaderID=0x%08x' % p.read_u32())
        print('    skinIndex=%d' % p.read_u32())
        print('}')


    def serialize_62009(netid, p: common.PacketReader):
        print('SN_TgchatServerInfo {')
        print('    host=%s' % p.read_str())
        print('    port=%d' % p.read_u16())
        print('    gameID=%d' % p.read_u32())
        print('    serverID=%d' % p.read_u32())
        print('    userID=0x%08x' % p.read_u32())
        print('    gamename=%s' % p.read_wstr())
        print('    chatname=%s' % p.read_wstr())
        print('    playncname=%s' % p.read_wstr())
        n = p.read_u16()
        print('    signature_count=%d' % n)
        for _ in range(n):
            print('    signature[] %02x' % p.read_u8())
        print('    serverType=%d' % p.read_u8())
        print('}')

    def serialize_62011(netid, p: common.PacketReader):
        print('SN_DoConnectChannelServer {')
        n = p.read_u16()
        print('    addresses_count=%d' % n)
        for _ in range(n):
            ip = p.read_u32()
            port = p.read_u16()
            isp_len = p.read_u16()
            print('    addresses[] ip=%d port=%d isp_len=%d' % (ip, port, isp_len))
            isp = p.read_raw(isp_len * 2)
            print('    addresses[] isp=%s' % isp.decode('utf-16-le', errors='replace'))
        nick_len = p.read_u16()
        print('    nickname_len=%d' % nick_len)
        nick = p.read_raw(nick_len * 2)
        print('    nickname=%s' % nick.decode('utf-16-le', errors='replace'))
        print('    instantKey=%d' % p.read_u32())
        print('    reasonCode=%d' % p.read_u32())
        print('}')
