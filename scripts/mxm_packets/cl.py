# client packets
from . import common

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)
    
class ClientSerializer:

    def serialize_60002(netid, p: common.PacketReader):
        print('CQ_FirstHello {')
        print('    dwProtocolCRC=%#x' % p.read_u32())
        print('    dwErrorCRC=%#x' % p.read_u32())
        print('    version=%#x' % p.read_u32())
        print('    unknown=%d' % p.read_u8())
        print('}')
        
    def serialize_60008(netid, p: common.PacketReader):
        print('CQ_Authenticate {')
        print('    name="%s"' % p.read_wstr())
        print('}')

        # TODO reverse the rest

    def serialize_60009(netid, p: common.PacketReader):
        print('CQ_AuthenticateGameServer {')
        print('    name="%s"' % p.read_wstr())
        print('    instantKey=%d' % p.read_i32())
        print('    var=%d' % p.read_u32())
        print('    b1=%d' % p.read_u8())
        print('}')

    def serialize_60014(netid, p: common.PacketReader):
        print('ReadyToLoadCharacter {}')

    def serialize_60016(netid, p: common.PacketReader):
        print('CN_GameMapLoaded {}')

    def serialize_60022(netid, p: common.PacketReader):
        print('CN_UpdatePosition {')
        print('    playerID=%d' % p.read_i32())
        print('    p3nPos=%s' % read_Vec3(p))
        print('    p3nDirection=%s' % read_Vec3(p))
        print('    p3nEye=%s' % read_Vec3(p))
        print('    nRotate=%d' % p.read_f32())
        print('    nSpeed=%d' % p.read_f32())
        print('    nState=%d' % p.read_i32())
        print('    nActionIDX=%d' % p.read_i32())
        print('}')

    def serialize_60032(netid, p: common.PacketReader):
        print('CQ_GameIsReady {}')
        
    def serialize_60033(netid, p: common.PacketReader):
        print('CQ_LoadingComplete {}')

    def serialize_60034(netid, p: common.PacketReader):
        print('CN_MapIsLoaded {}')

    def serialize_60051(netid, p: common.PacketReader):
        print('CQ_GetCharacterInfo {')
        print('    characterID=%d' % p.read_i32())
        print('}')

    def serialize_60053(netid, p: common.PacketReader):
        print('CQ_GamePlayerTag {')
        print('    characterID=%d' % p.read_i32())
        print('}')

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
        print('    rankingType=%d' % p.read_u8())
        print('}')

    def serialize_60167(netid, p: common.PacketReader):
        print('CQ_180 {')
        print('    charcterID=%d' % p.read_i32())
        print('}')

    def serialize_60235(netid, p: common.PacketReader):
        print('CQ_LoadingProgressData {')
        print('    progress=%d' % p.read_u8())
        print('}')

    def serialize_60245(netid, p: common.PacketReader):
        print('CQ_RTT_Time {')
        print('    time=%d' % p.read_i32())
        print('}')

