import pyshark
import sys
import struct
import os
from mxm_packets.cl import *
from mxm_packets.sv import *

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
        print('Unknown {}')
    else:
        f(netid, data)
    print('')

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

client_spitter = PacketSpitter('cl')
server_spitter = PacketSpitter('sv')

last_time = 0
for p in cap:
    if hasattr(p, 'ip') and hasattr(p, 'tcp') and hasattr(p.tcp, 'payload') and (p.tcp.srcport == '11900' or p.tcp.dstport == '11900'):
        # get frame time
        time = last_time
        if hasattr(p.frame_info, 'time_relative'):
            time = p.frame_info.time_relative
            last_time = time

        info = '%s > %s ' % (p.ip.src, p.ip.dst) + ' (' + time + ')'

        if len(p.tcp.payload.binary_value) >= 4:
            if p.tcp.dstport == '11900': # client to server
                print('>>>> Client ::', info)
                client_spitter.push(p.tcp.payload.binary_value)
                
            if p.tcp.srcport == '11900': # server to client
                print('>>>> Server ::', info)
                server_spitter.push(p.tcp.payload.binary_value)
        else:
            print('payload too small (%d)' % len(p.tcp.payload.binary_value))

print('server remaining bytes = %d' % len(server_spitter.buff))
print('client remaining bytes = %d' % len(client_spitter.buff))