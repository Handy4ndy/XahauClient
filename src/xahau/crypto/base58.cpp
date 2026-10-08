// base58.cpp - Xahau Base58Check (aligned with ripple-address-codec)
#include "base58.h"

#include "hash.h"
#include "../platform/xahau_platform.h"

#include <string.h>

namespace xahau {

static const char kAlphabet[] = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

static int alphabetIndex(char c) {
  for (int i = 0; i < 58; ++i) {
    if (kAlphabet[i] == c) return i;
  }
  return -1;
}

static void sha256d4(const uint8_t* data, size_t len, uint8_t out4[4]) {
  uint8_t h1[32], h2[32];
  sha256(data, len, h1);
  sha256(h1, 32, h2);
  memcpy(out4, h2, 4);
  memset(h1, 0, sizeof(h1));
  memset(h2, 0, sizeof(h2));
}

/** Decode to little-endian digits then reverse to big-endian; pad left to expectedLen. */
static Status base58DecodePad(const char* input, uint8_t* outBytes, size_t expectedLen) {
  if (!input || !outBytes) return Status::InvalidArgument;
  uint8_t tmp[64];
  memset(tmp, 0, sizeof(tmp));
  size_t bytelen = 0;
  size_t inputlen = strlen(input);
  for (size_t i = 0; i < inputlen; ++i) {
    int digit = alphabetIndex(input[i]);
    if (digit < 0) return Status::DecodeFailed;
    uint32_t carry = (uint32_t)digit;
    for (size_t j = 0; j < bytelen; ++j) {
      carry += (uint32_t)tmp[j] * 58u;
      tmp[j] = (uint8_t)(carry & 0xFF);
      carry >>= 8;
    }
    while (carry) {
      if (bytelen >= sizeof(tmp)) return Status::BufferTooSmall;
      tmp[bytelen++] = (uint8_t)(carry & 0xFF);
      carry >>= 8;
    }
  }
  for (size_t i = 0; i < bytelen / 2; ++i) {
    uint8_t t = tmp[i];
    tmp[i] = tmp[bytelen - 1 - i];
    tmp[bytelen - 1 - i] = t;
  }
  if (bytelen > expectedLen) return Status::DecodeFailed;
  // left-pad zeros
  size_t pad = expectedLen - bytelen;
  memset(outBytes, 0, pad);
  memcpy(outBytes + pad, tmp, bytelen);
  return Status::Ok;
}

static Status verifyChecksum(uint8_t* bytes, size_t totalLen, size_t payloadLen) {
  if (totalLen != payloadLen + 4) return Status::DecodeFailed;
  uint8_t chk[4];
  sha256d4(bytes, payloadLen, chk);
  if (memcmp(chk, bytes + payloadLen, 4) != 0) return Status::DecodeFailed;
  return Status::Ok;
}

Status base58DecodeCheck(const char* input, uint8_t* out, size_t outCap, size_t& outLen) {
  // Generic path: decode without fixed pad, then verify (used less often).
  uint8_t raw[64];
  memset(raw, 0, sizeof(raw));
  size_t bytelen = 0;
  size_t inputlen = strlen(input);
  for (size_t i = 0; i < inputlen; ++i) {
    int digit = alphabetIndex(input[i]);
    if (digit < 0) return Status::DecodeFailed;
    uint32_t carry = (uint32_t)digit;
    for (size_t j = 0; j < bytelen; ++j) {
      carry += (uint32_t)raw[j] * 58u;
      raw[j] = (uint8_t)(carry & 0xFF);
      carry >>= 8;
    }
    while (carry) {
      if (bytelen >= sizeof(raw)) return Status::BufferTooSmall;
      raw[bytelen++] = (uint8_t)(carry & 0xFF);
      carry >>= 8;
    }
  }
  for (size_t i = 0; i < bytelen / 2; ++i) {
    uint8_t t = raw[i];
    raw[i] = raw[bytelen - 1 - i];
    raw[bytelen - 1 - i] = t;
  }
  if (bytelen < 5) return Status::DecodeFailed;
  // Count leading zeros in base58 ('r' in Xahau alphabet is index 0)
  size_t zeros = 0;
  while (zeros < inputlen && input[zeros] == kAlphabet[0]) ++zeros;
  size_t total = bytelen + zeros;
  if (total > sizeof(raw)) return Status::BufferTooSmall;
  if (zeros) {
    memmove(raw + zeros, raw, bytelen);
    memset(raw, 0, zeros);
  }
  if (total < 5) return Status::DecodeFailed;
  uint8_t chk[4];
  sha256d4(raw, total - 4, chk);
  if (memcmp(chk, raw + total - 4, 4) != 0) return Status::DecodeFailed;
  size_t payload = total - 4;
  if (payload > outCap) return Status::BufferTooSmall;
  memcpy(out, raw, payload);
  outLen = payload;
  return Status::Ok;
}

Status base58EncodeCheck(const uint8_t* payload, size_t payloadLen, char* out, size_t outCap) {
  if (!payload || !out || payloadLen + 4 > 64) return Status::InvalidArgument;
  uint8_t raw[64];
  memcpy(raw, payload, payloadLen);
  sha256d4(raw, payloadLen, raw + payloadLen);
  size_t total = payloadLen + 4;

  size_t zeros = 0;
  while (zeros < total && raw[zeros] == 0) ++zeros;

  uint8_t b58[80];
  size_t b58Len = 0;
  for (size_t i = zeros; i < total; ++i) {
    uint32_t carry = raw[i];
    for (size_t j = 0; j < b58Len; ++j) {
      carry += (uint32_t)b58[j] << 8;
      b58[j] = (uint8_t)(carry % 58);
      carry /= 58;
    }
    while (carry) {
      b58[b58Len++] = (uint8_t)(carry % 58);
      carry /= 58;
    }
  }
  size_t need = zeros + b58Len + 1;
  if (need > outCap) return Status::BufferTooSmall;
  size_t idx = 0;
  for (size_t i = 0; i < zeros; ++i) out[idx++] = kAlphabet[0];
  for (size_t i = b58Len; i > 0; --i) out[idx++] = kAlphabet[b58[i - 1]];
  out[idx] = '\0';
  return Status::Ok;
}

Status decodeClassicAddress(const char* address, uint8_t accountId[20]) {
  uint8_t raw[25];
  Status st = base58DecodePad(address, raw, 25);
  if (st != Status::Ok) return st;
  st = verifyChecksum(raw, 25, 21);
  if (st != Status::Ok) return st;
  if (raw[0] != 0x00) return Status::InvalidAddress;
  memcpy(accountId, raw + 1, 20);
  return Status::Ok;
}

Status encodeClassicAddress(const uint8_t accountId[20], char out[36]) {
  uint8_t payload[21];
  payload[0] = 0x00;
  memcpy(payload + 1, accountId, 20);
  return base58EncodeCheck(payload, 21, out, 36);
}

Status decodeEd25519Seed(const char* seed, uint8_t entropy[16]) {
  uint8_t raw[23];
  Status st = base58DecodePad(seed, raw, 23);
  if (st != Status::Ok) return st;
  st = verifyChecksum(raw, 23, 19);
  if (st != Status::Ok) return st;
  if (raw[0] != 0x01 || raw[1] != 0xE1 || raw[2] != 0x4B) return Status::InvalidSeed;
  memcpy(entropy, raw + 3, 16);
  return Status::Ok;
}

Status decodeSecp256k1Seed(const char* seed, uint8_t entropy[16]) {
  uint8_t raw[21];
  Status st = base58DecodePad(seed, raw, 21);
  if (st != Status::Ok) return st;
  st = verifyChecksum(raw, 21, 17);
  if (st != Status::Ok) return st;
  if (raw[0] != 0x21) return Status::InvalidSeed;
  memcpy(entropy, raw + 1, 16);
  return Status::Ok;
}

Status encodeEd25519Seed(const uint8_t entropy[16], char* out, size_t outCap) {
  if (!entropy || !out) return Status::InvalidArgument;
  // Prefix 0x01 0xE1 0x4B (ripple-address-codec ed25519 seed) + 16 entropy
  uint8_t payload[19];
  payload[0] = 0x01;
  payload[1] = 0xE1;
  payload[2] = 0x4B;
  memcpy(payload + 3, entropy, 16);
  Status st = base58EncodeCheck(payload, 19, out, outCap);
  secureZero(payload, sizeof(payload));
  return st;
}

Status encodeSecp256k1Seed(const uint8_t entropy[16], char* out, size_t outCap) {
  if (!entropy || !out) return Status::InvalidArgument;
  // Prefix 0x21 (family seed) + 16 entropy
  uint8_t payload[17];
  payload[0] = 0x21;
  memcpy(payload + 1, entropy, 16);
  Status st = base58EncodeCheck(payload, 17, out, outCap);
  secureZero(payload, sizeof(payload));
  return st;
}

}  // namespace xahau
