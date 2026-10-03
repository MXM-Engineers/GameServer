# Game servers

## Building

Requires CMake (>= 3.26) and a C++14 compiler.

For the command line, first load the MSVC environment (finds Visual Studio
via vswhere, no hardcoded path):

```bat
setup.bat
```

Then use the presets:

```bat
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

Presets use Ninja. Intermediates go in `build/debug` or `build/release`;
all binaries land in `build/`. Debug binaries get a `_dbg` suffix.

## Running

Working directory must be `build/` (`gamedata/` and `login.cfg` /
`hub.cfg` / `game.cfg` / `matchmaker.cfg` are resolved from CWD; missing
cfg files use compiled defaults). Start each process yourself; nothing
launches the others:

1. `mm_srv`
2. `hub_srv` and `game_srv` (both connect to the matchmaker on startup)
3. `login_srv`
4. Start the MxM client with these command line arguments: `/LogEncryption /AuthMethod:local /Network:dev /PacketEncryption:0 /AutoJoinGame /AutoLoginID:USERNAME`

Game-server physics currently supports the shipped DeathMatch collision data
(`160000094`). Set `DevForcedMap=160000094` in `game.cfg` when testing queues
that select another map; unsupported physics maps assert rather than using
the wrong collision scene.

## Networking

Windows `Server` sockets use IOCP with one completion worker per server, not
one thread per client. Game, hub, and matchmaker workers block until there is
I/O or control work and are no longer pinned to the same CPU. Linux and
standalone internal `AsyncConnection` callers retain their existing paths.

Receive buffers publish complete TCP frames only. Slow receivers retain
their send buffers and partial-send cursor. Disconnects retire outstanding
operations before publishing the disconnected handle and reusing its slot;
shutdown wakes the worker and drains those operations.

Run `network_tests.exe` from `build/` for fragmented/coalesced input,
200 simultaneous peers, 5 MiB backpressure delivery, full-capacity slot reuse,
stale handles, and bounded shutdown/resource cleanup. Debug uses the `_dbg`
suffix. `physics_tests.exe` covers the native physics and input-replay paths.

Release measurements with the unchanged client reduced median socket-send
to completed actor update from 22.93 to 10.34 ms, and the observed maximum
from 78.23 to 16.54 ms. The three network workers used 0.12% of one CPU core
combined in the measured single-client run, versus 100.44% in the retained
polling baseline. Tick-driven game dispatch remains unchanged. These
measurements do not establish full client/server physics parity.

## Physics

The server uses native x64 PhysX/NxCharacter 2.8.4 with client-derived controller
dimensions, skin width, step height, slope limit, climbing mode, and collision groups.
Gameplay positions represent controller feet.

Ordinary jumps use extracted per-master directional vertical animation curves
and `CreatureGravity` from client data; there is no tunable jump impulse.
See [physics alignment](doc/physics_alignment.md) for extraction and evidence.
Horizontal skill motion also uses extracted authored curves, including
directional/random branches and per-branch timing. Supported skill movement
uses authored command/clip timelines, cancels stale effects on replacement/tag,
and releases movement after the final graph tick. STATE_BLOCK is not treated
as a WASD lock. DeathMatch includes the enabled ART props, authored primitive
and convex actors, native scaled meshes, and gate/wall collision commands.
Human position snapshots are not seek targets; transmitted directions are
preserved. Timestamped CN/CQ inputs use bounded server-history catch-up;
casts, jumps and tags retain action ownership during replay. The native x64
backend is retained. Same-QPC input-delivery and remote-presentation parity
remain open; catch-up does not predict inputs that have not arrived.
See the [remaining roadmap](doc/physics_roadmap.md).
Bit-identical PhysX 2.8.4 behavior is not claimed.

## Code

Do not use the STL when handling strings (or even at all if possible).

## Packet Mining

https://www.youtube.com/watch?v=qmJoTYWCL5I

**Requires python 3.7.X, 3.8.X is broken at the moment.**

## Profiling

Configure with `-DPROFILE=ON` (example: `cmake -S . -B build -DPROFILE=ON -G "Visual Studio 18 2026" -A x64`)

Run `game_srv.exe` as administrator. After a session, open the capture file in the `capture` folder using `profiler/Tracy.exe`



