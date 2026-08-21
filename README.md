# Game servers

## Building

Requires CMake (>= 3.20) and a C++14 compiler.

For the command line, first load the MSVC environment (finds Visual Studio
via vswhere, no hardcoded path):

```bat
setup.bat
```

Then configure and build with either generator:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

or the Visual Studio generator (multi-config: Debug and Release in one tree):

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

Debug builds are configured the same way (`--config Debug`) and produce
`_dbg`-suffixed binaries. Outputs land in `build/src/` (servers) and
`build/tools/` (lea, navmesh, col) under per-configuration subfolders.

## Running

1. Start `login_srv` and `game_srv` (from `build/src/Release/`)
2. Start The MxM client with these command line arguments: `/LogEncryption /AuthMethod:local /Network:dev /PacketEncryption:0 /AutoJoinGame /AutoLoginID:USERNAME`

## Code

Do not use the STL when handling strings (or even at all if possible).

## Packet Mining

https://www.youtube.com/watch?v=qmJoTYWCL5I

**Requires python 3.7.X, 3.8.X is broken at the moment.**

## Profiling

Configure with `-DPROFILE=ON` (example: `cmake -S . -B build -DPROFILE=ON -G "Visual Studio 18 2026" -A x64`)

Run `game_srv.exe` as administrator. After a session, open the capture file in the `capture` folder using `profiler/Tracy.exe`



