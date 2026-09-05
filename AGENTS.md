# AGENTS.md

MxM private server stack. The client (MXMClient_DP_p3.exe) is the wire authority — packets are reversed against it in Ghidra, then transcribed into `src/common/protocol.h`, `src/common/packet_validator.h`, `src/common/packet_serialize.h`, and the `scripts/mxm_packets/*.py` decoders.

## Rules

- Do not write comments.
- Packet structs carry the real prefixes (`CA_`/`CQ_`/`CN_` for client packets, `SA_`/`SN_`/`SQ_` for server packets) as named by the client's loggers in Ghidra.
- Validate incoming packets with `ValidatePacket<T>` before parsing, then NT_LOG them with `PacketSerialize<T>` once, then use the values.
- Header-only packets (0-byte payload) skip validation but still get NT_LOG'd.
- Servers run from `build/`. Ports: login 10900, hub 11900, mm 13900.
