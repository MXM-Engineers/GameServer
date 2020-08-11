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

