// secp256k1_math.h — scalar helpers for Xahau key derivation / ECDSA
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace xahau {

/** secp256k1 curve order n (big-endian). */
extern const uint8_t kSecp256k1N[32];
/** n/2 for low-S canonicalization. */
extern const uint8_t kSecp256k1HalfN[32];

/** Compare big-endian 32-byte: <0, 0, >0. */
int cmpBe32(const uint8_t a[32], const uint8_t b[32]);

/** out = (a + b) mod n */
bool secp256k1ModAdd(const uint8_t a[32], const uint8_t b[32], uint8_t out[32]);

/** If s > n/2, s = n - s (low-S). */
bool secp256k1CanonicalizeS(uint8_t s[32]);

/**
 * ripple-keypairs deriveScalar: SHA-512 half loop until in (0, n).
 * bytes || [discrim BE u32]? || counter BE u32
 */
bool secp256k1DeriveScalar(const uint8_t* bytes, size_t bytesLen, bool useDiscrim, uint32_t discrim,
                           uint8_t out32[32]);

}  // namespace xahau
