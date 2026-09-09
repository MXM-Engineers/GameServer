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

- Horizontal graph interpolation is currently linear. Exact client animation curves and their data sources have not been reproduced.
- STATE_BLOCK and command-delay scheduling follows the available XML structure; exact client scheduler semantics remain unverified.
- The existing action loader flattens stance variants. Last-declaration graph selection is retained rather than introducing an unverified stance model.
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

## Suggested next verification targets

Compare client and server trajectories for starts/stops, diagonal travel, wall sliding, corners, steps and slopes near the configured limits, ordinary jumps and ceilings, actor collision categories, and skill motion. Repeat under different client frame rates and network delays. Measure position, grounding, collision flags, and action transitions rather than judging only final endpoints or appearance.
