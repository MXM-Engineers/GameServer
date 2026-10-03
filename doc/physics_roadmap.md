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
| Jump transition details | All five directional jumps are accepted in actual-client runs and all 200 default authored profiles pass collision regression. Network/action launch offsets and exact collision flags remain unequal. | Exercised; exact transition parity open |
| Controller behavior | Both x86 runtimes at client float24 precision exactly replay 3,359 retained moves/flags. Production native x64 replay is within 0.014218 units with one flag difference; public precision24 does not eliminate the x64 residual. | Native cutover and causal numerical evidence established; exact x64 parity open |
| Collision scene coverage | DeathMatch loads all 168 enabled ART props, authored box/convex actors, nine scaled meshes and gate/wall PHYSICS timelines. All 194 named actor categories/masks/materials/collision-enable states match the frozen client scene. Other maps and two gate ray-query flags remain open. | DeathMatch authored scene implemented and exercised |
| Hit volumes | Resolve how class-specific hit volumes interact with gameplay queries. They are distinct from locomotion capsules. | Not started |
| Network reconciliation | Approved CN/CQ replay retains 512 before-step frames and 2,048 events, learns clock mappings from actual timestamp/handler-time pairs, and never seeks packet positions. Vanilla measurements isolated Windows polling/CPU-affinity contention. IOCP reduced median socket-send to completed actor update from 22.93 to 10.34 ms; the unchanged raw-QPC 25-unit physics gate still fails. | Input replay and completion-driven networking exercised; strict live parity open |

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

Scope: existing supported movement effects, not a new combat-command or general stance system. Live unfocused owner captures verify Sniper A and moving Shirk. Remote animation and delayed-graph presentation still need comparison against authoritative updates. The execute handler swaps the wire duration/origin fields in its local stack before both owner and remote setters; both cache origin at +0x194 and duration at +0x19C.

## Server network latency: measured cutover

Windows game/hub/matchmaker `Server` sockets now use one IOCP completion worker
per server, without shared CPU pinning or per-client threads. Buffers remain
owned until their send/receive completions retire. TCP fragments are retained,
and stale handles cannot route to reused connection slots. Standalone internal
connectors and Linux retain their existing networking paths.

The unchanged Release scenario produced 44 correlated movement packets in the
retained polling baseline and 46 after cutover. Original packet hashes,
endpoints, TCP order, and native/decrypted actor/time fields establish identity.
Actual QPC timestamps were compared without trajectory retiming.

| Measurement | Polling baseline | IOCP |
| --- | --- | --- |
| Socket-send to receive-completion processing, median | 12.111 ms | 0.02135 ms |
| Socket-send to completed owner update, median | 22.9261 ms | 10.33695 ms |
| Socket-send to completed owner update, mean | 31.22257 ms | 8.90508 ms |
| Socket-send to completed owner update, observed maximum | 78.2333 ms | 16.5447 ms |
| Combined game/hub/mm network CPU, share of one core | 100.4431% | 0.1208% |

The CPU baseline is a separate approximately 75-second passive capture; the
approximately 78-second IOCP capture includes one active client. This is not
a throughput guarantee for 200 clients. The remaining receive-to-handler
median is 9.8443 ms; tick-driven dispatch is unchanged. Full original-client
and LEA hashes remained unchanged, temporary observers were removed, and the
original recording harness and final server sources were rebuilt.

Final-source Release and Debug/CHECKED runs passed all five real-socket tests
and all 25 physics regressions. Networking exercises 200 simultaneous peers,
5 MiB slow-reader delivery, full-capacity slot reuse, stale handles, and bounded
pending-operation shutdown/resource cleanup.

A final unmodified-client run through a transparent relay added at least
60 ms each direction. All five jumps applied; both skills cast, executed, and
completed; STOP and disconnect release worked with zero corrections. The
complete comparator still fails four old-master identity samples during the
delayed TAG acknowledgement, and full physics parity remains open. Neither
the raw-QPC comparison nor its 25-unit threshold was changed.

Evidence: `build/velqor/matching/network_completion_evidence.json`, with raw
captures in `20261003_122017_sol_iocp_vanilla` and
`20261003_124117_sol_iocp_rtt120`.

## Recommended broader sequence

1. Resolve the remaining same-request collision-flag/narrowphase differences and extend live comparisons to remote delayed-graph presentation, other skills, and collision boundaries.
2. Align state-dependent behavior only where transmitted state or gameplay requires it.
3. Resolve measured controller/networking discrepancies; extend maps and hit-volume coverage.

Horizontal curves and supported scheduling are implemented. DeathMatch uses complete PHYX v2 authored recipes and native scale-and-cook. Production same-request maximum error is 0.014218 units with one flag difference; native x64 and that measured residual are explicitly retained. Timestamped replay restores server history, reruns only the owner, preserves action ownership, and publishes packets/actions once; packet snapshots never become seek targets. All 25 Release/actual-CHECKED regressions pass. The original native-motion-to-handler gap did not distinguish client queuing from server reception; subsequent unchanged-client measurements identified Windows polling/affinity contention, and the approved server-only IOCP cutover reduced it. The latest direct run completes its control gates but still exceeds the unchanged 25-unit physics gate; the delayed-network run also exposes old-master identity samples during TAG acknowledgement. Full physics and remote-presentation parity remain open. The client remains vanilla, with observation-only instrumentation permitted; no correction stream, trajectory retiming, or relaxed tolerance conceals failures. Debug test executables are in `build/checked/`; run from `build/`. Evidence is in `physics_alignment.md`, `recipe284_evidence.json`, `input_catchup_evidence.json`, and `network_completion_evidence.json`.
