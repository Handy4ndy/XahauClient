// st_helpers.h - nested STArray / Issue / Oracle helpers
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

Status buildMemosArray(const char* memoUtf8, uint8_t* out, size_t outCap, size_t& outLen);

struct SignerEntryWire {
  uint8_t accountId[20];
  uint16_t weight;
};

Status buildSignerEntriesArray(const SignerEntryWire* entries, uint8_t count, uint8_t* out,
                               size_t outCap, size_t& outLen);

/** Encode 20-byte currency (3-char ASCII, 40-hex, or "XRP"/empty -> zeros). */
void encodeCurrency20(const char* currency, uint8_t out[20]);

/**
 * Oracle PriceDataSeries body (no outer F018 header - setEmbedded adds it):
 * one PriceData object for BaseAsset/QuoteAsset + optional AssetPrice/Scale.
 */
Status buildOraclePriceDataSeries(const char* baseAsset, const char* quoteAsset,
                                  uint64_t assetPrice, uint8_t scale, bool hasPrice,
                                  uint8_t* out, size_t outCap, size_t& outLen);

/** One asset/quote pair for multi-entry OracleSet (max 10 on-ledger). */
struct OraclePricePairWire {
  const char* baseAsset;
  const char* quoteAsset;
  uint64_t assetPrice;
  uint8_t scale;
  bool hasPrice;
};

/**
 * Oracle PriceDataSeries with N pairs (1..10).
 * Each object: PriceData header + optional AssetPrice/Scale + BaseAsset + QuoteAsset.
 */
Status buildOraclePriceDataSeriesN(const OraclePricePairWire* pairs, uint8_t count, uint8_t* out,
                                   size_t outCap, size_t& outLen);

/**
 * One HookParameter for Invoke / SetHook HookParameters STArray.
 * Protocol caps: name <= 32 bytes, value <= 256 bytes, <= 16 params.
 * Name/value are raw bytes (ASCII or binary); not hex-decoded here.
 */
struct HookParamWire {
  const uint8_t* name;
  uint8_t nameLen;   // 1..32
  const uint8_t* value;
  uint16_t valueLen; // 0..256
};

/**
 * Canonical HookParameters STArray body (no outer field header - setEmbedded adds it):
 *   [ HookParameter{ Name, Value } ... ] F1
 * Object header: type14 nth23 (0xE0 0x17); Name 7/24; Value 7/25.
 * Max count 16. Empty count returns InvalidArgument.
 */
Status buildHookParameters(const HookParamWire* params, uint8_t count, uint8_t* out, size_t outCap,
                           size_t& outLen);

}  // namespace xahau
