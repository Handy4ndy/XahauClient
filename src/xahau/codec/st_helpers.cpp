// st_helpers.cpp
#include "st_helpers.h"

#include "binary_write.h"
#include "st_number.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace xahau {

void encodeCurrency20(const char* currency, uint8_t out[20]) {
  memset(out, 0, 20);
  if (!currency || !currency[0] || strcmp(currency, "XRP") == 0) return;
  size_t len = strlen(currency);
  if (len == 3) {
    out[12] = (uint8_t)currency[0];
    out[13] = (uint8_t)currency[1];
    out[14] = (uint8_t)currency[2];
    return;
  }
  if (len == 40) {
    for (int i = 0; i < 20; ++i) {
      char hb[3] = {currency[i * 2], currency[i * 2 + 1], 0};
      out[i] = (uint8_t)strtoul(hb, nullptr, 16);
    }
  }
}

Status buildMemosArray(const char* memoUtf8, uint8_t* out, size_t outCap, size_t& outLen) {
  outLen = 0;
  if (!memoUtf8 || !memoUtf8[0] || !out) return Status::InvalidArgument;
  size_t mlen = strlen(memoUtf8);
  if (mlen > 220) return Status::InvalidField;
  size_t pos = 0;
  Status st = writeByte(out, pos, outCap, 0xEA);
  if (st != Status::Ok) return st;
  st = writeByte(out, pos, outCap, 0x7D);
  if (st != Status::Ok) return st;
  st = writeVLLength(out, pos, outCap, mlen);
  if (st != Status::Ok) return st;
  st = writeBytes(out, pos, outCap, reinterpret_cast<const uint8_t*>(memoUtf8), mlen);
  if (st != Status::Ok) return st;
  st = writeByte(out, pos, outCap, 0xE1);
  if (st != Status::Ok) return st;
  st = writeByte(out, pos, outCap, 0xF1);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

Status buildSignerEntriesArray(const SignerEntryWire* entries, uint8_t count, uint8_t* out,
                               size_t outCap, size_t& outLen) {
  outLen = 0;
  if (!entries || count == 0 || !out) return Status::InvalidArgument;
  size_t pos = 0;
  for (uint8_t i = 0; i < count; ++i) {
    if (entries[i].weight == 0) return Status::InvalidField;
    Status st = writeByte(out, pos, outCap, 0xEB);
    if (st != Status::Ok) return st;
    st = writeFieldHeader(out, pos, outCap, 1, 3);
    if (st != Status::Ok) return st;
    st = writeUInt16BE(out, pos, outCap, entries[i].weight);
    if (st != Status::Ok) return st;
    st = writeFieldHeader(out, pos, outCap, 8, 1);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, 20);
    if (st != Status::Ok) return st;
    st = writeBytes(out, pos, outCap, entries[i].accountId, 20);
    if (st != Status::Ok) return st;
    st = writeByte(out, pos, outCap, 0xE1);
    if (st != Status::Ok) return st;
    if (i + 1 == count) {
      st = writeByte(out, pos, outCap, 0xF1);
      if (st != Status::Ok) return st;
    }
  }
  outLen = pos;
  return Status::Ok;
}


static Status appendOraclePriceDataObject(const char* baseAsset, const char* quoteAsset,
                                          uint64_t assetPrice, uint8_t scale, bool hasPrice,
                                          bool lastObject, uint8_t* out, size_t& pos,
                                          size_t outCap) {
  uint8_t base[20], quote[20];
  encodeCurrency20(baseAsset, base);
  encodeCurrency20(quoteAsset, quote);

  // PriceData STObject header type 14 nth 32: type < 16, field >= 16 -> 0xE0, 0x20
  Status st = writeByte(out, pos, outCap, 0xE0);
  if (st != Status::Ok) return st;
  st = writeByte(out, pos, outCap, 0x20);
  if (st != Status::Ok) return st;

  if (hasPrice) {
    // AssetPrice UInt64 type 3 nth 23 -> 0x30, 0x17
    st = writeFieldHeader(out, pos, outCap, 3, 23);
    if (st != Status::Ok) return st;
    st = writeUInt64BE(out, pos, outCap, assetPrice);
    if (st != Status::Ok) return st;
    // Scale UInt8 type 16 nth 4. Omit when 0 (default).
    if (scale > 0) {
      st = writeFieldHeader(out, pos, outCap, 16, 4);
      if (st != Status::Ok) return st;
      st = writeByte(out, pos, outCap, scale);
      if (st != Status::Ok) return st;
    }
  }
  // Currency type 26 nth 1 and 2
  st = writeFieldHeader(out, pos, outCap, 26, 1);
  if (st != Status::Ok) return st;
  st = writeBytes(out, pos, outCap, base, 20);
  if (st != Status::Ok) return st;
  st = writeFieldHeader(out, pos, outCap, 26, 2);
  if (st != Status::Ok) return st;
  st = writeBytes(out, pos, outCap, quote, 20);
  if (st != Status::Ok) return st;
  st = writeByte(out, pos, outCap, 0xE1);
  if (st != Status::Ok) return st;
  if (lastObject) {
    st = writeByte(out, pos, outCap, 0xF1);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status buildOraclePriceDataSeries(const char* baseAsset, const char* quoteAsset,
                                  uint64_t assetPrice, uint8_t scale, bool hasPrice,
                                  uint8_t* out, size_t outCap, size_t& outLen) {
  outLen = 0;
  if (!out) return Status::InvalidArgument;
  size_t pos = 0;
  Status st = appendOraclePriceDataObject(baseAsset, quoteAsset, assetPrice, scale, hasPrice, true,
                                          out, pos, outCap);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

Status buildOraclePriceDataSeriesN(const OraclePricePairWire* pairs, uint8_t count, uint8_t* out,
                                   size_t outCap, size_t& outLen) {
  outLen = 0;
  if (!out || !pairs || count == 0 || count > 10) return Status::InvalidArgument;
  size_t pos = 0;
  for (uint8_t i = 0; i < count; ++i) {
    const OraclePricePairWire& p = pairs[i];
    if (!p.baseAsset || !p.quoteAsset) return Status::InvalidArgument;
    Status st = appendOraclePriceDataObject(p.baseAsset, p.quoteAsset, p.assetPrice, p.scale,
                                            p.hasPrice, (i + 1 == count), out, pos, outCap);
    if (st != Status::Ok) return st;
  }
  outLen = pos;
  return Status::Ok;
}







Status buildHookParameters(const HookParamWire* params, uint8_t count, uint8_t* out, size_t outCap,
                           size_t& outLen) {
  outLen = 0;
  if (!out || !params || count == 0 || count > 16) return Status::InvalidArgument;
  size_t pos = 0;
  for (uint8_t i = 0; i < count; ++i) {
    const HookParamWire& p = params[i];
    if (!p.name || p.nameLen == 0 || p.nameLen > 32) return Status::InvalidField;
    if (p.valueLen > 256) return Status::InvalidField;
    if (p.valueLen > 0 && !p.value) return Status::InvalidField;
    // HookParameter STObject type 14 nth 23 -> 0xE0, 0x17
    Status st = writeFieldHeader(out, pos, outCap, 14, 23);
    if (st != Status::Ok) return st;
    // HookParameterName Blob type 7 nth 24
    st = writeFieldHeader(out, pos, outCap, 7, 24);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, p.nameLen);
    if (st != Status::Ok) return st;
    st = writeBytes(out, pos, outCap, p.name, p.nameLen);
    if (st != Status::Ok) return st;
    // HookParameterValue Blob type 7 nth 25 (omit when empty? protocol allows empty VL)
    st = writeFieldHeader(out, pos, outCap, 7, 25);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, p.valueLen);
    if (st != Status::Ok) return st;
    if (p.valueLen > 0) {
      st = writeBytes(out, pos, outCap, p.value, p.valueLen);
      if (st != Status::Ok) return st;
    }
    st = writeByte(out, pos, outCap, 0xE1);
    if (st != Status::Ok) return st;
  }
  Status st = writeByte(out, pos, outCap, 0xF1);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

}  // namespace xahau
