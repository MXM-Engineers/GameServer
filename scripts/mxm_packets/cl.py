# client packets
from . import common

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)
    
class ClientSerializer:

    def serialize_60002(netid, p: common.PacketReader):
        print('CQ_FirstHello {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u8=%d' % p.read_u8())
        print('}')
    def serialize_60008(netid, p: common.PacketReader):
        print('CQ_Authenticate {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60009(netid, p: common.PacketReader):
        print('CQ_AuthenticateGameServer {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u8=%d' % p.read_u8())
        print('}')
    def serialize_60014(netid, p: common.PacketReader):
        print('CN_ReadyToLoadCharacter {}')
    def serialize_60016(netid, p: common.PacketReader):
        print('CN_GameMapLoaded {}')
    def serialize_60024(netid, p: common.PacketReader):
        print('CA_SetGameGvt {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60022(netid, p: common.PacketReader):
        print('CN_UpdatePosition {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec3=<unknown type vec3>')
        print('    field3_vec3=<unknown type vec3>')
        print('    field4_vec3=<unknown type vec3>')
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60032(netid, p: common.PacketReader):
        print('CQ_GameIsReady {}')
    def serialize_60033(netid, p: common.PacketReader):
        print('CQ_LoadingComplete {}')
    def serialize_60034(netid, p: common.PacketReader):
        print('CN_MapIsLoaded {}')
    def serialize_60038(netid, p: common.PacketReader):
        print('CN_PlayerTagCompleted {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60040(netid, p: common.PacketReader):
        print('CQ_PlayerCastSkill {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec3=<unknown type vec3>')
        print('    field4_vec_u16<u32>=<unknown type vec_u16<u32>>')
        print('    field5_13xdword=<unknown type 13xdword>')
        print('}')
    def serialize_60051(netid, p: common.PacketReader):
        print('CQ_GetCharacterInfo {}')
    def serialize_60053(netid, p: common.PacketReader):
        print('CQ_GamePlayerTag {}')
    def serialize_60073(netid, p: common.PacketReader):
        print('CQ_RequestAreaPopularity {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32[3]=<unknown type u32[3]>')
        print('    field4_u16 count + N x u32=<unknown type u16 count + N x u32>')
        print('    field5_u32[13] struct=<unknown type u32[13] struct>')
        print('}')
    def serialize_60074(netid, p: common.PacketReader):
        print('CQ_PartyCreate {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60075(netid, p: common.PacketReader):
        print('CQ_PartyModify {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60076(netid, p: common.PacketReader):
        print('CQ_PartyOptionModify {}')
    def serialize_60081(netid, p: common.PacketReader):
        print('Cl_60081 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60089(netid, p: common.PacketReader):
        print('CA_SortieRoomFound {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60097(netid, p: common.PacketReader):
        print('CQ_EnqueueGame {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60100(netid, p: common.PacketReader):
        print('CQ_MasterPick {}')
    def serialize_60101(netid, p: common.PacketReader):
        print('CQ_MasterUnpick {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60113(netid, p: common.PacketReader):
        print('CQ_PlayerJump {}')
    def serialize_60145(netid, p: common.PacketReader):
        print('CQ_GetGuildProfile {}')
    def serialize_60146(netid, p: common.PacketReader):
        print('CQ_GetGuildMemberList {}')
    def serialize_60148(netid, p: common.PacketReader):
        print('CQ_GetGuildHistoryList {}')
    def serialize_60151(netid, p: common.PacketReader):
        print('CQ_TierRecord {}')
    def serialize_60167(netid, p: common.PacketReader):
        print('CQ_GetGuildRankingSeasonList {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60178(netid, p: common.PacketReader):
        print('CN_GameUpdatePosition {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field3_vec2 (2 x u32)=<unknown type vec2 (2 x u32)>')
        print('    field4_vec2 (2 x u32)=<unknown type vec2 (2 x u32)>')
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u8=%d' % p.read_u8())
        print('    field8_u32=%d' % p.read_u32())
        print('    field9_u32=%d' % p.read_u32())
        print('    field10_u8=%d' % p.read_u8())
        print('}')
    def serialize_60179(netid, p: common.PacketReader):
        print('CN_GameUpdateRotation {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec2 (2 x u32)=<unknown type vec2 (2 x u32)>')
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60180(netid, p: common.PacketReader):
        print('Cl_60180 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('    field4_u8=%d' % p.read_u8())
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('}')
    def serialize_60235(netid, p: common.PacketReader):
        print('CQ_LoadingProgressData {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60245(netid, p: common.PacketReader):
        print('CQ_RTT_Time {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60265(netid, p: common.PacketReader):
        print('Cl_60265 {')
        print('    field1_blob[u8]=<unknown type blob[u8]>')
        print('}')
    def serialize_60272(netid, p: common.PacketReader):
        print('CQ_RequestCalendar {')
        print('    field1_u64/raw8=<unknown type u64/raw8>')
        print('}')
    def serialize_60003(netid, p: common.PacketReader):
        print('CQ_UserLogin {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_wstr="%s"' % p.read_wstr())
        print('    field3_wstr="%s"' % p.read_wstr())
        print('    field4_wstr="%s"' % p.read_wstr())
        print('    field5_u32=%d' % p.read_u32())
        print('}')
    def serialize_60004(netid, p: common.PacketReader):
        print('Cl_60004 {')
        print('    field1_str="%s"' % p.read_str())
        print('    field2_str="%s"' % p.read_str())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60005(netid, p: common.PacketReader):
        print('ConfirmGatewayInfo {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60006(netid, p: common.PacketReader):
        print('Cl_60006 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec_u16<{u32,u16}>=<unknown type vec_u16<{u32,u16}>>')
        print('    field3_vec_u16<{u32,u16,u16,u32}>=<unknown type vec_u16<{u32,u16,u16,u32}>>')
        print('}')
    def serialize_60007(netid, p: common.PacketReader):
        print('EnterQueue {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec_u16<{u32,u16}>=<unknown type vec_u16<{u32,u16}>>')
        print('    field4_vec_u16<{u32,u16,u16,u32}>=<unknown type vec_u16<{u32,u16,u16,u32}>>')
        print('}')
    def serialize_60010(netid, p: common.PacketReader):
        print('Cl_60010 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60012(netid, p: common.PacketReader):
        print('Cl_60012 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60015(netid, p: common.PacketReader):
        print('Cl_60015 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60017(netid, p: common.PacketReader):
        print('Cl_60017 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec_u16<{u32,u32}>=<unknown type vec_u16<{u32,u32}>>')
        print('}')
    def serialize_60020(netid, p: common.PacketReader):
        print('Cl_60020 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60023(netid, p: common.PacketReader):
        print('CN_GamePlayerSyncActionStateOnly {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60028(netid, p: common.PacketReader):
        print('Cl_60028 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_wstr="%s"' % p.read_wstr())
        print('    field3_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60029(netid, p: common.PacketReader):
        print('Cl_60029 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60030(netid, p: common.PacketReader):
        print('Cl_60030 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60031(netid, p: common.PacketReader):
        print('Cl_60031 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60036(netid, p: common.PacketReader):
        print('Cl_60036 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60037(netid, p: common.PacketReader):
        print('Cl_60037 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60039(netid, p: common.PacketReader):
        print('Cl_60039 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_13xdword=<unknown type 13xdword>')
        print('}')
    def serialize_60041(netid, p: common.PacketReader):
        print('Cl_60041 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u8=%d' % p.read_u8())
        print('}')
    def serialize_60042(netid, p: common.PacketReader):
        print('Cl_60042 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60044(netid, p: common.PacketReader):
        print('Cl_60044 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60047(netid, p: common.PacketReader):
        print('Cl_60047 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60049(netid, p: common.PacketReader):
        print('SetNickname {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u16 count + N x {u32,u32}=<unknown type u16 count + N x {u32,u32}>')
        print('}')
    def serialize_60052(netid, p: common.PacketReader):
        print('CQ_SetLeaderCharacter {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60054(netid, p: common.PacketReader):
        print('Cl_60054 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32[3]=<unknown type u32[3]>')
        print('    field3_u32[3]=<unknown type u32[3]>')
        print('    field4_u32[3]=<unknown type u32[3]>')
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60055(netid, p: common.PacketReader):
        print('Cl_60055 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60056(netid, p: common.PacketReader):
        print('Cl_60056 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60060(netid, p: common.PacketReader):
        print('Cl_60060 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_wstr="%s"' % p.read_wstr())
        print('    field3_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60061(netid, p: common.PacketReader):
        print('Cl_60061 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60062(netid, p: common.PacketReader):
        print('Cl_60062 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60068(netid, p: common.PacketReader):
        print('Cl_60068 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60069(netid, p: common.PacketReader):
        print('Cl_60069 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60070(netid, p: common.PacketReader):
        print('Cl_60070 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60071(netid, p: common.PacketReader):
        print('CQ_RoomEquipSkill {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32[13] struct=<unknown type u32[13] struct>')
        print('}')
    def serialize_60072(netid, p: common.PacketReader):
        print('CQ_RoomSwapSkill {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32[3]=<unknown type u32[3]>')
        print('    field4_u16 count + N x u32=<unknown type u16 count + N x u32>')
        print('    field5_u32[13] struct=<unknown type u32[13] struct>')
        print('}')
    def serialize_60077(netid, p: common.PacketReader):
        print('Cl_60077 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60078(netid, p: common.PacketReader):
        print('Cl_60078 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60079(netid, p: common.PacketReader):
        print('Cl_60079 {')
        print('    field1_u16 count + N x u8=<unknown type u16 count + N x u8>')
        print('}')
    def serialize_60080(netid, p: common.PacketReader):
        print('Cl_60080 {')
        print('    field1_u16 count + N x u8=<unknown type u16 count + N x u8>')
        print('}')
    def serialize_60082(netid, p: common.PacketReader):
        print('Cl_60082 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60083(netid, p: common.PacketReader):
        print('Cl_60083 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60084(netid, p: common.PacketReader):
        print('Cl_60084 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60085(netid, p: common.PacketReader):
        print('Cl_60085 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60086(netid, p: common.PacketReader):
        print('Cl_60086 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60087(netid, p: common.PacketReader):
        print('Cl_60087 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60088(netid, p: common.PacketReader):
        print('Cl_60088 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60090(netid, p: common.PacketReader):
        print('CN_SortieRoomConfirm {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60091(netid, p: common.PacketReader):
        print('Cl_60091 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60092(netid, p: common.PacketReader):
        print('Cl_60092 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60093(netid, p: common.PacketReader):
        print('Cl_60093 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60094(netid, p: common.PacketReader):
        print('Cl_60094 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_u8=%d' % p.read_u8())
        print('}')
    def serialize_60095(netid, p: common.PacketReader):
        print('Cl_60095 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_blob(u16 len + bytes)=<unknown type blob(u16 len + bytes)>')
        print('    field3_array<u32>(u16 count + N x u32)=<unknown type array<u32>(u16 count + N x u32)>')
        print('}')
    def serialize_60096(netid, p: common.PacketReader):
        print('Cl_60096 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_blob(u16 len + bytes)=<unknown type blob(u16 len + bytes)>')
        print('}')
    def serialize_60098(netid, p: common.PacketReader):
        print('Cl_60098 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_wstring=<unknown type wstring>')
        print('}')
    def serialize_60102(netid, p: common.PacketReader):
        print('CQ_MasterReset {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60103(netid, p: common.PacketReader):
        print('CQ_ReadySortieRoom {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60104(netid, p: common.PacketReader):
        print('Cl_60104 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60105(netid, p: common.PacketReader):
        print('Cl_60105 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60106(netid, p: common.PacketReader):
        print('Cl_60106 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60107(netid, p: common.PacketReader):
        print('Cl_60107 {')
        print('    field1_array<u32>(u16 count + N x u32) + u32 + u32 + u8=<unknown type array<u32>(u16 count + N x u32) + u32 + u32 + u8>')
        print('}')
    def serialize_60108(netid, p: common.PacketReader):
        print('Cl_60108 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60109(netid, p: common.PacketReader):
        print('Cl_60109 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u8=%d' % p.read_u8())
        print('}')
    def serialize_60110(netid, p: common.PacketReader):
        print('Cl_60110 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60111(netid, p: common.PacketReader):
        print('Cl_60111 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60112(netid, p: common.PacketReader):
        print('Cl_60112 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60115(netid, p: common.PacketReader):
        print('Cl_60115 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60116(netid, p: common.PacketReader):
        print('Cl_60116 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60119(netid, p: common.PacketReader):
        print('Cl_60119 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60120(netid, p: common.PacketReader):
        print('Cl_60120 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60121(netid, p: common.PacketReader):
        print('Cl_60121 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60122(netid, p: common.PacketReader):
        print('Cl_60122 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60123(netid, p: common.PacketReader):
        print('Cl_60123 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60124(netid, p: common.PacketReader):
        print('Cl_60124 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60125(netid, p: common.PacketReader):
        print('Cl_60125 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60126(netid, p: common.PacketReader):
        print('Cl_60126 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60127(netid, p: common.PacketReader):
        print('Cl_60127 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60128(netid, p: common.PacketReader):
        print('Cl_60128 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60129(netid, p: common.PacketReader):
        print('Cl_60129 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60130(netid, p: common.PacketReader):
        print('CQ_JukeboxQueueSong {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60132(netid, p: common.PacketReader):
        print('Cl_60132 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60133(netid, p: common.PacketReader):
        print('Cl_60133 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60137(netid, p: common.PacketReader):
        print('Cl_60137 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60139(netid, p: common.PacketReader):
        print('Cl_60139 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60141(netid, p: common.PacketReader):
        print('Cl_60141 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60142(netid, p: common.PacketReader):
        print('Cl_60142 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60144(netid, p: common.PacketReader):
        print('Cl_60144 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60149(netid, p: common.PacketReader):
        print('Cl_60149 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60150(netid, p: common.PacketReader):
        print('Cl_60150 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60152(netid, p: common.PacketReader):
        print('Cl_60152 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60153(netid, p: common.PacketReader):
        print('Cl_60153 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60154(netid, p: common.PacketReader):
        print('Cl_60154 {')
        print('    field1_wstr="%s"' % p.read_wstr())
        print('}')
    def serialize_60155(netid, p: common.PacketReader):
        print('Cl_60155 {')
        print('    field1_bytes[7]=<unknown type bytes[7]>')
        print('}')
    def serialize_60156(netid, p: common.PacketReader):
        print('Cl_60156 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60157(netid, p: common.PacketReader):
        print('Cl_60157 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60158(netid, p: common.PacketReader):
        print('Cl_60158 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('}')
    def serialize_60159(netid, p: common.PacketReader):
        print('Cl_60159 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60160(netid, p: common.PacketReader):
        print('Cl_60160 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_wstring=<unknown type wstring>')
        print('    field3_10 x u8 struct=<unknown type 10 x u8 struct>')
        print('}')
    def serialize_60161(netid, p: common.PacketReader):
        print('Cl_60161 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60162(netid, p: common.PacketReader):
        print('Cl_60162 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_wstring=<unknown type wstring>')
        print('    field4_10 x u8 struct=<unknown type 10 x u8 struct>')
        print('}')
    def serialize_60163(netid, p: common.PacketReader):
        print('Cl_60163 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60165(netid, p: common.PacketReader):
        print('Cl_60165 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_u8=%d' % p.read_u8())
        print('}')
    def serialize_60166(netid, p: common.PacketReader):
        print('Cl_60166 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u16=%d' % p.read_u16())
        print('    field3_u8=%d' % p.read_u8())
        print('}')
    def serialize_60168(netid, p: common.PacketReader):
        print('Cl_60168 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u16=%d' % p.read_u16())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u8=%d' % p.read_u8())
        print('}')
    def serialize_60169(netid, p: common.PacketReader):
        print('Cl_60169 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u16=%d' % p.read_u16())
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60170(netid, p: common.PacketReader):
        print('Cl_60170 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60171(netid, p: common.PacketReader):
        print('Cl_60171 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60172(netid, p: common.PacketReader):
        print('Cl_60172 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_u8=%d' % p.read_u8())
        print('    field4_u8=%d' % p.read_u8())
        print('    field5_u8=%d' % p.read_u8())
        print('    field6_u8=%d' % p.read_u8())
        print('    field7_u8=%d' % p.read_u8())
        print('    field8_7 x u8 struct=<unknown type 7 x u8 struct>')
        print('    field9_u32=%d' % p.read_u32())
        print('    field10_u8=%d' % p.read_u8())
        print('}')
    def serialize_60173(netid, p: common.PacketReader):
        print('Cl_60173 {')
        print('    field1_u16=%d' % p.read_u16())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60174(netid, p: common.PacketReader):
        print('Cl_60174 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60175(netid, p: common.PacketReader):
        print('Cl_60175 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60176(netid, p: common.PacketReader):
        print('Cl_60176 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_wstring=<unknown type wstring>')
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_wstring=<unknown type wstring>')
        print('    field6_wstring=<unknown type wstring>')
        print('    field7_u8=%d' % p.read_u8())
        print('}')
    def serialize_60177(netid, p: common.PacketReader):
        print('Cl_60177 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_wstring=<unknown type wstring>')
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60181(netid, p: common.PacketReader):
        print('Cl_60181 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('}')
    def serialize_60182(netid, p: common.PacketReader):
        print('Cl_60182 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60183(netid, p: common.PacketReader):
        print('Cl_60183 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field6_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field7_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field8_u32=%d' % p.read_u32())
        print('    field9_u32=%d' % p.read_u32())
        print('}')
    def serialize_60184(netid, p: common.PacketReader):
        print('Cl_60184 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60185(netid, p: common.PacketReader):
        print('Cl_60185 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60186(netid, p: common.PacketReader):
        print('Cl_60186 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60187(netid, p: common.PacketReader):
        print('Cl_60187 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field6_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('    field9_u32=%d' % p.read_u32())
        print('}')
    def serialize_60188(netid, p: common.PacketReader):
        print('Cl_60188 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('}')
    def serialize_60189(netid, p: common.PacketReader):
        print('Cl_60189 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60191(netid, p: common.PacketReader):
        print('Cl_60191 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60193(netid, p: common.PacketReader):
        print('Cl_60193 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('}')
    def serialize_60195(netid, p: common.PacketReader):
        print('Cl_60195 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_u16=%d' % p.read_u16())
        print('    field6_u16=%d' % p.read_u16())
        print('    field7_u16=%d' % p.read_u16())
        print('    field8_u16=%d' % p.read_u16())
        print('    field9_u16=%d' % p.read_u16())
        print('    field10_u16=%d' % p.read_u16())
        print('    field11_u16=%d' % p.read_u16())
        print('    field12_u16=%d' % p.read_u16())
        print('    field13_u16=%d' % p.read_u16())
        print('    field14_wstring=<unknown type wstring>')
        print('    field15_u16=%d' % p.read_u16())
        print('    field16_u32=%d' % p.read_u32())
        print('    field17_u32=%d' % p.read_u32())
        print('    field18_u16=%d' % p.read_u16())
        print('    field19_u16=%d' % p.read_u16())
        print('    field20_u16=%d' % p.read_u16())
        print('    field21_u8=%d' % p.read_u8())
        print('    field22_u8=%d' % p.read_u8())
        print('    field23_u16=%d' % p.read_u16())
        print('    field24_u8=%d' % p.read_u8())
        print('    field25_u32=%d' % p.read_u32())
        print('    field26_u32=%d' % p.read_u32())
        print('    field27_u32=%d' % p.read_u32())
        print('    field28_u32=%d' % p.read_u32())
        print('    field29_u32=%d' % p.read_u32())
        print('    field30_u8=%d' % p.read_u8())
        print('    field31_u8=%d' % p.read_u8())
        print('}')
    def serialize_60196(netid, p: common.PacketReader):
        print('Cl_60196 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_u16=%d' % p.read_u16())
        print('    field6_u16=%d' % p.read_u16())
        print('    field7_u16=%d' % p.read_u16())
        print('    field8_u16=%d' % p.read_u16())
        print('    field9_u16=%d' % p.read_u16())
        print('    field10_u16=%d' % p.read_u16())
        print('    field11_u16=%d' % p.read_u16())
        print('    field12_u16=%d' % p.read_u16())
        print('    field13_u16=%d' % p.read_u16())
        print('    field14_wstring=<unknown type wstring>')
        print('    field15_u16=%d' % p.read_u16())
        print('    field16_u32=%d' % p.read_u32())
        print('    field17_u32=%d' % p.read_u32())
        print('    field18_u32=%d' % p.read_u32())
        print('    field19_u32=%d' % p.read_u32())
        print('    field20_u32=%d' % p.read_u32())
        print('    field21_u32=%d' % p.read_u32())
        print('    field22_u32=%d' % p.read_u32())
        print('    field23_u32=%d' % p.read_u32())
        print('    field24_u8=%d' % p.read_u8())
        print('    field25_u8=%d' % p.read_u8())
        print('}')
    def serialize_60197(netid, p: common.PacketReader):
        print('Cl_60197 {')
        print('    field1_u8 flags=<unknown type u8 flags>')
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_vec3 (3 x u32)=<unknown type vec3 (3 x u32)>')
        print('    field5_vec3 (3 x u32), optional=<unknown type vec3 (3 x u32), optional>')
        print('    field6_u32, optional=<unknown type u32, optional>')
        print('}')
    def serialize_60198(netid, p: common.PacketReader):
        print('Cl_60198 {')
        print('    field1_array: u16 count + N x {u32,u32,u32,u32,u32,u32,u8} (28-byte element)=<unknown type array: u16 count + N x {u32,u32,u32,u32,u32,u32,u8} (28-byte element)>')
        print('}')
    def serialize_60199(netid, p: common.PacketReader):
        print('Cl_60199 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60200(netid, p: common.PacketReader):
        print('Cl_60200 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60202(netid, p: common.PacketReader):
        print('Cl_60202 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60203(netid, p: common.PacketReader):
        print('Cl_60203 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60205(netid, p: common.PacketReader):
        print('Cl_60205 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_wstring=<unknown type wstring>')
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_vec3=<unknown type vec3>')
        print('}')
    def serialize_60206(netid, p: common.PacketReader):
        print('Cl_60206 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_u32=%d' % p.read_u32())
        print('    field6_u32=%d' % p.read_u32())
        print('    field7_u32=%d' % p.read_u32())
        print('    field8_u32=%d' % p.read_u32())
        print('    field9_u32=%d' % p.read_u32())
        print('    field10_u32=%d' % p.read_u32())
        print('    field11_u32=%d' % p.read_u32())
        print('    field12_u32=%d' % p.read_u32())
        print('    field13_u32=%d' % p.read_u32())
        print('}')
    def serialize_60207(netid, p: common.PacketReader):
        print('Cl_60207 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_u32=%d' % p.read_u32())
        print('}')
    def serialize_60208(netid, p: common.PacketReader):
        print('Cl_60208 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60209(netid, p: common.PacketReader):
        print('Cl_60209 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60210(netid, p: common.PacketReader):
        print('CQ_WhisperSend {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_wstring=<unknown type wstring>')
        print('}')
    def serialize_60211(netid, p: common.PacketReader):
        print('Cl_60211 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60214(netid, p: common.PacketReader):
        print('Cl_60214 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60218(netid, p: common.PacketReader):
        print('Cl_60218 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60219(netid, p: common.PacketReader):
        print('Cl_60219 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_wstring=<unknown type wstring>')
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60220(netid, p: common.PacketReader):
        print('Cl_60220 {')
        print('    field1_u64=%d' % p.read_u64())
        print('}')
    def serialize_60221(netid, p: common.PacketReader):
        print('Cl_60221 {')
        print('    field1_u64=%d' % p.read_u64())
        print('}')
    def serialize_60222(netid, p: common.PacketReader):
        print('Cl_60222 {')
        print('    field1_vector<u64>=<unknown type vector<u64>>')
        print('}')
    def serialize_60223(netid, p: common.PacketReader):
        print('Cl_60223 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_vector<u64>=<unknown type vector<u64>>')
        print('}')
    def serialize_60226(netid, p: common.PacketReader):
        print('Cl_60226 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60227(netid, p: common.PacketReader):
        print('Cl_60227 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60229(netid, p: common.PacketReader):
        print('Cl_60229 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60230(netid, p: common.PacketReader):
        print('Cl_60230 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60232(netid, p: common.PacketReader):
        print('Cl_60232 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('}')
    def serialize_60233(netid, p: common.PacketReader):
        print('Cl_60233 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec3=<unknown type vec3>')
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('}')
    def serialize_60236(netid, p: common.PacketReader):
        print('Cl_60236 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60237(netid, p: common.PacketReader):
        print('Cl_60237 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60239(netid, p: common.PacketReader):
        print('Cl_60239 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60240(netid, p: common.PacketReader):
        print('Cl_60240 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60241(netid, p: common.PacketReader):
        print('Cl_60241 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60242(netid, p: common.PacketReader):
        print('Cl_60242 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60243(netid, p: common.PacketReader):
        print('Cl_60243 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60244(netid, p: common.PacketReader):
        print('Cl_60244 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60246(netid, p: common.PacketReader):
        print('Cl_60246 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60247(netid, p: common.PacketReader):
        print('Cl_60247 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60248(netid, p: common.PacketReader):
        print('Cl_60248 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60249(netid, p: common.PacketReader):
        print('Cl_60249 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_vec3=<unknown type vec3>')
        print('}')
    def serialize_60250(netid, p: common.PacketReader):
        print('Cl_60250 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_blob[u8]=<unknown type blob[u8]>')
        print('}')
    def serialize_60252(netid, p: common.PacketReader):
        print('Cl_60252 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('}')
    def serialize_60253(netid, p: common.PacketReader):
        print('Cl_60253 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u8=%d' % p.read_u8())
        print('}')
    def serialize_60254(netid, p: common.PacketReader):
        print('Cl_60254 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_array u16 count + N x {u32,u32,u32}=<unknown type array u16 count + N x {u32,u32,u32}>')
        print('}')
    def serialize_60259(netid, p: common.PacketReader):
        print('Cl_60259 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60260(netid, p: common.PacketReader):
        print('Cl_60260 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60261(netid, p: common.PacketReader):
        print('Cl_60261 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60262(netid, p: common.PacketReader):
        print('Cl_60262 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u8=%d' % p.read_u8())
        print('}')
    def serialize_60263(netid, p: common.PacketReader):
        print('Cl_60263 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60266(netid, p: common.PacketReader):
        print('Cl_60266 {')
        print('    field1_u16=%d' % p.read_u16())
        print('    field2_u16=%d' % p.read_u16())
        print('    field3_u16=%d' % p.read_u16())
        print('    field4_blob[u8]=<unknown type blob[u8]>')
        print('}')
    def serialize_60268(netid, p: common.PacketReader):
        print('Cl_60268 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60269(netid, p: common.PacketReader):
        print('Cl_60269 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60270(netid, p: common.PacketReader):
        print('Cl_60270 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60273(netid, p: common.PacketReader):
        print('Cl_60273 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60274(netid, p: common.PacketReader):
        print('Cl_60274 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60276(netid, p: common.PacketReader):
        print('Cl_60276 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60277(netid, p: common.PacketReader):
        print('Cl_60277 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u32=%d' % p.read_u32())
        print('}')
    def serialize_60278(netid, p: common.PacketReader):
        print('Cl_60278 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_u16=%d' % p.read_u16())
        print('    field3_u16=%d' % p.read_u16())
        print('}')
    def serialize_60279(netid, p: common.PacketReader):
        print('Cl_60279 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60280(netid, p: common.PacketReader):
        print('Cl_60280 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60281(netid, p: common.PacketReader):
        print('Cl_60281 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60282(netid, p: common.PacketReader):
        print('Cl_60282 {')
        print('    field1_u8=%d' % p.read_u8())
        print('}')
    def serialize_60283(netid, p: common.PacketReader):
        print('Cl_60283 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60284(netid, p: common.PacketReader):
        print('Cl_60284 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60285(netid, p: common.PacketReader):
        print('Cl_60285 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60286(netid, p: common.PacketReader):
        print('Cl_60286 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60287(netid, p: common.PacketReader):
        print('Cl_60287 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60288(netid, p: common.PacketReader):
        print('Cl_60288 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60289(netid, p: common.PacketReader):
        print('Cl_60289 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60290(netid, p: common.PacketReader):
        print('Cl_60290 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60291(netid, p: common.PacketReader):
        print('Cl_60291 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60292(netid, p: common.PacketReader):
        print('Cl_60292 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60294(netid, p: common.PacketReader):
        print('Cl_60294 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60295(netid, p: common.PacketReader):
        print('Cl_60295 {')
        print('    field1_struct=<unknown type struct>')
        print('}')
    def serialize_60296(netid, p: common.PacketReader):
        print('Cl_60296 {')
        print('    field1_u8=%d' % p.read_u8())
        print('    field2_u32=%d' % p.read_u32())
        print('    field3_u32=%d' % p.read_u32())
        print('    field4_u32=%d' % p.read_u32())
        print('    field5_struct=<unknown type struct>')
        print('}')
    def serialize_60297(netid, p: common.PacketReader):
        print('Cl_60297 {')
        print('    field1_u16=%d' % p.read_u16())
        print('    field2_u16=%d' % p.read_u16())
        print('}')
    def serialize_60298(netid, p: common.PacketReader):
        print('Cl_60298 {')
        print('    field1_u32=%d' % p.read_u32())
        print('    field2_blob=<blob>' % p.read_raw(0))
        print('}')
    def serialize_60299(netid, p: common.PacketReader):
        print('Cl_60299 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60301(netid, p: common.PacketReader):
        print('Cl_60301 {')
        print('    field1_string=<unknown type string>')
        print('    field2_vector=<unknown type vector>')
        print('}')
    def serialize_60302(netid, p: common.PacketReader):
        print('Cl_60302 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_wstring=<unknown type wstring>')
        print('    field3_wstring=<unknown type wstring>')
        print('    field4_wstring=<unknown type wstring>')
        print('    field5_wstring=<unknown type wstring>')
        print('    field6_wstring=<unknown type wstring>')
        print('    field7_u16=%d' % p.read_u16())
        print('}')
    def serialize_60304(netid, p: common.PacketReader):
        print('Cl_60304 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u8=%d' % p.read_u8())
        print('    field3_wstring=<unknown type wstring>')
        print('}')
    def serialize_60306(netid, p: common.PacketReader):
        print('Cl_60306 {')
        print('    field1_u16=%d' % p.read_u16())
        print('}')
    def serialize_60308(netid, p: common.PacketReader):
        print('Cl_60308 {')
        print('    field1_wstring=<unknown type wstring>')
        print('}')
    def serialize_60312(netid, p: common.PacketReader):
        print('Cl_60312 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_vector=<unknown type vector>')
        print('}')
    def serialize_60313(netid, p: common.PacketReader):
        print('Cl_60313 {')
        print('    field1_wstring=<unknown type wstring>')
        print('    field2_u64=%d' % p.read_u64())
        print('    field3_wstring=<unknown type wstring>')
        print('    field4_wstring=<unknown type wstring>')
        print('    field5_string=<unknown type string>')
        print('}')
    def serialize_60011(netid, p: common.PacketReader):
        print('Cl_60011 {}')
    def serialize_60013(netid, p: common.PacketReader):
        print('Cl_60013 {}')
    def serialize_60019(netid, p: common.PacketReader):
        print('Cl_60019 {}')
    def serialize_60021(netid, p: common.PacketReader):
        print('CN_ReadyToLoadGameMap {}')
    def serialize_60025(netid, p: common.PacketReader):
        print('Cl_60025 {}')
    def serialize_60026(netid, p: common.PacketReader):
        print('Cl_60026 {}')
    def serialize_60027(netid, p: common.PacketReader):
        print('Cl_60027 {}')
    def serialize_60035(netid, p: common.PacketReader):
        print('ConfirmLogin {}')
    def serialize_60043(netid, p: common.PacketReader):
        print('Cl_60043 {}')
    def serialize_60045(netid, p: common.PacketReader):
        print('Cl_60045 {}')
    def serialize_60046(netid, p: common.PacketReader):
        print('Cl_60046 {}')
    def serialize_60048(netid, p: common.PacketReader):
        print('Cl_60048 {}')
    def serialize_60050(netid, p: common.PacketReader):
        print('CheckDupNickname {}')
    def serialize_60057(netid, p: common.PacketReader):
        print('Cl_60057 {}')
    def serialize_60058(netid, p: common.PacketReader):
        print('Cl_60058 {}')
    def serialize_60059(netid, p: common.PacketReader):
        print('CQ_RoomEquipWeapon {}')
    def serialize_60065(netid, p: common.PacketReader):
        print('Cl_60065 {}')
    def serialize_60066(netid, p: common.PacketReader):
        print('Cl_60066 {}')
    def serialize_60067(netid, p: common.PacketReader):
        print('Cl_60067 {}')
    def serialize_60099(netid, p: common.PacketReader):
        print('Cl_60099 {}')
    def serialize_60114(netid, p: common.PacketReader):
        print('CN_ChannelChatMessage {}')
    def serialize_60117(netid, p: common.PacketReader):
        print('Cl_60117 {}')
    def serialize_60118(netid, p: common.PacketReader):
        print('Cl_60118 {')
        print('    field1_u32=%d' % p.read_u32())
        print('}')
    def serialize_60131(netid, p: common.PacketReader):
        print('Cl_60131 {}')
    def serialize_60134(netid, p: common.PacketReader):
        print('Cl_60134 {}')
    def serialize_60135(netid, p: common.PacketReader):
        print('Cl_60135 {}')
    def serialize_60138(netid, p: common.PacketReader):
        print('Cl_60138 {}')
    def serialize_60140(netid, p: common.PacketReader):
        print('Cl_60140 {}')
    def serialize_60143(netid, p: common.PacketReader):
        print('Cl_60143 {}')
    def serialize_60147(netid, p: common.PacketReader):
        print('Cl_60147 {}')
    def serialize_60164(netid, p: common.PacketReader):
        print('Cl_60164 {}')
    def serialize_60201(netid, p: common.PacketReader):
        print('Cl_60201 {}')
    def serialize_60204(netid, p: common.PacketReader):
        print('Cl_60204 {}')
    def serialize_60212(netid, p: common.PacketReader):
        print('Cl_60212 {}')
    def serialize_60213(netid, p: common.PacketReader):
        print('Cl_60213 {}')
    def serialize_60215(netid, p: common.PacketReader):
        print('Cl_60215 {}')
    def serialize_60216(netid, p: common.PacketReader):
        print('Cl_60216 {}')
    def serialize_60217(netid, p: common.PacketReader):
        print('Cl_60217 {}')
    def serialize_60224(netid, p: common.PacketReader):
        print('Cl_60224 {}')
    def serialize_60225(netid, p: common.PacketReader):
        print('Cl_60225 {}')
    def serialize_60228(netid, p: common.PacketReader):
        print('Cl_60228 {}')
    def serialize_60231(netid, p: common.PacketReader):
        print('Cl_60231 {}')
    def serialize_60234(netid, p: common.PacketReader):
        print('Cl_60234 {}')
    def serialize_60238(netid, p: common.PacketReader):
        print('Cl_60238 {}')
    def serialize_60251(netid, p: common.PacketReader):
        print('Cl_60251 {}')
    def serialize_60255(netid, p: common.PacketReader):
        print('Cl_60255 {}')
    def serialize_60256(netid, p: common.PacketReader):
        print('Cl_60256 {}')
    def serialize_60257(netid, p: common.PacketReader):
        print('Cl_60257 {}')
    def serialize_60264(netid, p: common.PacketReader):
        print('Cl_60264 {}')
    def serialize_60267(netid, p: common.PacketReader):
        print('Cl_60267 {}')
    def serialize_60271(netid, p: common.PacketReader):
        print('Cl_60271 {}')
    def serialize_60293(netid, p: common.PacketReader):
        print('Cl_60293 {}')
    def serialize_60303(netid, p: common.PacketReader):
        print('Cl_60303 {}')
    def serialize_60305(netid, p: common.PacketReader):
        print('Cl_60305 {}')
    def serialize_60307(netid, p: common.PacketReader):
        print('Cl_60307 {}')
    def serialize_60309(netid, p: common.PacketReader):
        print('Cl_60309 {}')
    def serialize_60310(netid, p: common.PacketReader):
        print('Cl_60310 {}')
    def serialize_60311(netid, p: common.PacketReader):
        print('Cl_60311 {}')
