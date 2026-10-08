// st_number.cpp — Xahau Number: int64 mantissa BE + int32 exponent BE
#include "st_number.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

namespace xahau {

// 10^18 .. 10^19-1 target range for mantissa magnitude
static const uint64_t kMinMantissaU = 1000000000000000000ULL;       // 10^18
static const uint64_t kMaxMantissaU = 9999999999999999999ULL;       // 10^19-1 (fits in u64? 10^19-1 > 2^63)
// Use uint64 for abs mantissa up to 2^63-1 after clamp
static const int64_t kMaxInt64 = 9223372036854775807LL;
static const int32_t kMinExp = -32768;
static const int32_t kMaxExp = 32768;
static const int32_t kZeroExp = (int32_t)(-2147483647 - 1);  // INT32_MIN

static void writeI64BE(uint8_t* p, int64_t v) {
  uint64_t u = (uint64_t)v;
  for (int i = 7; i >= 0; --i) {
    p[i] = (uint8_t)(u & 0xFF);
    u >>= 8;
  }
}

static void writeI32BE(uint8_t* p, int32_t v) {
  uint32_t u = (uint32_t)v;
  p[0] = (uint8_t)((u >> 24) & 0xFF);
  p[1] = (uint8_t)((u >> 16) & 0xFF);
  p[2] = (uint8_t)((u >> 8) & 0xFF);
  p[3] = (uint8_t)(u & 0xFF);
}

static int64_t readI64BE(const uint8_t* p) {
  uint64_t u = 0;
  for (int i = 0; i < 8; ++i) u = (u << 8) | p[i];
  return (int64_t)u;
}

static int32_t readI32BE(const uint8_t* p) {
  uint32_t u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
  return (int32_t)u;
}

Status encodeStNumber(const char* decimal, uint8_t out12[12]) {
  if (!decimal || !out12) return Status::InvalidArgument;

  // Parse simple decimal / optional scientific
  bool neg = false;
  const char* p = decimal;
  if (*p == '+') ++p;
  if (*p == '-') {
    neg = true;
    ++p;
  }
  if (!*p) return Status::InvalidField;

  char digits[48];
  size_t dlen = 0;
  int expAdj = 0;
  bool seenDot = false;
  bool seenDigit = false;

  while (*p && dlen + 1 < sizeof(digits)) {
    if (*p >= '0' && *p <= '9') {
      digits[dlen++] = *p;
      seenDigit = true;
      if (seenDot) --expAdj;
    } else if (*p == '.' && !seenDot) {
      seenDot = true;
    } else if ((*p == 'e' || *p == 'E') && seenDigit) {
      ++p;
      bool eneg = false;
      if (*p == '+') ++p;
      if (*p == '-') {
        eneg = true;
        ++p;
      }
      int e = 0;
      if (*p < '0' || *p > '9') return Status::InvalidField;
      while (*p >= '0' && *p <= '9') {
        e = e * 10 + (*p - '0');
        ++p;
      }
      expAdj += eneg ? -e : e;
      break;
    } else {
      return Status::InvalidField;
    }
    ++p;
  }
  digits[dlen] = '\0';
  if (!seenDigit) return Status::InvalidField;

  // Strip leading zeros
  size_t start = 0;
  while (start + 1 < dlen && digits[start] == '0') ++start;

  // Zero
  bool allZero = true;
  for (size_t i = start; i < dlen; ++i) {
    if (digits[i] != '0') {
      allZero = false;
      break;
    }
  }
  if (allZero) {
    memset(out12, 0, 12);
    writeI32BE(out12 + 8, kZeroExp);
    return Status::Ok;
  }

  // Strip trailing zeros (increase exponent)
  while (dlen > start + 1 && digits[dlen - 1] == '0') {
    --dlen;
    ++expAdj;
  }

  // Build mantissa as decimal string slice digits[start..dlen)
  // Use uint64 accumulation carefully — may need big steps via repeated *10
  // For embedded: use double-uint64 style limited length
  // Max digits we'll keep before normalize: 20
  char mantStr[24];
  size_t mlen = dlen - start;
  if (mlen > 20) {
    // truncate with round
    size_t keep = 19;
    memcpy(mantStr, digits + start, keep);
    mantStr[keep] = '\0';
    expAdj += (int)(mlen - keep);
    mlen = keep;
  } else {
    memcpy(mantStr, digits + start, mlen);
    mantStr[mlen] = '\0';
  }

  // Parse to uint64 (magnitude)
  uint64_t mag = 0;
  for (size_t i = 0; i < mlen; ++i) {
    if (mag > (UINT64_MAX / 10)) return Status::InvalidField;
    mag = mag * 10 + (uint64_t)(mantStr[i] - '0');
  }
  int32_t exp = (int32_t)expAdj;

  // Grow mantissa toward 10^18
  while (mag > 0 && mag < kMinMantissaU && exp > kMinExp) {
    if (mag > UINT64_MAX / 10) break;
    mag *= 10;
    --exp;
  }

  // Shrink if > 10^19-1 or > INT64_MAX
  int lastDigit = -1;
  while (mag > kMaxMantissaU || mag > (uint64_t)kMaxInt64) {
    if (exp >= kMaxExp) return Status::InvalidField;
    lastDigit = (int)(mag % 10);
    mag /= 10;
    ++exp;
  }
  if (lastDigit >= 5) {
    mag += 1;
    if (mag > (uint64_t)kMaxInt64) {
      if (exp >= kMaxExp) return Status::InvalidField;
      lastDigit = (int)(mag % 10);
      mag /= 10;
      ++exp;
      if (lastDigit >= 5) mag += 1;
    }
  }

  if (exp < kMinExp || (mag > 0 && mag < kMinMantissaU && exp <= kMinExp)) {
    // underflow → zero
    memset(out12, 0, 12);
    writeI32BE(out12 + 8, kZeroExp);
    return Status::Ok;
  }
  if (exp > kMaxExp) return Status::InvalidField;

  int64_t signedMant = (int64_t)mag;
  if (neg) signedMant = -signedMant;

  writeI64BE(out12, signedMant);
  writeI32BE(out12 + 8, exp);
  return Status::Ok;
}

Status decodeStNumber(const uint8_t in12[12], char* out, size_t outCap) {
  if (!in12 || !out || outCap < 2) return Status::InvalidArgument;
  int64_t mant = readI64BE(in12);
  int32_t exp = readI32BE(in12 + 8);
  if (mant == 0) {
    out[0] = '0';
    out[1] = '\0';
    return Status::Ok;
  }
  // Simple scientific form: mant * 10^exp
  bool neg = mant < 0;
  uint64_t mag = (uint64_t)(neg ? -mant : mant);
  char tmp[48];
  int n = snprintf(tmp, sizeof(tmp), "%s%llue%d", neg ? "-" : "", (unsigned long long)mag, (int)exp);
  if (n < 0 || (size_t)n >= outCap) return Status::BufferTooSmall;
  memcpy(out, tmp, (size_t)n + 1);
  return Status::Ok;
}

}  // namespace xahau
