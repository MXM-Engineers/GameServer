---
name: packet-reversing
description: Reverse-engineer a full MxM client packet from binary to protocol.h. Starts at NetworkManager_RegisterRecvPackets, names the handler + logger functions in Ghidra, derives the wire layout from the validator, extracts field names/types from the logger, then fixes src/common/protocol.h and regenerates the sv.py serializer. Use for any netid 62001-62608 when the client binary (MXMClient_DP_p3.exe) is the authority.
---

# MxM Packet Reversal

Full workflow to turn one server->client packet into a verified `protocol.h` struct + wire serializer. The Ghidra program is the authority; the repo files are its transcription. Never guess from repo headers — decompile first.

## Project facts (constants)

- Binary: `D:/Projets/MxM/Version_15062017/Client.Win32/MXMClient_DP_p3.exe` — **base-0 dump**; Ghidra maps at base `0x400000`. Raw-file reads: `fileoff(addr) = rva2off(addr - 0x400000)` (section table: main `vaddr=0x1000, roff=0x600`).
- Registry: `NetworkManager_RegisterRecvPackets` @ `0xa15245` (608 netids 62001-62608, each netid -> validator). Send side: `NetworkManager_RegisterSendPackets` @ `0x976966`.
- Validator primitive: `PacketReader_CanRead` @ `0x6bd969` (bounds check).
- MCP tools: `xd://mcp__ghidra_mcp_*` (write JSON args via the `write` tool). `batch_decompile` accepts `{"functions": "0x…,0x…"}`. Rename: `rename_function_by_address` (`function_address` + `new_name`). Create function: `create_function` (`address`, `name`).
- Extraction artifacts (gitignored `build/`): `recv_packets.json` (netid->handler addr), `recv_layouts.json` (netid->canread sequence), `logger_fields.json` (netid->{logger, name, fields}).
- Naming convention: **underscores**, e.g. `ValidatePacket_62001`, `LogPacket_SN_GameModifyActor`, `LogPST_Pos3`, `LogST_BaseStat`. The MCP tool's PascalCase warnings are cosmetic — ignore them.

## Phase 1 — Registry -> handler -> layout

1. **Locate the handler.** Either read `build/recv_packets.json` for the netid, or decompile `NetworkManager_RegisterRecvPackets` (0xa15245) and find the `(netid, validator)` pair.
2. **Create + name the handler function** in Ghidra (validators are often un-created thunks/stubs):
   - `create_function` at the validator address, then `rename_function_by_address` -> `ValidatePacket_<netid>`.
3. **Decompile the validator.** The wire layout is the sequence of `PacketReader_CanRead` (0x6bd969) calls in order: each argument is a field size. Record the full size list and its sum.
   - `recv_layouts.json["<netid>"]["seq"]` may already have it — flatten `"s"` (size) / `"h"` (handler-call) entries.
   - A validator that only calls the name logger (no CanRead) or is a stub = payloadless packet.

## Phase 2 — Logger -> names -> struct fix

1. **Find the logger.** `scripts/mxm_packets/sv_name.py` maps netid -> name fn with `@addr` comments; or take the packet's wide name string (e.g. `SN_GAME_MODIFY_ACTOR[`) and list xrefs to it — the xref'd code is the logger.
2. **Name it** `LogPacket_SN_<Name>` (0x97-0x9a region).
3. **Decompile the logger.** Extract, in order:
   - **Field names**: the wide string pushed right before each value (e.g. `u__oldID__027cd740` = `oldID:`).
   - **Types + offsets**: direct reads `param_1[N]` give the in-memory offset; the load width gives the type (`movzx … byte` = u8/bool, `movss` = f32, plain `mov` = u32, `(undefined1)` cast = u8).
   - **Sub-formatter calls**: a field whose value comes from a *call* (e.g. `FUN_009acdc7(local_2c)` then a name push) is a nested struct. **NEVER mark it "variable-size (string/vector)" without decompiling the callee** — this was the original bug.
4. **Decompile + name each sub-formatter**: `LogPST_<Type>` for structs, `LogST_<Type>` for vectors.
   - Classify: 3 float reads (`fX,fY,fZ`, wide `ST_POS3[`) -> `float3`, 12 bytes.
   - `(end - begin) >> 3` count math with `:VEC(` suffix -> vector of 8-byte elements (e.g. `ST_BASE_STAT`: `{u8 type, f32 value}` = 5 wire bytes, in-memory stride 8).
   - Nested vectors: loop body calls another formatter per element.
5. **Fix `src/common/protocol.h`** — rules, in priority order:
   - **Sizes win**: the validator CanRead sum is the wire truth; the struct must total exactly that.
   - **Names fill**: logger names/types map onto the sizes in order.
   - **`unk_N` pads**: leftover validated bytes with no logger name.
   - **VEC convention** for variable parts: `u16 count; T arr[1];` (ASSERT counts arr[1] as one element).
   - `ASSERT_SIZE(<Struct>, <validator sum>)` must match exactly.
   - The logger reads a **padded in-memory struct**; the wire is packed — the offset pattern can drift (e.g. `bDirectionToNearPC` is u8 at offset 56 but a following u32 aligns it). Always resolve offsets against the packed wire, not the logger's raw offsets.

## Phase 3 — Serializer + verification

1. **Regenerate `serialize_<netid>` in `scripts/mxm_packets/sv.py`** (class `ServerSerializer`) from the fixed struct. Reader API: `read_u8/u16/u32/i64/f32`, VEC = `read_u16` count + loop.
2. **Verify**:
   - `cmake --build build --config Release` — 0 errors (ASSERT_SIZE proves the struct = validator bytes).
   - `python -m py_compile scripts/mxm_packets/sv.py`.
   - Serializer smoke: build a synthetic packet `size(u16) + netid(u16) + payload` (header is **4 bytes**, u16+u16 — `read_header()` consumes 4), feed it, assert `p.buff == b''` (consumed exactly).
   - Full capture smoke (if serializers changed): `wireshark_to_raw.py` on `Minigame Night.pcapng` reproduces 43,921 segments.
3. **Don't commit** unless the user explicitly says so.

## Client->server (CQ_/CN_) packets — send-site reversal

The client CONSTRUCTS these; its builder functions are the wire truth. `NetworkManager_RegisterSendPackets` (0x976966) registers only (category, netid) — no function pointers.

**Find the builder:** byte-pattern search for the push-imm32 encoding of the netid: bytes `68 <LE32>` (e.g. 60003 -> `68 63 ea 00 00`). Exclude hits inside NetworkManager_RegisterSendPackets (~0x976966-0x9778f6). Expect ~3 sites per netid: an unanalyzed stub (~0x58xxxx, alt-builder-init FUN_0056ea00), the authoritative builder (0x9c/0x9d region), and a pass-through wrapper.

**Builder shape:** `FUN_0092f208(netid)` BeginPacket -> ordered typed writer calls into a stack buffer -> u16 size patch -> vtbl+0x18 transmit.

**Writer taxonomy (verified):**
| fn | type |
|---|---|
| FUN_009701f1 | wstr (u16 len + len*2 utf16) — reads its own length |
| FUN_009c360b | str (u16 len + ansi) |
| FUN_0056e0db / 0056e086 / 0056e130 | u32 |
| FUN_0056e02f / 0056dfd8 | u16 |
| FUN_0056df84 / 0056e298 / 0056df30 | u8 |
| FUN_0056e1de | raw 8-byte (u64) |
| FUN_009c317b | vec<u16 count> of {u32,u32} |
| FUN_009c3069 / 0056dc65 | vec<u16 count> of u32 |
| FUN_009c302c / 0056dbd4 | blob<u16 count> of u8 |
| FUN_009c3203 / 009c3245 / 009c31bd(28B elems via FUN_0096fb54) | vector writers |
| FUN_009c328b | vec<u64> |
| FUN_0096ecde / 0096ecbc | vec3 / vec2 |
| FUN_0095c81c | 13 consecutive dwords |

Unknown writers: decompile once, classify by evidence (byte width, movss=float, len*2=wstr, explicit size=blob).

Empty-body packets are real (header-only): confirm by absence of append calls between BeginPacket and the size patch.

## Pitfalls (all hit in practice)

- **Sub-formatter misclassification** (`SN_GameModifyActor`): Vec3/BaseStat fields read via sub-calls were tagged "variable-size", dropping 24 bytes and **drifting the name<->size alignment** — downstream fields got wrong types (`bDirectionToNearPC` u32 instead of u8, `seed` u8 instead of u32) and the tail collapsed into 12 garbage `unk_`. The sub-call is the type; never skip it.
- **Logger vs validator size mismatch**: the capture can contain packets shorter than their validator (e.g. `SN_ActiveIngameEventList` 2-byte payload vs 4-byte struct). The parser must not crash — `wireshark_to_raw.py` catches `struct.error` and prints `(!) payload shorter than struct`.
- **Base-0 file offsets**: every raw-file read subtracts 0x400000; Ghidra addresses are used verbatim in MCP calls.
- **`float3`/`f32`/`u8`/`u16`/`u32`/`i64` exist in protocol.h** — reuse them; do not invent types.
- **Copy-paste serializers**: `serialize_62550` was a duplicate of 62468 — when regenerating, diff against the hand-written block for the same name.
