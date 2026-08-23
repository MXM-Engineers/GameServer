# client packets
from . import common

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)
    
def read_Vec2(p):
    x, y = p.read_f32(), p.read_f32()
    return 'Vec2(%g, %g)' % (x, y)

class ClientSerializer:

    def serialize_60002(netid, p: common.PacketReader):
        print('CQ_FirstHello {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u8())
        print('}')
    def serialize_60008(netid, p: common.PacketReader):
        print('CQ_Authenticate {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60009(netid, p: common.PacketReader):
        print('CQ_AuthenticateGameServer {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u8())
        print('}')
    def serialize_60014(netid, p: common.PacketReader):
        print('CN_ReadyToLoadCharacter {')
        print('}')
    def serialize_60016(netid, p: common.PacketReader):
        print('CN_GameMapLoaded {')
        print('}')
    def serialize_60024(netid, p: common.PacketReader):
        print('CA_SetGameGvt {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60022(netid, p: common.PacketReader):
        print('CN_GamePlayerSyncByInt {')
        print('    characterID=0x%08x' % p.read_u32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDir=%s' % read_Vec3(p))
        print('    p3nEye=%s' % read_Vec3(p))
        print('    nRotate=%g' % p.read_f32())
        print('    nSpeed=%g' % p.read_f32())
        print('    nState=%d' % p.read_i32())
        print('    nActionIDX=%d' % p.read_i32())
        print('}')
    def serialize_60032(netid, p: common.PacketReader):
        print('CQ_GameIsReady {')
        print('}')
    def serialize_60033(netid, p: common.PacketReader):
        print('CQ_LoadingComplete {')
        print('}')
    def serialize_60034(netid, p: common.PacketReader):
        print('CA_CityLobbyJoinCity {')
        print('}')
    def serialize_60038(netid, p: common.PacketReader):
        print('CN_PlayerTagCompleted {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60040(netid, p: common.PacketReader):
        print('CQ_PlayerCastSkill {')
        print('    playerID=0x%08x' % p.read_u32())
        print('    skillID=%d' % p.read_u32())
        print('    p3npos=%s' % read_Vec3(p))
        n = p.read_u16()
        print('    targetList_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        print('    posStruct={')
        print('        pos=%s' % read_Vec3(p))
        print('        destPos=%s' % read_Vec3(p))
        print('        moveDir=%s' % read_Vec2(p))
        print('        rot=%s' % read_Vec3(p))
        print('        speed=%g' % p.read_f32())
        print('        clientTime=%d' % p.read_i32())
        print('    }')
        print('}')
    def serialize_60051(netid, p: common.PacketReader):
        print('CQ_GetCharacterInfo {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60053(netid, p: common.PacketReader):
        print('CQ_GamePlayerTag {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60073(netid, p: common.PacketReader):
        print('CQ_RequestAreaPopularity {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60074(netid, p: common.PacketReader):
        print('CQ_PartyCreate {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60075(netid, p: common.PacketReader):
        print('CQ_PartyModify {')
        n = p.read_u16()
        print('    memberIDs_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        print('    unk1=%d' % p.read_u32())
        print('    unk2=%d' % p.read_u32())
        print('    unk3=%d' % p.read_u8())
        print('}')
    
    def serialize_60076(netid, p: common.PacketReader):
        print('CQ_PartyOptionModify {')
        print('    optionIndex=%d' % p.read_u8())
        print('    optionValue=%d' % p.read_u8())
        print('}')
    
    def serialize_60081(netid, p: common.PacketReader):
        print('Cl_60081 {')
        print('}')
    def serialize_60089(netid, p: common.PacketReader):
        print('CA_SortieRoomFound {')
        print('    field1=%d' % p.read_i64())
        print('}')
    def serialize_60097(netid, p: common.PacketReader):
        print('CQ_EnqueueGame {')
        print('}')
    def serialize_60100(netid, p: common.PacketReader):
        print('CQ_MasterPick {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60101(netid, p: common.PacketReader):
        print('CQ_MasterUnpick {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60113(netid, p: common.PacketReader):
        print('CQ_PlayerJump {')
        bJumpFlags = p.read_u8()
        print('    bJumpFlags=%d' % bJumpFlags)
        print('    unkZero=%d' % p.read_u32())
        print('    skinOrCharID=%d' % p.read_u32())
        print('    unkParam=%d' % p.read_u32())
        jumpPos = read_Vec2(p)
        print('    jumpPos=%s' % jumpPos)
        if not (bJumpFlags & 0x10):
            print('    destPos=%s' % read_Vec3(p))
        print('}')
    def serialize_60145(netid, p: common.PacketReader):
        print('CQ_GetGuildProfile {')
        print('}')
    def serialize_60146(netid, p: common.PacketReader):
        print('CQ_GetGuildMemberList {')
        print('}')
    def serialize_60148(netid, p: common.PacketReader):
        print('CQ_GetGuildHistoryList {')
        print('}')
    def serialize_60151(netid, p: common.PacketReader):
        print('CQ_TierRecord {')
        print('}')
    def serialize_60167(netid, p: common.PacketReader):
        print('CQ_GetGuildRankingSeasonList {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60178(netid, p: common.PacketReader):
        print('CN_GameUpdatePosition {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    currPos=%s' % read_Vec3(p))
        print('    unkVec2a=%s' % read_Vec2(p))
        print('    unkVec2b=%s' % read_Vec2(p))
        print('    unkU32a=%d' % p.read_u32())
        print('    unkU32b=%d' % p.read_u32())
        print('    unkU8a=%d' % p.read_u8())
        print('    unkU32c=%d' % p.read_u32())
        print('    unkU32d=%d' % p.read_u32())
        print('    unkU8b=%d' % p.read_u8())
        print('}')
    
    def serialize_60179(netid, p: common.PacketReader):
        print('CN_GameUpdateRotation {')
        print('    entityID=0x%08x' % p.read_u32())
        print('    rotation=%s' % read_Vec2(p))
        print('    unkU32=%d' % p.read_u32())
        print('}')
    
    def serialize_60180(netid, p: common.PacketReader):
        print('CQ_WeaponState {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u8())
        print('    field4=%d' % p.read_u8())
        print('    field5=%d' % p.read_u32())
        print('    field6=%d' % p.read_u32())
        print('}')
    def serialize_60235(netid, p: common.PacketReader):
        print('CQ_LoadingProgressData {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60245(netid, p: common.PacketReader):
        print('CQ_RTT_Time {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60265(netid, p: common.PacketReader):
        print('CQ_NcguardData {')
        n = p.read_u16()
        print('    guardPayload_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        print('}')
    
    def serialize_60272(netid, p: common.PacketReader):
        print('CQ_RequestCalendar {')
        print('    dateToken=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('}')
    
    def serialize_60003(netid, p: common.PacketReader):
        print('CQ_UserLogin {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%s' % p.read_wstr())
        print('    field3=%s' % p.read_wstr())
        print('    field4=%s' % p.read_wstr())
        print('    field5=%d' % p.read_u32())
        print('}')
    def serialize_60004(netid, p: common.PacketReader):
        print('CQ_UserLoginByNp {')
        print('    field1=%s' % p.read_str())
        print('    field2=%s' % p.read_str())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60005(netid, p: common.PacketReader):
        print('ConfirmGatewayInfo {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%s' % p.read_wstr())
        print('}')
    def serialize_60006(netid, p: common.PacketReader):
        print('CN_StationLatency {')
        print('    elapsedMs=%d' % p.read_u32())
        n = p.read_u16()
        print('    stationPings_count=%d' % n)
        for _ in range(n):
            ip = p.read_u32()
            rtt = p.read_u16()
            print('        ip=%d.%d.%d.%d rtt=%dms' % (ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff, rtt))
        n = p.read_u16()
        print('    stationStats_count=%d' % n)
        for _ in range(n):
            id2 = p.read_u32()
            a = p.read_u16()
            b = p.read_u16()
            v = p.read_f32()
            print('        entry={ id=0x%08x counter=%d unkU16=%d value=%g }' % (id2, a, b, v))
        print('}')
    def serialize_60007(netid, p: common.PacketReader):
        print('EnterQueue {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        n = p.read_u16()
        print('    field3_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        n = p.read_u16()
        print('    field4_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        print('}')
    def serialize_60010(netid, p: common.PacketReader):
        print('CQ_HandoverAuthRequestToSpectator {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60012(netid, p: common.PacketReader):
        print('CQ_ItemMerge {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60015(netid, p: common.PacketReader):
        print('CQ_ItemUse {')
        print('    itemInstanceUID=0x%08x' % p.read_u32())
        print('}')
    def serialize_60017(netid, p: common.PacketReader):
        print('CQ_SellShopItem {')
        print('    shopRef=%d' % p.read_u32())
        n = p.read_u16()
        print('    sellEntries_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d)' % (p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60020(netid, p: common.PacketReader):
        print('CQ_GetGroundItem {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60023(netid, p: common.PacketReader):
        print('CN_GamePlayerSyncActionStateOnly {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u8())
        print('    field4=%d' % p.read_u32())
        print('    field5=%d' % p.read_u32())
        print('    field6=%d' % p.read_u32())
        print('    field7=%d' % p.read_u32())
        print('    field8=%d' % p.read_u32())
        print('}')
    def serialize_60028(netid, p: common.PacketReader):
        print('CN_CommunityChat {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%s' % p.read_wstr())
        print('    field3=%s' % p.read_wstr())
        print('}')
    def serialize_60029(netid, p: common.PacketReader):
        print('CQ_BuyShopItem {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60030(netid, p: common.PacketReader):
        print('CQ_BuyCShopItem {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60031(netid, p: common.PacketReader):
        print('CQ_ItemDelete {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60036(netid, p: common.PacketReader):
        print('CN_RequestShopItemList {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60037(netid, p: common.PacketReader):
        print('CQ_ItemChange {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60039(netid, p: common.PacketReader):
        print('CQ_PreCastSkill {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=<unknown>')
        print('}')
    def serialize_60041(netid, p: common.PacketReader):
        print('CQ_ExecuteSkill {')
        print('    charID=%d' % p.read_u32())
        print('    skillID=%d' % p.read_u32())
        print('    targetPos=%s' % read_Vec3(p))
        n = p.read_u16()
        print('    targetIDs_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        print('    unkParams=[%s]' % ', '.join(str(p.read_u32()) for _ in range(13)))
        print('}')
    
    def serialize_60042(netid, p: common.PacketReader):
        print('CQ_SwitchOnToggleSkill {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60044(netid, p: common.PacketReader):
        print('CQ_PlayerCancelStance {')
        print('}')
    def serialize_60047(netid, p: common.PacketReader):
        print('CQ_AntiHackAuth {')
        n = p.read_u16()
        print('    authData_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        print('}')
    
    def serialize_60049(netid, p: common.PacketReader):
        print('SetNickname {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60052(netid, p: common.PacketReader):
        print('CQ_SetLeaderCharacter {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60054(netid, p: common.PacketReader):
        print('Cl_60054 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60055(netid, p: common.PacketReader):
        print('CQ_SkillUpgradeIngame {')
        print('    unkIngameContext=%d' % p.read_u32())
        print('    skillUpgradeId=%d' % p.read_u32())
        print('}')
    def serialize_60056(netid, p: common.PacketReader):
        print('Cl_60056 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60060(netid, p: common.PacketReader):
        print('CQ_GearEquip {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60061(netid, p: common.PacketReader):
        print('CQ_GearUnequip {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60062(netid, p: common.PacketReader):
        print('CQ_GearSwap {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u8())
        print('    field3=%d' % p.read_u8())
        print('}')
    def serialize_60068(netid, p: common.PacketReader):
        print('CQ_CharacterUnlock {')
        print('}')
    def serialize_60069(netid, p: common.PacketReader):
        print('CQ_SkillUnlock {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60070(netid, p: common.PacketReader):
        print('CQ_SkillSelect {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60071(netid, p: common.PacketReader):
        print('CQ_RoomEquipSkill {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60072(netid, p: common.PacketReader):
        print('CQ_RoomSwapSkill {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60077(netid, p: common.PacketReader):
        print('CQ_PartySwapTeam {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u8())
        print('}')
    def serialize_60078(netid, p: common.PacketReader):
        print('CQ_PartyInvite {')
        print('    field1=<unknown>')
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60079(netid, p: common.PacketReader):
        print('CQ_PartyInviteResponse {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60080(netid, p: common.PacketReader):
        print('CQ_PartyInviteRecommendResponse {')
        print('    targetName=%s' % p.read_wstr())
        print('    bAccept=%d' % p.read_u8())
        print('}')
    
    def serialize_60082(netid, p: common.PacketReader):
        print('Cl_60082 {')
        print('}')
    def serialize_60083(netid, p: common.PacketReader):
        print('CQ_RejectPartyRoom {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('    field3=%s' % p.read_wstr())
        print('}')
    def serialize_60084(netid, p: common.PacketReader):
        print('CQ_JoinPartyAsSpectator {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%s' % p.read_wstr())
        print('}')
    def serialize_60085(netid, p: common.PacketReader):
        print('CQ_LeavePartyRoom {')
        print('}')
    def serialize_60086(netid, p: common.PacketReader):
        print('CQ_BreakUpPartyRoom {')
        print('}')
    def serialize_60087(netid, p: common.PacketReader):
        print('CQ_PartyKickAll {')
        print('}')
    def serialize_60088(netid, p: common.PacketReader):
        print('CQ_QuickRunArena {')
        print('}')
    def serialize_60090(netid, p: common.PacketReader):
        print('CN_SortieRoomConfirm {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60091(netid, p: common.PacketReader):
        print('CQ_RestartGame {')
        print('}')
    def serialize_60092(netid, p: common.PacketReader):
        print('CQ_RestartVote {')
        print('}')
    def serialize_60093(netid, p: common.PacketReader):
        print('Cl_60093 {')
        print('}')
    def serialize_60094(netid, p: common.PacketReader):
        print('CQ_RestartVote2 {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60095(netid, p: common.PacketReader):
        print('Cl_60095 {')
        print('}')
    def serialize_60096(netid, p: common.PacketReader):
        print('Cl_60096 {')
        print('}')
    def serialize_60098(netid, p: common.PacketReader):
        print('CQ_CancelMatching {')
        print('}')
    def serialize_60102(netid, p: common.PacketReader):
        print('CQ_MasterReset {')
        print('}')
    def serialize_60103(netid, p: common.PacketReader):
        print('CQ_ReadySortieRoom {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60104(netid, p: common.PacketReader):
        print('CQ_WeaponUpgrade {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60105(netid, p: common.PacketReader):
        print('CQ_InventoryExpand {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60106(netid, p: common.PacketReader):
        print('CQ_InventoryRearrange {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60107(netid, p: common.PacketReader):
        print('Cl_60107 {')
        print('}')
    def serialize_60108(netid, p: common.PacketReader):
        print('CN_RemoteActivated {')
        print('    unkU32_1=%d' % p.read_u32())
        print('    unkU32_2=%d' % p.read_u32())
        print('    unkU32_3=%d' % p.read_u32())
        print('    unkU32_4=%d' % p.read_u32())
        print('    unkU32_5=%d' % p.read_u32())
        print('    unkU32_6=%d' % p.read_u32())
        print('    unkU32_7=%d' % p.read_u32())
        print('    unkU32_8=%d' % p.read_u32())
        print('    unkU32_9=%d' % p.read_u32())
        print('    unkU32_10=%d' % p.read_u32())
        print('    unkWstr=%s' % p.read_wstr())
        print('    unkStr=%s' % p.read_str())
        print('    unkU32_13=%d' % p.read_u32())
        print('    unkU32_14=%d' % p.read_u32())
        print('    unkU32_15=%d' % p.read_u32())
        print('    unkU32_16=%d' % p.read_u32())
        print('    unkU32_17=%d' % p.read_u32())
        print('    unkU32_18=%d' % p.read_u32())
        print('    unkU32_19=%d' % p.read_u32())
        print('    unkU32_20=%d' % p.read_u32())
        print('    unkU32_21=%d' % p.read_u32())
        print('    unkU32_22=%d' % p.read_u32())
        print('    unkU32_23=%d' % p.read_u32())
        print('    unkU32_24=%d' % p.read_u32())
        print('    unkU32_25=%d' % p.read_u32())
        print('    unkU32_26=%d' % p.read_u32())
        print('    unkU32_27=%d' % p.read_u32())
        print('    unkU32_28=%d' % p.read_u32())
        print('    unkU32_29=%d' % p.read_u32())
        print('    unkU32_30=%d' % p.read_u32())
        print('    unkU32_31=%d' % p.read_u32())
        print('}')
    
    def serialize_60109(netid, p: common.PacketReader):
        print('CN_RemoteHitRemote {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('    field5=%d' % p.read_u32())
        print('    field6=%d' % p.read_u32())
        print('    field7=%d' % p.read_u32())
        print('    field8=%d' % p.read_u8())
        print('    field9=%s' % read_Vec3(p))
        print('    field10=%s' % read_Vec3(p))
        print('    field11=%d' % p.read_u32())
        print('}')
    def serialize_60110(netid, p: common.PacketReader):
        print('CQ_DestroyRemote {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%s' % read_Vec3(p))
        print('}')
    def serialize_60111(netid, p: common.PacketReader):
        print('CQ_RequestSpAction {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%s' % read_Vec3(p))
        print('}')
    def serialize_60112(netid, p: common.PacketReader):
        print('Cl_60112 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60115(netid, p: common.PacketReader):
        print('CQ_SelectSkin {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60116(netid, p: common.PacketReader):
        print('Cl_60116 {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60119(netid, p: common.PacketReader):
        print('Cl_60119 {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60120(netid, p: common.PacketReader):
        print('CQ_FriendSetComrade {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60121(netid, p: common.PacketReader):
        print('CQ_FriendUnsetComrade {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60122(netid, p: common.PacketReader):
        print('CQ_FriendSetFavorite {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60123(netid, p: common.PacketReader):
        print('CQ_FriendUnsetFavorite {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60124(netid, p: common.PacketReader):
        print('CQ_FriendRemove {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60125(netid, p: common.PacketReader):
        print('CQ_CommunityStatusMessage {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60126(netid, p: common.PacketReader):
        print('CQ_CommunityChannelMove {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60127(netid, p: common.PacketReader):
        print('CQ_CommunityBlockAdd {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60128(netid, p: common.PacketReader):
        print('CQ_GetUserInfo {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60129(netid, p: common.PacketReader):
        print('CQ_JukeboxEnqueue {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60130(netid, p: common.PacketReader):
        print('CQ_JukeboxQueueSong {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60132(netid, p: common.PacketReader):
        print('Cl_60132 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60133(netid, p: common.PacketReader):
        print('CQ_CreateGuild {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%s' % p.read_wstr())
        print('}')
    def serialize_60137(netid, p: common.PacketReader):
        print('CQ_KickGuildMember {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60139(netid, p: common.PacketReader):
        print('Cl_60139 {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60141(netid, p: common.PacketReader):
        print('CQ_RespondGuildJoinRequest {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60142(netid, p: common.PacketReader):
        print('CQ_InviteGuildMember {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60144(netid, p: common.PacketReader):
        print('CQ_RespondGuildInvitation {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60149(netid, p: common.PacketReader):
        print('CQ_GetGuildPublicProfile {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60150(netid, p: common.PacketReader):
        print('Cl_60150 {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60152(netid, p: common.PacketReader):
        print('CQ_EditGuildNotice {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60153(netid, p: common.PacketReader):
        print('CQ_EditGuildIntro {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60154(netid, p: common.PacketReader):
        print('CQ_EditGuildTag {')
        print('    field1=%s' % p.read_wstr())
        print('}')
    def serialize_60155(netid, p: common.PacketReader):
        print('CQ_SetGuildInterest {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60156(netid, p: common.PacketReader):
        print('CQ_SetGuildEmblem {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60157(netid, p: common.PacketReader):
        print('CQ_SetGuildJoinType {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60158(netid, p: common.PacketReader):
        print('CQ_CreateGuildMembership {')
        print('    field1=<unknown>')
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60159(netid, p: common.PacketReader):
        print('CQ_DeleteGuildMembership {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60160(netid, p: common.PacketReader):
        print('CQ_EditGuildMembership {')
        print('    field1=%d' % p.read_u8())
        print('    field2=<unknown>')
        print('    field3=<unknown>')
        print('}')
    def serialize_60161(netid, p: common.PacketReader):
        print('CQ_DonateToGuild {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60162(netid, p: common.PacketReader):
        print('CQ_ExtendGuildLimitedSkill {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('    field3=<unknown>')
        print('    field4=<unknown>')
        print('}')
    def serialize_60163(netid, p: common.PacketReader):
        print('CQ_BuyGuildSkill {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60165(netid, p: common.PacketReader):
        print('CQ_GetGuildRank {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u8())
        print('    field3=%d' % p.read_u8())
        print('}')
    def serialize_60166(netid, p: common.PacketReader):
        print('CQ_GetRecommendedGuildList {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u16())
        print('    field3=%d' % p.read_u8())
        print('}')
    def serialize_60168(netid, p: common.PacketReader):
        print('CQ_GetGuildRankingList {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u16())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u8())
        print('}')
    def serialize_60169(netid, p: common.PacketReader):
        print('CQ_GetGuildRanking {')
        print('    rankingType=%d' % p.read_u8())
        print('    seasonNo=%d' % p.read_u16())
        print('    unkQueryOrGuildName=%s' % p.read_wstr())
        print('}')
    
    def serialize_60170(netid, p: common.PacketReader):
        print('CQ_ChatChannelInviteFeedback {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60171(netid, p: common.PacketReader):
        print('Cl_60171 {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60172(netid, p: common.PacketReader):
        print('Cl_60172 {')
        print('    field1=<unknown>')
        print('    field2=%d' % p.read_u8())
        print('    field3=%d' % p.read_u8())
        print('    field4=%d' % p.read_u8())
        print('    field5=%d' % p.read_u8())
        print('    field6=%d' % p.read_u8())
        print('    field7=%d' % p.read_u8())
        print('    field8=<unknown>')
        print('    field9=%d' % p.read_u32())
        print('    field10=%d' % p.read_u8())
        print('}')
    def serialize_60173(netid, p: common.PacketReader):
        print('Cl_60173 {')
        print('    field1=%d' % p.read_u16())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60174(netid, p: common.PacketReader):
        print('Cl_60174 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60175(netid, p: common.PacketReader):
        print('Cl_60175 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60176(netid, p: common.PacketReader):
        print('Cl_60176 {')
        print('    field1=<unknown>')
        print('    field2=<unknown>')
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('    field5=<unknown>')
        print('    field6=<unknown>')
        print('    field7=%d' % p.read_u8())
        print('}')
    def serialize_60177(netid, p: common.PacketReader):
        print('Cl_60177 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=<unknown>')
        print('    field3=<unknown>')
        print('}')
    def serialize_60181(netid, p: common.PacketReader):
        print('CQ_SyncActionMove {')
        print('    actorID=0x%08x' % p.read_u32())
        print('    actionID=%d' % p.read_u32())
        print('    destPos=%s' % read_Vec3(p))
        print('}')
    
    def serialize_60182(netid, p: common.PacketReader):
        print('CQ_AISyncMoveMotion {')
        print('    actorID=0x%08x' % p.read_u32())
        print('    unkU32b=%d' % p.read_u32())
        print('    unkU32c=%d' % p.read_u32())
        print('    unkU32d=%d' % p.read_u32())
        print('    unkU32e=%d' % p.read_u32())
        print('    motionVec=%s' % read_Vec3(p))
        print('    unkTail1=%d' % p.read_u32())
        print('    unkTail2=%d' % p.read_u32())
        print('}')
    
    def serialize_60183(netid, p: common.PacketReader):
        print('CQ_AISyncBehaviorMotion {')
        print('    actorID=0x%08x' % p.read_u32())
        print('    behaviorID=%d' % p.read_u32())
        print('    unkU8=%d' % p.read_u8())
        print('    pos1=%s' % read_Vec3(p))
        print('    pos2=%s' % read_Vec3(p))
        print('    pos3=%s' % read_Vec3(p))
        print('    pos4=%s' % read_Vec3(p))
        print('    unkTail1=%d' % p.read_u32())
        print('    unkTail2=%d' % p.read_u32())
        print('}')
    
    def serialize_60184(netid, p: common.PacketReader):
        print('CQ_AISyncSpeedRatio {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60185(netid, p: common.PacketReader):
        print('CQ_AISyncRotateRatio {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60186(netid, p: common.PacketReader):
        print('CQ_AISyncSceneRoot {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60187(netid, p: common.PacketReader):
        print('CQ_AISyncTarPos {')
        print('    actorID=0x%08x' % p.read_u32())
        print('    unkU32=%d' % p.read_u32())
        print('    tarPos1=%s' % read_Vec3(p))
        print('    tarPos2=%s' % read_Vec3(p))
        print('    tarPos3=%s' % read_Vec3(p))
        print('    tarPos4=%s' % read_Vec3(p))
        print('    unkTail1=%d' % p.read_u32())
        print('    unkTail2=%d' % p.read_u32())
        print('    unkTail3=%d' % p.read_u32())
        print('}')
    
    def serialize_60188(netid, p: common.PacketReader):
        print('CQ_AISyncTarPosToServer {')
        print('    actorID=0x%08x' % p.read_u32())
        print('    unkU32=%d' % p.read_u32())
        print('    tarPos=%s' % read_Vec3(p))
        print('    unkVec2=%s' % read_Vec2(p))
        print('    unkTail1=%d' % p.read_u32())
        print('    unkTail2=%d' % p.read_u32())
        print('    unkTail3=%d' % p.read_u32())
        print('    unkTail4=%d' % p.read_u32())
        print('}')
    
    def serialize_60189(netid, p: common.PacketReader):
        print('CQ_RemoteSyncCreateFromCreatorId {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60191(netid, p: common.PacketReader):
        print('CQ_RemoteSyncTarget {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60193(netid, p: common.PacketReader):
        print('Cl_60193 {')
        print('    unkA=%d' % p.read_u32())
        print('    unkB=%d' % p.read_u32())
        print('    unkPos=%s' % read_Vec3(p))
        print('}')
    
    def serialize_60195(netid, p: common.PacketReader):
        print('CQ_GamePlayerSyncPhy {')
        print('    unkId1=%d' % p.read_u32())
        print('    unkId2=%d' % p.read_u32())
        print('    unkId3=%d' % p.read_u32())
        print('    currPos=%s' % read_Vec3(p))
        print('    unkShort1=%d' % p.read_u16())
        print('    unkShort2=%d' % p.read_u16())
        print('    unkShort3=%d' % p.read_u16())
        print('    unkShort4=%d' % p.read_u16())
        print('    unkShort5=%d' % p.read_u16())
        print('    unkShort6=%d' % p.read_u16())
        print('    unkShort7=%d' % p.read_u16())
        print('    unkShort8=%d' % p.read_u16())
        print('    unkShort9=%d' % p.read_u16())
        print('    charName=%s' % p.read_wstr())
        print('    unkU16=%d' % p.read_u16())
        print('    unkA=%d' % p.read_u32())
        print('    unkB=%d' % p.read_u32())
        print('    unkU16b=%d' % p.read_u16())
        print('    unkU16c=%d' % p.read_u16())
        print('    unkU16d=%d' % p.read_u16())
        print('    unkU8a=%d' % p.read_u8())
        print('    unkU8b=%d' % p.read_u8())
        print('    unkU16e=%d' % p.read_u16())
        print('    unkU8c=%d' % p.read_u8())
        print('    unkC=%d' % p.read_u32())
        print('    unkD0=%d' % p.read_u32())
        print('    unkD1=%d' % p.read_u32())
        print('    unkD2=%d' % p.read_u32())
        print('    unkD3=%d' % p.read_u32())
        print('    unkU8d=%d' % p.read_u8())
        print('    unkU8e=%d' % p.read_u8())
        print('}')
    
    def serialize_60196(netid, p: common.PacketReader):
        print('CQ_GamePlayerSyncStatus {')
        print('    unkId1=%d' % p.read_u32())
        print('    unkId2=%d' % p.read_u32())
        print('    unkId3=%d' % p.read_u32())
        print('    currPos=%s' % read_Vec3(p))
        print('    unkShort1=%d' % p.read_u16())
        print('    unkShort2=%d' % p.read_u16())
        print('    unkShort3=%d' % p.read_u16())
        print('    unkShort4=%d' % p.read_u16())
        print('    unkShort5=%d' % p.read_u16())
        print('    unkShort6=%d' % p.read_u16())
        print('    unkShort7=%d' % p.read_u16())
        print('    unkShort8=%d' % p.read_u16())
        print('    unkShort9=%d' % p.read_u16())
        print('    charName=%s' % p.read_wstr())
        print('    unkU16=%d' % p.read_u16())
        print('    unkA=%d' % p.read_u32())
        print('    unkB=%d' % p.read_u32())
        print('    unkC=%d' % p.read_u32())
        print('    unkD=%d' % p.read_u32())
        print('    unkE=%d' % p.read_u32())
        print('    unkF=%d' % p.read_u32())
        print('    unkG=%d' % p.read_u32())
        print('    unkU8a=%d' % p.read_u8())
        print('    unkU8b=%d' % p.read_u8())
        print('}')
    
    def serialize_60197(netid, p: common.PacketReader):
        print('CQ_RevivePartyMemberUseMyLifeCount {')
        bPresenceFlags = p.read_u8()
        print('    bPresenceFlags=%d' % bPresenceFlags)
        print('    unkID1=%d' % p.read_u32())
        print('    targetMemberID=%d' % p.read_u32())
        print('    revivePos=%s' % read_Vec3(p))
        if not (bPresenceFlags & 0x08):
            print('    optionalPos=%s' % read_Vec3(p))
        if not (bPresenceFlags & 0x10):
            print('    lifeCount=%d' % p.read_u32())
        print('}')
    def serialize_60198(netid, p: common.PacketReader):
        print('Cl_60198 {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60199(netid, p: common.PacketReader):
        print('CQ_SinglemodeGameHello {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60200(netid, p: common.PacketReader):
        print('CQ_SinglemodeSelectedCharacter {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60202(netid, p: common.PacketReader):
        print('Cl_60202 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60203(netid, p: common.PacketReader):
        print('Cl_60203 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60205(netid, p: common.PacketReader):
        print('Cl_60205 {')
        print('    field1=<unknown>')
        print('    field2=%d' % p.read_u32())
        print('    field3=<unknown>')
        print('    field4=%d' % p.read_u32())
        print('    field5=%s' % read_Vec3(p))
        print('}')
    def serialize_60206(netid, p: common.PacketReader):
        print('Cl_60206 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('    field5=%d' % p.read_u32())
        print('    field6=%d' % p.read_u32())
        print('    field7=%d' % p.read_u32())
        print('    field8=%d' % p.read_u32())
        print('    field9=%d' % p.read_u32())
        print('    field10=%d' % p.read_u32())
        print('    field11=%d' % p.read_u32())
        print('    field12=%d' % p.read_u32())
        print('    field13=%d' % p.read_u32())
        print('}')
    def serialize_60207(netid, p: common.PacketReader):
        print('Cl_60207 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('    field5=%d' % p.read_u32())
        print('}')
    def serialize_60208(netid, p: common.PacketReader):
        print('CQ_RewardSelectSlot {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60209(netid, p: common.PacketReader):
        print('CQ_RewardSelectSlotAllRandom {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60210(netid, p: common.PacketReader):
        print('CQ_WhisperSend {')
        print('    field1=<unknown>')
        print('    field2=<unknown>')
        print('}')
    def serialize_60211(netid, p: common.PacketReader):
        print('Cl_60211 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60214(netid, p: common.PacketReader):
        print('Cl_60214 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60218(netid, p: common.PacketReader):
        print('Cl_60218 {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60219(netid, p: common.PacketReader):
        print('CQ_MailWrite {')
        print('    field1=<unknown>')
        print('    field2=<unknown>')
        print('    field3=<unknown>')
        print('}')
    def serialize_60220(netid, p: common.PacketReader):
        print('CQ_MailRead {')
        print('    field1=%d' % p.read_i64())
        print('}')
    def serialize_60221(netid, p: common.PacketReader):
        print('CQ_MailMove {')
        print('    field1=%d' % p.read_i64())
        print('}')
    def serialize_60222(netid, p: common.PacketReader):
        print('CQ_MailGetAttachmentMulti {')
        n = p.read_u16()
        print('    attachmentUIDs_count=%d' % n)
        for _ in range(n):
            print('        0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('}')
    
    def serialize_60223(netid, p: common.PacketReader):
        print('CQ_MailGetNewMail {')
        print('    unkFlag=%d' % p.read_u8())
        n = p.read_u16()
        print('    mailUIDs_count=%d' % n)
        for _ in range(n):
            print('        0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('}')
    
    def serialize_60226(netid, p: common.PacketReader):
        print('CQ_WarehouseItemDelete {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60227(netid, p: common.PacketReader):
        print('Cl_60227 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60229(netid, p: common.PacketReader):
        print('Cl_60229 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60230(netid, p: common.PacketReader):
        print('Cl_60230 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60232(netid, p: common.PacketReader):
        print('Cl_60232 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('}')
    def serialize_60233(netid, p: common.PacketReader):
        print('Cl_60233 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%s' % read_Vec3(p))
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('}')
    def serialize_60236(netid, p: common.PacketReader):
        print('CQ_SortieMasterBan {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60237(netid, p: common.PacketReader):
        print('CQ_SortieMasterAssign {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60239(netid, p: common.PacketReader):
        print('Cl_60239 {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60240(netid, p: common.PacketReader):
        print('Cl_60240 {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60241(netid, p: common.PacketReader):
        print('Cl_60241 {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60242(netid, p: common.PacketReader):
        print('Cl_60242 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60243(netid, p: common.PacketReader):
        print('Cl_60243 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60244(netid, p: common.PacketReader):
        print('CN_Ping {')
        print('    rttMs=%d' % p.read_u32())
        print('}')
    def serialize_60246(netid, p: common.PacketReader):
        print('CQ_CompleteNonSyncEvents {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60247(netid, p: common.PacketReader):
        print('CQ_InteractionCasting {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60248(netid, p: common.PacketReader):
        print('CQ_InteractionCancel {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60249(netid, p: common.PacketReader):
        print('CQ_PlayerSyncTeleport {')
        print('    objectID=0x%08x' % p.read_u32())
        print('    pos=%s' % read_Vec3(p))
        print('}')
    
    def serialize_60250(netid, p: common.PacketReader):
        print('CQ_SaveSettings {')
        print('    unkFlag=%d' % p.read_u8())
        n = p.read_u16()
        print('    settingsData_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        print('}')
    
    def serialize_60252(netid, p: common.PacketReader):
        print('CQ_ItemTrade {')
        print('    unkContextId=%d' % p.read_u32())
        print('    itemDocId=0x%08x' % p.read_u32())
        print('    count=%d' % p.read_u8())
        print('}')
    def serialize_60253(netid, p: common.PacketReader):
        print('CQ_SkinChipDissolve {')
        print('    unkZero=%d' % p.read_u8())
        print('    chipItemDocId=0x%08x' % p.read_u32())
        print('    craftCount=%d' % p.read_u8())
        print('}')
    def serialize_60254(netid, p: common.PacketReader):
        print('CQ_OlympicBestRecord {')
        print('    unkFlag=%d' % p.read_u8())
        n = p.read_u16()
        print('    records_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60259(netid, p: common.PacketReader):
        print('Cl_60259 {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60260(netid, p: common.PacketReader):
        print('Cl_60260 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60261(netid, p: common.PacketReader):
        print('Cl_60261 {')
        print('    text=%s' % p.read_wstr())
        print('}')
    
    def serialize_60262(netid, p: common.PacketReader):
        print('Cl_60262 {')
        print('    text=%s' % p.read_wstr())
        print('    category=%d' % p.read_u8())
        print('}')
    
    def serialize_60263(netid, p: common.PacketReader):
        print('Cl_60263 {')
        print('    text=%s' % p.read_wstr())
        print('}')
    
    def serialize_60266(netid, p: common.PacketReader):
        print('Cl_60266 {')
        print('    field1=%d' % p.read_u16())
        print('    field2=%d' % p.read_u16())
        print('    field3=%d' % p.read_u16())
        print('}')
    def serialize_60268(netid, p: common.PacketReader):
        print('CQ_RequestVoting {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60269(netid, p: common.PacketReader):
        print('Cl_60269 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60270(netid, p: common.PacketReader):
        print('CQ_Vote {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60273(netid, p: common.PacketReader):
        print('CQ_CalendarDetail {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60274(netid, p: common.PacketReader):
        print('CQ_StageSkillSelect {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60276(netid, p: common.PacketReader):
        print('CQ_NP_CharacterSkinUnlock {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60277(netid, p: common.PacketReader):
        print('CQ_NP_WarehouseExpand {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60278(netid, p: common.PacketReader):
        print('CQ_NP_InventoryExpand {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60279(netid, p: common.PacketReader):
        print('CQ_NP_ReviveByCoin {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60280(netid, p: common.PacketReader):
        print('CQ_NP_RewardBuyChance {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60281(netid, p: common.PacketReader):
        print('CQ_NP_RewardOpenAllSlots {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60282(netid, p: common.PacketReader):
        print('CQ_BuyGift {')
        print('    field1=%d' % p.read_u8())
        print('}')
    def serialize_60283(netid, p: common.PacketReader):
        print('CQ_MasterTrainingEvent {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60284(netid, p: common.PacketReader):
        print('CQ_PromotionAttend {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60285(netid, p: common.PacketReader):
        print('CQ_CharacterPropertyUpgrade {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60286(netid, p: common.PacketReader):
        print('CQ_CharacterPropertyReset {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60287(netid, p: common.PacketReader):
        print('CQ_ChatGetInviteeGameAccountID {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60288(netid, p: common.PacketReader):
        print('CQ_UseCoupon {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60289(netid, p: common.PacketReader):
        print('CQ_NotifyChattingChannelStatus {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60290(netid, p: common.PacketReader):
        print('CQ_RequestToken {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60291(netid, p: common.PacketReader):
        print('Cl_60291 {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60292(netid, p: common.PacketReader):
        print('Cl_60292 {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60294(netid, p: common.PacketReader):
        print('Cl_60294 {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60295(netid, p: common.PacketReader):
        print('CQ_TirednessRequest {')
        print('    unkDword1=%d' % p.read_u32())
        print('    unkDword2=%d' % p.read_u32())
        print('    unkQword=0x%016x' % int.from_bytes(p.read_raw(8), 'little'))
        print('    unkWord1=%d' % p.read_u16())
        print('    unkDword3=%d' % p.read_u32())
        print('    unkDword4=%d' % p.read_u32())
        print('    unkDword5=%d' % p.read_u32())
        print('    unkDword6=%d' % p.read_u32())
        print('    unkDword7=%d' % p.read_u32())
        print('    unkDword8=%d' % p.read_u32())
        print('    unkWstr1=%s' % p.read_wstr())
        print('    unkWstr2=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkTripleVec_count=%d' % n)
        for _ in range(n):
            print('        (%d, %d, %d)' % (p.read_u32(), p.read_u32(), p.read_u32()))
        print('}')
    
    def serialize_60296(netid, p: common.PacketReader):
        print('Cl_60296 {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('    field5=<unknown>')
        print('}')
    def serialize_60297(netid, p: common.PacketReader):
        print('Cl_60297 {')
        print('    field1=%d' % p.read_u16())
        print('    field2=%d' % p.read_u16())
        print('}')
    def serialize_60298(netid, p: common.PacketReader):
        print('CQ_MasterSupplyRequest {')
        print('    masterActorID=%d' % p.read_u32())
        n = p.read_u16()
        print('    supplies_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u8())
        print('}')
    def serialize_60299(netid, p: common.PacketReader):
        print('Cl_60299 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60301(netid, p: common.PacketReader):
        print('Cl_60301 {')
        print('    unkStr=%s' % p.read_str())
        n = p.read_u16()
        print('    unkStructVec_count=%d' % n)
        for _ in range(n):
            print('        ' + p.read_raw(8).hex())
        print('}')
    
    def serialize_60302(netid, p: common.PacketReader):
        print('CN_NotifyMyVoiceStatus {')
        for i in range(1, 7):
            print('    voiceStr%d=%s' % (i, p.read_wstr()))
        print('    voiceKind=%d' % p.read_u32())
        print('}')
    def serialize_60304(netid, p: common.PacketReader):
        print('Cl_60304 {')
        print('    field1=<unknown>')
        print('    field2=%d' % p.read_u8())
        print('    field3=<unknown>')
        print('}')
    def serialize_60306(netid, p: common.PacketReader):
        print('Cl_60306 {')
        print('    unkValue=%d' % p.read_u32())
        print('}')
    
    def serialize_60308(netid, p: common.PacketReader):
        print('Cl_60308 {')
        print('    field1=<unknown>')
        print('}')
    def serialize_60312(netid, p: common.PacketReader):
        print('Cl_60312 {')
        print('    unkWstr=%s' % p.read_wstr())
        n = p.read_u16()
        print('    unkEntryVec_count=%d' % n)
        for _ in range(n):
            print('        ' + p.read_raw(96).hex())
        print('}')
    
    def serialize_60313(netid, p: common.PacketReader):
        print('Cl_60313 {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=<unknown>')
        print('    field3=%s' % p.read_wstr())
        print('    field4=%s' % p.read_wstr())
        print('    field5=<unknown>')
        print('}')
    def serialize_60011(netid, p: common.PacketReader):
        print('Cl_60011 {')
        print('}')
    def serialize_60013(netid, p: common.PacketReader):
        print('CN_EnterCityChannelForce {')
        print('}')
    def serialize_60019(netid, p: common.PacketReader):
        print('CA_PrepareHandoverToChannel {')
        print('}')
    def serialize_60021(netid, p: common.PacketReader):
        print('CN_ReadyToLoadGameMap {')
        print('}')
    def serialize_60025(netid, p: common.PacketReader):
        print('CQ_ReturnToGame {')
        print('}')
    def serialize_60026(netid, p: common.PacketReader):
        print('CQ_ReturnToCity {')
        print('}')
    def serialize_60027(netid, p: common.PacketReader):
        print('CQ_HandoverToChannel {')
        print('}')
    def serialize_60035(netid, p: common.PacketReader):
        print('ConfirmLogin {')
        print('}')
    def serialize_60043(netid, p: common.PacketReader):
        print('CQ_SwitchOffToggleSkill {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60045(netid, p: common.PacketReader):
        print('CQ_RequestSummaryInfoEach {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60046(netid, p: common.PacketReader):
        print('CQ_RequestSummaryReward {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('    field3=%d' % p.read_u32())
        print('}')
    def serialize_60048(netid, p: common.PacketReader):
        print('Cl_60048 {')
        n = p.read_u16()
        print('    unkBlob_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        print('}')
    
    def serialize_60050(netid, p: common.PacketReader):
        print('CheckDupNickname {')
        print('    nickname=%s' % p.read_wstr())
        print('}')
    
    def serialize_60057(netid, p: common.PacketReader):
        print('Cl_60057 {')
        print('    field1=%d' % p.read_u8())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60058(netid, p: common.PacketReader):
        print('Cl_60058 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60059(netid, p: common.PacketReader):
        print('CQ_RoomEquipWeapon {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60065(netid, p: common.PacketReader):
        print('CQ_MasterGearRepair {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60066(netid, p: common.PacketReader):
        print('CQ_MasterGearRename {')
        print('    gearSlot=%d' % p.read_u8())
        print('    newName=%s' % p.read_wstr())
        print('}')
    
    def serialize_60067(netid, p: common.PacketReader):
        print('CQ_MasterGearAdd {')
        print('}')
    def serialize_60099(netid, p: common.PacketReader):
        print('CQ_CancelMatchingTrollPenalty {')
        print('}')
    def serialize_60114(netid, p: common.PacketReader):
        print('CN_ChannelChatMessage {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%s' % p.read_wstr())
        print('}')
    def serialize_60117(netid, p: common.PacketReader):
        print('CQ_FriendStateChange {')
        print('    field1=%s' % p.read_wstr())
        print('    field2=%d' % p.read_u8())
        print('}')
    def serialize_60118(netid, p: common.PacketReader):
        print('Cl_60118 {')
        print('    field1=%d' % p.read_u32())
        print('}')
    def serialize_60131(netid, p: common.PacketReader):
        print('Cl_60131 {')
        print('}')
    def serialize_60134(netid, p: common.PacketReader):
        print('CQ_DissolveGuild {')
        print('}')
    def serialize_60135(netid, p: common.PacketReader):
        print('Cl_60135 {')
        print('}')
    def serialize_60138(netid, p: common.PacketReader):
        print('CQ_QuitGuild {')
        print('}')
    def serialize_60140(netid, p: common.PacketReader):
        print('CQ_GetGuildJoinRequestList {')
        print('}')
    def serialize_60143(netid, p: common.PacketReader):
        print('CQ_GetGuildInvitationList {')
        print('}')
    def serialize_60147(netid, p: common.PacketReader):
        print('CQ_GetGuildMemberStatusList {')
        print('}')
    def serialize_60164(netid, p: common.PacketReader):
        print('CQ_GetGuildRankList {')
        print('}')
    def serialize_60201(netid, p: common.PacketReader):
        print('Cl_60201 {')
        print('}')
    def serialize_60204(netid, p: common.PacketReader):
        print('Cl_60204 {')
        print('}')
    def serialize_60212(netid, p: common.PacketReader):
        print('Cl_60212 {')
        print('}')
    def serialize_60213(netid, p: common.PacketReader):
        print('Cl_60213 {')
        print('}')
    def serialize_60215(netid, p: common.PacketReader):
        print('Cl_60215 {')
        print('}')
    def serialize_60216(netid, p: common.PacketReader):
        print('Cl_60216 {')
        print('}')
    def serialize_60217(netid, p: common.PacketReader):
        print('Cl_60217 {')
        print('}')
    def serialize_60224(netid, p: common.PacketReader):
        print('Cl_60224 {')
        print('}')
    def serialize_60225(netid, p: common.PacketReader):
        print('Cl_60225 {')
        print('}')
    def serialize_60228(netid, p: common.PacketReader):
        print('Cl_60228 {')
        print('}')
    def serialize_60231(netid, p: common.PacketReader):
        print('Cl_60231 {')
        print('}')
    def serialize_60234(netid, p: common.PacketReader):
        print('CA_Heartbeat {')
        print('}')
    def serialize_60238(netid, p: common.PacketReader):
        print('Cl_60238 {')
        print('}')
    def serialize_60251(netid, p: common.PacketReader):
        print('CQ_RefreshWaitingQueue {')
        print('}')
    def serialize_60255(netid, p: common.PacketReader):
        print('CQ_PveRefillBattery {')
        print('}')
    def serialize_60256(netid, p: common.PacketReader):
        print('Cl_60256 {')
        print('}')
    def serialize_60257(netid, p: common.PacketReader):
        print('Cl_60257 {')
        print('}')
    def serialize_60264(netid, p: common.PacketReader):
        print('Cl_60264 {')
        print('}')
    def serialize_60267(netid, p: common.PacketReader):
        print('Cl_60267 {')
        print('}')
    def serialize_60271(netid, p: common.PacketReader):
        print('Cl_60271 {')
        print('}')
    def serialize_60293(netid, p: common.PacketReader):
        print('Cl_60293 {')
        print('}')
    def serialize_60303(netid, p: common.PacketReader):
        print('Cl_60303 {')
        print('}')
    def serialize_60305(netid, p: common.PacketReader):
        print('Cl_60305 {')
        print('}')
    def serialize_60307(netid, p: common.PacketReader):
        print('Cl_60307 {')
        print('}')
    def serialize_60309(netid, p: common.PacketReader):
        print('Cl_60309 {')
        print('}')
    def serialize_60310(netid, p: common.PacketReader):
        print('Cl_60310 {')
        print('}')
    def serialize_60311(netid, p: common.PacketReader):
        print('Cl_60311 {')
        print('}')
    def serialize_60018(netid, p: common.PacketReader):
        print('Cl_60018 {')
        print('}')
    def serialize_60063(netid, p: common.PacketReader):
        print('Cl_60063 {')
        print('    unkID=%d' % p.read_u32())
        n = p.read_u16()
        print('    unkBlob_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        n = p.read_u16()
        print('    unkList_count=%d' % n)
        for _ in range(n):
            print('        %d' % p.read_u32())
        print('}')
    
    def serialize_60064(netid, p: common.PacketReader):
        print('Cl_60064 {')
        print('    unkID=%d' % p.read_u32())
        n = p.read_u16()
        print('    unkBlob_count=%d' % n)
        for _ in range(n):
            print('        %02x' % p.read_u8())
        print('}')
    
    def serialize_60136(netid, p: common.PacketReader):
        print('Cl_60136 {')
        print('}')
    def serialize_60190(netid, p: common.PacketReader):
        print('CQ_RemoteSyncCreateFromRemoteDoc {')
        print('    field1=%d' % p.read_u32())
        print('    field2=<unknown>')
        print('}')
    def serialize_60192(netid, p: common.PacketReader):
        print('CQ_RemoteSyncForecastCollision {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60194(netid, p: common.PacketReader):
        print('Cl_60194 {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%s' % p.read_wstr())
        print('    field3=%d' % p.read_u32())
        print('    field4=%d' % p.read_u32())
        print('}')
    def serialize_60258(netid, p: common.PacketReader):
        print('Cl_60258 {')
        print('}')
    def serialize_60275(netid, p: common.PacketReader):
        print('CQ_NP_PVERefillBattery {')
        print('    field1=%d' % p.read_u32())
        print('    field2=%d' % p.read_u32())
        print('}')
    def serialize_60300(netid, p: common.PacketReader):
        print('Cl_60300 {')
        print('    field1=%s' % p.read_wstr())
        print('}')
