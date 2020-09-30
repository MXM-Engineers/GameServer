# Packet Encryption

MxM uses LEA Encryption (https://en.wikipedia.org/wiki/LEA_(cipher))

## Key
1. We make a 16B key with the ProtocolCrc, ErrorCrc, some Module Version number (always the same), client IP as DWORD + client port (CreateLeaKey16)
2. We blow it up to 32B and do some operation on it
3. We generate filters (3, they are all the same) with the 32B key

## Decryption / Encryption
* We use the filter to do some fancy xoring (LeaEncryptImpl)