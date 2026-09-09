---
name: vtable-reversing
description: Reverse MSVC vtables in Ghidra through the ghidra-mcp toolset. Locate a vtable start via the RTTI CompleteObjectLocator slot at vptr-4, from constructors that store the vtable pointer, or from embedded class-name strings; then create a struct data type made of function-pointer members, apply it at the vtable address, and create + name a function at every slot target. Use to resolve any virtual call site (e.g. `call [reg + 0xa0]`), to enumerate a class hierarchy from a constructor/factory, or to find which per-class method implements a behavior.
---

# MxM Vtable Reversal (MSVC x86, 32-bit)

Workflow to turn an unresolved indirect call site (`(**(code **)(*obj + 0xNN))(...)`) or an
unknown class into named, decompilable per-slot functions. The Ghidra program
(`/MXMClient_DP_p3.exe`, base `0x400000`) is the authority. Applies to every MSVC
`__thiscall` class in the client.

## Project facts (constants)

- MCP tools: `xd://mcp__ghidra_mcp_*` via the `write` tool (JSON args). Core set:
  `decompile_function`, `get_function_by_address`, `get_xrefs_to`/`get_xrefs_from`,
  `read_memory`, `create_function`, `rename_function_by_address`, `import_data_types`,
  `apply_data_type`, `save_program`.
- Program: `/MXMClient_DP_p3.exe`.
- Client option/gravity evidence and the jump chain (with names applied) are documented in
  `doc/physics_alignment.md`.
- Known vtable region for the `StatePlayerActionState*` family: `0x02a5b2e0`-`0x02a5b4a0`
  (class name strings embedded right after each run). Confirmed vtables there:
  `0x02a5b280` (JumpStart), `0x02a5b30c` (JumpLoop), `0x02a5b398` (JumpEnd) — 34 slots each.
- Naming convention: underscores (`StateJumpStart_OnEnter`, `PhysController_IsAirborne`).
  The MCP rename warnings about PascalCase/verbs are cosmetic — ignore them.

## Phase 1 — Pin the vtable address

Any ONE of these anchors gives the exact start. Cross-check with a second when unsure.

1. **RTTI CompleteObjectLocator at `vptr - 4`.** An object's vtable pointer (`*obj`) points
   at the first virtual; the dword immediately before (`vptr[-4]`) is a pointer into the
   `0x02cc0xxx` RTTI COL range. So: read the four bytes at `vptr - 4`, then
   `get_xrefs_to(<that COL addr>)` — the single DATA xref answer IS the vtable start minus 4.
   This is the cleanest "is this really a vtable?" check and also gives you the class's RTTI.
2. **Constructor/factory.** A ctor always stores the vtable early: `mov dword ptr [reg],
   offset .rdata:vtable`. Decompile the allocation+init function (found from the XML/data
   reader dispatch or from `operator_new` size callers) and read the immediate.
   - Example: `FUN_01c09612` (state id 0x96 JUMP_LOOP) ends `*param_1 = &PTR_LAB_02a5b30c`
     -> vtable = `0x02a5b30c`.
3. **Data xref of a known slot target.** If you know a method belongs to a class, list its
   xrefs: `get_xrefs_to(<method addr>)`. Every `[DATA]` answer is a vtable slot; the run
   containing that slot is the vtable. `0x01b71ad3` (generic state dispatch) appears in
   dozens of vtables this way.
4. **Embedded class names.** RTTI class name strings (plain ASCII, e.g.
   `StatePlayerActionStateJumpStart`) sit in `.rdata` immediately after that class's vtable
   run — a convenient sanity check and a naming source.

## Phase 2 — Measure the run, create the type

1. **Count the slots.** `read_memory` from the vtable start for ~0x120 bytes and decode
   4-byte little-endian entries. In this binary each `StatePlayerActionState*` vtable spans
   exactly 34 entries / 136 bytes (0x88) up to the next class's COL slot. General rule:
   span = (address of the next COL slot) - (vtable start). Sibling classes repeat a long
   identical base-prefix tail; only the leading slots are class-specific overrides.
2. **Create the struct type.** One C import builds a reusable type:
   ```
   typedef void (*VFN)(void);
   typedef struct StatePlayerActionStateVtbl {
     VFN m0; VFN m1; ... VFN m33;
   } StatePlayerActionStateVtbl;
   ```
   via `import_data_types` (`source` field). You do NOT need `__thiscall`; a plain function
   pointer is a typed 4-byte pointer member. Keep member count = measured slot count.
3. **Apply it.** `apply_data_type` at the vtable start. Every slot is now a typed pointer
   member; member `mK` sits at offset `K*4` (so slot +0xa0 = `m40`).

## Phase 3 — Resolve every slot to a function

1. Applying the type does NOT create function bodies. For each target that is not yet a
   function (decompile returns "No function found", or `get_function_by_address` fails),
   call `create_function` with `address`, a name, and `disassemble_first: true`.
   - Batch: read the vtable memory once, decode the targets, create the missing ones.
2. **Name them** (`rename_function_by_address` / `rename_function`) from the decompiled
   body once it exists:
   - slot 0: scalar-deleting destructor (`StateJumpStart_scalarDtord`), slot 1: virtual dtor.
   - 6-byte `mov eax, imm; ret` = static-data accessor (`StateJumpStart_GetStaticData`).
   - bodies that `operator_new(size)` + call a per-class ctor + wire it = instantiator
     (`StateJumpStart_OnEnter` creates the 0x48-byte leaf via `FUN_01c09642`).
   - a method shared verbatim across many vtables (`0x01b71ad3`) = base-class default;
     per-class behavior lives in the overridden leading slots.
3. Verify with the now-typed members: decompile two siblings' same-index slots and diff —
   the delta IS the class-specific behavior.

## Pitfalls (all hit in practice)

- **COL is at vptr-4, not inside the run.** The dword BEFORE the vtable is a
  `0x02cc0xxx` pointer; dwords INSIDE the run are all code pointers. A non-code dword in
  your "run" means you misaligned the start by 4 (or crossed into the next class's COL).
- **Don't hand-count boundaries.** Two sibling vtables are adjacent in `.rdata` with their
  class-name strings after them; consecutive runs repeat the same base-prefix entries, so
  guessing a span by "pattern restart" is unreliable. Use the COL anchors.
- **Apply too big overlaps the next vtable.** Size the struct from measured span, not a
  round number.
- **create_function is required after apply.** Typed pointers alone don't make bodies;
  `decompile_function` on an un-created target reports "No function found".
- **Direct calls are not virtual.** A function found only via `[UNCONDITIONAL_CALL]` xrefs
  (e.g. `MoveController_UpdateFrame`) is NOT a vtable member — the class stores a different
  address at that slot; resolve through the object's vtable, not the function.
- **Slot semantics come from decompilation, not index math.** Same index can be an
  accessor in one class and an instantiator in another; classify each body.
- **Ignore the PascalCase/verb warnings** on rename/create — repo + client convention is
  underscores.
