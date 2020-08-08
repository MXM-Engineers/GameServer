import pyshark
import sys
import struct

cap = pyshark.FileCapture(sys.argv[1])

client_buff = bytes()
server_buff = bytes()

def handle_client_data(data):
    global client_buff
    client_buff += data.binary_value

def handle_server_data(data):
    global server_buff
    server_buff += data.binary_value

for p in cap:
    if hasattr(p, 'ip') and hasattr(p, 'tcp') and hasattr(p.tcp, 'payload') and (p.tcp.srcport == '11900' or p.tcp.dstport == '11900'):
        if p.tcp.dstport == '11900': # client to server
            handle_client_data(p.tcp.payload)
        if p.tcp.srcport == '11900': # server to client
            handle_server_data(p.tcp.payload)

        print('%s > %s' % (p.ip.src, p.ip.dst))


# print(client_buff)
# print(server_buff)

print("client")
cur = 0
i = 0
while cur < len(client_buff):
    header = struct.unpack("HH", client_buff[cur:cur+4])
    print(header)

    f = open('cl_%d_%d.raw' % (i, header[1]), 'wb')
    f.write(client_buff[cur:cur+header[0]])
    f.close()

    cur += header[0]
    i += 1

print("server")
cur = 0
i = 0
while cur < len(server_buff):
    header = struct.unpack("HH", server_buff[cur:cur+4])
    print(header)

    f = open('sv_%d_%d.raw' % (i, header[1]), 'wb')
    f.write(server_buff[cur:cur+header[0]])
    f.close()

    cur += header[0]
    i += 1
