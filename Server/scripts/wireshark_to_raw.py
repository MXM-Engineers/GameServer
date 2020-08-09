import pyshark
import sys
import struct
import os

if len(sys.argv) < 3:
    print('Usage: wireshark_to_raw.py capture_file "output/dir"')
    exit(1)

output_dir = sys.argv[2]
cap = pyshark.FileCapture(sys.argv[1])

next_packet_id = 1

class PacketSpitter:
    def __init__(self, prefix):
        self.buff = bytes()
        self.order = -1
        self.prefix = prefix

    def extract_packet(self, size, netid):
        print(size, netid)

        if self.order == -1:
            global next_packet_id
            self.order = next_packet_id
            next_packet_id += 1

        f = open(os.path.join(output_dir, '%03d_%s_%d.raw' % (self.order, self.prefix, netid)), 'wb')
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

for p in cap:
    if hasattr(p, 'ip') and hasattr(p, 'tcp') and hasattr(p.tcp, 'payload') and (p.tcp.srcport == '11900' or p.tcp.dstport == '11900'):
        print('%s > %s' % (p.ip.src, p.ip.dst))

        if p.tcp.dstport == '11900': # client to server
            client_spitter.push(p.tcp.payload.binary_value)
        if p.tcp.srcport == '11900': # server to client
            server_spitter.push(p.tcp.payload.binary_value)