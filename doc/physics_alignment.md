# Physics alignment

## Goal and status

Align server physics with `MXMClient_DP_p3.exe` using the supplied native x64 PhysX 2.8.4 SDK and legacy Nx controller API. Client-evidenced controller settings, authored movement, and live owner trajectory comparisons inform the implementation. Exact client/server physics parity is not established.

The client uses PhysX 2.8.4 through Gamebryo/NiPhysX. Its SDK creation call at `0x01125e5c` passes version `0x02080400`, matching the packaged SDK headers and successful server creation call. The supplied x86 DLLs differ byte-for-byte from the client's DLLs; the server uses x64 binaries. Matching SDK version does not establish identical builds or numerical results.

## Client evidence

The principal Ghidra evidence is:

- `0x01c2cf8f`: capsule-controller factory and effective controller settings.
- `0x01b7b097`: controller wrapper, dimensions, and collision mask.
- `0x01a471c1`, `0x0194b929`: entity/controller setup and MoveController component data.
- `0x01b7b360`: gameplay position derived from controller center minus its vertical offset.
- `0x01903c99`: collision-group enum values.
- `0x01c2de24`, `0x01c2e83d`: collision-name classification and group assignment.
- `0x01b7b6fa`, `0x01b7bb29`: controller movement and save/move/restore destination queries.

### Controller contract

| Setting | Client capsule path | Server implementation |
| --- | --- | --- |
| Movement radius | `ActorRadius`, 100 in supplied master data | Loaded from MoveController data |
| Capsule cylinder height | `ActorHeight`, 70 in supplied master data | Loaded from MoveController data |
| Skin/contact offset | 10 | 10 |
| Step offset | 70 | 70 |
| Slope limit | Approximately 65 degrees | `cos(65 degrees)` |
| Climbing mode | Descriptor +0x50 = 1; NxCharacter constructor copies it and move at `0x10013480` enables constrained climbing | Constrained |
| Up axis | Z | Z |
| Gameplay position | Feet-relative | Feet-relative |

Radius and skin must be considered together. The old server used radius 110 with contact offset 0.1, giving a rough inflated radius of 110.1, versus the client's 100 + 10. This was not simply a 10% size discrepancy. Matching the complete configuration is necessary.

MoveController dimensions are distinct from the class-specific PhysXMesh collider dimensions. The latter were not substituted into locomotion.

## Implemented changes

### Controller creation, movement, and lifetime

- Create controllers at a center computed from the supplied feet position, rather than treating feet as a center.
- Read live CCT state with `NxController::getDebugPosition()`. The supplied SDK's `getPosition()` returns published, filtered state; sharpness-zero movement must not use it for gameplay.
- Preserve feet when resizing the active master controller; skip unchanged dimensions.
- Use the client PC collision mask `0x1c7f06` consistently for ordinary movement, forced movement, and destination queries.
- Keep PC controllers non-blocking to each other, consistent with the mask excluding controller group 3.
- Assign explicit query groups to static collision shapes. Terrain and fences block PC movement; aim-only geometry does not.
- Restore pose, grounding state, and velocity after destination queries.
- Record grounding, clear downward velocity on landing, and clear upward velocity on ceiling contact.
- Remove the production infinite Z=0 ground plane previously added for visualization.
- Release controllers, their manager, and remaining scene-owned actors before releasing the scene.
- Preserve the full `u32` cooked-mesh payload length instead of narrowing it to `u16`.

### World movement and skill timing

- Obtain locomotion dimensions through const MoveController getters and resize on master changes when needed.
- Keep gravity independent of walking permission and action-transition gates.
- Require grounding and no pending or active ordinary jump before accepting another jump.
- Progress horizontal skill graphs over their advertised duration instead of applying their full displacement immediately.
- Execute the first action command and process zero-delay command batches without skipping them.
- Apply WARP through the forced-destination motor, with native local-mode 50-unit segmentation and collision filtering.
- Do not echo a WARP position to its owner: the unchanged client executes its own action locally. Ordinary observer movement updates remain intact.
- Retain WARP walking permission, authored state-gate release and destination ownership through historical replay; state cancellation releases the destination and mask together.
- Leave ordinary movement's owner-skip behavior and all packet wire layouts unchanged.

### Map collision scene

The shipped physics assets cover DeathMatch map `160000094` only. The game now asserts that the selected physical map is supported, after applying `DevForcedMap`, rather than silently simulating DeathMatch geometry for a different advertised map.

The scene includes:

- All 168 enabled ART props in document order, producing 170 authored actors, with terrain in the observed Prop group 2.
- Main and sidewall GuardrailMob actors as FenceAll and Aim geometry as Aim.
- Authored wall triangle/convex actors at each selected area transform, using groups 19/11 and the dynamic interaction mask.
- The two spawn gates, with their authored boxes, masks and collision enable/disable lifecycle.

PHYX version 2 carries original NXS streams, materials and actor/body/shape recipes. Original triangle and convex streams are loaded directly; non-unity triangle instances use native SDK scale-and-cook, cached by asset/mesh/scale. Instance rotation follows the measured `Rx(-x) * Ry(-y) * Rz(-z)` convention. Existing wall selection, identities and spawn metadata are preserved. Additional maps remain unsupported.

Debug-scene callers were migrated to explicit collision groups and feet-based reset positions.

## Verification performed

### Builds

Full Debug and Release builds passed. After the final scene-lifetime correction, both game-server configurations were rebuilt successfully.

### Isolated runtime scenarios

The scenarios exercised actual PhysX and server simulation code without launching the game client:

- Feet-based creation and feet-preserving resize.
- Client-derived contact, step, slope, and climbing settings.
- Falling through Z=0 in an empty scene, proving removal of the invisible floor.
- Consistent Step, Move, and query filtering against terrain, fences, aim geometry, and another PC controller.
- Query preservation of airborne state and velocity.
- Floor grounding and ceiling response.
- A 50-unit step climbed; a 120-unit step blocked.
- A 60-degree slope climbed; a 70-degree slope blocked.
- A cooked mesh with a 601,234-byte payload loaded successfully.
- XML-derived world controller dimensions.
- Rejection of repeated airborne jump impulses.
- Continued gravity during an input lock.
- Horizontal graph midpoint and endpoint timing.
- Delayed WARP execution and owner endpoint correction at execution time.
- Zero surviving shapes after all test scenes were destroyed.

The tests exposed a scene cleanup leak: releasing a PhysX scene removes actors but does not destroy them. Explicit actor release fixed the leak, and the tightened scenario passed.

Temporary verification sources, binaries, and response files were removed after execution.

### Server startup

The normal matchmaker, hub, game, and login processes reached readiness using managed consoles and log-based readiness checks. Hub and game registered with the matchmaker, and the game initialized physics and created its development match.

Initial probe-based launch attempts produced early exits. Serialized console startup with log-based readiness was stable; the cause of the earlier exits was not isolated, and no unrelated networking workaround was added.

Startup verification is historical, not a promise that those processes remain running. Subsequent supervisor notifications reported the game exiting with code 0 and hub, login, and matchmaker exiting with code 1.

## Remaining parity gaps

- Horizontal authored curves and live owner Sniper A/Shirk comparisons are implemented. Other skills, runtime graph modifiers, and remote presentation need broader coverage. See `physics_roadmap.md`.
- Supported action scheduling now uses client-evidenced command/clip timing and has passed real-runtime smoke coverage. Live remote-client presentation of delayed graphs remains unverified.
- The action loader selects one stance timeline to avoid duplicate effects, preserving last-GRAPH stance selection where present. General stance switching is not implemented.
- Only DeathMatch collision assets are supported. Additional maps require verified geometry, transforms, collision categories, and dynamic-state behavior.
- The separate role of class-specific hit volumes still needs further alignment work.
- General owner prediction/reconciliation is not implemented. Input replay does not supply future input or the client's notification-consumption phase.
- Live trajectories remain unequal near jumps and collision boundaries. Matching SDK versions and scene recipes do not establish identical numerical results.

## Ordinary jump alignment

The earlier impulse estimate and universal constant-speed-fall conclusion were incorrect. Ordinary player jumps use authored vertical animation graphs, not a fixed launch impulse.

### Verified client path

- `0x009e88bd` dispatches jump input through `0x01a4be16`, entering JUMP_START; `0x01319225` sends `CQ_PlayerJump`.
- `0x01b62bac` loads animation motion graphs. `0x01b62dee` reads `V_Time` and `V_Y0` through `V_Y10`.
- `0x01b61e65` installs the active animation's graph through `0x01a4d95a`. A positive graph duration overrides the animation duration; otherwise the animation duration supplies the time domain.
- `0x01babb58` configures graph duration, `0x01bab98d` installs samples, and `0x01baaf45` applies playback/amplitude scaling.
- `0x0238bfb4` performs piecewise-linear interpolation over evenly spaced samples. `0x01bab26f` evaluates the vertical graph; `0x01bab1fd` advances time and returns the height difference, using a zero baseline at launch.
- `0x01baaeee` applies that difference to Z. While the graph is present, even a zero graph delta suppresses ordinary velocity-driven vertical displacement.
- `0x01a4f3c1` integrates vertical velocity using `DAT_0312aa38 * dt`. The option loader `0x005e2cfc` populates this global from `CreatureGravity`; the extracted `Design/GAMEINFO/Option.xml` value is `-2100`.
- `0xc4750000` represents **-980.0f**, not -1000. The probe-based `0x01ad339c` path does contain a 980-unit/s descent, but it is not the universal player jump law.

### Server implementation and data

`scripts/extract_jump_motion.py` imports the extracted client StatePlay resources, matches them against repository `CREATURE_CHARACTER.xml` and `AniLength.xml`, and writes `gamedata/JumpMotion.xml`. Regenerate with:

```text
python scripts/extract_jump_motion.py "D:/Projets/MxM/MxM_12147 extracted/Data"
```

The generated data contains 200 profiles: Stand, Front, Left, Right, and Back for all 40 loaded masters. Direction routing follows each MoveType node's CaseValue names and child order, not a globally assumed order. The extraction selects base-master resources and `STANCE_DEFAULT`; RNB's unconditional jump graph supplies all directions. Statesman's alternate stance differs by one float ULP in one duration; the base value is preserved. Materially different branch curves fail extraction rather than being silently flattened.

All current profiles use animation duration because `V_Time` is zero. Durations are not uniformly 0.6 seconds. AndroA's 0.6-second standing graph peaks at 350 units; its moving graphs peak at 330.

The content loader requires complete, finite profiles and negative finite gravity. `JumpMotion::Sample` performs allocation-free, clamped piecewise-linear sampling. World admits a grounded request only when no jump graph or pending jump is active, then progresses request, state entry and first-playing installation on successive updates. `PhysicsScene::InstallJumpGraph` installs the already-admitted profile without a second grounding test or scalar impulse. Gravity continues accumulating while authored height differences own Z; completion returns control to falling. PhysX constrains every displacement against geometry.

World input preserves the jump packet's movement direction and converts its facing to world yaw for profile selection. Master tagging selects the active master's data. Existing movement locks and owner-skip replication behavior are unchanged. The unused jump/gravity tweakables and their debug controls were removed.

### Verification and limits

A throwaway executable linked the real content loader, World, replication, and PhysX implementation. It passed:

- All 200 profiles at every sample knot, interval midpoint, and endpoint.
- Full standing trajectory through an actual capsule controller: 36 ticks at 60 Hz, peak 350.000 units.
- Grounded launch, airborne-repeat rejection, landing, and another jump after landing.
- Destination-query restoration during a jump.
- Low-ceiling collision and return to the floor.
- Falling without a graph, matching the discrete `CreatureGravity` integration.
- World-level selection of all five directions, rotated facing, master tagging, and movement-lock rejection.

`cmake --build build --config Release` passed for all server and tool targets. Regenerating the jump data from the extracted resources produced a byte-identical XML file. Temporary smoke sources, executable, project, object files, and captured output were removed after verification.

Ghidra graph, jump-state, integration, and option functions were named, and misleading prior annotations were corrected. No side-by-side live-client recording was performed. Exact state-transition/collision timing across PhysX versions, alternate stances, and non-default graph playback/amplitude modifiers still require live comparison or broader state-system support; this implementation does not claim full client physics parity.

## Horizontal animation graph alignment

The server now consumes `gamedata/HorizontalMotion.xml`, reproducibly extracted from actual StatePlay XML by `scripts/extract_horizontal_motion.py`. It contains 71 skill-reachable class/action profiles and 156 branches, preserving MoveType, Random case labels, HorizonRotate, per-animation duration, and variable H_Y sample counts. Positive H_Time overrides AniLength; nonzero H_Y0 is preserved.

Client evidence:

- `0x01b62dee` reads graph time and H_Y samples; `0x0238bfb4` samples them with uniform piecewise-linear interpolation.
- `0x01b61e65` installs the horizontal animation graph through `0x01a4dc6b` and `0x01babc79`. `0x01babbe6` requires cached graph metadata; `0x01bab6d5` scales samples using cached endpoint distance and the signed final knot, not ActionBase Param1. The owner execute path supplies the required duration and distance before delayed animation activation.
- `0x01bab102` returns successive sample differences with a zero previous value at initial graph time. `0x01baabea` applies them along the graph's facing and HorizonRotate; the caller reads entity CB0 plus PI. Existing world-yaw conversion requires subtracting HorizonRotate.
- The actual Random selector is `0x01c055e6`, selecting through the authored cumulative CaseValue thresholds using an RNG call with bounds 0 and 100. `0x01c05839` is a timeout selector, not Random.
- `0x01c083b4` reads entity B50 for MoveType selection. Server selection uses transmitted movement input and facing sectors; recreating the client field's producer chain is not required unless it explains a packet-observable mismatch.

World applies successive sample differences through the controller, including the client-derived signed endpoint clamp described below. Collisions clip only that step; removing an obstacle does not cause accumulated endpoint catch-up. Cast endpoints describe authored terminal travel, and packet layouts remain unchanged. The implementation uses existing server RNG rather than a fixed Random branch. Private client RNG sequence matching is not a requirement; unreconciled, observable motion disagreements require further work.

Historical server-only verification: all Release targets built; real World/PhysX smoke passed nonlinear Assassin travel (-286.05 rather than command -300), Defender control (900), Sniper initial-sample and endpoint behavior (-314), ESPER directional rotation (750), Sniper SHIRK endpoint (375), both standing random branches, and obstacle clipping/removal without catch-up. Explicit-duration profiles were loaded and exercised. XML regeneration was byte-identical. Temporary verification artifacts were removed. Later live owner verification is recorded below.

Full remaining work is tracked in `physics_roadmap.md`.

## Packet-observable action scheduling

ActionBase loading retains the selected clip sequence, each clip's one-loop AniLength and AnimationLoop count, and each command's owning clip. Nominal execution/completion offsets remain diagnostic metadata, not the runtime dispatch clock. Negative-animation-index commands start globally; clip-owned commands wait for the first playing update of that clip. Readiness is independent of another command's later deadline, and command effects/notifications execute once across replay.

Client evidence:

- `0x01ab799b` loads Delay, AniLength, command type, and animation index. `0x01ab8da1` sorts through `0x01ac6bb5` and computes clip-relative offsets. `0x01ac6021` dispatches commands; timer delays are converted to milliseconds.
- STATE_BLOCK dispatch at `0x01ab929e` queues a timer. Its consumer `0x01abfbf4` calls `0x01a4bbf5` to release a state-transition gate; `0x01a4b98d` resets the gate on state entry. This is distinct from a WASD velocity lock and from the PC_StateBlock table. The server's former `lockedMoveUntil` interpretation was removed.
- GRAPH_MOVE_HORZ dispatch at `0x01ac085d` and WARP at `0x01ab9fc3` inform the supported movement effects.
- Owner GRAPH_MOVE_HORZ at `0x01ac085d` starts at the current graph clock and ends at that clock plus duration. Its Delay argument is not an additional owner wait. STATE_BLOCK retains its timer delay; WARP delay is relative to its clip's actual activation.
- The actual SN_ExecuteSkill handler is `0x00a05e94`. On the owner path it commits execution through `0x0132aa06`, then caches graph endpoints, origin distance, and duration through `0x01a4bd46`/`0x01bab857` when bApply is set. It does not install physical graph movement immediately. Send full metadata at normal execute-commit even for delayed graphs; the animation activates movement later. SN_CastSkill uses `0x00a05b81`. `0x009f6a03` is the actor-enter 62029 graph path, not execute 62036.

Runtime graphs apply their final sample delta once, suppress stale ordinary movement for that tick, and stop touching input thereafter. Replacement and tag cancel pending programs before their old effects execute. WARP uses the native owner action without an owner-position echo. Statesman's overlapping DEFAULT/C command rows no longer execute twice; the existing last-graph stance choice remains the supported selection rather than introducing general stance switching.

Historical server-only verification used a temporary executable with real loaded content, World, PhysX, and Replication frame queues. The full Release build passed. These results predate the live owner corrections below:

| Scenario | Result |
| --- | --- |
| Assassin SKILL_3 | Authored endpoint -286.049988; movement blocked during graph and resumed afterward |
| Assassin SKILL_2 | Program still live after first 0.1-second clip; completed after the 1.266667-second sequence |
| Sniper SKILL_1 | Historical smoke checked the 1.0-second onset but omitted delayed-graph metadata; live validation subsequently exposed and corrected the owner's missing movement |
| Pending Sniper graph | Replacement before and across onset left position unchanged |
| Sniper SKILL_2 | Initial H_Y0=-3 applied |
| Defender SKILL_4 | Terminal position 900, then 906.666687 with fresh input while the 1.5-second action remained live |
| Priest SHIRK | No movement/correction before 0.5 seconds; displacement 1000 and one correction afterward; no repeat |
| Pending Priest WARP | Replacement and tag prevented later displacement and correction |
| Assassin SHIRK | Zero-delay WARP applied with one correction |

The Defender terminal regression failed the first runtime smoke at position 880. Refreshing the final graph-owned input and blocking ordinary integration for that tick corrected it; the rerun passed. These checks exercise server motion and replication queues, not a live network/client capture. Remote delayed-graph animation/presentation remains unverified; private client state machines are not an implementation requirement.

## Live owner graph and walking verification

Commits `e7c5ddd` and `06e2e40` add the unfocused Velqor harness and the verified physics corrections. The client authority has SHA256 `df32850dd8e7a2329ddca2dc57a3eb310a620f4a6477bbb135049a938f477583`.

- Preserve transmitted human movement direction rather than repeatedly steering toward a stale position-derived goal. Bots retain destination-based movement; zero human direction stops without seeking a packet snapshot.
- `0x01bab102` advances graph time before sampling. The first owned tick applies `Sample(dt)-0`, including a nonzero first knot, and excludes ordinary walking even when its delta is zero.
- On the owner 62036 path, the setter receives origin distance before duration: Movement+0x194 is origin distance and +0x19C is duration. `0x01babbe6` rejects a near-zero +0x19C, explaining the missing A movement when delayed graphs were sent without metadata.
- `0x01bab17a` upper-clamps the scaled sample to the endpoint chord. With the emitted endpoints this is equivalent to min(raw, final) for positive final knots and max(raw, final) for negative ones. The clamp releases if a nonmonotonic curve returns through the endpoint; it is not an irreversible stop or a clamp to zero.
- GRAPH_MOVE_HORZ Param2 controls collision filtering (`0x01ac085d`, `0x01b7b495`), not travel permission. The lower-error travel-gate experiment was rejected. Delaying execute-commit was also rejected.

The default `owner_sniper_forward_then_a` scenario now holds forward across Shirk activation. Run `20260913_physics_clamp` exercised the actual unfocused client and server:

| Measurement | Result |
| --- | --- |
| A client/server displacement | 378.001395 / 378.000019 |
| A displacement disagreement | 0.001376 |
| A server maximum excursion | 378.000019, previously 424.000 |
| A raw paired error, mean / maximum | 6.872450 / 139.008631 |
| Moving Shirk first graph tick | 103.136082; preceding input speed 595.200012; ordinary velocity zero |
| Corrections, desktop mutations, client focus | 0 / 0 / 0 |
| Pose pairs | 2507 |

Release `game_srv` built successfully; all 11 diagnostic regression tests passed. STOP and disconnect released held input. All owned processes exited and all four server configuration files were restored byte-for-byte. Raw traces and machine-readable evidence are retained under `build/velqor/runs/` and `build/velqor/physics_alignment.json`.

The physics verdict remains **diverged**. No timestamp shifts, tolerance changes, forced corrections, or endpoint teleports were used. The baseline A window had mean/max error 248.043335/424.000111 with a stationary client; whole-run baseline scores are not directly comparable because later probe stimuli and ledge approaches differ.

At this stage the controller defect was measured but not corrected: at matching XY the server could remain grounded 70 units above the client's floor, then leave the ledge later. In `20260913_physics_duration`, the exact peak difference was [-50.254886, -0.000485, -333.141296], magnitude 336.910488. The moving-Shirk run had a 370.279694 whole-run maximum. The subsequent linked-controller investigation and corrections are recorded below.

Remote `0x01a4a2f2` caches metadata before starting the action and does not immediately install a graph. The execute handler `0x00a05e94` swaps the wire duration/origin fields in its local stack before either setter. Both owner and remote cache origin at Movement+0x194 and duration at +0x19C; the earlier opposite-order claim was incorrect. Actual remote playback, near-zero terminal knots, and broader skill/state coverage remain unverified.

## PhysX 4 linked-controller baseline

The wire client and its actual `NxCharacter.dll` and `PhysXCore.dll` were reversed together. Matching descriptor constants was insufficient: the previous Px4 overlap-recovery path created the 70-unit perch, but merely disabling recovery allowed a 120-unit box to be traversed. Both stock choices were rejected.

Before the native SDK cutover, commit `7a7b792` built a pinned local CCT from PhysX `a2c0428acab643e60618c681b501e86f7fd558cc` while retaining the 4.1.2 core backend. Its recorded source closure, license, private-DLL ABI and local changes are historical baseline evidence; that compatibility CCT has now been removed:

- No penetration/MTD recovery in production, and no Px4 SIDE-sensor pass. DOWN contact sets the DOWN flag; constrained-climb and mesh-slope gates follow `NxCharacter` `0x100129d0`.
- Primitive zero-distance hits are rejected; geometric mesh zero-distance hits remain eligible. Boxes are cached as analytic OBBs, not twelve triangles, so primitive and mesh rules cannot be accidentally conflated.
- Mesh sweeps use the Px4 geometric path (`eASSUME_NO_INITIAL_OVERLAP`), not its default initial-overlap result with `normal=-direction` and undefined contact position. The latter caused a phantom ceiling and a 494-unit moving-jump replay error. The linked legacy `PhysXCore` sweep at `0x10194ce0` has no such short-circuit.
- The wrapper preserves cached state for component-wise strict float-epsilon zero requests, invalidates the cache once, and uses 50-unit chunks only when selected. Final-chunk flags overwrite previous flags. The client selects ordinary segmentation when horizontal speed exceeds 2000, not for every jump or fall.
- Gameplay feet use `float(center.z)-float(offset)`, not `float(center.z-offset)`. Movement requests use `(float_position+displacement)-float_position`; sub-ULP travel does not accumulate invisibly in the extended controller center.
- Horizontal graph XY and active vertical graph Z are combined into one controller request. A horizontal graph suppresses ordinary vertical displacement but still integrates gravity velocity. Graph time advances by client-style f32 addition once per world update; the first active sample is dt, including delayed activation. Lane simulation time advances with fixed simulation ticks, not wall-loop jitter.

### Runtime and same-request evidence

`20260913_sol_legacy_laws` completed the full owner scenario, including TAG and ledge travel, with 3,411 independent pose pairs. All five standing/front/back/left/right jumps were accepted and applied; native peaks were 329.998963–329.999270 units. Sniper A first server displacement was 27.801588, not the previous two-dt sample; moving Shirk first displacement was 103.136255. The native Shirk first request selected segmentation; subsequent graph and ordinary jump requests did not.

| Measurement | Observed result |
| --- | --- |
| Raw-QPC maximum / mean position error | 166.582262 / 15.925870 |
| A maximum / mean raw-QPC error | 111.208427 / 5.580421 |
| A endpoint disagreement | 0.001721 units |
| Matched XY (<1 unit), both actually grounded | 2,483 pairs; maximum Z difference 0.000061 |
| Server / client corrections | 0 / 0 |
| Desktop mutations / client focus / cursor movement | 0 / 0 / 0 |
| Source-linked request replay, class 6 | 2,501 requests; max 0.079655, mean 0.001240; two flag differences |
| Source-linked request replay, class 5 | 821 requests; max 0.093694, mean 0.048821; no flag differences |

The replay uses each master's first actual pose only, then every captured displacement, active mask and segmentation choice unchanged. There were no trajectory discontinuities, intermediate resets or retiming. It compares the same gameplay float-coordinate contract as the client; raw extended SDK coordinates are not substituted. Raw traces, requests and results are retained in `build/velqor/runs/20260913_sol_legacy_laws/`, `legacy_laws_cct_motion_replay.json` and `legacy_laws_evidence.json`.

The actual physics verdict is still **diverged**, not exact parity. Network/action phase is measured separately: server jump/graph activation preceded native positive/graph requests by approximately 18–85 ms. Client walk inputs are batched; after CQ cast transmission the owner can continue walking until SA activation, and the owner graph starts from its live feet, not the transmitted cast position. No CQ-position snap, timestamp shift, extra integration ticks, correction stream or tolerance change was introduced to hide that discrepancy.

Cross-SDK narrowphase equivalence remains unproven. The two replay flag differences are a tiny negative landing request and the final horizontal Shirk request. The legacy sphere/triangle sweep also contains a 0.01 directional triangle-normal distance adjustment absent from the selected Px4 path. Legacy NXS cooking, extra AIobj geometry, wall transform conventions, other maps, runtime stance/modifier paths and remote playback still need evidence; none was silently replaced or declared equivalent.

### Regression and input safety

Fifteen real-World/real-CCT and packet-boundary regressions pass with both Release and checked Debug backends: loaded-map walking/falling/walls, float-position requests, 50-unit climb versus 120-unit rejection, segmented multi-step traversal, mesh slopes, ceiling/resize, query restoration, PC filtering, all 200 loaded ordinary jump profiles, human direction, delayed/immediate graph lifecycle, airborne horizontal graphs and hostile movement packets. All Release targets and all eleven Python evidence regressions pass. Debug tests use the existing `_dbg` postfix convention.

Ordinary client speed is rejected when nonfinite, negative or above `50*64*60 = 192000` units/s in both CN movement and CQ cast-position validation. This is a work-safety boundary (up to 64 horizontal 50-unit segments per frame), not an XML movement-speed claim or a displacement clamp. It prevents untrusted huge speeds from making f32 `remaining-=50` non-progressing or overflowing the norm. Trusted authored graph requests and the client segmentation algorithm are unchanged. The new hostile-packet regression failed before the fix and passes afterward.

All owned processes exited after both recorded runs. The four server configuration files were restored byte-for-byte. Final source was exercised again after the input-validation and Debug-output fixes in `20260913_sol_final_laws`: 3,414 independent pairs, raw-QPC maximum/mean 111.209290/13.305603, A endpoint disagreement 0.001725, all five jumps accepted/applied, both skills complete, and zero corrections, focus hits or desktop mutations. Its verdict is complete but physics remains **diverged**.

The final capture also replays continuously from each master's initial pose without resets: class 6 has 2,541 moves, maximum/mean error 0.079655/0.001180; class 5 has 824 moves, maximum/mean 0.094823/0.042263. All 3,365 final collision flags agree. This does not erase the preceding capture's two boundary flag mismatches or establish exact numerical parity. Raw requests and results are retained in `final_laws_cct_motion_replay.json`.


## Native PhysX 2.8.4 cutover

`external/physx-2.8.4/` packages 204 unchanged SDK headers and the supplied x64 Release/CHECKED binaries and import libraries. `provenance.json` records every vendor file's size and SHA-256. The original installation and inactive PhysX 4 package remain untouched. No standalone SDK license/EULA was present in the installation; original vendor notices are retained and no PhysX 4 license is claimed.

The production backend and collision cooker use actual Nx 2.8.4 APIs. The local PhysX 4 controller fork, Px types, old filter callback, foundation/dispatcher/PVD setup and old build links are removed. Scene-local material 0 uses the observed friction/restitution 0.5/0.5; authored prop materials and dynamic wall material 0.5/0 are preserved separately. SDK shutdown releases cached original/scaled triangle and convex meshes after scenes release controllers and actors. Debug-window cleanup releases its test scene, including partial initialization.

Both supplied cores hard-import `cudart64_30_9.dll`; packaging it is required even with `NO_HARDWARE` and software scenes. The supplied CHECKED loader also requests unsuffixed core/cooking filenames. Debug executables therefore live in `build/checked/`, with original CHECKED core/cooking bytes deployed as `PhysXCore64.dll` and `PhysXCooking64.dll`; loader/controller retain CHECKED names. Release stays in `build/`. Both configurations run with `build/` as the working directory. Module paths and file hashes prove actual CHECKED loading rather than a shared-directory Release-core fallback.

The initial native cutover regenerated three mesh-only assets and the legacy cylinder. The authored-scene cutover below replaces the consumed assets with PHYX v2 recipes; the unused cylinder load and old per-file fields are removed.

### Positive same-request changes

The preserved `20260913_sol_final_laws` requests were replayed without retiming or pose resets, using the first observed pose per master and the same float-coordinate, displacement, mask and segmentation contract:

| Capture | Requests | PhysX 4 max / mean error | Native 2.8.4 max / mean error | Maximum reduction |
| --- | --- | --- | --- | --- |
| Class 6 | 2,541 | 0.079655 / 0.001180 | 0.004181 / 0.000486 | 94.75% |
| Class 5 | 824 | 0.094823 / 0.042263 | 0.016234 / 0.009815 | 82.88% |

All six retained request streams ran successfully. The earlier legacy capture also improves to class 6 max/mean 0.004183/0.000545 and class 5 0.016573/0.010081. This is a collision-kernel improvement, not proof of complete gameplay parity. Native replay has two class-6 boundary flag differences in each retained final/legacy capture; class 5 has none. The final PhysX 4 capture had no flag differences, so numerical improvement did not improve every metric.

### Actual-client and complete-scene evidence

`20261002_sol_nx284` completed the unchanged jump/A/moving-Shirk/TAG/ledge scenario with every required gate satisfied, 3,409 independent pose pairs, all five jumps accepted/applied, both skills completed, and zero server/client corrections, guard mutations or client-focus events. A endpoint-displacement disagreement was 0.001467 units. The raw-QPC maximum/mean error was **188.087094/15.826756**, worse than the previous capture's 111.209290/13.305603. The live physics verdict remains **diverged**; no network/action synchronization improvement is claimed.

Current-capture replay includes the actual logged `12 A type` wall configuration and all three collision meshes at each authored area transform. Class 6 has 2,536 requests, max/mean 0.004195/0.000486 and one flag difference; class 5 has 824 requests, max/mean 0.016232/0.006305 and no flag differences. One continuous controller across TAG processes all 3,360 requests with max/mean 0.016017/0.002002 and one flag difference. Real CHECKED core/controller replay produces the same result.

An initial static-only replay omitted the selected dynamic walls and produced a spurious class-5 582-unit discrepancy. Restoring the observed wall configuration resolved it without changing requests, resetting poses or shifting time. The incomplete-scene result is retained as invalid measurement evidence, not a backend result.

All Release and Debug server/tool targets build. Fifteen real-World/real-CCT and packet-boundary regressions pass with both Release and actual CHECKED cores; all eleven Python evidence regressions pass. Native CHECKED cooking succeeds and reproduces the source geometry. The 120-unit step regression retains rejection, grounding and unchanged per-tick bounds; its obsolete PhysX 4 stopping-distance pin was removed rather than repinned to a new implementation coordinate.

Actual Nx scene teardown, repeated teardown and null/partial-scene teardown were exercised. The debug GUI teardown itself was not run because desktop window manipulation is forbidden. The four server configs and pre-existing `scripts/lea.dll` bytes were restored exactly; all owned runtime processes exited.

Evidence: `build/velqor/nx284_evidence.json`, `nx284_cct_motion_replay.json`, `nx284_complete_scene_replay.json`, `nx284_cooked/final_regressions.json` and `build/velqor/runs/20261002_sol_nx284/`. Temporary replay sources/binaries are removed after proof. Matching the supplied SDK version does not remove client-build, x86/x64, boundary-flag, stance/modifier, other-map or network/action timing uncertainties.

## Authored scene, floating-point and input causality

### Permanent changes

- `common/collision_asset.h`, the collision tool and physics loader share PHYX v2: original cooked meshes plus complete supported material, actor, body and shape records.
- The 21 shipped DeathMatch asset files are regenerated from original NIFs. ART `Level.xml` and `ActionBase_Dynamic.xml` are copied byte-identically from client data.
- `GameXmlContent` loads enabled ART props and supported dynamic PHYSICS timelines. `World` binds gate/wall actors once and applies authored collision changes at action-relative tick boundaries. Gates disable collision on DYNAMIC_OPEN; walls disable at 2.4 seconds and enable on CLOSE at 1.9 seconds. Existing map phase and wall configuration selection are unchanged.
- Dynamic masks follow `0x01b3f735`: walls use `0xc7e06`, gates `0xa06`. `0x01b40139` supplies the group bit, zero team lane and interaction-mask lane. Group/name classification and authored materials remain separate.
- Human CN/CQ position snapshots are not destinations. `0x009c37c9` copies the same current position into both positional fields, reads native movement direction at entity `+0xC98`, and reads speed through the walk-stat getter. Zero direction means zero native movement even when speed is positive. Bots retain their explicit goal-seeking path.
- Jump replacement cancels pending programs and clears same-tick horizontal graph ownership/delta before physics. Tag/cast clear replaced jump graphs. This scene cutover introduced no arbitrary timestamp shift, positional seek or corrective stream; the subsequently approved bounded input replay is described below.

### Native replay evidence

The complete captured scene has 194 named prop/gate/wall actors and 12 player/controller/hit-volume actors. The production replay builds the 194 named actors from shipped recipes and source XML, then creates one queried controller; other PC/hit-volume groups are outside its locomotion mask. All 21 actually loaded mesh vertex multisets and oriented triangle topologies match the client exactly, including nine runtime-scaled meshes and the original convex wall mesh. Group, mask, material and actor collision-disable state agree for all 194 named actors after supplying the observed frozen gate state. At this stage two open-gate ray-query flags remained unequal; the later native PHYSICS consumer correction below resolves that bit, not general hit-query/presentation parity.

Continuous replay initializes only from the first raw centre and consumes every unchanged request in original order. There are no per-row resets, baseline alignment, time shifts or tolerance changes:

| Backend | Class 6: 2,533 requests, max/mean centre error | Class 5: 826 requests, max/mean centre error | Collision flag differences |
| --- | --- | --- | --- |
| Original client x86, observed float24 precision | 0 / 0 | 0 / 0 | 0 |
| Supplied x86 SDK, observed float24 precision | 0 / 0 | 0 / 0 | 0 |
| Supplied x64 SDK, exact captured scene | 0.005967 / 0.000566 | 0.014218 / 0.007302 | 1 class-6, 0 class-5 |
| Production x64 loader/controller | 0.005967 / 0.000566 | 0.014218 / 0.007302 | 1 class-6, 0 class-5 |

Both x86 float24 replays have exact before/after centres and feet for all 3,359 requests. Raising x86 precision to 53 reproduces the small extended-coordinate residual and the same class-6 flag difference. Calling the public precision24 API on x64 does not remove it. The supplied/client DLL byte differences are therefore not evidence of an incompatible kernel for these inputs; arithmetic precision is measured independently.

Sixteen real-World/real-CCT and packet-boundary regressions pass in Release and actual CHECKED Debug, including raw human direction/zero-direction STOP and closed/open gate clipping. The obsolete scalar-impulse ceiling/velocity pin was removed; ceiling coverage now uses an actual authored jump, clipping, landing and feet-preserving resize.

### Live timing remains divergent

`20261002_sol_recipe284` completed all control gates with 3,426 independent raw-QPC pairs, zero corrections/focus hits/guard mutations, but maximum/mean error **506.782384/60.867916**. `20261002_sol_recipe284_final` also completed all control gates and applied all five jumps; 3,429 independent pairs have maximum/mean **184.262086/43.732275**. Neither run meets the unchanged 25-unit live threshold.

The first run measures native forward motion at QPC `60415955951500` and the matching game-handler `input_move` trace at `60416089683800`, about 133.7 ms later. Its transmitted client time matches onset, while its duplicated position is still the pre-move spawn position. Sender `0x009d218b` is dispatched through `0x009d2e43` from `0x01ad1138`. This capture did not timestamp the actual socket send or server socket reception; attributing the entire gap to client delivery was unsupported.

At STOP, both trajectories are stationary but retain a 69.468403-unit offset, approximately seven walking ticks. A's total travel agrees at 378 units while inheriting that origin offset. The 506-unit peak occurs when the inherited XY offset makes the client cross a ledge and land before the server. The previous human position-seeking fallback partly concealed this offset; restoring it would be hidden reconciliation, not a physics fix.

Timestamped catch-up was subsequently approved and implemented below. The prior captures measure handler dispatch, not socket arrival: a threshold violation before that trace does not prove the packet was unavailable in the server kernel or pending receive buffer. Sparse input timestamps still do not expose every client frame delta. Vanilla send/receive/buffering stages must be measured independently before assigning the delay.

Evidence: `build/velqor/matching/recipe284_evidence.json`, `sdk_loaded_geometry_evidence.json`, `production_matched_evidence.json`, `production_geometry_evidence.json`, `production_actor_evidence.json` and the two named live-run directories. Original SDK/client/input-DLL bytes are unchanged; all owned live processes exited and server configs were restored byte-for-byte. Temporary native replay sources/targets/binaries are removed after proof. Debug GUI visualization was not exercised because desktop manipulation is forbidden.

## Bounded timestamped input catch-up

The approved backend remains native x64. `input_clock.h`, `World`, game handlers and replication now consume real timestamped input events rather than latching all human requests at receipt time.

- Owner-main `SN_GameCreateActor` publication enables the player's timeline; it is not a clock epoch or epoch floor. Native `+0x10F8` starts in the creature constructor and advances only for the active local master, but it accumulates client frame deltas rather than measuring elapsed server wall time. The observed lower envelope of real `receivedTime-clientTime` samples maps CN movement and CQ cast timestamps into process-relative server seconds.
- Two publication-based models were rejected by runtime evidence. In `20261002_sol_catchup`, treating publication as the exact epoch ran movement about 78.13 ms early. In `20261002_sol_catchup_verified`, the first native frame delta is 1.35568047 seconds; the first CN carries 1.35568 after only 0.19946 seconds of publication-relative wall time. Treating publication as a minimum epoch rejected that genuine input and its cast. Mapping now retains an observed client/server reference pair, avoiding a guessed startup origin and cancellation in a large absolute epoch subtraction.
- Cast admission includes its movement/rotation snapshot atomically. Equal effective timestamps retain arrival sequence. Out-of-order events inside the retained interval are accepted. Invalid, future, expired or over-capacity requests do not commit clock/ledger mutations. Human packet positions remain observations, never seek targets.
- Rotation, action sync, jump and tag use their actual receipt times. Jump and tag packets have no client timestamp; none is synthesized. Each master has separate clock generations and activation intervals. Returning to a frozen master retains the last observed counter lower bound, then learns the resumed epoch from real packets; elapsed server time is not substituted for its paused client counter.
- History retains 512 before-step frames, approximately 8.53 seconds at 60 Hz, and 2,048 input records per player. Before-step snapshots retain the actual previous server-time boundary, raw SDK centre/dimensions, velocities, grounding/contact normal, jump graph, graph collision ownership, input, master selection and active skill programs. Storage is allocated at world initialization; replay does not allocate per step.
- Late input restores only saved server state and reruns the owner's fixed steps with historical gate/wall collision flags. Other owners' programs do not advance. Other CCTs use group 3, outside the enabled locomotion mask; current peer-controller poses therefore do not affect these sweeps. The current scene flags are restored afterward. SDK simulate/flush/fetch and replication still run once per published tick.
- Clock refinement retimes only events whose old and new times both remain recoverable. Rows crossing the oldest before-step boundary keep their committed times; they do not cause a fresh STOP to be rejected. The earlier rejection caused continuing movement after STOP in `20261002_sol_catchup_clock` and is covered by a history-edge regression.
- Random graph selection is cached per cast. Cast/execute, actor-action publication and action trace notifications use per-input emission ledgers, preventing historical replay from retriggering client animation, publishing a completed WARP transition again or appearing as a new execution/completion.
- Incoming CN/CQ safety checks bound both the speed field and direction-times-speed work. Finite, non-unit directions remain unnormalized; hostile effective speeds above 192,000 units/s or unrepresentable direction norms are dropped rather than clamped.
- Human locomotion integrates raw direction times speed across mapped event-time partitions inside each retained server interval. START/STOP no longer acquire or lose an entire walking quantum when their phases cross a 60 Hz boundary. The motor displacement is explicit; gravity, authored jump/graph sampling and SDK simulation remain fixed-tick. No client snapshot is used as an integration target.

### Exercised verification

All 25 real-World/real-SDK regressions pass in Release and actual CHECKED Debug. They cover timestamped movement and STOP, lower-envelope refinement and rejected-cast atomicity, late graph travel with once-only replication/actor action, other-owner program isolation, out-of-order events and expired/future bounds, tag-domain rollback and feet-preserving resize, and refinement at the history boundary. A returning-master regression fails before removal of the estimated paused counter and passes using real resumed-clock observations. A startup regression accepts the actual 1.35568047-second accumulated counter while still rejecting a future advance.

`20261002_sol_catchup512` exercises 34 catch-ups and 2,197 historical steps, including one 502-step replay. All five jumps start, both casts/executes publish once, STOP/disconnect release completes, and the final stationary client/server distance is 1.387377 units. Its 3,448 independent raw-QPC pairs have maximum/mean error **318.518599/14.731622**. The unchanged 25-unit verdict remains **diverged**. Intermediate action trace rows repeated during historical replay; their emission-ledger correction is part of the final implementation, not a change to physical replay or the gate.

Final source is exercised in `20261002_sol_catchup_observed`: all control/cast/jump/TAG/STOP/disconnect gates complete; 37 catch-ups replay 649 historical steps, with a maximum 462-step replay. Each skill has exactly one cast, execute and completion; the six command identities publish once. All five jumps start. There are 3,454 independent raw-QPC pairs, maximum/mean **374.216742/12.424538**, and final stationary distance **20.054880**. Both captures have zero corrections, focus hits and guard mutations. The final 25-unit physics verdict is still **diverged**, not parity.

In the final capture, native nonzero movement begins at `102423630661800`; the corresponding game-handler trace is at `102423727149400`, 96.49 ms later. Its first threshold violation precedes that handler trace, not necessarily socket reception. Replay fixes handled input history and current state; no future input, client frame delta or jump/tag timestamp is invented. Client/SDK/input-DLL bytes are preserved; all owned processes exited, and all four configs match their backups byte-for-byte.

In the earlier `20261002_sol_catchup512` capture, native movement begins at QPC `100438868885000`; the matching game-handler trace is at `100439017083900`, 148.20 ms later. The first 29.759594-unit divergence at `100438902629000` precedes that handler trace. This is not a measured client socket-send or server socket-receive delay. Later jump/ledge and frame/action phase differences remain measured failures, not accepted numerical x64 parity.

Packet layouts, original client/SDK binaries and the pre-existing injector DLL remain unchanged. Run artifacts and final verification are retained under `build/velqor/runs/` and `build/velqor/matching/input_catchup_evidence.json`. This is input replay, not a position-correction stream or a relaxed comparison gate.

## Elapsed-time walking after IOCP

Checkpoint `f24ef4a` preserves the authored scene, bounded replay and completion-driven networking before this physics continuation.

Native `Creature_IntegrateVelocityAndGraphs` at `0x01a4f3c1` advances ordinary XY by raw direction times walk speed times frame delta, then increments the active local master's `+0x10F8` counter by that delta. Both Ghidra's original executable and the live client hash to `df32850dd8e7a2329ddca2dc57a3eb310a620f4a6477bbb135049a938f477583`. Client post samples match the corresponding final raw controller centre to within 0.00000755 units; visual/filtered sampling is not the walking offset.

The post-IOCP capture `20261003_122017_sol_iocp_vanilla` leaves the owner 9.924354 units behind the transmitted/native A start after STOP and clock refinement. Its approximately 0.899992943-second hold spans 54 native frames, but whole-tick input gating can retain only 53 walking quanta. This is a delivered-input duration error, separate from the unsent-input and notification/animation-onset offsets.

The actual World/CCT regression fails before correction at a start phase of 0.0166701004: 525.759888 units versus the source interval's 535.675720. After `World::SimulatePlayer` partitions the interval at input events and supplies the integrated motor displacement to `PhysicsScene::StepBody`, all three start phases travel 535.675598. Raw non-unit directions remain unnormalized; STOP ends horizontal input without freezing gravity. Other-owner programs and once-only notifications retain their existing contracts. Obsolete whole-tick transient position pins and internal `simTicks` assertions were removed rather than re-pinned.

All 26 real-World/SDK regressions pass in Release and actual CHECKED Debug. In `20261003_145635_sol_elapsed_motor`, all control gates complete and A's start discrepancy falls to 0.002765 units without seeking the packet pose. Four native jump requests arrive and all four start; the front-jump trial emits no native jump RPC. Both skills execute and complete, six commands publish once, STOP/disconnect work, and there are zero corrections.

`20261003_151338_sol_motor_rtt120` adds at least 60 ms each way through an unchanged-byte relay. A's start discrepancy is 0.004835 units; all five received jumps start, both skills complete, and STOP/disconnect work with zero corrections. The client-to-server byte count and complete stream hash agree. The server-to-client relay has 490 unsent tail bytes at managed shutdown.

Full parity is not claimed. The unchanged raw-QPC 25-unit gate still reports divergence: direct maximum/mean 139.011814/9.923035 over 3,426 independent pairs; delayed maximum/mean 543.666565/41.241426 over 3,440 pairs. Delayed TAG acknowledgement also retains four old-master identity samples. No fixed action delay, trajectory retiming, tolerance adjustment or correction stream conceals these differences. Client/input-DLL hashes are unchanged, all owned processes exit, and configurations are restored byte-for-byte.

Evidence: `build/velqor/matching/elapsed_walking_evidence.json` and the two named live captures.

## Action return-path measurement and TAG acknowledgement

`20261003_174337_sol_return_baseline` observes original server WSASend submission, original completed client WSARecv bytes, and native `SN_CastSkill`/`SN_ExecuteSkill` entry. Complete packet FNV hashes, net IDs, sizes and endpoints match nine cast/execute/jump packets. Submission to client receive-completion observation ranges from 0.0088 to 0.0304 ms, median 0.0166 ms. The normal A cast arrives in 0.0243 ms; native consumption follows 8.2951 ms later. Nagle/delayed-ACK is not supported as the source of the observed long action gap, so socket policy was not changed.

The native cast handler at `0x00a05b81` installs the owner action; execute at `0x00a05e94` caches graph endpoints. There is no incoming 70 ms deadline on that consumption path. The historical A graph request occurs 1,083.3935 ms after cast consumption, versus the old server's 1,014.8808 ms after cast acceptance. The clip observation and correction below resolve the animation-progression part of this difference; notification consumption remains a separate measured phase.

`compare.py` now requires an explicit same-owner, same-epoch, contiguous master-slot swap and corresponding `tag_apply` evidence before allowing an acknowledgement transition. The exact previous native identity is accepted only until the actual new native identity appears. Pending samples remain pose pairs with their original QPC timestamps, old/new identities and full position error. Missing acknowledgements, unrelated identity changes, wrong class/actor, post gaps and epoch crossings remain failures. No arbitrary acknowledgement timeout, class whitelist, pose deletion or trajectory shift was introduced.

Rebuilding `20261003_151338_sol_motor_rtt120` retains the four former identity-failure poses: 3,440 independent pairs become 3,444. The maximum raw position error remains exactly 543.666565 units and physics remains diverged. Nineteen evidence regressions pass, including a retained 50-unit pending-ack error and gap/epoch cases that fail before conservative binding guards.

The final clean-harness run `20261003_192545_sol_tag_ack_rtt120` adds at least 60 ms per direction. Four old-master samples are retained during a 73.1383 ms TAG acknowledgement interval; identity resolves without errors, and all control gates complete. All five jumps start, both skills complete, STOP/disconnect work and corrections remain zero. Its 3,437 independent raw-QPC pairs still have maximum/mean error 286.260153/40.837203; the unchanged 25-unit physics gate is not passed.

Final source passes five real-socket and 26 physics regressions in both Release and Debug/CHECKED. Temporary return-path hooks are removed, original server/harness binaries rebuilt, client/input-DLL hashes preserved, configurations restored and owned runtime processes exited. Evidence: `build/velqor/matching/return_path_tag_evidence.json`, `tag_ack_report_motor_rtt120.json` and the two named new captures.

## Native clip progression and rounded action clock

Observation-only hooks in `20261003_sol_clip_owner_transitions` establish that A enters its three clips at frames 1534, 1566 and 1598, then fires their command callbacks at 1535, 1567 and 1599. The middle clip has three loops but one continuous elapsed clock; it does not reset elapsed at each loop. Its elapsed value is 0.499992371 after 30 playing updates and 0.516658783 after 31. GRAPH activates on the third clip's callback, not on the nominal sum of earlier AniLength values.

`0x01b62732` resets clip elapsed and the once-only callback sentinel. `0x01b625db` tests old elapsed against duration; parent `0x01c06a0d` checks completion before updating the child. A completed clip enters its successor with elapsed zero, without transferring overshoot; that successor's first playing update follows entry. The server now preserves these phases and waits for all clips, pending effects and graph completion before finishing the program.

`0x005eb5ff` adds the QPC-derived capped frame delta to an `f32` application clock, then publishes the difference from the previous rounded clock. Consequently, direct `elapsed += float(1/60)` is not the same arithmetic. The deterministic 60 Hz server lane uses `dt = float(clock + float(1/60)) - clock` for clip progression and retains that clock in replay snapshots. It does not hardcode the observed 0.0166664124 delta, insert a bonus tick, or invent the client's occasional QPC hitches. The subsequent motor correction below extends that rounded quantum to graph sampling, gravity and SDK simulation without changing lane cadence.

Both previously failing physical regressions now pass: first-clip graph entry does not move immediately, and A does not start at its nominal prefix deadline. Airborne ownership checks the actual graph-entry height; late-cast replay compares against an independently simulated on-time owner rather than pinning an incidental graph sample count. All 26 physics and five real-socket regressions pass in Release and actual CHECKED Debug; all 19 evidence regressions pass.

`20261009_sol_clip_clock_local` completes all five jumps, both skills and every control/identity gate with zero corrections. A's server graph starts at logical elapsed 1.083333 seconds; the remaining native-versus-server graph phase is 16.5657 ms. A's endpoint disagreement is 0.001248 units, but its raw onset-window maximum is 27.808493 units. Full raw-QPC physics remains diverged: 3,429 pairs, maximum/mean error 163.705894/7.773095. Client bytes, raw timestamps and the 25-unit gate are unchanged.

`20261009_sol_clip_clock_rtt120` adds at least 60 ms in each direction and likewise completes all five jumps, both skills and all control/identity gates without corrections. Its 3,433 raw pairs retain maximum/mean error 268.478331/29.104031. Return-path/native-consumption phase, pre-arrival inputs, and ordinary-jump launch remain explicit failures, not numerical SDK parity. Both runs restore all four configs and leave no owned processes.

## Native ordinary-jump phases and motor quantum

`20261009_sol_jump_phase_before` records all five native requests, actual `StateJumpStart_Enter`, vertical graph installation and first height difference. Each request occurs one frame before state entry; graph installation and its first sample occur on the following frame. `0x01c099d1` enters without installing the graph; playing MotionControl update calls `0x01b61e65` and `0x01a4d95a`. The server now retains Requested and Entered phases, cached profile/facing and update ownership in before-step movement snapshots. A duplicate pending/active request cannot restart the jump; tag and replacement cast clear pending work.

The measured endpoint difference was also a clock error, not a new curve or an extra end-state delay. At an application clock around 50 seconds, 36 rounded updates reach 0.599990845, still below the Sniper 0.6-second duration. Native applies the small terminal remainder on update 37 before gravity resumes. Direct `float(1/60)` completed one update earlier. The deterministic rounded quantum now feeds graph sampling, gravity, bot motor displacement and SDK simulation; elapsed human walking still uses the actual mapped input intervals. Explicit StepBody/FinishStep duration arguments migrate every caller without a default or compatibility shim. Historical replay derives the same quantum from its saved lane time.

The real-CCT terminal-remainder test and World request/entry/first-playing test both fail before their respective fixes and pass afterward. All 28 physics regressions, including all 200 authored profiles, pass in Release and actual CHECKED Debug. Native graph installation does not reset accumulated vertical velocity; its server setter preserves that ownership rule.

In `20261009_sol_jump_phases_local`, native and server first positive samples differ by only 2.19–2.32 ms across all five jumps, versus the earlier 25.8–32.0 ms launch gap. Native graph delta is 33.1661644 and the float-position CCT request is 33.1661377. All controls and both skills complete, identity resolves, corrections remain zero and configs restore. The raw 3,422-pair maximum/mean remains 148.752103/6.058575, not parity.

`20261009_sol_jump_phases_rtt120` likewise completes all five jumps and both skills with zero corrections and retained TAG-ack poses. Its 3,442 raw pairs still have maximum/mean error 283.132695/36.095226. CQ jump has no timestamp; none is invented. Pre-arrival input intervals, native notification consumption, QPC hitches and ledge/contact-history feedback remain explicit failures. A blanket grounded-only slope suppression is not supported by the original integrator.

## Match-local motor clock and native SDK substeps

The rounded motor clock must not inherit the dedicated simulation lane's uptime. At an absolute clock of 262,144 seconds, `float(clock + float(1/60)) - clock` becomes 0.03125 seconds; at 524,288 seconds it becomes zero. A new World now fixes an immutable physics epoch on its first update. Normal steps and historical replay derive their internal physics clock from that epoch, while lane/frame times, input mapping and captured QPC timestamps remain unchanged. The real-World/CCT regression starting after three days of lane uptime fails before this correction and passes afterward against an independent fresh-lane simulation.

Read-only `NxScene::getTiming` in `20261009_sol_match_epoch_timing` also proves that the native scene does not use the SDK descriptor defaults:

| Setting | Native DeathMatch scene | SDK default | Current server |
| --- | --- | --- | --- |
| Maximum SDK timestep, seconds | 0.0333333351 | 0.0166666675 | 0.0333333351 |
| Maximum SDK iterations | 1 | 8 | 1 |
| SDK timestep method | `NX_TIMESTEP_FIXED` | `NX_TIMESTEP_FIXED` | `NX_TIMESTEP_FIXED` |

These SDK substeps are distinct from the 60 Hz motor lane and its rounded graph/gravity quantum. All 29 physics regressions pass in Release and actual CHECKED Debug after both corrections. The temporary SDK timing observer is removed and the original observation harness rebuilt before these final-source scenarios:

| Unchanged-client scenario | Independent raw pairs | Maximum error | Mean error | Physics verdict |
| --- | ---: | ---: | ---: | --- |
| `20261009_sol_scene_timing_local` | 3,426 | 149.076164 | 5.986643 | diverged |
| `20261009_sol_scene_timing_rtt120` | 3,432 | 301.132622 | 33.567550 | diverged |

Both runs complete all five jumps, both skills and the required control/identity gates without corrections. The relay measures at least 60.2563 ms client-to-server and 60.0280 ms server-to-client. Its client stream forwards all 4,027 bytes with equal hashes; the ordered server stream has 392 unforwarded tail bytes at managed shutdown, not mutated packets. All four configs restore and no owned processes survive. This verifies the source corrections, not full parity: the original 25-unit gate, raw timestamps, vanilla client and accepted x64 same-request residual remain unchanged.

## Omitted animation index and genuine global commands

The original loader at `0x01ab7c1d`–`0x01ab7c37` promotes `AnimationIndex == -1` to zero when `AnimationId` is neither -1 nor zero. The server had left such rows global, so Mechanic Shirk and several RODMASTER, PHOTOG, MONDOZAX, RNB and ANDROA graphs moved at skill birth instead of the first playing callback. The loader now copies the exact native predicate before building clip metadata. Genuine global rows remain global and still dispatch at action entry; no stance, delay or callback rule is changed.

The real-World/CCT Mechanic regression fails before this correction because the birth pose has already moved. All 30 physics regressions pass in Release and actual CHECKED afterward, including first-sample ownership and the physical 900-unit endpoint. The unchanged observation harness additionally exercises Mechanic Shirk, TAG to Sniper and the existing full owner scenario:

| Unchanged-client scenario | Independent raw pairs | Maximum error | Mean error | Physics verdict |
| --- | ---: | ---: | ---: | --- |
| `20261009_sol_omitted_index_after` | 3,687 | 98.324584 | 6.472072 | diverged |
| `20261009_sol_omitted_index_rtt120` | 3,704 | 314.371385 | 29.878574 | diverged |

Both runs complete three casts/executes/completions, both TAG transitions, all five jumps and every required control/identity gate with zero corrections. The Mechanic command now waits for first-playing ownership instead of executing at elapsed zero. The delayed relay measures at least 60.0501 ms client-to-server and 60.0709 ms server-to-client; all 4,233 client bytes match, and 588 server tail bytes remain unforwarded at managed shutdown. Configs restore and no owned processes survive. The earlier broader-class capture used an unbound TAG input and is retained only as Mechanic before-fix evidence, not a complete scenario or comparable whole-run baseline. Exact evidence is in `native_clip_progression_evidence.json`.

## Native SDK dispatch and PHYSICS query state

A `getTiming` snapshot establishes descriptor values, not when the SDK advances. Read-only wrapper/API hooks in `20261009_sol_sdk_tick_observation` prove the missing dispatch law at `0x0112a7d2`: skip while the current application clock is not ahead of the fetched simulation target; otherwise ceil the float-rounded interval minus `1e-6` into 1/30-second steps. The target advances by the uncapped count, while actual SDK iterations cap at eight. The original call uses `simulate(iterations * timestep + 1.0f)` under that fixed iteration cap. The capture records 3,876 wrapper frames, 1,935 actual SDK calls and 1,941 skipped frames, including the cap and multi-step cases. The server now preserves this policy in its independently owned published SDK clock; owner-history replay does not advance it.

The real rigid-body regression fails before this correction: the first published box position stays at 500 instead of advancing to 498. Afterward it advances on the native ceiling boundary, stays unchanged on the intervening motor frame, and limits a long gap to the native batch cap. CCT sweep order is unchanged; the SDK still follows current motor/controller movement. Temporary client hooks are removed and the original observation harness rebuilt.

The two ray-query discrepancies were a separate omitted PHYSICS side effect. Payload dispatch `0x01aa5a40` routes REMOVE=false to `0x01b3eaeb`, which toggles both actor `NX_AF_DISABLE_COLLISION` and every shape's `NX_SF_DISABLE_RAYCASTING` from the first enable boolean. The second boolean selects physical-property removal, not an independent raycast setting. Supported DeathMatch TF/FF parsing and cooked flags were already correct; no gate-name or recipe-bit special case is added. Scene history now preserves the actual ray bit as well as actor flags.

The real SDK ray regression hits a closed gate, misses the opened gate and hits it after closing again; it fails before the correction and passes afterward. All 31 physics regressions pass in Release and actual CHECKED. Runtime comparison retains the initial native opposite-gate acknowledgement interval: its two actors remain closed at frames 1224–1225 and open at 1226. All four stable gate collision/ray states then match the real server. No state policy, timestamp or pose is changed to force simultaneous acknowledgement.

| Original-harness final-law scenario | Independent raw pairs | Maximum error | Mean error | Physics verdict |
| --- | ---: | ---: | ---: | --- |
| `20261009_sol_physics_laws_clean_local` | 3,690 | 61.074791 | 4.317192 | diverged |
| `20261009_sol_physics_laws_clean_rtt120` | 3,702 | 314.422659 | 32.222802 | diverged |

Both scenarios complete three skills, all five jumps, two TAG transitions and every control/identity gate without corrections. All configs restore; no owned processes survive. Client and server timing/flag observers are removed from final source. The accepted x64 numerical residual and original 25-unit raw-QPC gate remain unchanged. Evidence: `native_sdk_dispatch_evidence.json` and `native_gate_query_evidence.json`.

## WARP direction, segmented movement and action ownership

The resumed WARP checkpoint had removed the owner `SN_PlayerSyncMove` echo that made the client move twice, but still used the transmitted direction before the native command's reset. In `20261009_sol_warp_resume_moving_before`, forward remains held while native frame 1353 clears direction and requests `[-318.197754, -318.198242, 0]`; the server requests the opposite displacement.

`0x01a4bfc1` calls virtual slot `+0x118` with the zero vector before `0x01a4c4d4` selects the destination. The primary vtable at `0x02a41308` and the player tables resolve that slot to `0x01a4c1c3`, which writes `+0xC98/+0xC9C` while the movement-lock counter is zero. The supported WARP path therefore uses body yaw, not raw packet direction. No unit clamp or changed walking-input bound was added. The active-command guard at `+0xC4C` also confirms that a second installed destination is ignored, not overwritten.

The native local-mode predicate at `0x01a49527` selects segmented movement when XY displacement divided by frame delta exceeds 2,000 units/s. The forced server path now uses its existing 50-unit CCT segments. Its real-SDK staircase regression fails before the correction at `(270, 0, 50)` and passes afterward at `(900, 0, 150)`. The unreachable `horizontalGraphActive` destination-iteration proxy and its constants were removed; that proxy was not the native movement-graph predicate.

Observation-only `warp_ownership` records in `20261009_sol_warp_resume_gait` distinguish walking permission from destination residual. Native effective walking speed changes from 644.799988 to zero one frame before WARP, despite a nonzero input direction and empty horizontal graph vectors. It returns to 644.799988 when the action leaves WARP. The current five authored WARP action keys omit horizontal motion graphs; their server programs now retain this walking ownership without freezing ordinary gravity after the forced displacement reaches zero.

`STATE_BLOCK` releases the state-transition gate; it does not directly restore walking speed. Known held movement services that gate without waiting for another packet; fresh explicit behaviour changes remain separate from ordinary behaviour snapshots. Cancellation clears the destination, walking ownership and ignored-unit mask atomically, as `0x01a4ad85`, `0x01ad356f` and `0x01a4ae7e` do. A successor clip's first-playing callback no longer clears the destination. Invalid-caster cleanup also releases both destination and mask. Replay retains the gate and physical state, while a per-input ledger publishes the return action once.

All 35 real-World/SDK regressions pass in Release and actual CHECKED Debug. Added coverage exercises held-direction reset, yaw fallback, blocked destination ownership and restoration, successive climbable steps, an early movement request held behind the state gate, and late input replay through WARP cancellation without duplicate actor-action publication. The temporary native observer was removed and the original harness rebuilt before final-source captures.

| Moving-WARP scenario | Independent raw pairs | Maximum error, units | Mean error, units | Physics verdict |
| --- | ---: | ---: | ---: | --- |
| Before: `20261009_sol_warp_resume_moving_before` | 3,687 | 896.674184 | 212.001091 | diverged |
| Final local: `20261009_sol_warp_resume_final` | 3,694 | 101.242037 | 8.625411 | diverged |
| Final added RTT: `20261009_sol_warp_resume_rtt120` | 3,709 | 557.461516 | 52.032521 | diverged |

Both final runs accept and execute three skills: WARP cancels on return to movement, and the two Sniper skills complete normally. All five received jumps start; all required control/identity, modifier, STOP and disconnect gates pass with zero corrections. The relay adds at least 60.3183 ms client-to-server and 60.0301 ms server-to-client, forwarding all 4,393/264,862 bytes with equal stream hashes in both directions. All four configurations restore byte-for-byte, no owned processes survive, and original client/LEA hashes are preserved.

Full parity remains unproven. The local first WARP endpoint still differs by 10.743151 units; the original raw-QPC 25-unit gate is unchanged and fails in both final captures. The retained pre-send witness separately proves a 29.759594-unit violation 49.2296 ms before the first movement socket submission, with no intervening outgoing game packet. Future unsent input cannot be supplied by server reception or replay. Native notification-consumption phase, contact history and broader WARP/remote/state coverage remain explicit gaps, not tuned delays or accepted parity.

Evidence: `build/velqor/matching/native_warp_evidence.json`, `native_pre_send_causality_evidence.json` and the named runs. The final local run retains its exact `scenario.json` for reproduction.

## Held-input return and native frame-time timers

Checkpoint `b95e957` commits the verified action-clock, SDK-dispatch and WARP work above. The next local peak is ordinary walking and ledge departure before Sniper A, not an ordinary-jump curve: the client is at Z=562.765320 while the server is at Z=659.696228, with a 101.242037-unit raw error.

The client resumes held movement before its next position packet. In `20261009_sol_warp_resume_final`, native walking resumes at frame 1362 and QPC `286795598373500`; the next movement handler is 97.7662 ms later. `MainCharInput_UpdateMove` at `0x01a9874a` checks the live release gate (`0x01a98971`) and held-direction flag (`0x01a9897a`), then requests `NONE_BEHAVIORSTATE` through virtual `+0x88`. That request is independent of effective walking speed. Behaviour state 18 is not a run-animation state; directional movement is selected separately through `+0xF8`.

The server now services the already-received nonzero direction each update. It neither invents a future key release nor waits for another movement packet. Ordinary `NONE_BEHAVIORSTATE` snapshots and move-state IDs are not explicit cancellations; a zero-direction STOP keeps WARP ownership until natural completion or a genuine explicit behaviour change. Fresh behaviour observations are callback arguments, not cached requests that mature after a closed gate. Two real-World/SDK regressions fail before this correction: held movement cannot return without another packet, and a standing/stopped owner loses unit filtering after a stale snapshot.

The remaining early release was a separate timer law. The action timer uses `EntityManager+0x14`; constructor `0x01a2ad35` calls `0x012e32cf(0)`, selecting its frame-time clock rather than `GetTickCount`. `0x012e3509` multiplies the published `f32` frame delta by 1,000, truncates it to an integer and adds it to the timer counter without fractional carry. At 60 Hz, each frame contributes 16 ms. `MainChar_FrameUpdate` at `0x01a287c3` processes input and actor motion before this timer service. Expiry at `0x012e3628` is counter greater than or equal to deadline; `0x01abfbf4` validates the live action frame before opening the gate.

Supported state gates now retain integer milliseconds and the last service update in replay snapshots. The timer advances once per owner update after action processing; input consumes expiry on a following update. The correction does not add a fitted delay or a hardcoded bonus tick. Its physical boundary regression fails before the correction and passes afterward; the preceding incidental flat-second boundary pin was replaced by packet-independent return coverage.

| Moving WARP ownership interval | Native duration, ms | Server duration, ms |
| --- | ---: | ---: |
| `20261009_sol_frame_timer_local` | 216.6622 | 216.6501 |

All 38 real-World/SDK regressions pass in Release and actual CHECKED Debug. Coverage includes zero-speed held requests, first return without a new packet, once-only return publication after historical replay, delivered STOP before the gate, fresh/stale ordinary snapshots after release, integer timer boundaries and actual collision queries through the retained unit filter.

| Final-source scenario | Independent raw pairs | Maximum error, units | Mean error, units | Physics verdict |
| --- | ---: | ---: | ---: | --- |
| Moving local: `20261009_sol_frame_timer_local` | 3,694 | 460.743593 | 4.466713 | diverged |
| Moving added RTT: `20261009_sol_frame_timer_rtt120` | 3,700 | 546.715077 | 36.096685 | diverged |
| Standing local: `20261009_sol_frame_timer_standing` | 3,686 | 76.959013 | 5.979287 | diverged |

Every final scenario accepts and executes three skills, starts all five jumps and passes the required control/identity, modifier, STOP and disconnect gates without corrections. Moving WARP cancels on the held request; standing WARP completes naturally with no premature cancellation. The relay adds at least 60.0253/60.0211 ms in the client/server directions. All 4,413 client bytes forward with equal hashes; 196 server tail bytes remain unforwarded at managed shutdown. Configurations restore byte-for-byte, original client/LEA hashes remain unchanged and no owned processes survive. No native observers were added for this continuation.

The local ordinary walking/A window's peak falls to 38.830448 units, but full parity is not established. Instantaneous WARP onset still produces a raw-QPC phase spike; the final moving return is 9.0706 ms ahead of native consumption. Standing native mask ownership is 433.2699 ms versus 449.9817 ms from server command to completion. These measured gaps remain failures, not retimed trajectories or a changed 25-unit gate.

The cast-origin difference is not an invariant extra walking stage. Builder `0x009c6b61`, caller `0x009d42fd` and sampler `0x009c37c9` transmit the pre-integrator position and counter; the request path does not install the action. Owner installation is reached through `SN_CastSkill` at `0x00a05b81`, then `0x0132a4d5` and `0x01339674`. The vanilla owner continues until that notification is consumed; its exact consumption instant is not transmitted to the server and changes with network delay. No extra walking quantum or invented cast-entry timestamp was added.

Evidence: `build/velqor/matching/native_held_return_evidence.json`, the named final captures and the preserved scenario from `20261009_sol_warp_resume_final/scenario.json`.



## Suggested next verification targets

Compare client and server trajectories for starts/stops, diagonal travel, wall sliding, corners, steps and slopes near the configured limits, ordinary jumps and ceilings, actor collision categories, and skill motion. Repeat under different client frame rates and network delays. Measure position, grounding, collision flags, and action transitions rather than judging only final endpoints or appearance.
