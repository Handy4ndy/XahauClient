// bip66.h - minimal BIP66 DER encode for ECDSA r||s
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace xahau {

/** Encode 32-byte big-endian r,s into DER; returns length or 0 on failure. */
size_t bip66DerEncode(const uint8_t r[32], const uint8_t s[32], uint8_t* out, size_t outCap);

}  // namespace xahau
