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

## Code

Do not use the STL when handling strings (or even at all if possible).

## Packet Mining

https://www.youtube.com/watch?v=qmJoTYWCL5I

**Requires python 3.7.X, 3.8.X is broken at the moment.**

## Profiling

Configure with `-DPROFILE=ON` (example: `cmake -S . -B build -DPROFILE=ON -G "Visual Studio 18 2026" -A x64`)

Run `game_srv.exe` as administrator. After a session, open the capture file in the `capture` folder using `profiler/Tracy.exe`



