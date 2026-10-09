# Physics alignment roadmap

Baseline: commit `3122b12` aligns ordinary jumps with client XML motion curves. Client binary and extracted XML evidence inform server behavior; the goal is packet-compatible gameplay, not a clone of client internals.

## Scope: packet-observable behavior

The server must consume transmitted inputs, simulate gameplay-relevant movement/collisions, and send results the client interprets correctly. Client state-machine structure, internal movement-type fields, RNG sequence, and PhysX implementation identity are not standalone parity requirements. Investigate them only when necessary to explain an observable protocol/gameplay discrepancy.

`CQ_PlayerCastSkill::PosStruct` already carries position, destination, movement direction, rotation, speed, and client time. Validate motion selection against that packet state. Random branch differences matter only when they create divergent movement that the protocol cannot reconcile; reproducing the client's private RNG is not a goal by itself.

| Area | What remains | Status |
| --- | --- | --- |
| Horizontal skill motion | Authored execution, owner graph metadata, first-tick ownership and signed endpoint clamping are verified in live Sniper A/Shirk and Mechanic Shirk runs. Moving Assassin WARP now exercises native direction reset, segmentation and action cancellation. Broader skill, modifier, WARP and remote coverage remains open. | Implemented; broader live owner coverage established |
| Action scheduling | Selected clips, continuous loop elapsed, first-playing command ownership and rounded `f32` action-clock progression are implemented and exercised. WARP can leave its clip through an actual input state transition after the authored gate releases. Native notification-consumption phase, actual QPC hitches and remote playback remain unequal or unverified. | Native clip and supported WARP transition rules matched; raw phase still measured |
| State-dependent movement | Current authored WARP actions retain independent walking ownership and state-gate release through replay. General stance variants, graph duration/amplitude modifiers and other state-specific movement permissions remain open. Ordinary jump extraction currently uses base-master/default-stance profiles. | Supported WARP ownership exercised; general state-dependent behavior open |
| Jump transition details | Requested/entered/first-playing phases and the rounded-clock terminal remainder are implemented and verified for all five directions. All 200 default authored profiles pass collision regression. Native consumption/network phase, contact-history feedback and exact collision flags remain unequal. | Native entry phases matched; raw trajectory parity open |
| Controller behavior | Both x86 runtimes at client float24 precision exactly replay 3,359 retained moves/flags. Production native x64 replay is within 0.014218 units with one flag difference; public precision24 does not eliminate the x64 residual. | Native cutover and causal numerical evidence established; exact x64 parity open |
| Collision scene coverage | DeathMatch loads all 168 enabled ART props, authored box/convex actors, nine scaled meshes and gate/wall PHYSICS timelines. The native enable command now also controls shape ray queries, preserved through scene replay. Other maps and general hit-query/presentation remain open. | Authored scene and native SDK/query-state laws exercised |
| Hit volumes | Resolve how class-specific hit volumes interact with gameplay queries. They are distinct from locomotion capsules. | Not started |
| Network reconciliation | Approved CN/CQ replay retains 512 before-step frames and 2,048 events, learns real clock mappings, and never seeks packet positions. IOCP and elapsed locomotion are exercised. TAG validation now retains observed pending-ack poses at unchanged QPC timestamps and rejects unsupported identity transitions. The unchanged raw-QPC 25-unit gate still fails. | Input replay, completion networking, elapsed motor and TAG-ack validation exercised; strict live parity open |

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

- Runtime commands belong to selected clips. Entry resets elapsed; loops share one continuous full-clip clock; completion is checked before update; the successor fires on its first following playing update. Nominal offsets remain metadata, not dispatch deadlines. Timer/WARP Delay is relative to actual activation; owner GRAPH does not add Delay.
- STATE_BLOCK releases a client state-transition gate after Delay. It is not a WASD velocity lock. The incorrect server movement lock was removed; private PC_StateBlock logic is not cloned.
- Delayed graph motion and WARP execute on their authored boundaries. Graphs apply the terminal delta once and relinquish movement on the following tick. Replacement/tag cancels old work without sampling it.
- One SN_ExecuteSkill is emitted at normal execute-commit, with full graph metadata even for delayed onset. The actual owner handler at `0x00a05e94` caches that metadata before the animation installs movement; omitting it prevented A from moving. Delaying the packet changes execute semantics and is not the solution. SN_CastSkill carries the actual action state. Packet layouts are unchanged.
- Historical runtime smoke covered nominal Sniper 1.0-second onset, cancellation, nonzero H_Y0, Priest WARP and multi-clip lifetimes. The native-observed clip correction supersedes the nominal onset pin: the rounded-clock server starts A at logical 1.083333 seconds without a fixed delay.
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

## Post-IOCP physics continuation

Checkpoint `f24ef4a` preserves the previously verified work. The next reproduced
error was whole-tick locomotion quantization, not a different jump/graph curve
or a filtered client pose sample. A 0.899992943-second hold could travel only
525.759888 units instead of 535.675720 when its input phase crossed a tick
boundary.

Human walking now integrates raw direction times speed across the actual
mapped event intervals. The fixed-tick physics motor receives that displacement;
gravity, authored graphs, SDK simulation, packet layouts and client behavior
are unchanged. All 26 Release/CHECKED regressions pass; their phase case now
travels 535.675598 units. Existing whole-tick transient/implementation-state
pins were removed rather than re-pinned.

The direct and at-least-120-ms-added-RTT runs retain zero corrections and prove
the delivered-input origin fix, not global wall-clock parity. The direct run
receives/starts four jumps; the delayed run receives/starts all five. Both
skills complete, and STOP/disconnect work. Native action onset and pre-arrival
input intervals remain separate from this corrected motor duration. The
unchanged 25-unit comparator still diverges; delayed TAG acknowledgement also
has four old-master identity samples. See `physics_alignment.md` and
`build/velqor/matching/elapsed_walking_evidence.json`.

## Return-path diagnosis and TAG validation

Passive packet-hash correlation disproves the proposed Nagle explanation for
the measured action gap: nine cast/execute/jump packets reach the client
receive API in 0.0088–0.0304 ms. Native A consumption then waits 8.2951 ms;
graph motion begins 1,083.3935 ms after that consumption. No socket option,
fixed action delay, client timing or packet metadata was changed.

TAG acknowledgement is now an explicit validation state, not simultaneous
master switching. Same-owner/epoch/contiguous slot-swap evidence and the real
native identity change bound it. Four historical poses are retained rather
than discarded; the historical raw maximum remains 543.666565 units. Missing
acknowledgements, gaps, epoch changes and unknown identities still fail.

Nineteen evidence tests pass. The final clean at-least-120-ms-added-RTT scenario
completes all control/identity gates, applies all five jumps and both skills,
and retains zero corrections. Its raw physics maximum/mean remains
286.260153/40.837203, not parity. Final-source five networking and 26 physics
regressions pass in Release and Debug/CHECKED. See
`build/velqor/matching/return_path_tag_evidence.json`.

## Native clip progression: corrected

`NativeClipBoundaries`, `AuthoredClipData` and `NativeFrameClock` supplied binary/data evidence; the parent verified the integrated change. A's middle three-loop clip has continuous elapsed, not three separately rounded restarts. Its callbacks occur on the first playing update after each entry. `MxMApplication_MeasureTime` publishes the difference between successive rounded `f32` application clocks; direct `elapsed += float(1/60)` misses this arithmetic.

The server retains per-clip readiness, clock and once-only effects through replay. Both previously failing physical graph-entry regressions pass. The local and at-least-120-ms-added-RTT scenarios complete all five jumps, both skills and all control/identity gates without corrections, with configs restored. Raw maximum/mean error remains 163.705894/7.773095 locally and 268.478331/29.104031 with added RTT. Native-consumption phase and ordinary-jump launch remain separate failures at the unchanged 25-unit gate.

Evidence: `build/velqor/matching/native_clip_progression_evidence.json`, `20261009_sol_clip_clock_local` and `20261009_sol_clip_clock_rtt120`. Temporary clip observers are removed; their raw captures are retained. Subsequent jump-clock and launch work is recorded in `physics_alignment.md`.

## Match-local clock and SDK substeps: corrected

An immutable per-match physics epoch prevents a new match from inheriting the dedicated lane's multi-day `f32` timestep degradation. The real-World/CCT uptime regression fails before the correction and passes afterward; replay keeps the same epoch and original frame timestamps. Read-only native `getTiming` proves fixed SDK substeps with a 1/30-second maximum and one iteration, rather than the SDK defaults. The server descriptor now matches that policy without changing the 60 Hz motor lane.

All 29 physics regressions pass in Release and actual CHECKED Debug. The original observation harness, after removal of the temporary timing observer, completes unchanged-client local and at-least-120-ms-added-RTT scenarios. Both retain all five jumps, both skills, zero corrections and complete control/identity gates. Their raw trajectories still fail the original threshold; exact counts, errors and relay shutdown accounting are recorded in `physics_alignment.md`.


## SDK dispatch and gate query state: corrected

Native wrapper/API observation distinguishes the fixed SDK descriptor from its advance policy. The server now mirrors ceiling-rounded 30 Hz batches, intervening skipped frames, the uncapped target advance and eight-iteration cap, without advancing SDK time during owner replay. The real rigid-body boundary regression fails before the correction and passes afterward.

The native PHYSICS consumer proves that enable controls both actor collision and shape raycasting; REMOVE is a separate destruction branch, not a second query boolean. Supported DeathMatch commands now preserve that law and actual scene-history ray state. A real ray query hits a closed gate, misses an opened gate and hits again after closing. Stable states of all four gate actors match the original client; the initial two-frame opposite-gate consumption interval remains explicit evidence, not a new state-policy rule.

All 31 physics regressions pass in Release and actual CHECKED. Original-harness local and at-least-120-ms-added-RTT scenarios complete three skills, all five jumps, two TAG transitions and all required controls without corrections. Source and harness observers are removed after proof; raw physics remains divergent. Exact evidence and final-law metrics are recorded in `physics_alignment.md`, `native_sdk_dispatch_evidence.json` and `native_gate_query_evidence.json`.

## Resumed WARP alignment: corrected

DeepSeek V4.1 Flash reviewers supplied native direction and lifecycle evidence; the parent reproduced and verified the changes. WARP clears native movement direction before destination selection, so the server uses body yaw instead of stale transmitted direction. Forced local movement now uses the native 50-unit segmentation policy; a real-SDK staircase fails before correction and passes afterward.

Current WARP programs preserve walking permission separately from forced displacement and gravity. Known held movement requests can cancel them after the authored `STATE_BLOCK` gate opens without another packet; actual behaviour changes remain distinct from ordinary movement snapshots. Cancellation releases destination and collision mask together rather than waiting for the entire animation. Replay restores this state and publishes return actions once. The owner-position echo, its obsolete notification ledger and the invalid destination-iterator proxy are removed.

All 35 physics regressions pass in Release and actual CHECKED. Final unchanged-client moving-WARP local and added-120-ms-RTT scenarios complete all required controls, identity, five jumps, STOP and disconnect checks without corrections. WARP cancels on movement return and both Sniper skills complete. Both relay streams forward entirely with equal hashes; configurations restore and no owned runtime survives.

Full raw-QPC physics still diverges at the original 25-unit threshold. The first local WARP endpoint retains a 10.743151-unit phase/origin discrepancy, and pre-send evidence proves a separate information limit before any movement packet exists. Exact counts, errors, binary hashes and observer cleanup are recorded in `physics_alignment.md` and `build/velqor/matching/native_warp_evidence.json`.

## Held-input and frame-time timer continuation

Checkpoint `b95e957` is committed. Native `MainCharInput_UpdateMove` services a held direction when the authored state gate opens; it does not wait for the next position packet. Fresh explicit behaviour changes are now separate from cached ordinary snapshots, so zero-direction STOP and standing owners do not cancel prematurely.

The gameplay timer is a frame-time counter, not a wall-clock deadline: each frame contributes truncated integer milliseconds without fractional carry, and timer callbacks run after input/actor processing. Supported WARP gates now preserve that counter and service phase through replay. No bonus tick, trajectory shift or new reconciliation stream was added.

All 38 Release/CHECKED physics regressions pass, including failing-before/passing-after held-return, unit-filter and timer-boundary scenarios. Final unchanged-client moving local, added-120-ms-RTT and standing runs pass all required controls and five jumps without corrections. WARP ownership duration now matches the measured native local interval; full raw-QPC parity still fails the original 25-unit gate. Native notification consumption, pre-send information, standing clip-end phase and broader state/remote coverage remain explicit gaps.

Exact final metrics, relay tail accounting, restoration checks and native addresses are recorded in `physics_alignment.md` and `build/velqor/matching/native_held_return_evidence.json`. The continued correction is separate from the committed checkpoint.



## Recommended broader sequence

1. Resolve the remaining same-request collision-flag/narrowphase differences and extend live comparisons to remote delayed-graph presentation, other skills, and collision boundaries.
2. Align state-dependent behavior only where transmitted state or gameplay requires it.
3. Resolve measured controller/networking discrepancies; extend maps and hit-volume coverage.

Horizontal curves, supported scheduling, native clip phases and ordinary-jump entry/terminal rules are implemented. DeathMatch uses complete PHYX v2 recipes, native scale-and-cook and the observed fixed SDK substep policy. Native x64 PhysX2.8.4 and its accepted 0.014218-unit/one-flag same-request residual are retained. Timestamped replay restores server history, advances only the owner and publishes effects once; packet poses never become seek targets. Match-local motor clocks do not inherit dedicated-lane uptime. Elapsed human locomotion and TAG acknowledgement validation retain actual timestamps and pending poses. Native notification-consumption phase, contact-history feedback, unavailable future inputs and broader remote/state coverage remain open at the unchanged 25-unit gate. The client stays vanilla; no correction stream, retiming or relaxed tolerance conceals failures. Run Debug test executables from `build/checked/` with working directory `build/`; verification counts and raw evidence are maintained in `physics_alignment.md`.
