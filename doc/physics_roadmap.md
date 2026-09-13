# Physics alignment roadmap

Baseline: commit `3122b12` aligns ordinary jumps with client XML motion curves. Client binary and extracted XML evidence inform server behavior; the goal is packet-compatible gameplay, not a clone of client internals.

## Scope: packet-observable behavior

The server must consume transmitted inputs, simulate gameplay-relevant movement/collisions, and send results the client interprets correctly. Client state-machine structure, internal movement-type fields, RNG sequence, and PhysX implementation identity are not standalone parity requirements. Investigate them only when necessary to explain an observable protocol/gameplay discrepancy.

`CQ_PlayerCastSkill::PosStruct` already carries position, destination, movement direction, rotation, speed, and client time. Validate motion selection against that packet state. Random branch differences matter only when they create divergent movement that the protocol cannot reconcile; reproducing the client's private RNG is not a goal by itself.

| Area | What remains | Status |
| --- | --- | --- |
| Horizontal skill motion | Authored execution, owner graph metadata, first-tick ownership, and signed endpoint clamping are verified in live Sniper A/Shirk runs. Broader skill, modifier, and remote coverage remains open. | Implemented; live owner coverage established |
| Action scheduling | Authored command/clip timing and cancellation are implemented. Full graph metadata accompanies normal execute-commit; physical activation remains delayed. Remote playback and owner onset offsets remain open. | Implemented; live owner coverage established |
| State-dependent movement | Preserve stance variants instead of flattening them. Support relevant runtime graph duration/amplitude modifiers and state-specific movement paths. Ordinary jump extraction currently uses base-master/default-stance profiles. | Not started |
| Jump transition details | Compare actual launch, graph completion, landing, and ceiling-contact timing against the client. Curve math is verified; exact state/collision timing is not. | Not started |
| Controller behavior | Preserved human direction fixes stale-goal steering. Live captures expose a stable server perch exactly 70 units above the client floor and delayed ledge departure. Resolve CCT behavior rather than changing matching constants. | Measured discrepancy; correction unproven |
| Collision scene coverage | Only DeathMatch is supported. Other maps need verified geometry, transforms, collision groups, and dynamic-object behavior. | Not started |
| Hit volumes | Resolve how class-specific hit volumes interact with gameplay queries. They are distinct from locomotion capsules. | Not started |
| Network reconciliation | Owner-skip replication and targeted warp corrections are not complete reconciliation. Live zero-correction captures measure onset offsets; no artificial clock shifts or corrective teleports were introduced. | Measured; general reconciliation open |

## Horizontal skill motion: implemented

Grok 4.6 agents handled client-semantic reversing, authored-data extraction, runtime implementation, and independent review. The orchestrator coordinated shared contracts and verified the integrated result.

Acceptance: evidence-backed graph timing, sampling, and collision application; reproducible actual XML values; coverage of supported packet-driven skills; real-runtime trajectory verification. No invented curves, silent linear fallback, or packet layout changes.

### Implemented and exercised

- Grok 4.6 implementation agents: `HorizontalRuntime`, `HorizontalData`. Independent Grok 4.6 review agents: `HorizontalAudit`, `HorizontalDataAudit`.
- `scripts/extract_horizontal_motion.py` generates `gamedata/HorizontalMotion.xml`: 71 reachable class/action profiles, 156 authored branches. 142 branches use AniLength; 14 use positive H_Time; 21 have nonzero H_Y0.
- The loader preserves directional and random branches, per-branch animation durations and HorizonRotate, and variable H_Y sample counts. Material direction variants on different AnimationIndex values are retained.
- World applies authored sample differences with the client-derived signed endpoint clamp. The first tick samples dt with a zero previous value and owns movement even during held walking input. Owner execute metadata supplies duration and signed-curve scaling context; ActionBase Param1 is not substituted.
- All Release server/tool targets built. A real World/PhysX smoke executable passed nonlinear Assassin motion (-286.05, not Param1 -300), Defender linear control (900), Sniper nonzero initial samples, explicit-duration data, ESPER directional rotation, both Sniper SHIRK random branches, and wall removal without endpoint catch-up.
- Regenerating the XML produced a byte-identical file. Temporary verification artifacts were removed after execution.

### Open packet-level checks for this row

- Extend the live Sniper A/Shirk coverage to other skills and remote observers. Reconstruct the client `entity+0xB50` producer only where transmitted state fails to explain an observable selection mismatch.
- Check whether independently selected random branches cause observable movement disagreements and whether existing packet fields reconcile them. Matching private client RNG state/sequence is out of scope by itself.
- Extend live coverage beyond the owner and current graph profiles. Scheduling selects one supported stance timeline to avoid duplicate effects; Statesman retains the last GRAPH_MOVE_HORZ stance. General stance switching remains open.

## Action scheduling: implemented

Grok 4.6 agents `ScheduleData` and `ScheduleRuntime` implemented authored timeline loading and runtime effects. `ScheduleAudit` independently reviewed command and packet consumers; the orchestrator ran the integrated Release build and real World/PhysX/Replication smoke.

- Command times use animation-group offsets and authored Delay, not cumulative command waits. AnimationLoop contributes to clip duration. Commands are ordered by execution deadline; action duration includes the selected clip sequence.
- STATE_BLOCK releases a client state-transition gate after Delay. It is not a WASD velocity lock. The incorrect server movement lock was removed; private PC_StateBlock logic is not cloned.
- Delayed graph motion and WARP execute on their authored boundaries. Graphs apply the terminal delta once and relinquish movement on the following tick. Replacement/tag cancels old work without sampling it.
- One SN_ExecuteSkill is emitted at normal execute-commit, with full graph metadata even for delayed onset. The actual owner handler at `0x00a05e94` caches that metadata before the animation installs movement; omitting it prevented A from moving. Delaying the packet changes execute semantics and is not the solution. SN_CastSkill carries the actual action state. Packet layouts are unchanged.
- Real-runtime smoke passed Sniper graph onset at 1.0 seconds, cancellation before/across onset, nonzero H_Y0, Priest WARP at 0.5 seconds with one correction, pending WARP replacement/tag cancellation, zero-delay WARP, and multi-clip lifecycle coverage.
- A terminal-tick regression was reproduced and fixed: Defender reached 880 instead of 900 because stale input moved it backwards after the final graph delta. It now reaches 900, then 906.666687 with fresh movement input while the action remains live.

Scope: existing supported movement effects, not a new combat-command or general stance system. Live unfocused owner captures now verify Sniper A and moving Shirk. Remote animation and delayed-graph presentation still need comparison against authoritative updates; the remote metadata setter's float order differs from the owner path.

## Recommended broader sequence

1. Resolve the measured CCT perch/ledge discrepancy and extend live comparisons to remote delayed-graph presentation, ordinary jumps, other skills, and collision boundaries.
2. Align state-dependent behavior only where transmitted state or gameplay requires it.
3. Resolve measured controller/networking discrepancies; extend maps and hit-volume coverage.

Horizontal curve execution and supported scheduling are implemented, with live owner corrections in `06e2e40`. The final run has 378-unit A travel on both sides without the previous 424-unit server overshoot, but the physics verdict remains diverged. Detailed evidence and limitations are in `physics_alignment.md` and `build/velqor/physics_alignment.json`.
