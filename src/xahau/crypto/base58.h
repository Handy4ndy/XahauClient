// base58.h — Xahau Base58Check (alphabet + seeds + classic addresses)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

Status base58DecodeCheck(const char* input, uint8_t* out, size_t outCap, size_t& outLen);
Status base58EncodeCheck(const uint8_t* payload, size_t payloadLen, char* out, size_t outCap);

/** Decode classic r-address → 20-byte AccountID. */
Status decodeClassicAddress(const char* address, uint8_t accountId[20]);
/** Encode 20-byte AccountID → classic r-address (null-terminated). */
Status encodeClassicAddress(const uint8_t accountId[20], char out[36]);

/** Ed25519 seed sEd… → 16-byte entropy. */
Status decodeEd25519Seed(const char* seed, uint8_t entropy[16]);
/** Family seed s… → 16-byte entropy (secp256k1). */
Status decodeSecp256k1Seed(const char* seed, uint8_t entropy[16]);

/**
 * Encode 16-byte entropy → Ed25519 family seed (sEd…).
 * @param outCap Must be ≥ 32 (null-terminated string; typical length ~29).
 */
Status encodeEd25519Seed(const uint8_t entropy[16], char* out, size_t outCap);
/**
 * Encode 16-byte entropy → classic secp256k1 family seed (s…).
 * @param outCap Must be ≥ 32 (null-terminated string; typical length ~29).
 */
Status encodeSecp256k1Seed(const uint8_t entropy[16], char* out, size_t outCap);

/** Recommended capacity for family seed strings including NUL. */
static constexpr size_t kFamilySeedMaxChars = 31;
static constexpr size_t kFamilySeedBufSize = kFamilySeedMaxChars + 1;  // 32

}  // namespace xahau
