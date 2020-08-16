# client packets
from . import common

def read_Vec3(p):
    x, y, z = p.read_f32(), p.read_f32(), p.read_f32()
    return 'Vec3(%g, %g, %g)' % (x, y ,z)
    
class ClientSerializer:

    def serialize_60002(netid, data):
        p = common.PacketReader(data)

        print('CQ_FirstHello {')
        print('    dwProtocolCRC=%#x' % p.read_u32())
        print('    dwErrorCRC=%#x' % p.read_u32())
        print('    version=%d' % p.read_u32())
        print('    unknown=%d' % p.read_u8())
        print('}')
        
    def serialize_60008(netid, data):
        p = common.PacketReader(data)

        print('CQ_Authenticate {')
        print('    name="%s"' % p.read_wstr())
        print('}')

        # TODO reverse the rest

    def serialize_60022(netid, data):
        p = common.PacketReader(data)

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

    def serialize_60051(netid, data):
        p = common.PacketReader(data)

        print('CQ_GetCharacterInfo {')
        print('    characterID=%d' % p.read_i32())
        print('}')

    def serialize_60145(netid, data):
        print('CQ_GetGuildProfile {}')

    def serialize_60146(netid, data):
        print('CQ_GetGuildMemberList {}')

    def serialize_60148(netid, data):
        print('CQ_GetGuildHistoryList {}')

    def serialize_60151(netid, data):
        print('CQ_TierRecord {}')

    def serialize_60167(netid, data):
        p = common.PacketReader(data)
        
        print('CQ_GetGuildRankingSeasonList {')
        print('    rankingType=%d' % p.read_u8())
        print('}')

