from ghidra.program.util.string import *
from ghidra.program.model.address import AddressSet
from ghidra.util.task import ConsoleTaskMonitor

logfile = open("mxm_autoname.log", "w")

def LOG(string):
    s = str(string)
    print(s)
    logfile.write(s + '\n')

def getRerencesTo(addr):
    rm = currentProgram.getReferenceManager()
    refIt = rm.getReferencesTo(addr)
    refs = []
    for r in refIt:
        refs.append(r)
    return refs

class Callback(ghidra.program.util.string.FoundStringCallback):
    def stringFound(self, string):
        s = string.getString(currentProgram.getMemory())
        if s.endswith('[') and (s.startswith('SA_') or s.startswith('SQ_') or s.startswith('SN_')):
            packet_name = s[:-1]
            LOG("\n'%s'" % packet_name)

            fm = currentProgram.getFunctionManager()
            cm = currentProgram.getCodeManager()

            refs = getRerencesTo(string.getAddress())
            if len(refs) > 1:
                LOG("ERROR: Too many string refs (%d)" % len(refs))
                return

            addr = refs[0].getFromAddress()
            LOG(addr)

            func_serialize = fm.getFunctionContaining(addr)
            LOG("Packet_Serialize(): " + str(func_serialize))

            packet_serialize_func_addr = func_serialize.getEntryPoint().getOffset()

            refs = getRerencesTo(func_serialize.getEntryPoint())
            if len(refs) > 1:
                LOG("ERROR: Too many func_serialize() refs (%d)" % len(refs))
                return

            addr = refs[0].getFromAddress()
            LOG(addr)
            func_handle_packet = fm.getFunctionContaining(addr)
            LOG("HandlePacket_xxxxx(): " + str(func_handle_packet))

            refs = getRerencesTo(func_handle_packet.getEntryPoint())
            if len(refs) > 1:
                LOG("ERROR: Too many func_handle_packet() refs (%d)" % len(refs))
                return

            addr = refs[0].getFromAddress()
            LOG(addr)
            func_packet_handler = fm.getFunctionContaining(addr)
            LOG(func_packet_handler)

            if func_packet_handler.getEntryPoint().getOffset() != 0x9727fa:
                LOG("ERROR: Packet not handled by main packet handler function")
                return
            
            inst = cm.getInstructionAt(addr)
            for i in range(0, 6):
                inst = inst.getPrevious()

            refs = inst.getReferenceIteratorTo()
            for r in refs: break
            packet_id = (r.getFromAddress().getOffset() - 0x0975caa) / 4 + 62002
            LOG(packet_id)

            packets.append((packet_name, packet_serialize_func_addr, packet_id))

            



searcher = ghidra.program.util.string.StringSearcher(currentProgram, 5, 1, True, True)

packets = []
callback = Callback()
searcher.search(None, callback, True, monitor)

def getPacketID(p):
    return p[2]

packets.sort(key=getPacketID)

for p in packets:
    LOG(p)

logfile.close()

f = open("sv_name.py", "w")
f.write("class ServerPacketName:\n")

for p in packets:
    f.write("    def name_%d(netid):\n" % p[2])
    f.write("        print('%s {} @%x')\n\n" % (p[0], p[1]))

f.close()