# client packets
from . import common

class ClientSerializer:

    def serialize_60002(netid, data):
        p = common.PacketReader(data)

        print('Hello {')
        print('    dwProtocolCRC=%#x' % p.read_u32())
        print('    dwErrorCRC=%#x' % p.read_u32())
        print('    version=%d' % p.read_u32())
        print('    unknown=%d' % p.read_u8())
        print('}')
        
    def serialize_60008(netid, data):
        p = common.PacketReader(data)

        print('RequestConnectGame {')
        print('    name="%s"' % p.read_wstr())
        print('}')

        # TODO reverse the rest

