// keys.h — seed derivation and keypair management (Ed25519 + secp256k1)
#pragma once

#include "../core/xahau_error.h"
#include "../core/xahau_types.h"
#include "base58.h"

namespace xahau {

/**
 * Derive KeyPair from classic family seed (base58).
 * - sEd… → Curve::Ed25519
 * - s… (non-Ed) → Curve::Secp256k1
 */
Status deriveKeysFromSeed(const char* seedBase58, KeyPair& out);

/** Explicit curve override. */
Status deriveKeysFromSeed(const char* seedBase58, Curve curve, KeyPair& out);

/**
 * Encode a family seed from 16 bytes of entropy (does **not** generate randomness).
 * Caller must supply cryptographically secure entropy (e.g. hardware RNG).
 * - Curve::Ed25519 → sEd…
 * - Curve::Secp256k1 → classic s…
 * @param outCap Must be ≥ kFamilySeedBufSize (32).
 *
 * Application note: after encrypting the seed into a vault, it is not recoverable
 * from the device. Always instruct the user to copy and store the seed offline
 * at generation time.
 */
Status seedFromEntropy(const uint8_t entropy[16], Curve curve, char* out, size_t outCap);

/**
 * Encode seed from entropy and derive the KeyPair (same security notes as seedFromEntropy).
 * seedOut may be null if only keys are needed (not recommended for user-facing generate).
 */
Status generateKeyPairFromEntropy(const uint8_t entropy[16], Curve curve, char* seedOut,
                                  size_t seedOutCap, KeyPair& out);

void secureClear(KeyPair& keys);

Status accountIdFromPublicKey(const uint8_t pubKey[33], uint8_t accountId[20]);

// Address helpers live in base58.h (decodeClassicAddress / encodeClassicAddress).

}  // namespace xahau
