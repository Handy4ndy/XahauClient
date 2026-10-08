// hash.h - portable SHA-256 / SHA-512 / RIPEMD-160
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace xahau {

void sha256(const uint8_t* data, size_t len, uint8_t out[32]);
void sha512(const uint8_t* data, size_t len, uint8_t out[64]);
/** First 32 bytes of SHA-512 (Xahau half-SHA512). */
void sha512Half(const uint8_t* data, size_t len, uint8_t out[32]);
void ripemd160(const uint8_t* data, size_t len, uint8_t out[20]);

/** Incremental SHA-256 (for RFC6979 / uECC_sign_deterministic). */
struct Sha256Ctx {
  uint32_t state[8];
  uint64_t bitCount;
  uint8_t buffer[64];
  size_t bufferLen;
};

void sha256Init(Sha256Ctx& ctx);
void sha256Update(Sha256Ctx& ctx, const uint8_t* data, size_t len);
void sha256Finish(Sha256Ctx& ctx, uint8_t out[32]);

/** AccountID = RIPEMD160(SHA256(pubkey33)). */
void accountIdFromPubKey33(const uint8_t pubKey[33], uint8_t accountId[20]);

}  // namespace xahau

