// bip66.cpp
#include "bip66.h"

#include <string.h>

namespace xahau {

static size_t trimInteger(const uint8_t* in, size_t inlen, uint8_t* out) {
  size_t i = 0;
  while (i < inlen && in[i] == 0) ++i;
  size_t len = inlen - i;
  if (len == 0) {
    out[0] = 0;
    return 1;
  }
  if (in[i] & 0x80) {
    out[0] = 0x00;
    memcpy(out + 1, in + i, len);
    return len + 1;
  }
  memcpy(out, in + i, len);
  return len;
}

size_t bip66DerEncode(const uint8_t r[32], const uint8_t s[32], uint8_t* out, size_t outCap) {
  if (!r || !s || !out) return 0;
  uint8_t rbuf[34], sbuf[34];
  size_t rtrim = trimInteger(r, 32, rbuf);
  size_t strim = trimInteger(s, 32, sbuf);
  size_t seqlen = 2 + rtrim + 2 + strim;
  if (seqlen + 2 > outCap) return 0;
  size_t pos = 0;
  out[pos++] = 0x30;
  out[pos++] = (uint8_t)seqlen;
  out[pos++] = 0x02;
  out[pos++] = (uint8_t)rtrim;
  memcpy(out + pos, rbuf, rtrim);
  pos += rtrim;
  out[pos++] = 0x02;
  out[pos++] = (uint8_t)strim;
  memcpy(out + pos, sbuf, strim);
  pos += strim;
  return pos;
}

}  // namespace xahau
