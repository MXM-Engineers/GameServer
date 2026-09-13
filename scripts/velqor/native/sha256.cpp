#include "sha256.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

int VelqorFileSha256(const wchar_t* path, unsigned char out[32]) {
  HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    return 0;
  }
  BCRYPT_ALG_HANDLE alg = NULL;
  BCRYPT_HASH_HANDLE hash = NULL;
  int ok = 0;
  if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0) {
    DWORD obj_len = 0;
    DWORD cb = 0;
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&obj_len, sizeof(obj_len), &cb, 0) == 0) {
      unsigned char* obj = (unsigned char*)HeapAlloc(GetProcessHeap(), 0, obj_len);
      if (obj) {
        if (BCryptCreateHash(alg, &hash, obj, obj_len, NULL, 0, 0) == 0) {
          unsigned char buf[1 << 16];
          for (;;) {
            DWORD n = 0;
            if (!ReadFile(file, buf, sizeof(buf), &n, NULL)) {
              break;
            }
            if (n == 0) {
              ok = (BCryptFinishHash(hash, out, 32, 0) == 0);
              break;
            }
            if (BCryptHashData(hash, buf, n, 0) != 0) {
              break;
            }
          }
          BCryptDestroyHash(hash);
        }
        HeapFree(GetProcessHeap(), 0, obj);
      }
    }
    BCryptCloseAlgorithmProvider(alg, 0);
  }
  CloseHandle(file);
  return ok;
}

int VelqorSha256EqHex(const unsigned char got[32], const char* hex) {
  static const char* kDigits = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    unsigned char v = got[i];
    char a = kDigits[v >> 4];
    char b = kDigits[v & 15];
    char c = hex[i * 2];
    char d = hex[i * 2 + 1];
    if (c >= 'A' && c <= 'F') {
      c = (char)(c - 'A' + 'a');
    }
    if (d >= 'A' && d <= 'F') {
      d = (char)(d - 'A' + 'a');
    }
    if (a != c || b != d) {
      return 0;
    }
  }
  return hex[64] == 0;
}
