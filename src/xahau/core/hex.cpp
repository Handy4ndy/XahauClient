// hex.cpp
#include "hex.h"

#include <string.h>

namespace xahau {

static char nybble(uint8_t v) {
  return static_cast<char>(v < 10 ? ('0' + v) : ('A' + (v - 10)));
}

Status bytesToHex(const uint8_t* data, size_t len, char* out, size_t outCap) {
  if (!out || outCap < len * 2 + 1) return Status::BufferTooSmall;
  if (len && !data) return Status::InvalidArgument;
  for (size_t i = 0; i < len; ++i) {
    out[i * 2] = nybble((data[i] >> 4) & 0x0F);
    out[i * 2 + 1] = nybble(data[i] & 0x0F);
  }
  out[len * 2] = '\0';
  return Status::Ok;
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

Status hexToBytes(const char* hex, uint8_t* out, size_t outCap, size_t& outLen) {
  if (!hex || !out) return Status::InvalidArgument;
  size_t n = strlen(hex);
  if (n % 2 != 0) return Status::InvalidArgument;
  size_t bytes = n / 2;
  if (bytes > outCap) return Status::BufferTooSmall;
  for (size_t i = 0; i < bytes; ++i) {
    int hi = hexVal(hex[i * 2]);
    int lo = hexVal(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return Status::DecodeFailed;
    out[i] = static_cast<uint8_t>((hi << 4) | lo);
  }
  outLen = bytes;
  return Status::Ok;
}

}  // namespace xahau
