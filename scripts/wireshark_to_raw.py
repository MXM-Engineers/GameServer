import pyshark
import sys
import struct
import os
from mxm_packets.cl import *
from mxm_packets.sv import *
from mxm_packets.sv_name import *
from ctypes import *

# lea API
leaDll = WinDLL("lea.dll")
leaDll.GenerateKey.restype = c_void_p
leaDll.GenerateKey.argtypes = (c_char_p,)
leaDll.Decrypt.argtypes = (c_void_p, POINTER(c_ubyte), c_int32)
leaCurrentKey = None

def GenerateKey(ipPortStr):
    global leaCurrentKey
    leaCurrentKey = leaDll.GenerateKey(c_char_p(ipPortStr.encode('ascii')))

def Decrypt(data):
    c_data = (c_ubyte * len(data))(*data)
    leaDll.Decrypt(leaCurrentKey, c_data, c_int32(len(data)))
    return list(c_data)
# -----------

ClientEncryptedIDs = [ 60178, 60179 ]
ServerEncryptedIDs = [ 0xf398, 0xf399, 0xf39C, 0xf39D, 0xf422, 0xf3B0, 0xf3AE, 0xf3AF, 0xf3AC, 0xf3A7, 0xf3A3, 0xf3A4, 0xf3A6, 0xf3A0, 0xf3A1, 0xf3A5, 0xf3AB, 0xf3A9, 0xf3E4, 0xf3B1, 0xf3B2, 0xf3B5, 0xf3B4, 0xf31B, 0xf31C, 0xf316, 0xf317, 0xf318, 0xf434,  ]

def packet_handle_cl(netid, data):
    if netid in ClientEncryptedIDs:
        print("!Encrypted")
        # data[4:] = Decrypt(data[4:])

def packet_serialize_cl(netid, data):
    f = getattr(ClientSerializer, 'serialize_%d' % netid, None)
    if f == None:
        print('Unknown {}')
    else:
        p = common.PacketReader(data)
        p.read_header() # skip header
        f(netid, p)
    print('')

def packet_serialize_sv(netid, data):
    f = getattr(ServerSerializer, 'serialize_%d' % netid, None)
    if f == None:
        f = getattr(ServerPacketName, 'name_%d' % netid, None)
        if f:
            f(netid)
        else:
            print('Unknown {}')
    else:
        p = common.PacketReader(data)
        p.read_header() # skip header
        f(netid, p)
    print('')

    # SN_DoConnectGameServer
    # add the port to the scan_list
    # TODO: remove it on disconnect
    if netid == 62010: 
        p = common.PacketReader(data)
        p.read_header() # skip header
        scan_list.append(str(p.read_u16()))

def packet_handle_sv(netid, data):
    if netid == 62002:
        p = common.PacketReader(data)
        p.read_header() # skip header
        p.read_u32() # dwProtocolCRC
        p.read_u32() # dwErrorCRC
        p.read_u8() # sverType
        ip = (p.read_u8(), p.read_u8(), p.read_u8(), p.read_u8())
        port = p.read_u16()
        GenerateKey("%d.%d.%d.%d:%d" % (ip[3], ip[2], ip[1], ip[0], port))

    elif netid in ServerEncryptedIDs:
        print("!Encrypted")
        data[4:] = Decrypt(data[4:])

class PacketSpitter:
    def __init__(self, prefix):
        self.buff = bytes()
        self.order = -1
        self.prefix = prefix

    def extract_packet(self, size, netid):
        if self.order == -1:
            global next_packet_id
            self.order = next_packet_id
            next_packet_id += 1

        print('(o=%d netid=%d size=%d)' % (self.order, netid, size))

        if size > 10240 or netid < 60000 or netid > 63000:
            print("ERROR: invalid packet (netid=%d size=%d)" % (netid, size))
            exit(1)

        data = bytearray(self.buff[:size])

        if self.prefix == 'cl':
            packet_handle_cl(netid, data)
            packet_serialize_cl(netid, data)
        elif self.prefix == 'sv':
            packet_handle_sv(netid, data)
            packet_serialize_sv(netid, data)

        f = open(os.path.join(output_dir, '%d_%s_%d.raw' % (self.order, self.prefix, netid)), 'wb')
        f.write(data)
        f.close()

        self.buff = self.buff[size:]
        self.order = -1
        
    def push(self, data):
        self.buff += data

        packet_size, packet_netid = struct.unpack("HH", self.buff[:4])

        while packet_size <= len(self.buff):
            self.extract_packet(packet_size, packet_netid)
            if len(self.buff) >= 4:
                packet_size, packet_netid = struct.unpack("HH", self.buff[:4])
            else:
                if len(self.buff) > 0:
                    print("ERROR: buff is too small to contain a NetHeader (%d)" % len(self.buff))
                break


# Script start
if len(sys.argv) < 3:
    print('Usage: wireshark_to_raw.py capture_file "output/dir"')
    exit(1)

output_dir = sys.argv[2]
cap = pyshark.FileCapture(sys.argv[1])

next_packet_id = 1

client_spitter = PacketSpitter('cl')
server_spitter = PacketSpitter('sv')

scan_list = [
    '11900'
]

last_time = 0
for p in cap:
    if hasattr(p, 'ip') and hasattr(p, 'tcp') and hasattr(p.tcp, 'payload'):
        is_client = False
        is_server = False
        if p.tcp.dstport in scan_list:
            is_client = True
        elif p.tcp.srcport in scan_list:
            is_server = True
        else:
            continue

        # get frame time
        time = last_time
        if hasattr(p.frame_info, 'time_relative'):
            time = p.frame_info.time_relative
            last_time = time

        info = '%s:%s > %s:%s ' % (p.ip.src, p.tcp.srcport, p.ip.dst, p.tcp.dstport) + ' (' + time + ')'

        if len(p.tcp.payload.binary_value) >= 4:
            if is_client: # client to server
                print('>>>> Client ::', info)
                client_spitter.push(p.tcp.payload.binary_value)
                
            elif is_server: # server to client
                print('>>>> Server ::', info)
                server_spitter.push(p.tcp.payload.binary_value)
        else:
            print('payload too small (%d)' % len(p.tcp.payload.binary_value))

print('server remaining bytes = %d' % len(server_spitter.buff))
print('client remaining bytes = %d' % len(client_spitter.buff))