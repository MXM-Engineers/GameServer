# Physics alignment: initial implementation

## Goal and status

Align server physics with `MXMClient_DP_p3.exe` while retaining the existing PhysX 4.1.2 backend. This commit implements verified client controller settings and corrects server-side collision and movement inconsistencies. It does not establish exact client/server physics parity.

The client uses PhysX 2.8.4 through Gamebryo/NiPhysX and the legacy Nx controller API. Its SDK creation call at `0x01125e5c` passes version `0x02080400`. Different SDK generations can still produce different collision results even with matching parameters.

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
| Climbing mode | Constrained | Constrained |
| Up axis | Z | Z |
| Gameplay position | Feet-relative | Feet-relative |

Radius and skin must be considered together. The old server used radius 110 with contact offset 0.1, giving a rough inflated radius of 110.1, versus the client's 100 + 10. This was not simply a 10% size discrepancy. Matching the complete configuration is necessary.

MoveController dimensions are distinct from the class-specific PhysXMesh collider dimensions. The latter were not substituted into locomotion.

## Implemented changes

### Controller creation, movement, and lifetime

- Create controllers at a center computed from the supplied feet position, rather than treating feet as a center.
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

Existing wall selection, identities, and spawn metadata are preserved. The cooker was not rewritten: supplied map meshes are already in map space and wall meshes are actor-local. General NIF transform handling remains an audit concern for additional assets.

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

- Horizontal authored curves are implemented; live validation of packet-provided movement state and outbound motion updates remains open. Client-internal movement-state and RNG replication are not standalone goals. See `physics_roadmap.md`.
- Supported action scheduling now uses client-evidenced command/clip timing and has passed real-runtime smoke coverage. Live remote-client presentation of delayed graphs remains unverified.
- The action loader selects one stance timeline to avoid duplicate effects, preserving last-GRAPH stance selection where present. General stance switching is not implemented.
- Only DeathMatch collision assets are supported. Additional maps require verified geometry, transforms, collision categories, and dynamic-state behavior.
- The separate role of class-specific hit volumes still needs further alignment work.
- General owner prediction/reconciliation is not implemented by this commit. Targeted warp correction is not a complete reconciliation system.
- No side-by-side live client trajectory comparison was performed. SDK 2.8.4 versus 4.1.2 edge cases remain to be measured.

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
- `0x01b61e65` installs the horizontal animation graph through `0x01a4dc6b` and `0x01babc79`. The local zero-target installation through `0x01babbe6`/`0x01bab6d5` retains raw authored scale, rather than forcing travel to ActionBase Param1.
- `0x01bab102` returns successive sample differences with a zero previous value at initial graph time. `0x01baabea` applies them along the graph's facing and HorizonRotate; the caller reads entity CB0 plus PI. Existing world-yaw conversion requires subtracting HorizonRotate.
- The actual Random selector is `0x01c055e6`, selecting through the authored cumulative CaseValue thresholds using an RNG call with bounds 0 and 100. `0x01c05839` is a timeout selector, not Random.
- `0x01c083b4` reads entity B50 for MoveType selection. Server selection uses transmitted movement input and facing sectors; recreating the client field's producer chain is not required unless it explains a packet-observable mismatch.

World applies each authored displacement delta through the actual controller. Collisions clip only that step; removing an obstacle does not cause accumulated endpoint catch-up. Cast output endpoints describe authored travel, and packet layouts remain unchanged. The implementation uses existing server RNG rather than a fixed Random branch. Private client RNG sequence matching is not a requirement; only unreconciled, observable motion disagreements require further work.

Verification: all Release targets built; real World/PhysX smoke passed nonlinear Assassin travel (-286.05 rather than command -300), Defender control (900), Sniper initial-sample and endpoint behavior (-314), ESPER directional rotation (750), Sniper SHIRK endpoint (375), both standing random branches, and obstacle clipping/removal without catch-up. Explicit-duration profiles were loaded and exercised. XML regeneration was byte-identical. Temporary verification artifacts were removed. No live client recording was performed.

Full remaining work is tracked in `physics_roadmap.md`.

## Packet-observable action scheduling

ActionBase timing is normalized when loading content. Commands retain authored Delay and carry execution/completion offsets from action start. Clip offsets include prior AnimationIndex groups and AnimationLoop; command delays do not accumulate across rows. Equal-deadline commands retain animation-index/command-type order. The selected clip sequence determines animation duration, and each program caches its completion boundary.

Client evidence:

- `0x01ab799b` loads Delay, AniLength, command type, and animation index. `0x01ab8da1` sorts through `0x01ac6bb5` and computes clip-relative offsets. `0x01ac6021` dispatches commands; timer delays are converted to milliseconds.
- STATE_BLOCK dispatch at `0x01ab929e` queues a timer. Its consumer `0x01abfbf4` calls `0x01a4bbf5` to release a state-transition gate; `0x01a4b98d` resets the gate on state entry. This is distinct from a WASD velocity lock and from the PC_StateBlock table. The server's former `lockedMoveUntil` interpretation was removed.
- GRAPH_MOVE_HORZ dispatch at `0x01ac085d` and WARP at `0x01ab9fc3` inform the supported movement effects.
- SN_ExecuteSkill handler `0x006bb547` and graph installation at `0x01a4df6a` apply network graph movement immediately; ST_GRAPH_MOVE_DATA has no delay field. A later execute packet also resets the receiving animation sample time. The server therefore emits one execute notification at cast, with graph override only for immediate graph onset. Delayed motion follows the server timeline and normal position replication. SN_CastSkill carries the action state consumed by `0x006b9d62`.

Runtime graphs apply their final sample delta once, suppress stale ordinary movement for that tick, and stop touching input thereafter. Replacement and tag cancel pending programs before their old effects execute. WARP queues a single owner correction when applied. Statesman's overlapping DEFAULT/C command rows no longer execute twice; the existing last-graph stance choice remains the supported selection rather than introducing general stance switching.

Verification used a temporary executable with real loaded content, World, PhysX, and Replication frame queues. The full Release build passed. Observed results:

| Scenario | Result |
| --- | --- |
| Assassin SKILL_3 | Authored endpoint -286.049988; movement blocked during graph and resumed afterward |
| Assassin SKILL_2 | Program still live after first 0.1-second clip; completed after the 1.266667-second sequence |
| Sniper SKILL_1 | No motion before 1.0-second onset; -111.206146 at graph time 0.066667 versus expected -111.206047; one cast-time execute without immediate graph override and no second execute |
| Pending Sniper graph | Replacement before and across onset left position unchanged |
| Sniper SKILL_2 | Initial H_Y0=-3 applied |
| Defender SKILL_4 | Terminal position 900, then 906.666687 with fresh input while the 1.5-second action remained live |
| Priest SHIRK | No movement/correction before 0.5 seconds; displacement 1000 and one correction afterward; no repeat |
| Pending Priest WARP | Replacement and tag prevented later displacement and correction |
| Assassin SHIRK | Zero-delay WARP applied with one correction |

The Defender terminal regression failed the first runtime smoke at position 880. Refreshing the final graph-owned input and blocking ordinary integration for that tick corrected it; the rerun passed. These checks exercise server motion and replication queues, not a live network/client capture. Remote delayed-graph animation/presentation remains unverified; private client state machines are not an implementation requirement.

## Suggested next verification targets

Compare client and server trajectories for starts/stops, diagonal travel, wall sliding, corners, steps and slopes near the configured limits, ordinary jumps and ceilings, actor collision categories, and skill motion. Repeat under different client frame rates and network delays. Measure position, grounding, collision flags, and action transitions rather than judging only final endpoints or appearance.
