import pyshark
import sys
import struct
import os
from mxm_packets.cl import *
from mxm_packets.sv import *
from mxm_packets.sv_name import *

if len(sys.argv) < 3:
    print('Usage: wireshark_to_raw.py capture_file "output/dir"')
    exit(1)

output_dir = sys.argv[2]
cap = pyshark.FileCapture(sys.argv[1])

next_packet_id = 1

def packet_serialize_cl(netid, data):
    f = getattr(ClientSerializer, 'serialize_%d' % netid, None)
    if f == None:
        print('Unknown {}')
    else:
        f(netid, data)
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
        f(netid, data)
    print('')

    # SN_DoConnectGameServer
    # add the port to the scan_list
    # TODO: remove it on disconnect
    if netid == 62010: 
        p = common.PacketReader(data)
        scan_list.append(str(p.read_u16()))

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

        if self.prefix == 'cl':
            packet_serialize_cl(netid, self.buff[:size])
        elif self.prefix == 'sv':
            packet_serialize_sv(netid, self.buff[:size])

        f = open(os.path.join(output_dir, '%d_%s_%d.raw' % (self.order, self.prefix, netid)), 'wb')
        f.write(self.buff[:size])
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

        info = '%s > %s ' % (p.ip.src, p.ip.dst) + ' (' + time + ')'

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