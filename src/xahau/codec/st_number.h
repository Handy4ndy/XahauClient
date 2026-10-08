// st_number.h — Xahau STNumber (12-byte mantissa + exponent)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

/**
 * Encode decimal string ("1000000", "1.5", "-3e2") to 12-byte STNumber.
 * Matches ripple-binary-codec STNumber normalization.
 */
Status encodeStNumber(const char* decimal, uint8_t out12[12]);

/** Decode 12-byte STNumber to a simple decimal string (not always canonical scientific). */
Status decodeStNumber(const uint8_t in12[12], char* out, size_t outCap);

}  // namespace xahau
