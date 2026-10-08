// hash.cpp — compact portable hashes (public-domain style implementations)
#include "hash.h"

#include <string.h>

namespace xahau {

// ======================== SHA-256 ========================

namespace {

uint32_t rotr32(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

void sha256_compress(uint32_t state[8], const uint8_t block[64]) {
  static const uint32_t K[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
      0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
      0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
      0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
      0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
      0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
      0xc67178f2};
  uint32_t w[64];
  for (int i = 0; i < 16; ++i) {
    w[i] = (uint32_t)block[i * 4] << 24 | (uint32_t)block[i * 4 + 1] << 16 |
           (uint32_t)block[i * 4 + 2] << 8 | (uint32_t)block[i * 4 + 3];
  }
  for (int i = 16; i < 64; ++i) {
    uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
  uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
  for (int i = 0; i < 64; ++i) {
    uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
    uint32_t ch = (e & f) ^ ((~e) & g);
    uint32_t t1 = h + S1 + ch + K[i] + w[i];
    uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t t2 = S0 + maj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

}  // namespace

void sha256Init(Sha256Ctx& ctx) {
  ctx.state[0] = 0x6a09e667;
  ctx.state[1] = 0xbb67ae85;
  ctx.state[2] = 0x3c6ef372;
  ctx.state[3] = 0xa54ff53a;
  ctx.state[4] = 0x510e527f;
  ctx.state[5] = 0x9b05688c;
  ctx.state[6] = 0x1f83d9ab;
  ctx.state[7] = 0x5be0cd19;
  ctx.bitCount = 0;
  ctx.bufferLen = 0;
}

void sha256Update(Sha256Ctx& ctx, const uint8_t* data, size_t len) {
  if (!data && len) return;
  ctx.bitCount += (uint64_t)len * 8;
  size_t off = 0;
  if (ctx.bufferLen) {
    size_t need = 64 - ctx.bufferLen;
    size_t take = len < need ? len : need;
    memcpy(ctx.buffer + ctx.bufferLen, data, take);
    ctx.bufferLen += take;
    off += take;
    if (ctx.bufferLen == 64) {
      sha256_compress(ctx.state, ctx.buffer);
      ctx.bufferLen = 0;
    }
  }
  while (off + 64 <= len) {
    sha256_compress(ctx.state, data + off);
    off += 64;
  }
  if (off < len) {
    ctx.bufferLen = len - off;
    memcpy(ctx.buffer, data + off, ctx.bufferLen);
  }
}

void sha256Finish(Sha256Ctx& ctx, uint8_t out[32]) {
  uint8_t block[64];
  size_t rem = ctx.bufferLen;
  memcpy(block, ctx.buffer, rem);
  block[rem++] = 0x80;
  if (rem > 56) {
    memset(block + rem, 0, 64 - rem);
    sha256_compress(ctx.state, block);
    rem = 0;
  }
  memset(block + rem, 0, 56 - rem);
  uint64_t bits = ctx.bitCount;
  for (int i = 0; i < 8; ++i) block[63 - i] = (uint8_t)(bits >> (i * 8));
  sha256_compress(ctx.state, block);
  for (int i = 0; i < 8; ++i) {
    out[i * 4] = (uint8_t)(ctx.state[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(ctx.state[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(ctx.state[i] >> 8);
    out[i * 4 + 3] = (uint8_t)ctx.state[i];
  }
}

void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
  Sha256Ctx ctx;
  sha256Init(ctx);
  sha256Update(ctx, data, len);
  sha256Finish(ctx, out);
}

// ======================== SHA-512 ========================

namespace {

uint64_t rotr64(uint64_t x, uint32_t n) { return (x >> n) | (x << (64 - n)); }

void sha512_compress(uint64_t state[8], const uint8_t block[128]) {
  static const uint64_t K[80] = {
      0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
      0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
      0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
      0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
      0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
      0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
      0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
      0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
      0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
      0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
      0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
      0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
      0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
      0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
      0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
      0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
      0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
      0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
      0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
      0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};
  uint64_t w[80];
  for (int i = 0; i < 16; ++i) {
    w[i] = 0;
    for (int j = 0; j < 8; ++j) w[i] = (w[i] << 8) | block[i * 8 + j];
  }
  for (int i = 16; i < 80; ++i) {
    uint64_t s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
    uint64_t s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  uint64_t a = state[0], b = state[1], c = state[2], d = state[3];
  uint64_t e = state[4], f = state[5], g = state[6], h = state[7];
  for (int i = 0; i < 80; ++i) {
    uint64_t S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
    uint64_t ch = (e & f) ^ ((~e) & g);
    uint64_t t1 = h + S1 + ch + K[i] + w[i];
    uint64_t S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
    uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint64_t t2 = S0 + maj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

}  // namespace

void sha512(const uint8_t* data, size_t len, uint8_t out[64]) {
  uint64_t state[8] = {0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL,
                       0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
                       0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};
  uint8_t block[128];
  size_t off = 0;
  while (off + 128 <= len) {
    sha512_compress(state, data + off);
    off += 128;
  }
  size_t rem = len - off;
  memcpy(block, data + off, rem);
  block[rem++] = 0x80;
  if (rem > 112) {
    memset(block + rem, 0, 128 - rem);
    sha512_compress(state, block);
    rem = 0;
  }
  memset(block + rem, 0, 112 - rem);
  // 128-bit length; high 64 zero for our sizes
  memset(block + 112, 0, 8);
  uint64_t bits = (uint64_t)len * 8;
  for (int i = 0; i < 8; ++i) block[127 - i] = (uint8_t)(bits >> (i * 8));
  sha512_compress(state, block);
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) out[i * 8 + j] = (uint8_t)(state[i] >> (56 - j * 8));
  }
}

void sha512Half(const uint8_t* data, size_t len, uint8_t out[32]) {
  uint8_t full[64];
  sha512(data, len, full);
  memcpy(out, full, 32);
  memset(full, 0, sizeof(full));
}

// ======================== RIPEMD-160 ========================

namespace {

uint32_t rol32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

void ripemd160_compress(uint32_t h[5], const uint8_t block[64]) {
  static const uint32_t KL[5] = {0x00000000, 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xA953FD4E};
  static const uint32_t KR[5] = {0x50A28BE6, 0x5C4DD124, 0x6D703EF3, 0x7A6D76E9, 0x00000000};
  static const uint8_t RL[80] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                 7,  4,  13, 1,  10, 6,  15, 3,  12, 0,  9,  5,  2,  14, 11, 8,
                                 3,  10, 14, 4,  9,  15, 8,  1,  2,  7,  0,  6,  13, 11, 5,  12,
                                 1,  9,  11, 10, 0,  8,  12, 4,  13, 3,  7,  15, 14, 5,  6,  2,
                                 4,  0,  5,  9,  7,  12, 2,  10, 14, 1,  3,  8,  11, 6,  15, 13};
  static const uint8_t RR[80] = {5,  14, 7,  0,  9,  2,  11, 4,  13, 6,  15, 8,  1,  10, 3,  12,
                                 6,  11, 3,  7,  0,  13, 5,  10, 14, 15, 8,  12, 4,  9,  1,  2,
                                 15, 5,  1,  3,  7,  14, 6,  9,  11, 8,  12, 2,  10, 0,  4,  13,
                                 8,  6,  4,  1,  3,  11, 15, 0,  5,  12, 2,  13, 9,  7,  10, 14,
                                 12, 15, 10, 4,  1,  5,  8,  7,  6,  2,  13, 14, 0,  3,  9,  11};
  static const uint8_t SL[80] = {11, 14, 15, 12, 5,  8,  7,  9,  11, 13, 14, 15, 6,  7,  9,  8,
                                 7,  6,  8,  13, 11, 9,  7,  15, 7,  12, 15, 9,  11, 7,  13, 12,
                                 11, 13, 6,  7,  14, 9,  13, 15, 14, 8,  13, 6,  5,  12, 7,  5,
                                 11, 12, 14, 15, 14, 15, 9,  8,  9,  14, 5,  6,  8,  6,  5,  12,
                                 9,  15, 5,  11, 6,  8,  13, 12, 5,  12, 13, 14, 11, 8,  5,  6};
  static const uint8_t SR[80] = {8,  9,  9,  11, 13, 15, 15, 5,  7,  7,  8,  11, 14, 14, 12, 6,
                                 9,  13, 15, 7,  12, 8,  9,  11, 7,  7,  12, 7,  6,  15, 13, 11,
                                 9,  7,  15, 11, 8,  6,  6,  14, 12, 13, 5,  14, 13, 13, 7,  5,
                                 15, 5,  8,  11, 14, 14, 6,  14, 6,  9,  12, 9,  12, 5,  15, 8,
                                 8,  5,  12, 9,  12, 5,  14, 6,  8,  13, 6,  5,  15, 13, 11, 11};

  uint32_t w[16];
  for (int i = 0; i < 16; ++i) {
    w[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) |
           ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);
  }
  uint32_t al = h[0], bl = h[1], cl = h[2], dl = h[3], el = h[4];
  uint32_t ar = h[0], br = h[1], cr = h[2], dr = h[3], er = h[4];
  for (int j = 0; j < 80; ++j) {
    int rnd = j / 16;
    uint32_t fl, fr;
    switch (rnd) {
      case 0: fl = bl ^ cl ^ dl; break;
      case 1: fl = (bl & cl) | (~bl & dl); break;
      case 2: fl = (bl | ~cl) ^ dl; break;
      case 3: fl = (bl & dl) | (cl & ~dl); break;
      default: fl = bl ^ (cl | ~dl); break;
    }
    switch (rnd) {
      case 0: fr = br ^ (cr | ~dr); break;
      case 1: fr = (br & dr) | (cr & ~dr); break;
      case 2: fr = (br | ~cr) ^ dr; break;
      case 3: fr = (br & cr) | (~br & dr); break;
      default: fr = br ^ cr ^ dr; break;
    }
    uint32_t tl = rol32(al + fl + w[RL[j]] + KL[rnd], SL[j]) + el;
    al = el;
    el = dl;
    dl = rol32(cl, 10);
    cl = bl;
    bl = tl;
    uint32_t tr = rol32(ar + fr + w[RR[j]] + KR[rnd], SR[j]) + er;
    ar = er;
    er = dr;
    dr = rol32(cr, 10);
    cr = br;
    br = tr;
  }
  uint32_t t = h[1] + cl + dr;
  h[1] = h[2] + dl + er;
  h[2] = h[3] + el + ar;
  h[3] = h[4] + al + br;
  h[4] = h[0] + bl + cr;
  h[0] = t;
}

}  // namespace

void ripemd160(const uint8_t* data, size_t len, uint8_t out[20]) {
  uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  uint8_t block[64];
  size_t off = 0;
  while (off + 64 <= len) {
    ripemd160_compress(h, data + off);
    off += 64;
  }
  size_t rem = len - off;
  memcpy(block, data + off, rem);
  block[rem++] = 0x80;
  if (rem > 56) {
    memset(block + rem, 0, 64 - rem);
    ripemd160_compress(h, block);
    rem = 0;
  }
  memset(block + rem, 0, 56 - rem);
  uint64_t bits = (uint64_t)len * 8;
  for (int i = 0; i < 8; ++i) block[56 + i] = (uint8_t)(bits >> (i * 8));
  ripemd160_compress(h, block);
  for (int i = 0; i < 5; ++i) {
    out[i * 4] = (uint8_t)h[i];
    out[i * 4 + 1] = (uint8_t)(h[i] >> 8);
    out[i * 4 + 2] = (uint8_t)(h[i] >> 16);
    out[i * 4 + 3] = (uint8_t)(h[i] >> 24);
  }
}

void accountIdFromPubKey33(const uint8_t pubKey[33], uint8_t accountId[20]) {
  uint8_t sha[32];
  sha256(pubKey, 33, sha);
  ripemd160(sha, 32, accountId);
  memset(sha, 0, sizeof(sha));
}

}  // namespace xahau
