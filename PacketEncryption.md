# Packet Encryption

MxM uses LEA Encryption (https://en.wikipedia.org/wiki/LEA_(cipher))

## Key
1. We make a 16B key with the ProtocolCrc, ErrorCrc, some Version number (always the same), client IP as DWORD + client port (CreateLeaKey16)
2. We copy it once to make it 32B
3. We generate filters (3, they are all the same) with the 32B key

## Decryption / Encryption
* We use the filter to do some fancy xoring (LeaEncryptImpl)
* The key is updated in the process, altering further decryptions (so we have to keep the key state)