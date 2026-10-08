// secp256k1_math.cpp
#include "secp256k1_math.h"

#include "hash.h"

#include <string.h>

namespace xahau {

const uint8_t kSecp256k1N[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B, 0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41};

const uint8_t kSecp256k1HalfN[32] = {
    0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4, 0x50, 0x1D, 0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0};

int cmpBe32(const uint8_t a[32], const uint8_t b[32]) {
  for (int i = 0; i < 32; ++i) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return 0;
}

static bool isZero32(const uint8_t a[32]) {
  for (int i = 0; i < 32; ++i) {
    if (a[i]) return false;
  }
  return true;
}

// 256-bit big-endian subtract: out = a - b (a >= b)
static void subBe32(const uint8_t a[32], const uint8_t b[32], uint8_t out[32]) {
  int borrow = 0;
  for (int i = 31; i >= 0; --i) {
    int diff = (int)a[i] - (int)b[i] - borrow;
    if (diff < 0) {
      diff += 256;
      borrow = 1;
    } else {
      borrow = 0;
    }
    out[i] = (uint8_t)diff;
  }
}

// 256-bit add into 33-byte big-endian (carry in out[0])
static void addBe32to33(const uint8_t a[32], const uint8_t b[32], uint8_t out33[33]) {
  unsigned carry = 0;
  for (int i = 31; i >= 0; --i) {
    unsigned sum = (unsigned)a[i] + (unsigned)b[i] + carry;
    out33[i + 1] = (uint8_t)(sum & 0xFF);
    carry = sum >> 8;
  }
  out33[0] = (uint8_t)carry;
}

bool secp256k1ModAdd(const uint8_t a[32], const uint8_t b[32], uint8_t out[32]) {
  uint8_t sum33[33];
  addBe32to33(a, b, sum33);
  // reduce mod n: while sum >= n, sum -= n
  // represent as 33-byte; n is 32-byte
  uint8_t cur[33];
  memcpy(cur, sum33, 33);
  // If high byte or cur[1..] >= n
  for (;;) {
    if (cur[0] == 0 && cmpBe32(cur + 1, kSecp256k1N) < 0) break;
    // cur -= n (as 33-byte)
    uint8_t n33[33];
    n33[0] = 0;
    memcpy(n33 + 1, kSecp256k1N, 32);
    int borrow = 0;
    for (int i = 32; i >= 0; --i) {
      int diff = (int)cur[i] - (int)n33[i] - borrow;
      if (diff < 0) {
        diff += 256;
        borrow = 1;
      } else {
        borrow = 0;
      }
      cur[i] = (uint8_t)diff;
    }
  }
  memcpy(out, cur + 1, 32);
  return true;
}

bool secp256k1CanonicalizeS(uint8_t s[32]) {
  if (cmpBe32(s, kSecp256k1HalfN) <= 0) return true;
  subBe32(kSecp256k1N, s, s);
  return true;
}

bool secp256k1DeriveScalar(const uint8_t* bytes, size_t bytesLen, bool useDiscrim, uint32_t discrim,
                           uint8_t out32[32]) {
  if (!bytes || bytesLen == 0 || !out32) return false;
  uint8_t msg[80];
  if (bytesLen + (useDiscrim ? 8u : 4u) > sizeof(msg)) return false;

  for (uint32_t i = 0; i <= 0xFFFFFFFFu; ++i) {
    size_t off = 0;
    memcpy(msg + off, bytes, bytesLen);
    off += bytesLen;
    if (useDiscrim) {
      msg[off++] = (uint8_t)((discrim >> 24) & 0xFF);
      msg[off++] = (uint8_t)((discrim >> 16) & 0xFF);
      msg[off++] = (uint8_t)((discrim >> 8) & 0xFF);
      msg[off++] = (uint8_t)(discrim & 0xFF);
    }
    msg[off++] = (uint8_t)((i >> 24) & 0xFF);
    msg[off++] = (uint8_t)((i >> 16) & 0xFF);
    msg[off++] = (uint8_t)((i >> 8) & 0xFF);
    msg[off++] = (uint8_t)(i & 0xFF);

    uint8_t h[32];
    sha512Half(msg, off, h);
    if (isZero32(h)) continue;
    if (cmpBe32(h, kSecp256k1N) >= 0) continue;
    memcpy(out32, h, 32);
    memset(h, 0, 32);
    return true;
  }
  return false;
}

}  // namespace xahau
