from ghidra.program.util.string import *
from ghidra.program.model.address import AddressSet
from ghidra.util.task import ConsoleTaskMonitor
from ghidra.program.model.listing import FlowOverride

logfile = open("fix_flow_override.log", "w")

def LOG(string):
    s = str(string)
    print(s)
    logfile.write(s + '\n')

fm = currentProgram.getFunctionManager()
func = fm.getFunctionContaining(currentAddress)

LOG(currentAddress)
LOG(func)
LOG("")

if func != None:
    rm = currentProgram.getReferenceManager()
    refs = rm.getReferencesTo(func.getEntryPoint())
    addr_list = []
    for r in refs:
        addr_list.append(r.getFromAddress())

    count = len(addr_list)
    LOG("References: %d" % count)

    LOG("CLEAR")
    i = 0
    
    for addr in addr_list:
        LOG("[%05d/%05d] " % (i, count) + str(addr))
        #inst = getInstructionAt(addr)
        #if inst:
        #    inst.setFlowOverride(FlowOverride.NONE)
        #    inst.clearFallThroughOverride()
        clearListing(addr)
        i += 1

    LOG("DISASSEMBLE")
    i = 0
    for addr in addr_list:
        LOG("[%05d/%05d] " % (i, count) + str(addr))
        disassemble(addr)
        i += 1
else:
    LOG("ERROR: not a function!")

logfile.close()