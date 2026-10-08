// hex.h - hex encode/decode helpers
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "xahau_error.h"

namespace xahau {

/** out must hold 2*len+1 bytes. */
Status bytesToHex(const uint8_t* data, size_t len, char* out, size_t outCap);

/** Decode even-length hex into out; outLen set to byte count. */
Status hexToBytes(const char* hex, uint8_t* out, size_t outCap, size_t& outLen);

}  // namespace xahau
