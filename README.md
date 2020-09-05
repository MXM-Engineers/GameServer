# Game servers

## Building

1. Download GENie https://github.com/bkaradzic/GENie
2. Run `genie` once to see the usage
3. Generate your preferred method of building (example: `genie vs2019`)
4. Build using what you generated

## Running

1. Start `Login_debug` and `Game_debug`
2. Start The MxM client with these command line arguments: `/LogEncryption /AuthMethod:local /Network:dev /PacketEncryption:0 /AutoJoinGame /AutoLoginID:USERNAME`

## Code

Do not use the STL when handling strings (or even at all if possible).

## Packet Mining

https://www.youtube.com/watch?v=qmJoTYWCL5I

**Requires python 3.7.X, 3.8.X is broken at the moment.**



