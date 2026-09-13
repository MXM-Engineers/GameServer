#pragma once

int VelqorFileSha256(const wchar_t* path, unsigned char out[32]);
int VelqorSha256EqHex(const unsigned char got[32], const char* hex);
