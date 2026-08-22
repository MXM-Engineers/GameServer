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
        print('    playerID=%d' % p.read_u32())
        print('    nDisplayTitleIndex=%d' % p.read_u32())
        print('    nStatTitleIndex=%d' % p.read_u32())
        print('}')
    def serialize_62012(netid, p: common.PacketReader):
        print('SN_DoConnectSpectateServer {')
        print('    dwPort=%d' % p.read_u16())
        ip = p.read_raw(4)
        print('    dwIP=%d.%d.%d.%d' % (ip[0], ip[1], ip[2], ip[3]))
        print('    idcHash=%d' % p.read_u32())
        print('    instantKey=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
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
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('}')
    def serialize_62023(netid, p: common.PacketReader):
        print('SA_SellShopItem {')
        print('    Result=%d' % p.read_u32())
        print('    sellAmount=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_i64())
        print('}')
    def serialize_62024(netid, p: common.PacketReader):
        print('SN_CreateGroundItem {} @9900ee')
    def serialize_62027(netid, p: common.PacketReader):
        print('SN_GameModifyActor {')
        print('    oldID=%d' % p.read_u32())
        print('    newID=%d' % p.read_u32())
        print('    nType=%d' % p.read_u32())
        print('    nIDX=%d' % p.read_u32())
        print('    dwLocalID=%d' % p.read_u32())
        print('    SpawnType=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('    ownerID=%d' % p.read_u32())
        print('    bDirectionToNearPC=%d' % p.read_u32())
        print('    nAIWanderDistOverride=%d' % p.read_u32())
        print('    tagID=%d' % p.read_u32())
        print('    faction=%d' % p.read_u32())
        print('    classType=%d' % p.read_u32())
        print('    skinIndex=%d' % p.read_u32())
        print('    seed=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u8())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u16())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u32())
        print('}')
    def serialize_62037(netid, p: common.PacketReader):
        print('SN_SwitchOnToggleSkill {')
        print('    entity=%d' % p.read_u32())
        print('    ret=%d' % p.read_u32())
        print('    skillIndex=%d' % p.read_u32())
        print('    param1=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
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
        print('    characterID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    curActionID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
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
        print('    characterID=%d' % p.read_i32())
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
        print('    durationTime=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u8())
        print('    unk_13=%d' % p.read_u8())
        print('    unk_14=%d' % p.read_u32())
        print('    unk_15=%d' % p.read_u32())
        print('}')
    def serialize_62065(netid, p: common.PacketReader):
        print('SA_BuyShopItem {')
        print('    Result=%d' % p.read_u32())
        print('    m_IDX=%d' % p.read_u32())
        print('}')
    def serialize_62066(netid, p: common.PacketReader):
        print('SA_BuyCshopItem {')
        print('    result=%d' % p.read_u32())
        print('    itemIndex=%d' % p.read_u32())
        print('}')
    def serialize_62067(netid, p: common.PacketReader):
        print('SA_GiftFriendsAvailable {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62068(netid, p: common.PacketReader):
        print('SA_GiftFriendCharsAndSkins {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62069(netid, p: common.PacketReader):
        print('SA_BuyGift {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62070(netid, p: common.PacketReader):
        print('SN_PlayerSkillUpdate {')
        print('    characterID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u8())
        print('    unk_7=%d' % p.read_u8())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62079(netid, p: common.PacketReader):
        print('SN_PvpResult {')
        print('    gameEndReason=%d' % p.read_u32())
        print('    m_playTime=%d' % p.read_u32())
        print('    tierGameResult=%d' % p.read_u32())
        print('    tierTypeResult=%d' % p.read_u32())
        print('    curTierPoint=%d' % p.read_u32())
        print('    deltaTierPoint=%d' % p.read_u32())
        print('    deltaGuildRp=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u8())
        print('    unk_13=%d' % p.read_u16())
        print('    unk_14=%d' % p.read_u16())
        print('    unk_15=%d' % p.read_u16())
        print('    unk_16=%d' % p.read_u32())
        print('    unk_17=%d' % p.read_u32())
        print('    unk_18=%d' % p.read_u32())
        print('    unk_19=%d' % p.read_u8())
        print('    unk_20=%d' % p.read_u8())
        print('    unk_21=%d' % p.read_u16())
        print('    unk_22=%d' % p.read_u16())
        print('    unk_23=%d' % p.read_u16())
        print('    unk_24=%d' % p.read_u32())
        print('}')
    def serialize_62080(netid, p: common.PacketReader):
        print('SN_PvpResultScoreDeathmatch {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u16())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u8())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u16())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u32())
        print('    field_10=%d' % p.read_u8())
        print('    field_11=%d' % p.read_u8())
        print('    field_12=%d' % p.read_u8())
        print('    field_13=%d' % p.read_u8())
        print('    field_14=%d' % p.read_u8())
        print('    field_15=%d' % p.read_u32())
        print('    field_16=%d' % p.read_u32())
        print('    field_17=%d' % p.read_u32())
        print('    field_18=%d' % p.read_u32())
        print('    field_19=%d' % p.read_u32())
        print('    field_20=%d' % p.read_u32())
        print('    field_21=%d' % p.read_u32())
        print('    field_22=%d' % p.read_u32())
        print('}')
    def serialize_62081(netid, p: common.PacketReader):
        print('SN_PvpResultScoreOccupy {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u16())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u8())
        print('    field_6=%d' % p.read_u16())
        print('    field_7=%d' % p.read_u32())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u8())
        print('    field_10=%d' % p.read_u8())
        print('    field_11=%d' % p.read_u8())
        print('    field_12=%d' % p.read_u8())
        print('    field_13=%d' % p.read_u8())
        print('    field_14=%d' % p.read_u32())
        print('    field_15=%d' % p.read_u32())
        print('    field_16=%d' % p.read_u32())
        print('    field_17=%d' % p.read_u32())
        print('    field_18=%d' % p.read_u32())
        print('    field_19=%d' % p.read_u32())
        print('    field_20=%d' % p.read_u32())
        print('    field_21=%d' % p.read_u32())
        print('    field_22=%d' % p.read_u8())
        print('    field_23=%d' % p.read_u16())
        print('}')
    def serialize_62082(netid, p: common.PacketReader):
        print('SN_PvpResultScoreGot {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u16())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u16())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u8())
        print('    field_8=%d' % p.read_u8())
        print('    field_9=%d' % p.read_u32())
        print('    field_10=%d' % p.read_u16())
        print('    field_11=%d' % p.read_u8())
        print('    field_12=%d' % p.read_u8())
        print('    field_13=%d' % p.read_u16())
        print('    field_14=%d' % p.read_u8())
        print('    field_15=%d' % p.read_u8())
        print('    field_16=%d' % p.read_u16())
        print('    field_17=%d' % p.read_u32())
        print('    field_18=%d' % p.read_u32())
        print('    field_19=%d' % p.read_u8())
        print('    field_20=%d' % p.read_u8())
        print('    field_21=%d' % p.read_u8())
        print('    field_22=%d' % p.read_u8())
        print('    field_23=%d' % p.read_u8())
        print('    field_24=%d' % p.read_u32())
        print('    field_25=%d' % p.read_u32())
        print('    field_26=%d' % p.read_u32())
        print('    field_27=%d' % p.read_u32())
        print('    field_28=%d' % p.read_u32())
        print('    field_29=%d' % p.read_u32())
        print('    field_30=%d' % p.read_u32())
        print('    field_31=%d' % p.read_u32())
        print('    field_32=%d' % p.read_u32())
        print('    field_33=%d' % p.read_u32())
        print('    field_34=%d' % p.read_u8())
        print('    field_35=%d' % p.read_u8())
        print('    field_36=%d' % p.read_u8())
        print('    field_37=%d' % p.read_u8())
        print('    field_38=%d' % p.read_u8())
        print('    field_39=%d' % p.read_u8())
        print('    field_40=%d' % p.read_u8())
        print('    field_41=%d' % p.read_u16())
        print('    field_42=%d' % p.read_u32())
        print('    field_43=%d' % p.read_u32())
        print('    field_44=%d' % p.read_u8())
        print('    field_45=%d' % p.read_u8())
        print('    field_46=%d' % p.read_u8())
        print('    field_47=%d' % p.read_u8())
        print('    field_48=%d' % p.read_u8())
        print('    field_49=%d' % p.read_u32())
        print('    field_50=%d' % p.read_u32())
        print('    field_51=%d' % p.read_u32())
        print('    field_52=%d' % p.read_u32())
        print('    field_53=%d' % p.read_u32())
        print('    field_54=%d' % p.read_u32())
        print('    field_55=%d' % p.read_u32())
        print('    field_56=%d' % p.read_u32())
        print('    field_57=%d' % p.read_u32())
        print('    field_58=%d' % p.read_u32())
        print('    field_59=%d' % p.read_u8())
        print('}')
    def serialize_62083(netid, p: common.PacketReader):
        print('SN_PvpResultScoreSport {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u16())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u8())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u8())
        print('    field_10=%d' % p.read_u8())
        print('    field_11=%d' % p.read_u32())
        print('}')
    def serialize_62085(netid, p: common.PacketReader):
        print('SA_GetGroundItem {')
        print('    playerID=%d' % p.read_u32())
        print('    itemID=%d' % p.read_u32())
        print('    itemISN=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62086(netid, p: common.PacketReader):
        print('SN_StageClearResult {')
        print('    m_playerID=%d' % p.read_u32())
        print('    isRandomRestart=%d' % p.read_u32())
        print('    m_StageIndex=%d' % p.read_u32())
        print('    m_KillCount=%d' % p.read_u16())
        print('    m_DeathCount=%d' % p.read_u8())
        print('    m_PlayTime=%d' % p.read_u32())
        print('    m_ClearRank=%d' % p.read_u32())
        print('    m_CombatScore=%d' % p.read_u32())
        print('    m_baseRon=%d' % p.read_u32())
        print('    m_baseExp=%d' % p.read_u8())
        print('    m_attackScore=%d' % p.read_u32())
        print('    m_defenseScore=%d' % p.read_u32())
        print('    m_timeScore=%d' % p.read_u32())
        print('    m_deathTimeScore=%d' % p.read_u32())
        print('    m_totalScore=%d' % p.read_u32())
        print('    m_ranking=%d' % p.read_u32())
        print('    m_technicGrade=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u8())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u8())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u16())
        print('    unk_12=%d' % p.read_u16())
        print('}')
    def serialize_62087(netid, p: common.PacketReader):
        print('SN_DefenceClearResult {')
        print('    m_playerID=%d' % p.read_u32())
        print('    m_StageIndex=%d' % p.read_u32())
        print('    m_ClearStepIndex=%d' % p.read_u32())
        print('    m_ClearTime=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u16())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u16())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u16())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('    unk_15=%d' % p.read_u32())
        print('    unk_16=%d' % p.read_u32())
        print('    unk_17=%d' % p.read_u32())
        print('    unk_18=%d' % p.read_u8())
        print('    unk_19=%d' % p.read_u16())
        print('    unk_20=%d' % p.read_u16())
        print('    unk_21=%d' % p.read_u16())
        print('    unk_22=%d' % p.read_u32())
        print('    unk_23=%d' % p.read_u32())
        print('    unk_24=%d' % p.read_u32())
        print('    unk_25=%d' % p.read_u32())
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
        print('    m_SendStaffType=%d' % p.read_u16())
        print('    m_DurationMS=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62093(netid, p: common.PacketReader):
        print('SN_GetGroundItem {')
        print('    dwPlayerID=%d' % p.read_u32())
        print('    dwItemIndex=%d' % p.read_u32())
        print('    oid=%d' % p.read_u32())
        print('}')
    def serialize_62094(netid, p: common.PacketReader):
        print('SN_DestroyGroundItem {')
        print('    itemID=%d' % p.read_u32())
        print('}')
    def serialize_62095(netid, p: common.PacketReader):
        print('SN_GroundItemsSnapshot {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u8())
        print('    field_8=%d' % p.read_u8())
        print('    field_9=%d' % p.read_u8())
        print('    field_10=%d' % p.read_u32())
        print('    field_11=%d' % p.read_u32())
        print('    field_12=%d' % p.read_u32())
        print('}')
    def serialize_62096(netid, p: common.PacketReader):
        print('SA_WorldSetInfoList {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u32())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u32())
        print('}')
    def serialize_62099(netid, p: common.PacketReader):
        print('SN_AntihackAuth {')
        print('    field_0=%d' % p.read_u8())
        print('}')
    def serialize_62103(netid, p: common.PacketReader):
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

    def serialize_62104(netid, p: common.PacketReader):
        print('SN_MissionUpdate {')
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('}')
    def serialize_62105(netid, p: common.PacketReader):
        print('SA_DailyMissionChange {')
        print('    result=%d' % p.read_u32())
        print('    step=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('}')
    def serialize_62108(netid, p: common.PacketReader):
        print('SN_ChangedUserGradeInfo {')
        print('    activityPoint=%d' % p.read_u8())
        print('    activityRewardedState=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62110(netid, p: common.PacketReader):
        print('SN_StageRank {')
        print('    rank=%d' % p.read_i32())
        print('    timeScoreRank=%d' % p.read_i32())
        print('}')

    def serialize_62111(netid, p: common.PacketReader):
        print('SN_GetPublicGroundItem {')
        print('    playerID=%d' % p.read_u32())
        print('    itemID=%d' % p.read_u32())
        print('}')
    def serialize_62114(netid, p: common.PacketReader):
        print('SN_UpdateGamePlayerTagCooltime {')
        print('    playerID=%d' % p.read_u32())
        print('    tagCooltimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62115(netid, p: common.PacketReader):
        print('SN_UpdateCanCastSkillSlotUg {')
        print('    playerID=%d' % p.read_u32())
        print('    canCastSkillSlotUG=%d' % p.read_u8())
        print('}')
    def serialize_62116(netid, p: common.PacketReader):
        print('SN_ItemAcquisition {')
        print('    nReason=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62117(netid, p: common.PacketReader):
        print('SN_ItemUpdate {')
        print('    context=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_i64())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u8())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u8())
        print('    unk_13=%d' % p.read_u32())
        print('}')
    def serialize_62118(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62118 {')
        print('    field_0=%d' % p.read_u16())
        print('}')
    def serialize_62119(netid, p: common.PacketReader):
        print('SN_NicknameNeeded {')
        print('}')
    def serialize_62120(netid, p: common.PacketReader):
        print('SA_NicknameSet {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62130(netid, p: common.PacketReader):
        print('SN_ExtraCharacters {')
        print('    isSelected=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('    unk_15=%d' % p.read_u32())
        print('    unk_16=%d' % p.read_u8())
        print('    unk_17=%d' % p.read_u32())
        print('    unk_18=%d' % p.read_u8())
        print('    unk_19=%d' % p.read_u32())
        print('    unk_20=%d' % p.read_u32())
        print('    unk_21=%d' % p.read_u32())
        print('    unk_22=%d' % p.read_u32())
        print('    unk_23=%d' % p.read_u8())
        print('    unk_24=%d' % p.read_i64())
        print('    unk_25=%d' % p.read_u16())
        print('    unk_26=%d' % p.read_u8())
        print('    unk_27=%d' % p.read_u32())
        print('    unk_28=%d' % p.read_u8())
        print('    unk_29=%d' % p.read_u32())
        print('    unk_30=%d' % p.read_u8())
        print('    unk_31=%d' % p.read_u32())
        print('    unk_32=%d' % p.read_u32())
        print('    unk_33=%d' % p.read_u32())
        print('    unk_34=%d' % p.read_u32())
        print('    unk_35=%d' % p.read_u8())
        print('    unk_36=%d' % p.read_u8())
        print('    unk_37=%d' % p.read_u32())
        print('    unk_38=%d' % p.read_u32())
        print('    unk_39=%d' % p.read_u8())
        print('    unk_40=%d' % p.read_u8())
        print('    unk_41=%d' % p.read_u16())
        print('    unk_42=%d' % p.read_u32())
        print('    unk_43=%d' % p.read_u32())
        print('    unk_44=%d' % p.read_u8())
        print('    unk_45=%d' % p.read_u16())
        print('    unk_46=%d' % p.read_u16())
        print('    unk_47=%d' % p.read_u32())
        print('    unk_48=%d' % p.read_u32())
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
        print('    characterID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('}')
    def serialize_62133(netid, p: common.PacketReader):
        print('SA_SkillUpgradeIngame {')
        print('    result=%d' % p.read_u32())
        print('    mainID=%d' % p.read_u32())
        print('    subID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u8())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u16())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u32())
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
        print('    itemID=%d' % p.read_u32())
        print('}')
    def serialize_62137(netid, p: common.PacketReader):
        print('SA_ItemUnequip {')
        print('    result=%d' % p.read_u32())
        print('    itemID=%d' % p.read_u32())
        print('}')
    def serialize_62138(netid, p: common.PacketReader):
        print('SA_SelectAccountEquipment {')
        print('    result=%d' % p.read_u32())
        print('    etcType=%d' % p.read_u8())
        print('    itemDocIndex=%d' % p.read_u32())
        print('}')
    def serialize_62139(netid, p: common.PacketReader):
        print('SA_WeaponUnlock {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62140(netid, p: common.PacketReader):
        print('SA_WeaponEquip {')
        print('    result=%d' % p.read_u32())
        print('    characterID=%d' % p.read_u32())
        print('    weaponType=%d' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('}')
    def serialize_62141(netid, p: common.PacketReader):
        print('SA_GearEquip {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('    gearItemID=%d' % p.read_u32())
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
        print('    characterID=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('}')
    def serialize_62147(netid, p: common.PacketReader):
        print('SA_MastergearRename {')
        print('    result=%d' % p.read_u32())
        print('    masterGearNo=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62148(netid, p: common.PacketReader):
        print('SA_MastergearRepair {')
        print('    result=%d' % p.read_u32())
        print('    durabillity=%d' % p.read_u32())
        print('}')
    def serialize_62149(netid, p: common.PacketReader):
        print('SA_MastergearAdd {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('}')
    def serialize_62150(netid, p: common.PacketReader):
        print('SA_CharacterUnlock {')
        print('    result=%d' % p.read_u32())
        print('    characterDocIndex=%d' % p.read_u32())
        print('}')
    def serialize_62151(netid, p: common.PacketReader):
        print('SA_SkillUnlock {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('}')
    def serialize_62152(netid, p: common.PacketReader):
        print('SA_SkillSelect {')
        print('    characterID=%d' % p.read_u32())
        print('    result=%d' % p.read_u32())
        print('    dwSkillIndex=%d' % p.read_u32())
        print('    dwSkillSlot=%d' % p.read_u32())
        print('}')
    def serialize_62153(netid, p: common.PacketReader):
        print('SN_SkillSelect {')
        print('    userId=%d' % p.read_u32())
        print('    characterID=%d' % p.read_u32())
        print('    dwSkillIndex=%d' % p.read_u32())
        print('    dwSkillSlot=%d' % p.read_u32())
        print('}')
    def serialize_62154(netid, p: common.PacketReader):
        print('SA_SkillSwap {')
        print('    characterID=%d' % p.read_u32())
        print('    result=%d' % p.read_u32())
        print('    dwSlot1SkillIndex=%d' % p.read_u32())
        print('    dwSlot2SkillIndex=%d' % p.read_u32())
        print('}')
    def serialize_62155(netid, p: common.PacketReader):
        print('SN_SkillSwap {')
        print('    userId=%d' % p.read_u32())
        print('    characterID=%d' % p.read_u32())
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
        print('    voteTimeMS=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('}')
    def serialize_62160(netid, p: common.PacketReader):
        print('SN_RestartVoteFinish {')
        print('    voteResult=%d' % p.read_u8())
        print('    voteFailReason=%d' % p.read_u8())
        print('}')
    def serialize_62161(netid, p: common.PacketReader):
        print('SN_RestartVoteUpdate {')
        print('    field_0=%d' % p.read_u8())
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
        print('    isJoined=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
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
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62181(netid, p: common.PacketReader):
        print('SA_PartyInviteRecommend {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62182(netid, p: common.PacketReader):
        print('SN_PartyInviteResponse {')
        print('    accept=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62183(netid, p: common.PacketReader):
        print('SN_PartyInvite {')
        print('    channelID=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u16())
        print('    gameDefinitionType=%d' % p.read_u32())
        print('    stageRule=%d' % p.read_u32())
        print('    partyID=%d' % p.read_u32())
        print('    teamType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('}')
    def serialize_62184(netid, p: common.PacketReader):
        print('SN_PartyInviteRecommend {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62185(netid, p: common.PacketReader):
        print('SA_PartyInviteRecommendResponse {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62186(netid, p: common.PacketReader):
        print('SN_PartyInviteRecommendResponse {')
        print('    accept=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62187(netid, p: common.PacketReader):
        print('SA_PartyJoin {')
        print('    retval=%d' % p.read_u32())
        print('    partyID=%d' % p.read_u32())
        print('    stageType=%d' % p.read_u32())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefinitionType=%d' % p.read_u32())
        print('    alliesTeamType=%d' % p.read_u32())
        print('    enemiesTeamType=%d' % p.read_u16())
        print('    isSpectator=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u8())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u16())
        print('    unk_15=%d' % p.read_u8())
        print('    unk_16=%d' % p.read_u32())
        print('    unk_17=%d' % p.read_u32())
        print('    unk_18=%d' % p.read_u32())
        print('    unk_19=%d' % p.read_u32())
        print('    unk_20=%d' % p.read_u8())
        print('}')
    def serialize_62188(netid, p: common.PacketReader):
        print('SN_PartyJoin {')
        print('    userID=%d' % p.read_i32())
        print('    name="%s"' % p.read_wstr())
        print('    isBot=%d' % p.read_u8())
        print('    creatureIndex=%d' % p.read_i32())
        print('    isOwner=%d' % p.read_u8())
        print('    gametype=%d' % p.read_i32())
        print('    gameDefinitionType=%d' % p.read_i32())
        print('    teamType=%d' % p.read_i32())
        print('    teamSlotIndex=%d' % p.read_u8())
        print('}')

    def serialize_62189(netid, p: common.PacketReader):
        print('SA_PartyLeave {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62190(netid, p: common.PacketReader):
        print('SN_PartyLeave {')
        print('    userId=%d' % p.read_u32())
        print('    isOwner=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62209(netid, p: common.PacketReader):
        print('SN_MasterPick {')
        print('    userID=%d' % p.read_i32())
        count = p.read_u16()
        print('    characterSelectInfos(%d)=[' % count)
        while count > 0:
            print('    {')
            print('      characterID=%d' % p.read_i32())
            print('      creatureIndex=%d' % p.read_i32())
            print('      skillSlot1=%d' % p.read_i32())
            print('      skillSlot2=%d' % p.read_i32())
            print('    },')
            count -= 1
        print('    ]')

    def serialize_62210(netid, p: common.PacketReader):
        print('SA_MasterUnpick {')
        print('    retval=%d' % p.read_u32())
        print('    characterID=%d' % p.read_u32())
        print('}')
    def serialize_62211(netid, p: common.PacketReader):
        print('SN_MasterUnpick {')
        print('    userId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62212(netid, p: common.PacketReader):
        print('SA_MasterUnpickAll {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62213(netid, p: common.PacketReader):
        print('SN_MasterUnpickAll {')
        print('    userId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62221(netid, p: common.PacketReader):
        print('SA_InventoryExpand {')
        print('    result=%d' % p.read_u32())
        print('    invenTab=%d' % p.read_u32())
        print('    lineCount=%d' % p.read_u32())
        print('}')
    def serialize_62222(netid, p: common.PacketReader):
        print('SN_DespawnMonsters {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62223(netid, p: common.PacketReader):
        print('SN_DespawnMonster {')
        print('    objectID=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_i64())
        print('}')
    def serialize_62228(netid, p: common.PacketReader):
        print('SN_SummaryUpdate {')
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_i64())
        print('}')
    def serialize_62230(netid, p: common.PacketReader):
        print('SN_AddStatus {')
        b = p.read_u8()
        print('    excludedFieldBits=%d' % b)

        if (b & 1) == 0:
            print('    totalRatio=%g' % p.read_f32())
            print('    defeatFrom=%s' % read_Vec3(p))
            print('    defeatTo=%s' % read_Vec3(p))
            print('    defeatDist=%g' % p.read_f32())
            print('    defeatOriginDist=%g' % p.read_f32())
            print('    defeatDurationTime=%g' % p.read_f32())

        print('    statusID=%d' % p.read_i32())
        print('    bEnabled=%d' % p.read_u8())
        print('    targetID=%d' % p.read_i32())
        print('    casterID=%d' % p.read_i32())
        print('    isOverlap=%d' % p.read_u8())
        print('    overlapCount=%d' % p.read_u8())
        print('    customValue=%d' % p.read_u8())
        print('    durationTime=%g' % p.read_f32())
        print('    elapsedTime=%d' % p.read_i32())
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
        print('	damageSeqNum=%d' % p.read_i32())
        print('	remoteID=%d' % p.read_i32())
        print('	defID=%d' % p.read_i32())
        print('	attID=%d' % p.read_i32())
        print('	remoteDocIndex=%d' % p.read_i32())
        print('	remotePos=%s' % read_Vec3(p))
        print('	remoteDir=%s' % read_Vec3(p))
        print('	remoteForceDir=%s' % read_Vec3(p))
        print('	hitPos=%s' % read_Vec3(p))
        print('	hitDir=%s' % read_Vec3(p))
        print('	damageType=%d' % p.read_i32())
        print('	skillDocID=%d' % p.read_i32())
        print('	nRagePoint=%d' % p.read_i32())
        print('	nRelativeElement=%d' % p.read_i32())
        print('	hitNodeName="%s"' % p.read_str())
        print('	damage=%d' % p.read_i32())
        print('	partName="%s"' % p.read_str())
        print('	partDamage=%d' % p.read_i32())
        print('	masterGroupingDamage=%d' % p.read_i32())
        print('	optionalResultOfHit=%d' % p.read_i32())
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
        print('	remoteID=%d' % p.read_i32())
        print('	defID=%d' % p.read_i32())
        print('	penetrationCount=%d' % p.read_i32())
        print('	remotePos=%s' % read_Vec3(p))
        print('}')

    def serialize_62239(netid, p: common.PacketReader):
        print('SA_ChannelChat {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62240(netid, p: common.PacketReader):
        print('SN_ChatChannelJoin {')
        print('    channelType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62241(netid, p: common.PacketReader):
        print('SN_ChatChannelLeave {')
        print('    channelType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62243(netid, p: common.PacketReader):
        print('SN_ChatChannelUserJoin {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62244(netid, p: common.PacketReader):
        print('SN_ChatChannelUserLeave {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62245(netid, p: common.PacketReader):
        print('SN_FriendAdded {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_i64())
        print('}')
    def serialize_62246(netid, p: common.PacketReader):
        print('SA_FriendRemove {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62247(netid, p: common.PacketReader):
        print('SA_CommunityBlockAdd {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62248(netid, p: common.PacketReader):
        print('SA_CommunityBlockRemove {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62249(netid, p: common.PacketReader):
        print('SN_FriendRemoved {')
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62250(netid, p: common.PacketReader):
        print('SA_FriendStateChange {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62251(netid, p: common.PacketReader):
        print('SN_FriendStateChanged {')
        print('    nickname="%s"' % p.read_wstr())
        print('    newState=%d' % p.read_u8())
        print('    isOnLeavePenalty=%d' % p.read_u8())
        print('    stateUpdatedTime=%d' % p.read_i64())
        print('}')

    def serialize_62252(netid, p: common.PacketReader):
        print('SN_FriendNicknameChanged {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62256(netid, p: common.PacketReader):
        print('SN_FriendLeaderChanged {')
        print('    creatureIndex=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62260(netid, p: common.PacketReader):
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
    def serialize_62262(netid, p: common.PacketReader):
        print('SA_FriendRequest {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62263(netid, p: common.PacketReader):
        print('SN_FriendRequested {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('}')
    def serialize_62264(netid, p: common.PacketReader):
        print('SA_FriendRespond {')
        print('    retval=%d' % p.read_u32())
        print('    bAccept=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62265(netid, p: common.PacketReader):
        print('SN_FriendResponded {')
        print('    accept=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62266(netid, p: common.PacketReader):
        print('SA_FriendRequestLeaderInfo {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u8())
        print('    unk_12=%d' % p.read_u16())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('}')
    def serialize_62267(netid, p: common.PacketReader):
        print('SA_RequestLeaderInfo {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u8())
        print('    unk_12=%d' % p.read_u16())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('}')
    def serialize_62268(netid, p: common.PacketReader):
        print('SA_FriendSetComrade {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62269(netid, p: common.PacketReader):
        print('SA_FriendUnsetComrade {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62270(netid, p: common.PacketReader):
        print('SA_FriendSetFavorite {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62271(netid, p: common.PacketReader):
        print('SA_FriendUnsetFavorite {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62272(netid, p: common.PacketReader):
        print('SN_FriendPartycreation {')
        print('    stageIndex=%d' % p.read_u16())
        print('    gameType=%d' % p.read_u32())
        print('    gameDefType=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62273(netid, p: common.PacketReader):
        print('SA_RequestMissionReward {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
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
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62279(netid, p: common.PacketReader):
        print('SN_ExpGain {')
        print('    amount=%d' % p.read_u32())
        print('}')
    def serialize_62280(netid, p: common.PacketReader):
        print('SA_GetUserinfo {')
        print('    result=%d' % p.read_u32())
        print('    level=%d' % p.read_u16())
        print('    achievementScore=%d' % p.read_u16())
        print('    tierGrade=%d' % p.read_u32())
        print('    creatureIndex=%d' % p.read_u8())
        print('    power=%d' % p.read_u16())
        print('    viability=%d' % p.read_u32())
        print('    skinId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u8())
        print('    unk_8=%d' % p.read_u8())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u8())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('    unk_15=%d' % p.read_u8())
        print('    unk_16=%d' % p.read_i64())
        print('    unk_17=%d' % p.read_u16())
        print('    unk_18=%d' % p.read_u8())
        print('    unk_19=%d' % p.read_u32())
        print('    unk_20=%d' % p.read_u8())
        print('    unk_21=%d' % p.read_u32())
        print('    unk_22=%d' % p.read_u8())
        print('}')
    def serialize_62281(netid, p: common.PacketReader):
        print('SA_JukeboxEnqueue {')
        print('    result=%d' % p.read_u32())
        print('}')
    def serialize_62286(netid, p: common.PacketReader):
        print('SN_OpenEventMasters {')
        print('    field_0=%d' % p.read_u32())
        print('}')
    def serialize_62287(netid, p: common.PacketReader):
        print('SA_CreateGuild {')
        print('    result=%d' % p.read_u32())
        print('    masterTopPvpTierGrade=%d' % p.read_u16())
        print('    masterTopPvpTierPoint=%d' % p.read_u16())
        print('    masterAchievementScore=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62293(netid, p: common.PacketReader):
        print('SA_JoinGuild {')
        print('    result=%d' % p.read_u32())
        print('    guildJoinType=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62294(netid, p: common.PacketReader):
        print('SA_GetGuildjoinrequestlist {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_u16())
        print('}')
    def serialize_62295(netid, p: common.PacketReader):
        print('SA_RespondGuildjoinrequest {')
        print('    result=%d' % p.read_u32())
        print('    isApproval=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u8())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u16())
        print('    unk_12=%d' % p.read_i64())
        print('}')
    def serialize_62296(netid, p: common.PacketReader):
        print('SA_InviteGuildmember {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62297(netid, p: common.PacketReader):
        print('SA_GetGuildinvitationlist {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62298(netid, p: common.PacketReader):
        print('SA_RespondGuildinvitation {')
        print('    result=%d' % p.read_u32())
        print('    accept=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62301(netid, p: common.PacketReader):
        print('SA_GetGuildemblemlist {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62303(netid, p: common.PacketReader):
        print('SA_GetGuildpublicprofile {')
        print('    result=%d' % p.read_u32())
        print('    emblemIndex=%d' % p.read_u16())
        print('    guildLvl=%d' % p.read_u16())
        print('    memberMax=%d' % p.read_u32())
        print('    createdDate=%d' % p.read_u8())
        print('    dissolutionDate=%d' % p.read_u8())
        print('    joinType=%d' % p.read_u16())
        print('    memberNum=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62304(netid, p: common.PacketReader):
        print('SA_GetGuildpublicprofileFromUser {')
        print('    result=%d' % p.read_u32())
        print('    emblemIndex=%d' % p.read_u16())
        print('    guildLvl=%d' % p.read_u16())
        print('    memberMax=%d' % p.read_u32())
        print('    createdDate=%d' % p.read_u8())
        print('    dissolutionDate=%d' % p.read_u8())
        print('    joinType=%d' % p.read_u16())
        print('    memberNum=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u16())
        print('}')
    def serialize_62305(netid, p: common.PacketReader):
        print('SA_EditGuildnotice {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62306(netid, p: common.PacketReader):
        print('SA_EditGuildintro {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62307(netid, p: common.PacketReader):
        print('SA_EditGuildtag {')
        print('    result=%d' % p.read_u32())
        print('    guildFundCost=%d' % p.read_u16())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
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
        print('    oldMembershipId=%d' % p.read_u16())
        print('    newMembershipId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62312(netid, p: common.PacketReader):
        print('SA_SetGuildmaster {')
        print('    result=%d' % p.read_u32())
        print('    myMembershipId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62313(netid, p: common.PacketReader):
        print('SA_CreateGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62314(netid, p: common.PacketReader):
        print('SA_DeleteGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    regularMembershipId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62315(netid, p: common.PacketReader):
        print('SA_EditGuildmembership {')
        print('    result=%d' % p.read_u32())
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_i64())
        print('    unk_3=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('}')
    def serialize_62325(netid, p: common.PacketReader):
        print('SA_GetGuildrankrewardinfo {')
        print('    result=%d' % p.read_u32())
        print('    rankingType=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('}')
    def serialize_62326(netid, p: common.PacketReader):
        print('SA_GetRecommendedguildlist {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62327(netid, p: common.PacketReader):
        print('SA_SearchGuild {')
        print('    result=%d' % p.read_u32())
        print('    pageNo=%d' % p.read_u32())
        print('    rowsPerPage=%d' % p.read_u8())
        print('    maxPage=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62328(netid, p: common.PacketReader):
        print('SA_GetPlayingGuildpvplist {')
        print('    result=%d' % p.read_u32())
        print('    pageNo=%d' % p.read_u16())
        print('    rowsPerPage=%d' % p.read_u8())
        print('    maxPageNo=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u16())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u16())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62333(netid, p: common.PacketReader):
        print('SN_GuildJoinapproval {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62334(netid, p: common.PacketReader):
        print('SN_GuildtagChanged {')
        print('    guildFundCost=%d' % p.read_u16())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62335(netid, p: common.PacketReader):
        print('SN_GuildNotice {')
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62336(netid, p: common.PacketReader):
        print('SN_GuildmemberJoin {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u8())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u16())
        print('    unk_12=%d' % p.read_i64())
        print('}')
    def serialize_62337(netid, p: common.PacketReader):
        print('SN_GuildmemberQuit {')
        print('    isKickedOut=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u8())
        print('}')
    def serialize_62339(netid, p: common.PacketReader):
        print('SN_GuildmemberChgnickname {')
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62340(netid, p: common.PacketReader):
        print('SN_GuildmemberChgleaderclass {')
        print('    newLeaderClass=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62341(netid, p: common.PacketReader):
        print('SN_GuildmembershipAdded {')
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62342(netid, p: common.PacketReader):
        print('SN_GuildmembershipRemoved {')
        print('    membershipId=%d' % p.read_u32())
        print('}')
    def serialize_62343(netid, p: common.PacketReader):
        print('SN_GuildmembershipModified {')
        print('    membershipId=%d' % p.read_u32())
        print('    iconIndex=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62344(netid, p: common.PacketReader):
        print('SN_GuildmemberChgclass {')
        print('    oldMembershipId=%d' % p.read_u32())
        print('    newMembershipId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62345(netid, p: common.PacketReader):
        print('SN_GuildmasterDelegation {')
        print('    formerMasterMembershipId=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62346(netid, p: common.PacketReader):
        print('SN_GuildfundAdded {')
        print('    guildFundGain=%d' % p.read_u16())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
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
        print('    skillType=%d' % p.read_u16())
        print('    skillLvl=%d' % p.read_u8())
        print('    timeLimitHour=%d' % p.read_u8())
        print('    expiryDate=%d' % p.read_u16())
        print('    guildFundCost=%d' % p.read_i64())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62351(netid, p: common.PacketReader):
        print('SN_GuildskillUpgraded {')
        print('    skillType=%d' % p.read_u16())
        print('    skillLvl=%d' % p.read_u8())
        print('    timeLimitHour=%d' % p.read_u8())
        print('    expiryDate=%d' % p.read_u16())
        print('    guildFundCost=%d' % p.read_i64())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62352(netid, p: common.PacketReader):
        print('SN_GuildskillExtended {')
        print('    skillType=%d' % p.read_u16())
        print('    skillLvl=%d' % p.read_u8())
        print('    skillExtensionCount=%d' % p.read_u8())
        print('    extensionTimeHour=%d' % p.read_u16())
        print('    skillExpiryDate=%d' % p.read_u16())
        print('    guildFundCost=%d' % p.read_i64())
        print('    totalGuildFund=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62353(netid, p: common.PacketReader):
        print('SN_GuildconstraintChange {')
        print('    changeDate=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('}')
    def serialize_62354(netid, p: common.PacketReader):
        print('SN_GuildmissionCompleted {')
        print('    missionIndex=%d' % p.read_u8())
        print('    rewardedGuildFund=%d' % p.read_u32())
        print('    totalGuildFund=%d' % p.read_u16())
        print('    contributedGuildFund=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('}')
    def serialize_62355(netid, p: common.PacketReader):
        print('SN_GuildpvpResult {')
        print('    guildPvpResultType=%d' % p.read_u8())
        print('    rp=%d' % p.read_u32())
        print('    win=%d' % p.read_u16())
        print('    draw=%d' % p.read_u16())
        print('    lose=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62360(netid, p: common.PacketReader):
        print('SN_PlayerSyncMove {')
        print('    entityID=%d' % p.read_i32())
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
        print('    entityID=%d' % p.read_i32())
        print('    upperDir_x=%g' % p.read_f32())
        print('    upperDir_y=%g' % p.read_f32())
        print('    nRotate=%g' % p.read_f32())
        print('}')

    def serialize_62362(netid, p: common.PacketReader):
        print('SN_PlayerBlink {')
        print('    entityID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    nSpeed=%d' % p.read_f32())
        print('    nState=%d' % p.read_u32())
        print('    bGhostBlink=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u8())
        print('}')
    def serialize_62363(netid, p: common.PacketReader):
        print('SN_InvalidRemoteLog {')
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62364(netid, p: common.PacketReader):
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

    def serialize_62365(netid, p: common.PacketReader):
        print('SN_WeaponState {')
        print('    ownerID=%d' % p.read_i32())
        print('    weaponID=%d' % p.read_i32())
        print('    state=%d' % p.read_i32())
        print('    chargeLevel=%d' % p.read_u8())
        print('    firingCombo=%g' % p.read_u8())
        print('    result=%g' % p.read_i32())
        print('}')

    def serialize_62366(netid, p: common.PacketReader):
        print('SN_SyncActionMove {')
        print('    entityID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62367(netid, p: common.PacketReader):
        print('SN_SyncStanceType {')
        print('    entityID=%d' % p.read_u32())
        print('    StanceType=%d' % p.read_u32())
        print('    durationtimeSec=%d' % p.read_f32())
        print('}')
    def serialize_62368(netid, p: common.PacketReader):
        print('SN_AiSyncMoveMotion {')
        print('    entityID=%d' % p.read_u32())
        print('    dwMotionType=%d' % p.read_u32())
        print('    dwMotionTypeKey=%d' % p.read_u32())
        print('    dwAIMoveType=%d' % p.read_u32())
        print('    dwAIMoveVariationType=%d' % p.read_u32())
        print('    isAttack=%d' % p.read_u32())
        print('    targetId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62369(netid, p: common.PacketReader):
        print('SN_AiSyncBehaviorMotion {')
        print('    entityID=%d' % p.read_u32())
        print('    dwMotionType=%d' % p.read_u32())
        print('    stance=%d' % p.read_u8())
        print('    isAttack=%d' % p.read_u32())
        print('    targetId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('}')
    def serialize_62370(netid, p: common.PacketReader):
        print('SN_AiSyncCooltime {')
        print('    entityID=%d' % p.read_u32())
        print('    fCooltime=%d' % p.read_u16())
        print('    nodeID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('}')
    def serialize_62371(netid, p: common.PacketReader):
        print('SN_AiSyncSpeedRatio {')
        print('    entityID=%d' % p.read_u32())
        print('    fRatio=%d' % p.read_f32())
        print('}')
    def serialize_62372(netid, p: common.PacketReader):
        print('SN_AiSyncRotateRatio {')
        print('    entityID=%d' % p.read_u32())
        print('    fRatio=%d' % p.read_f32())
        print('}')
    def serialize_62373(netid, p: common.PacketReader):
        print('SN_AiSyncSceneroot {')
        print('    entityID=%d' % p.read_u32())
        print('    wantedYaw=%d' % p.read_f32())
        print('}')
    def serialize_62374(netid, p: common.PacketReader):
        print('SN_AiSyncTarpos {')
        print('    entityID=%d' % p.read_u32())
        print('    dwSentTime=%d' % p.read_u32())
        print('    fSpeed=%d' % p.read_f32())
        print('    fFrameRate=%d' % p.read_f32())
        print('    fUpForce=%d' % p.read_f32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('}')
    def serialize_62375(netid, p: common.PacketReader):
        print('SN_AiSyncTarget {')
        print('    entityID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('}')
    def serialize_62376(netid, p: common.PacketReader):
        print('SN_AiAddCom {')
        print('    entityID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62377(netid, p: common.PacketReader):
        print('SN_AiSyncBodyYaw {')
        print('    entityID=%d' % p.read_u32())
        print('    fradian=%d' % p.read_f32())
        print('}')
    def serialize_62378(netid, p: common.PacketReader):
        print('SN_AiSyncBodyPitch {')
        print('    entityID=%d' % p.read_u32())
        print('    fradian=%d' % p.read_f32())
        print('}')
    def serialize_62379(netid, p: common.PacketReader):
        print('SN_AiSyncPhy {')
        print('    entityID=%d' % p.read_u32())
        print('    fradian=%d' % p.read_f32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62380(netid, p: common.PacketReader):
        print('SN_AiSetTarget {')
        print('    entityID=%d' % p.read_u32())
        print('    targetEntityID=%d' % p.read_u32())
        print('}')
    def serialize_62381(netid, p: common.PacketReader):
        print('SN_AiSyncMonsterSkillTarget {')
        print('    entityID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62382(netid, p: common.PacketReader):
        print('SN_AiSetActionState {')
        b = p.read_u8()
        print('    excludedFieldBits=%d' % b)
        print('    entityID=%d' % p.read_i32())
        if (b & 2) == 0:
            print('    motionType=%d' % p.read_u16())
        if (b & 4) == 0:
            print('    lowerBodyYaw=%d' % p.read_u16())
        if (b & 8) == 0:
            print('    movementSpeed=%d' % p.read_u16())

        print('    pos=%s' % read_Vec3(p))
        print('}')
    def serialize_62383(netid, p: common.PacketReader):
        print('SN_AiSetMovetargetpos {')
        print('    entityID=%d' % p.read_u32())
        print('    movePresetType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62384(netid, p: common.PacketReader):
        print('SN_AiChangeMoveType {')
        print('    entityID=%d' % p.read_u32())
        print('    moveType=%d' % p.read_u32())
        print('}')
    def serialize_62385(netid, p: common.PacketReader):
        print('SN_RemoteSyncCreateFromCreatorId {')
        print('    ParentEntity=%d' % p.read_u32())
        print('    OwnerEntity=%d' % p.read_u32())
        print('    RemoteSeedID=%d' % p.read_u32())
        print('    FireRotationX=%d' % p.read_u32())
        print('    FireRotationY=%d' % p.read_u32())
        print('    FireRotationZ=%d' % p.read_u32())
        print('    Scale=%d' % p.read_u16())
        print('    LifeTime=%d' % p.read_u16())
        print('    HitInvalidTarget=%d' % p.read_u16())
        print('    RemoteCreatorID=%d' % p.read_u32())
        print('    ErrorAngleX=%d' % p.read_u32())
        print('    ErrorAngleY=%d' % p.read_u32())
        print('    ChargingLevel=%d' % p.read_u16())
        print('    GaugeLevel=%d' % p.read_u16())
        print('    FiringCount=%d' % p.read_u32())
        print('    ComboCount=%d' % p.read_u32())
        print('    TargetEntity=%d' % p.read_u16())
        print('    RemoteCreateFlags=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('}')
    def serialize_62386(netid, p: common.PacketReader):
        print('SN_RemoteSyncCreateFromRemoteDoc {')
        print('	ParentEntity=%d' % p.read_i32())
        print('	OwnerEntity=%d' % p.read_i32())
        print('	RemoteSeedID=%d' % p.read_i32())
        print('	FirePosition=%s' % read_Vec3(p))
        print('	FireRotationX=%d' % p.read_u16())
        print('	FireRotationY=%d' % p.read_u16())
        print('	FireRotationZ=%d' % p.read_u16())
        print('	TargetPosition=%s' % read_Vec3(p))
        print('	FireObject="%s"' % p.read_wstr())
        print('	Scale=%d' % p.read_u16())
        print('	LifeTime=%d' % p.read_i32())
        print('	HitInvalidTarget=%d' % p.read_i32())
        print('	RemoteDocIndex=%d' % p.read_i32())
        print('	TargetEntity=%d' % p.read_i32())
        print('	RemoteCreateFlags=%d' % p.read_u8())
        print('}')

    def serialize_62387(netid, p: common.PacketReader):
        print('SN_RemoteSnapshotFromRemoteDoc {')
        print('    ParentEntity=%d' % p.read_u32())
        print('    OwnerEntity=%d' % p.read_u32())
        print('    RemoteSeedID=%d' % p.read_u32())
        print('    FireRotationX=%d' % p.read_u32())
        print('    FireRotationY=%d' % p.read_u32())
        print('    FireRotationZ=%d' % p.read_u32())
        print('    Scale=%d' % p.read_u16())
        print('    LifeTime=%d' % p.read_u16())
        print('    DurationTime=%d' % p.read_u16())
        print('    HitInvalidTarget=%d' % p.read_u32())
        print('    RemoteDocIndex=%d' % p.read_u32())
        print('    TargetEntity=%d' % p.read_u32())
        print('    RemoteCreateFlags=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u8())
        print('}')
    def serialize_62388(netid, p: common.PacketReader):
        print('SN_UNKNOWN_62388 {')
        print('    field_0=%d' % p.read_u8())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u32())
        print('    field_8=%d' % p.read_u32())
        print('}')
    def serialize_62389(netid, p: common.PacketReader):
        print('SN_RemoteSyncForecastCollision {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u8())
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
        print('    objectID=%d' % p.read_u32())
        print('    docIndex=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62398(netid, p: common.PacketReader):
        print('SN_RevivePlayerAtStartingPoint {')
        print('    usn=%d' % p.read_u32())
        print('    activeID=%d' % p.read_u32())
        print('    inactiveID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62403(netid, p: common.PacketReader):
        print('SQ_KickFromGameserver {')
        print('    reason=%d' % p.read_u32())
        print('}')
    def serialize_62405(netid, p: common.PacketReader):
        print('SN_WarehouseItemUpdate {')
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
        print('    spSlotIndex=%d' % p.read_u32())
        print('    vipSlotCount=%d' % p.read_u32())
        print('    guildSkillSlotCount=%d' % p.read_u32())
        print('    pcCafeSlotCount=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u8())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u32())
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
        print('    isSpecialSlot=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('}')
    def serialize_62414(netid, p: common.PacketReader):
        print('SA_RewardSelectSlotAllRandom {')
        print('    retval=%d' % p.read_u32())
        print('    spSlotIndex=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62415(netid, p: common.PacketReader):
        print('SN_RewardStageEnd {')
        print('    spSlotIndex=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u16())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
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
        print('    orgItemID=%d' % p.read_u32())
        print('    orgInvenType=%d' % p.read_u32())
        print('    targetInvenType=%d' % p.read_u32())
        print('    targetSlot=%d' % p.read_u32())
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62419(netid, p: common.PacketReader):
        print('SN_MailList {')
        print('    packetNum=%d' % p.read_u8())
        print('    mailboxType=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_i64())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u16())
        print('    unk_9=%d' % p.read_u32())
        print('}')
    def serialize_62420(netid, p: common.PacketReader):
        print('SN_MailBlocklist {')
        print('    packetNum=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
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
        print('    sendUTCDate=%d' % p.read_u16())
        print('    expireUTCDate=%d' % p.read_u16())
        print('    money=%d' % p.read_u16())
        print('    exp=%d' % p.read_i64())
        print('    ccoin=%d' % p.read_i64())
        print('    guildFund=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('}')
    def serialize_62423(netid, p: common.PacketReader):
        print('SN_MailGetAttachmentResult {')
        print('    nErrorType=%d' % p.read_u32())
        print('    mailId=%d' % p.read_i64())
        print('    expireUTCDate=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62424(netid, p: common.PacketReader):
        print('SN_MailMoveResult {')
        print('    nErrorType=%d' % p.read_u32())
        print('}')
    def serialize_62427(netid, p: common.PacketReader):
        print('SN_GameEventActivated {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    eventIndexList=[%s]' %s)
        print('}')

    def serialize_62428(netid, p: common.PacketReader):
        print('SN_GameEventPending {')

        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1

        print('    eventIndexList=[%s]' %s)
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
        print('    characterID=%d' % p.read_u32())
        print('    supportKitItemIndex=%d' % p.read_u32())
        print('    cooltimeId=%d' % p.read_u32())
        print('    cooltimeSec=%d' % p.read_f32())
        print('}')
    def serialize_62433(netid, p: common.PacketReader):
        print('SN_NotifyCooltime {')
        print('    characterID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62434(netid, p: common.PacketReader):
        print('SN_MailListNewInbox {')
        print('    mailboxType=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_i64())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_i64())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u8())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u16())
        print('    unk_9=%d' % p.read_u32())
        print('}')
    def serialize_62435(netid, p: common.PacketReader):
        print('SN_PlayerServerPosition {')
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('}')
    def serialize_62436(netid, p: common.PacketReader):
        print('SN_RemoteServerPosition {')
        print('    remoteServerID=%d' % p.read_i64())
        print('    boundType=%d' % p.read_u32())
        print('    durationType=%d' % p.read_u32())
        print('    colorType=%d' % p.read_u32())
        print('    flag=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u8())
        print('}')
    def serialize_62437(netid, p: common.PacketReader):
        print('SN_MonsterServerPosition {')
        print('    monsterID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
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
        print('    field_0=%d' % p.read_u8())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u32())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
        print('    field_7=%d' % p.read_u32())
        print('    field_8=%d' % p.read_u32())
        print('    field_9=%d' % p.read_u32())
        print('    field_10=%d' % p.read_u32())
        print('    field_11=%d' % p.read_u8())
        print('    field_12=%d' % p.read_u32())
        print('    field_13=%d' % p.read_u32())
        print('    field_14=%d' % p.read_u32())
        print('    field_15=%d' % p.read_u32())
        print('}')
    def serialize_62442(netid, p: common.PacketReader):
        print('SA_PvpDetailRecord {')
        print('    gameType=%d' % p.read_u32())
        print('    pvpSaveType=%d' % p.read_u8())
        print('    olympicTotalPlayCount=%d' % p.read_u32())
        print('    olympicWorldRecord=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u16())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u16())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_i64())
        print('    unk_10=%d' % p.read_u32())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u16())
        print('}')
    def serialize_62443(netid, p: common.PacketReader):
        print('SN_SummaryRewardResult {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u8())
        print('    unk_5=%d' % p.read_i64())
        print('}')
    def serialize_62444(netid, p: common.PacketReader):
        print('SN_ReduceCooltime {')
        print('    characterID=%d' % p.read_u32())
        print('    cooltimeID=%d' % p.read_u32())
        print('    valueType=%d' % p.read_u32())
        print('    value=%d' % p.read_f32())
        print('}')
    def serialize_62445(netid, p: common.PacketReader):
        print('SN_BroadcastGamePingData {')
        print('    characterID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    pingData=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62452(netid, p: common.PacketReader):
        print('SN_BushObjectState {')
        print('    objectID=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62457(netid, p: common.PacketReader):
        print('SN_SortieMasterAssignPhaseStart {')
        print('    alliesLeaderUserId=%d' % p.read_u32())
        print('    enemiesLeaderUserId=%d' % p.read_u32())
        print('    timeSec=%d' % p.read_u32())
        print('    assignCount=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('}')
    def serialize_62464(netid, p: common.PacketReader):
        print('SN_SortieMasterAssignPhaseEnd {')
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u16())
        print('    unk_9=%d' % p.read_u32())
        print('}')
    def serialize_62470(netid, p: common.PacketReader):
        print('SA_TierStageRecord {')
        print('    seasonId=%d' % p.read_u8())
        print('    sectorId=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u8())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u16())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u32())
        print('    unk_14=%d' % p.read_u32())
        print('    unk_15=%d' % p.read_u32())
        print('    unk_16=%d' % p.read_u32())
        print('}')
    def serialize_62471(netid, p: common.PacketReader):
        print('SA_PvpRanking {')
        print('    seasonId=%d' % p.read_u8())
        print('    pvpSaveType=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('}')
    def serialize_62473(netid, p: common.PacketReader):
        print('SN_NotifyIsInSafeZone {')
        print('    inSafeZone=%d' % p.read_u8())
        print('}')
    def serialize_62475(netid, p: common.PacketReader):
        print('SA_PveRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u32())
        print('    highScoreClearRankType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u16())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
        print('    unk_9=%d' % p.read_u32())
        print('    unk_10=%d' % p.read_u16())
        print('    unk_11=%d' % p.read_u32())
        print('    unk_12=%d' % p.read_u32())
        print('    unk_13=%d' % p.read_u16())
        print('}')
    def serialize_62476(netid, p: common.PacketReader):
        print('SA_MyPveRanking {')
        print('    stageIndex=%d' % p.read_u32())
        print('    highScoreClearRankType=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u8())
        print('}')
    def serialize_62477(netid, p: common.PacketReader):
        print('SN_PveLastgameRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u8())
        print('    ranking=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u16())
        print('    unk_7=%d' % p.read_u32())
        print('}')
    def serialize_62478(netid, p: common.PacketReader):
        print('SN_DefenceLastgameRanking {')
        print('    stageId=%d' % p.read_u32())
        print('    packetNum=%d' % p.read_u8())
        print('    ranking=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u16())
        print('    unk_6=%d' % p.read_u32())
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
        print('    field_0=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62489(netid, p: common.PacketReader):
        print('SA_ChatChannelInvite {')
        print('    retval=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62490(netid, p: common.PacketReader):
        print('SN_ChatChannelInvite {')
        print('    channelCenter=%d' % p.read_u16())
        print('    channelType=%d' % p.read_u16())
        print('    bInvite=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u8())
        print('}')
    def serialize_62491(netid, p: common.PacketReader):
        print('SA_ChatChannelInviteFeedback {')
        print('    retval=%d' % p.read_u32())
        print('}')
    def serialize_62492(netid, p: common.PacketReader):
        print('SN_ChatChannelInviteFeedback {')
        print('    errorCode=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62493(netid, p: common.PacketReader):
        print('SN_InteractionStatus {')
        print('    state=%d' % p.read_i32())
        print('    objectID=%d' % p.read_i32())
        print('    interactionID=%d' % p.read_i32())
        print('    occupiedTeam=%d' % p.read_i32())
        print('    coolTime=%d' % p.read_f32())
        print('}')

    def serialize_62494(netid, p: common.PacketReader):
        print('SN_InteractionCancel {')
        print('    objectID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('}')
    def serialize_62495(netid, p: common.PacketReader):
        print('SN_InteractionCasting {')
        print('    objectID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('    castingTimeMS=%d' % p.read_u32())
        print('}')
    def serialize_62496(netid, p: common.PacketReader):
        print('SN_InteractionExecute {')
        print('    objectID=%d' % p.read_u32())
        print('    targetID=%d' % p.read_u32())
        print('    actionState=%d' % p.read_u32())
        print('}')
    def serialize_62497(netid, p: common.PacketReader):
        print('SQ_TeleportObject {')
        print('    objectID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62498(netid, p: common.PacketReader):
        print('SN_PlayerSyncTeleport {')
        print('    objectID=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62499(netid, p: common.PacketReader):
        print('SN_ChangeBattleState {')
        print('    objectID=%d' % p.read_u32())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('}')
    def serialize_62505(netid, p: common.PacketReader):
        print('SN_AIMONSTER_SYNC_MOVE {} @98ba71')
    def serialize_62506(netid, p: common.PacketReader):
        print('SN_AIMONSTER_SYNC_TURN {} @98bccb')
    def serialize_62507(netid, p: common.PacketReader):
        print('SA_ItemTrade {')
        print('    result=%d' % p.read_u32())
        print('    goldAmount=%d' % p.read_i64())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62508(netid, p: common.PacketReader):
        print('SA_ItemCraft {')
        print('    craftingItemType=%d' % p.read_u8())
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62509(netid, p: common.PacketReader):
        print('SA_ItemDisassemble {')
        print('    craftingItemType=%d' % p.read_u8())
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('}')
    def serialize_62510(netid, p: common.PacketReader):
        print('SA_OlympicBestRecord {')
        print('    personalRecord=%d' % p.read_u32())
        print('    worldRecord=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
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
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u8())
        print('    field_2=%d' % p.read_u32())
        print('    field_3=%d' % p.read_u8())
        print('    field_4=%d' % p.read_u8())
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
        print('    characterID=%d' % p.read_u32())
        print('    docIndex=%d' % p.read_u32())
        print('    coolTime=%d' % p.read_u32())
        print('}')
    def serialize_62518(netid, p: common.PacketReader):
        print('SN_TransformCharacter {')
        print('    characterID=%d' % p.read_u32())
        print('    docIndex=%d' % p.read_u32())
        print('    durationTime=%d' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('    SkillSlot1=%d' % p.read_u32())
        print('    SkillSlot2=%d' % p.read_u32())
        print('    ugSkill=%d' % p.read_u32())
        print('    baseMoveSpeed=%d' % p.read_u16())
        print('    action=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62519(netid, p: common.PacketReader):
        print('SN_ChangeCharacterMode {')
        print('    characterID=%d' % p.read_u32())
        print('    docIndex=%d' % p.read_u32())
        print('    weaponIndex=%d' % p.read_u32())
        print('    skillSlot1=%d' % p.read_u32())
        print('    skillSlot2=%d' % p.read_u32())
        print('    ugSkill=%d' % p.read_u32())
        print('    baseMoveSpeed=%d' % p.read_u16())
        print('    action=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u8())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62520(netid, p: common.PacketReader):
        print('SN_ItemOptiongroupList {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u16())
        print('    field_2=%d' % p.read_u8())
        print('    field_3=%d' % p.read_u32())
        print('    field_4=%d' % p.read_u8())
        print('    field_5=%d' % p.read_u32())
        print('    field_6=%d' % p.read_u32())
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
        print('    aniMoveSpeedTimeSec=%d' % p.read_f32())
        print('    progress=%d' % p.read_f32())
        print('    curLoopCount=%d' % p.read_u32())
        print('    maxLoopCount=%d' % p.read_u32())
        print('    serverTime=%d' % p.read_u32())
        print('    startTime=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_i64())
        print('    unk_2=%d' % p.read_i64())
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
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62529(netid, p: common.PacketReader):
        print('SN_NcguardMsg {')
        print('    field_0=%d' % p.read_u8())
        print('}')
    def serialize_62530(netid, p: common.PacketReader):
        print('SN_TenprotectPunishment {')
        print('    punishMode=%d' % p.read_u8())
        print('    unk_0=%d' % p.read_u16())
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u32())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u8())
        print('}')
    def serialize_62542(netid, p: common.PacketReader):
        print('SN_DateChanged {')
        print('}')


    def serialize_62543(netid, p: common.PacketReader):
        print('SN_CalendarUpdated {')
        print('}')
    def serialize_62544(netid, p: common.PacketReader):
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

    def serialize_62546(netid, p: common.PacketReader):
        print('SN_StageSkillAvailable {')
        print('    currentSkillIndex=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u32())
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
        print('    field_0=%d' % p.read_u8())
        print('    field_1=%d' % p.read_u32())
        print('    field_2=%d' % p.read_u16())
        print('}')
    def serialize_62551(netid, p: common.PacketReader):
        print('SN_ItemLifetimeExpired {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    itemDocIndexList=[%s]' % s)
        print('}')

    def serialize_62552(netid, p: common.PacketReader):
        print('SN_EffectLifetimeExpired {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    effectItemDocIndexList=[%s]' % s)
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
        print('SN_EffectLifetime {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(effectItemDocIndex=%d expireDateTime=%d)' % (p.read_i32(), p.read_i64())
            count -= 1
        print('    effectItemDocIndexList=[%s]' % s)
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
        print('    unk_0=%d' % p.read_u32())
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
        print('    entityID=%d' % p.read_u32())
        print('    param1=%d' % p.read_u32())
        print('    param2=%d' % p.read_u32())
        print('}')
    def serialize_62572(netid, p: common.PacketReader):
        print('SN_GMDisabledMasters {')
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '%d, ' % p.read_i32()
            count -= 1
        print('    masterList=[%s]' % s)
        print('}')
    def serialize_62573(netid, p: common.PacketReader):
        print('SN_GMDisabledSkins {')
        
        
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '"%s", ' % p.read_wstr()
            count -= 1
        
        print('    skinKeyList=[%s]' % s)
        print('}')
    def serialize_62574(netid, p: common.PacketReader):
        print('SN_GMDisabledStage {')
        
        count = p.read_u16()
        s = ''
        while count > 0:
            s += '(stageIndex=%d gameType=%d), ' % (p.read_u8(), p.read_u8())
            count -= 1
        
        print('    stageList=[%s]' % s)
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
        print('    unk_0=%d' % p.read_u8())
        print('    unk_1=%d' % p.read_u8())
        print('}')
    def serialize_62585(netid, p: common.PacketReader):
        print('SN_GameRecordInfoList {')
        print('    field_0=%d' % p.read_u32())
        print('    field_1=%d' % p.read_u16())
        print('    field_2=%d' % p.read_u8())
        print('}')
    def serialize_62586(netid, p: common.PacketReader):
        print('SA_ChatGetInviteeGameaccountid {')
        print('    result=%d' % p.read_u32())
        print('    inviteeCenterId=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('}')
    def serialize_62587(netid, p: common.PacketReader):
        print('SA_UseCoupon {')
        print('    result=%d' % p.read_u32())
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62588(netid, p: common.PacketReader):
        print('SN_NotifyChattingChannelStatus {')
        print('    reason=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u16())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u16())
        print('    unk_3=%d' % p.read_u16())
        print('    unk_4=%d' % p.read_u32())
        print('}')
    def serialize_62589(netid, p: common.PacketReader):
        print('SN_DefenceModeIngameInfo {')
        print('    currentStep=%d' % p.read_u32())
        print('    totalClearTime=%d' % p.read_u32())
        print('}')
    def serialize_62590(netid, p: common.PacketReader):
        print('SA_RequestToken {')
        print('    unk_0=%d' % p.read_u16())
        print('}')
    def serialize_62591(netid, p: common.PacketReader):
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

    def serialize_62592(netid, p: common.PacketReader):
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
        print('    unk_0=%d' % p.read_u32())
        print('    unk_1=%d' % p.read_u16())
        print('    unk_2=%d' % p.read_u32())
        print('    unk_3=%d' % p.read_u32())
        print('    unk_4=%d' % p.read_u32())
        print('    unk_5=%d' % p.read_u32())
        print('    unk_6=%d' % p.read_u32())
        print('    unk_7=%d' % p.read_u32())
        print('    unk_8=%d' % p.read_u32())
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
        print('    praiseCount=%d' % p.read_u16())
        print('    unk_0=%d' % p.read_u32())
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
                stations += 'gameServerIp=(%d.%d.%d.%d) pingServerIp=(%d.%d.%d.%d) port=%d' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8(), p.read_u16)
                station_count = -1

            s += '(idc=%d stations=[%s])' % (idc, stations)
            count -= 1
        
        print('    stationList[%s]' % s)

        print('}')

    def serialize_62010(netid, p: common.PacketReader):
        print('SN_DoConnectGameServer {')
        print('    dwPort=%d' % p.read_u16())
        print('    ip=[%d.%d.%d.%d]' % (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8()))
        print('    gameID=%d' % p.read_i32())
        print('    idcHash=%u' % p.read_u32())
        print('    nickname="%s"' % p.read_wstr())
        print('    instantKey=%d' % p.read_u32())
        print('}')

    def serialize_62025(netid, p: common.PacketReader):
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
        print('    objectID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('}')

    def serialize_62028(netid, p: common.PacketReader):
        print('SN_GameCreateSubActor {')
        print('    objectID=%d' % p.read_i32())
        print('    mainEntityID=%d' % p.read_i32())
        print('    nType=%d' % p.read_i32())
        print('    nIDX=%d' % p.read_i32())
        print('    dwLocalID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    spawnType=%d' % p.read_i32())
        print('    actionState=%d' % p.read_i32())
        print('    ownerID=%d' % p.read_i32())
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

        print('    objectID=%d' % p.read_i32())
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
        print('    objectID=%d' % p.read_i32())
        print('}')

    def serialize_62031(netid, p: common.PacketReader):
        print('SN_StatusSnapshot {')
        print('    objectID=%d' % p.read_i32())
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
        print('	entityID=%d' % p.read_i32())
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
        print('	entityID=%d' % p.read_i32())
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
        print('	characterID=%d' % p.read_i32())
        print('	ret=%d' % p.read_i32())
        print('	skillIndex=%d' % p.read_i32())
        print('}')

    def serialize_62048(netid, p: common.PacketReader):
        print('SN_PlayerSkillSlot {')
        print('    characterID=%d' % p.read_i32())

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
        print('    playerID=%d' % p.read_i32())
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
        print('    objectID=%d' % p.read_i32())
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
            print('      userID=%d' % p.read_i32())
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
            print('      userID=%d' % p.read_i32())
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
        print('    characterID=%d' % p.read_i32())
        print('    weaponDocIndex=%d' % p.read_i32())
        print('    additionnalOverHeatGauge=%d' % p.read_i32())
        print('    additionnalOverHeatGaugeRatio=%d' % p.read_i32())
        print('}')

    def serialize_62089(netid, p: common.PacketReader):
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

    def serialize_62090(netid, p: common.PacketReader):
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

    def serialize_62112(netid, p: common.PacketReader):
        print('SN_GamePlayerTag {')
        print('    result=%d' % p.read_i32())
        print('    mainID=%d' % p.read_i32())
        print('    subID=%d' % p.read_i32())
        print('    attackerID=%d' % p.read_i32())
        print('}')

    def serialize_62113(netid, p: common.PacketReader):
        print('SA_GetCharacterInfo {')
        print('    characterID=%d' % p.read_i32())
        print('    docIndex=%d' % p.read_i32())
        print('    class=%d' % p.read_i32())
        print('    hp=%d' % p.read_i32())
        print('    maxHp=%d' % p.read_i32())
        print('}')

    def serialize_62123(netid, p: common.PacketReader):
        print('SN_LeaderCharacter {')
        print('    leaderID=%d' % p.read_i32())
        print('    skinIndex=%d' % p.read_i32())
        print('}')

    def serialize_62124(netid, p: common.PacketReader):
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

    def serialize_62125(netid, p: common.PacketReader):
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

    def serialize_62126(netid, p: common.PacketReader):
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

    def serialize_62127(netid, p: common.PacketReader):
        print('SN_ProfileSkills {')
        print('    packetNum=%d' % p.read_u8())
        
        count = p.read_u16()
        print('    skills(%d)=[' % count)
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
                s += '(gearType=%d gearItemID=%d), ' % (p.read_i32(), p.read_i32())
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
            print('      userID=%d' % p.read_i32())
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
            print('      userID=%d' % p.read_i32())
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
            print('      userID=%d' % p.read_i32())
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
        print('    characterID=%d' % p.read_i32())
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
        print('    userID=%d' % p.read_i32())
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
            print('      userID=%d' % p.read_i32())

            s = 'characterID=%d, docIndex=%d, class=%d, hp=%d, maxHp=%d' % (p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32())
            print('      mainPc=( %s )' % s)
            s = 'characterID=%d, docIndex=%d, class=%d, hp=%d, maxHp=%d' % (p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32(), p.read_i32())
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
        print('    objectID=%d' % p.read_i32())
        print('    rotate=%g' % p.read_f32())
        print('    moveDir=(%g, %g)' % (p.read_f32(), p.read_f32()))
        print('    errorType=%d' % p.read_i32())

        if excludedFieldBits & 0x20 == 0:
            print('    startPos=%s' % read_Vec3(p))
        print('}')

    def serialize_62242(netid, p: common.PacketReader):
        print('SN_ChatChannelMessage {')
        print('    chatType=%d' % p.read_i32())
        print('    senderNickname="%s"' % p.read_wstr())
        print('    senderStaffType=%d' % p.read_u8())
        print('    chatMsg="%s"' % p.read_wstr())
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
        print('    leaderCsn=%d' % p.read_i32())
        print('    nickname="%s"' % p.read_wstr())
        print('    isLevelUp=%d' % p.read_u8())
        print('    level=%d' % p.read_u16())
        print('    currentLevelExp=%d' % p.read_i32())
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
        print('    guildTag="%s"' % p.read_wstr())
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
        print('    guildName="%s"' % p.read_wstr())
        print('    nick="%s"' % p.read_wstr())
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
        print('    usn=%d' % p.read_i32())
        print('    nickname="%s"' % p.read_wstr())
        print('    progressData=%d' % p.read_u8())
        print('    activeCreatureIndex=%d' % p.read_i32())
        print('    inactiveCreatureIndex=%d' % p.read_i32())
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
        print('    userID=%d' % p.read_i32())
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
