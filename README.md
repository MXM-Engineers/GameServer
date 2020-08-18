# Game servers

## Building

1. Download GENie https://github.com/bkaradzic/GENie
2. Copy `config.lua.0` and rename it to `config.lua`
3. Modify it to set the dependencies paths
4. Run `genie` once to see the usage
5. Generate your preferred method of building (example: `genie vs2019`)
6. Build using what you generated

## Running

1. Start `Login_debug` and `Game_debug`
2. Start The MxM client with these command line arguments: `/LogEncryption /AuthMethod:local /Network:dev /PacketEncryption:0 /AutoJoinGame /AutoLoginID:USERNAME`

## Packet Mining

https://www.youtube.com/watch?v=qmJoTYWCL5I

**Requires python 3.7.X, 3.8.X is broken at the moment.**

