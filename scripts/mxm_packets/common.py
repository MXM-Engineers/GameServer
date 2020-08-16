import struct

class PacketReader:
    def __init__(self, data):
        self.buff = data
        self.adv(4) # skip header

    def adv(self, am):
        self.buff = self.buff[am:]

    def read_i8(self):
        r = struct.unpack("b", self.buff[:1])[0]
        self.adv(1)
        return r

    def read_u8(self):
        r = struct.unpack("B", self.buff[:1])[0]
        self.adv(1)
        return r

    def read_u16(self):
        r = struct.unpack("H", self.buff[:2])[0]
        self.adv(2)
        return r

    def read_i32(self):
        r = struct.unpack("i", self.buff[:4])[0]
        self.adv(4)
        return r

    def read_u32(self):
        r = struct.unpack("I", self.buff[:4])[0]
        self.adv(4)
        return r

    def read_f32(self):
        r = struct.unpack("f", self.buff[:4])[0]
        self.adv(4)
        return r

    def read_i64(self):
        r = struct.unpack("q", self.buff[:8])[0]
        self.adv(8)
        return r

    def read_wstr(self):
        len = struct.unpack("H", self.buff[:2])[0] * 2
        self.adv(2)
        r = self.buff[:len].decode('utf-16')
        self.adv(len)
        return r

    def read_raw(self, len):
        r = self.buff[:len]
        self.adv(len)
        return r

        