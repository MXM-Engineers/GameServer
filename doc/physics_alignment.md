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
- Move the input-block deadline into player movement state. Blocking input no longer suppresses gravity.
- Require grounding and unblocked input before accepting an ordinary jump.
- Progress horizontal skill graphs over their advertised duration instead of applying their full displacement immediately.
- Execute the first action command and process zero-delay command batches without skipping them.
- Keep WARP commands instantaneous and collision-constrained.
- Send owner warp corrections through the existing `SN_PlayerSyncMove` packet. Corrections are queued until the completed replication frame, after cast/execute notifications, and use the final actor snapshot.
- Use the endpoint for both positional fields when an instantaneous ExecuteSkill position synchronization is emitted.
- Leave ordinary movement's owner-skip behavior and all packet wire layouts unchanged.

### Map collision scene

The shipped physics assets cover DeathMatch map `160000094` only. The game now asserts that the selected physical map is supported, after applying `DevForcedMap`, rather than silently simulating DeathMatch geometry for a different advertised map.

The scene includes:

- DeathMatch terrain as Static.
- Main and sidewall GuardrailMob meshes as FenceAll.
- Aim geometry as Aim.
- GuardrailMob, AIobj, and Collision meshes for each selected dynamic wall, using the wall actor's position and rotation.

Existing wall selection, identities, and spawn metadata are preserved. The cooker now uses native Nx 2.8.4 cooking while retaining the NIF extraction path: supplied map meshes are already in map space and wall meshes are actor-local. General NIF transform handling remains an audit concern for additional assets.

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
- General owner prediction/reconciliation is not implemented by this commit. Targeted warp correction is not a complete reconciliation system.
- Live owner comparisons expose a persistent 70-unit server controller perch and delayed ledge departure. Matching settings do not establish equivalent SDK 2.8.4/4.1.2 controller behavior.

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

The content loader requires complete, finite profiles and negative finite gravity. `JumpMotion::Sample` performs allocation-free, clamped piecewise-linear sampling. `PhysicsScene::StartJump` requires grounding and rejects an active jump; no scalar impulse is applied. Each physics step integrates gravity into velocity but replaces vertical displacement with the authored height difference while the jump graph is active. Graph completion returns control to velocity-based falling. PhysX still constrains the displacement against geometry.

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

ActionBase timing is normalized when loading content. Commands retain authored Delay and carry execution/completion offsets from action start. Clip offsets include prior AnimationIndex groups and AnimationLoop; command delays do not accumulate across rows. Equal-deadline commands retain animation-index/command-type order. The selected clip sequence determines animation duration, and each program caches its completion boundary.

Client evidence:

- `0x01ab799b` loads Delay, AniLength, command type, and animation index. `0x01ab8da1` sorts through `0x01ac6bb5` and computes clip-relative offsets. `0x01ac6021` dispatches commands; timer delays are converted to milliseconds.
- STATE_BLOCK dispatch at `0x01ab929e` queues a timer. Its consumer `0x01abfbf4` calls `0x01a4bbf5` to release a state-transition gate; `0x01a4b98d` resets the gate on state entry. This is distinct from a WASD velocity lock and from the PC_StateBlock table. The server's former `lockedMoveUntil` interpretation was removed.
- GRAPH_MOVE_HORZ dispatch at `0x01ac085d` and WARP at `0x01ab9fc3` inform the supported movement effects.
- The actual SN_ExecuteSkill handler is `0x00a05e94`. On the owner path it commits execution through `0x0132aa06`, then caches graph endpoints, origin distance, and duration through `0x01a4bd46`/`0x01bab857` when bApply is set. It does not install physical graph movement immediately. Send full metadata at normal execute-commit even for delayed graphs; the animation activates movement later. SN_CastSkill uses `0x00a05b81`. `0x009f6a03` is the actor-enter 62029 graph path, not execute 62036.

Runtime graphs apply their final sample delta once, suppress stale ordinary movement for that tick, and stop touching input thereafter. Replacement and tag cancel pending programs before their old effects execute. WARP queues a single owner correction when applied. Statesman's overlapping DEFAULT/C command rows no longer execute twice; the existing last-graph stance choice remains the supported selection rather than introducing general stance switching.

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

- Preserve transmitted human movement direction rather than repeatedly steering toward a stale position-derived goal. Bots and stopped input retain destination-based movement.
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

The production backend and collision cooker now use actual Nx 2.8.4 APIs. The locally patched PhysX 4 controller fork, Px types, old filter callback, foundation/dispatcher/PVD setup and old build links are removed. Scene-local material index 0 retains friction 1 and restitution 0. SDK shutdown owns global triangle meshes; scenes release controllers and actors first. Debug-window cleanup now releases its test scene, including partial initialization.

Both supplied cores hard-import `cudart64_30_9.dll`; packaging it is required even with `NO_HARDWARE` and software scenes. The supplied CHECKED loader also requests unsuffixed core/cooking filenames. Debug executables therefore live in `build/checked/`, with original CHECKED core/cooking bytes deployed as `PhysXCore64.dll` and `PhysXCooking64.dll`; loader/controller retain CHECKED names. Release stays in `build/`. Both configurations run with `build/` as the working directory. Module paths and file hashes prove actual CHECKED loading rather than a shared-directory Release-core fallback.

The three static assets were regenerated with the native cooker from the original client NIFs; all three extracted `.msh` files are byte-identical to the pre-cutover geometry. Names, coordinates, indices, winding and actor-local wall geometry are unchanged. `cylinder.physx_dynamic` was regenerated from the existing OBJ using native convex cooking; its consumer container layout is retained.

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

## Suggested next verification targets

Compare client and server trajectories for starts/stops, diagonal travel, wall sliding, corners, steps and slopes near the configured limits, ordinary jumps and ceilings, actor collision categories, and skill motion. Repeat under different client frame rates and network delays. Measure position, grounding, collision flags, and action transitions rather than judging only final endpoints or appearance.
