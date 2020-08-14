import socket
import sys
import threading

running = True
RECV_LEN = 8192

class ClientThread(threading.Thread):

    def __init__(self, client_socket, client_addr):
        self.sock = client_socket
        self.addr = client_addr
        self.recvMsgID = 0
        print('New connection: ', client_socket, client_addr)

    def receive(self):
        chunk = self.sock.recv(RECV_LEN)
        if chunk == b'':
            raise RuntimeError("socket connection broken")

        f = open('p%d.raw'%(self.recvMsgID), 'wb')
        f.write(chunk)
        f.close()
        self.recvMsgID = self.recvMsgID + 1
        return chunk

    def send_handshake(self):
        self.sock.send(b'\x14\x00\x32\xf2\x99\x51\x84\x28\x2c\x9e\x89\x93\x00\xd7\xe9\x41\x57\x19\xc5\x01')

    def run(self):
        while running:
            # handshake, receive from client
            msg = self.receive()
            self.send_handshake() # send handshake
            
            msg = self.receive()
            

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
sock.bind(('localhost', 10900))

sock.listen(1)
sock.settimeout(1.0)

print('Wating for a connection...')
while running:
    try:
        client_socket, client_addr = sock.accept()
        cl = ClientThread(client_socket, client_addr)
        cl.run()
    except socket.timeout:
        continue
    except IOError as msg:
        print(msg)
        continue
    except KeyboardInterrupt:
        running = False
        break
