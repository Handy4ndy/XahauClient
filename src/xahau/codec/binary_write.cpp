// binary_write.cpp
#include "binary_write.h"

#include <stdlib.h>
#include <string.h>

namespace xahau {

Status writeByte(uint8_t* buf, size_t& pos, size_t max, uint8_t v) {
  if (pos + 1 > max) return Status::BufferTooSmall;
  buf[pos++] = v;
  return Status::Ok;
}

Status writeBytes(uint8_t* buf, size_t& pos, size_t max, const uint8_t* data, size_t len) {
  if (len && !data) return Status::InvalidArgument;
  if (pos + len > max) return Status::BufferTooSmall;
  if (len) memcpy(buf + pos, data, len);
  pos += len;
  return Status::Ok;
}

Status writeUInt16BE(uint8_t* buf, size_t& pos, size_t max, uint16_t v) {
  if (pos + 2 > max) return Status::BufferTooSmall;
  buf[pos++] = static_cast<uint8_t>((v >> 8) & 0xFF);
  buf[pos++] = static_cast<uint8_t>(v & 0xFF);
  return Status::Ok;
}

Status writeUInt32BE(uint8_t* buf, size_t& pos, size_t max, uint32_t v) {
  if (pos + 4 > max) return Status::BufferTooSmall;
  buf[pos++] = static_cast<uint8_t>((v >> 24) & 0xFF);
  buf[pos++] = static_cast<uint8_t>((v >> 16) & 0xFF);
  buf[pos++] = static_cast<uint8_t>((v >> 8) & 0xFF);
  buf[pos++] = static_cast<uint8_t>(v & 0xFF);
  return Status::Ok;
}

Status writeUInt64BE(uint8_t* buf, size_t& pos, size_t max, uint64_t v) {
  if (pos + 8 > max) return Status::BufferTooSmall;
  for (int i = 7; i >= 0; --i) {
    buf[pos++] = static_cast<uint8_t>((v >> (i * 8)) & 0xFF);
  }
  return Status::Ok;
}

Status writeFieldHeader(uint8_t* buf, size_t& pos, size_t max, uint16_t type, uint16_t nth) {
  Status st;
  if (type < 16 && nth < 16) {
    return writeByte(buf, pos, max, static_cast<uint8_t>((type << 4) | nth));
  }
  if (type < 16 && nth >= 16) {
    st = writeByte(buf, pos, max, static_cast<uint8_t>(type << 4));
    if (st != Status::Ok) return st;
    return writeByte(buf, pos, max, static_cast<uint8_t>(nth));
  }
  if (type >= 16 && nth < 16) {
    st = writeByte(buf, pos, max, static_cast<uint8_t>(nth));
    if (st != Status::Ok) return st;
    return writeByte(buf, pos, max, static_cast<uint8_t>(type));
  }
  st = writeByte(buf, pos, max, 0);
  if (st != Status::Ok) return st;
  st = writeByte(buf, pos, max, static_cast<uint8_t>(type));
  if (st != Status::Ok) return st;
  return writeByte(buf, pos, max, static_cast<uint8_t>(nth));
}

Status writeFieldHeaderCode(uint8_t* buf, size_t& pos, size_t max, uint32_t code) {
  uint16_t type = static_cast<uint16_t>((code >> 16) & 0xFFFF);
  uint16_t nth = static_cast<uint16_t>(code & 0xFFFF);
  return writeFieldHeader(buf, pos, max, type, nth);
}

Status writeVLLength(uint8_t* buf, size_t& pos, size_t max, size_t len) {
  if (len <= 192) {
    return writeByte(buf, pos, max, static_cast<uint8_t>(len));
  }
  if (len <= 12480) {
    size_t n = len - 193;
    Status st = writeByte(buf, pos, max, static_cast<uint8_t>(193 + (n >> 8)));
    if (st != Status::Ok) return st;
    return writeByte(buf, pos, max, static_cast<uint8_t>(n & 0xFF));
  }
  if (len <= 918744) {
    size_t n = len - 12481;
    Status st = writeByte(buf, pos, max, static_cast<uint8_t>(241 + (n >> 16)));
    if (st != Status::Ok) return st;
    st = writeByte(buf, pos, max, static_cast<uint8_t>((n >> 8) & 0xFF));
    if (st != Status::Ok) return st;
    return writeByte(buf, pos, max, static_cast<uint8_t>(n & 0xFF));
  }
  return Status::InvalidArgument;
}

Status writeXrpAmount(uint8_t* buf, size_t& pos, size_t max, uint64_t drops) {
  uint64_t encoded = 0x4000000000000000ULL | drops;
  return writeUInt64BE(buf, pos, max, encoded);
}

Status writeMptAmount(uint8_t* buf, size_t& pos, size_t max, uint64_t value,
                      const uint8_t issuanceId[24]) {
  if (!issuanceId) return Status::InvalidArgument;
  // Max MPT quantity is 2^63-1.
  if (value > 0x7FFFFFFFFFFFFFFFULL) return Status::InvalidField;
  // Lead byte: not-IOU(0) | positive(1) | isMPT(1) | reserved 00000 → 0x60
  Status st = writeByte(buf, pos, max, 0x60);
  if (st != Status::Ok) return st;
  st = writeUInt64BE(buf, pos, max, value);
  if (st != Status::Ok) return st;
  return writeBytes(buf, pos, max, issuanceId, 24);
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

Status writeIssuedAmount(uint8_t* buf, size_t& pos, size_t max, const char* valueStr,
                         const char* currency, const uint8_t issuer[20]) {
  if (!valueStr || !currency || !issuer) return Status::InvalidArgument;

  bool negative = false;
  const char* p = valueStr;
  if (*p == '+') ++p;
  if (*p == '-') {
    negative = true;
    ++p;
  }

  char digits[64];
  size_t dlen = 0;
  int exp10 = 0;
  bool seenDot = false;
  while (*p && dlen < 63) {
    if (*p >= '0' && *p <= '9') {
      digits[dlen++] = *p;
      if (seenDot) --exp10;
    } else if (*p == '.') {
      seenDot = true;
    } else {
      return Status::InvalidField;
    }
    ++p;
  }
  digits[dlen] = '\0';

  uint64_t value64 = 0x8000000000000000ULL;  // zero issued
  if (dlen > 0) {
    size_t first = 0;
    while (first < dlen && digits[first] == '0') ++first;
    size_t sigLen = dlen - first;
    if (sigLen > 0) {
      uint64_t mantissa = 0;
      size_t take = sigLen > 16 ? 16 : sigLen;
      for (size_t i = 0; i < take; ++i) {
        mantissa = mantissa * 10ULL + static_cast<uint64_t>(digits[first + i] - '0');
      }
      int offset = static_cast<int>(sigLen - take) + exp10;
      const uint64_t ten15 = 1000000000000000ULL;
      while (mantissa < ten15 && offset > -1000) {
        mantissa *= 10ULL;
        --offset;
      }
      while (mantissa >= ten15 * 10ULL) {
        mantissa /= 10ULL;
        ++offset;
      }
      if (offset < -96) offset = -96;
      if (offset > 80) offset = 80;

      value64 = mantissa & 0x3FFFFFFFFFFFFFULL;
      // Xahau: not_xrp(1) | is_pos | exponent bias
      // Classic formula: hi = 512 + 256(if positive) + (exp+97); then (hi<<54)|mant
      uint64_t hi = 512ULL + (negative ? 0ULL : 256ULL) + static_cast<uint64_t>(offset + 97);
      value64 |= (hi << 54);
    }
  }

  Status st = writeUInt64BE(buf, pos, max, value64);
  if (st != Status::Ok) return st;

  uint8_t cur[20];
  memset(cur, 0, sizeof(cur));
  size_t curLen = strlen(currency);
  if (curLen == 3) {
    cur[12] = static_cast<uint8_t>(currency[0]);
    cur[13] = static_cast<uint8_t>(currency[1]);
    cur[14] = static_cast<uint8_t>(currency[2]);
  } else if (curLen == 40) {
    for (int i = 0; i < 20; ++i) {
      int hi = hexNibble(currency[i * 2]);
      int lo = hexNibble(currency[i * 2 + 1]);
      if (hi < 0 || lo < 0) return Status::InvalidField;
      cur[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
  } else {
    return Status::InvalidField;
  }

  st = writeBytes(buf, pos, max, cur, 20);
  if (st != Status::Ok) return st;
  return writeBytes(buf, pos, max, issuer, 20);
}

}  // namespace xahau
