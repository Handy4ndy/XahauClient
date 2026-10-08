// phase4_extra.h - OracleSet (Xahau PriceOracle). EncodeOnly XRPL-port families
// (DID/Credential/AMM/MPT/Domain/Delegate/Batch/Vault/XChain) removed from library
// source 2026-09-28; see docs/ENCODE_ONLY_STUBS.md and harness bak archive.
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

// Oracle
#ifndef XAHAU_ORACLE_MAX_PAIRS
#define XAHAU_ORACLE_MAX_PAIRS 10
#endif
/** One on-ledger price pair (base/quote currencies + scaled integer price). */
struct OraclePricePair {
  char baseAsset[16];
  char quoteAsset[16];
  uint64_t assetPrice;
  uint8_t scale;
  bool hasPrice;
  OraclePricePair() : assetPrice(0), scale(0), hasPrice(false) {
    baseAsset[0] = quoteAsset[0] = '\0';
  }
};

struct OracleSetParams {
  TxCommon common;
  uint32_t oracleDocumentID;
  uint32_t lastUpdateTime;
  // Single-pair convenience (used when pairCount == 0)
  char baseAsset[16];
  char quoteAsset[16];
  uint64_t assetPrice;
  uint8_t scale;
  bool hasPrice;
  // Multi-pair (1..XAHAU_ORACLE_MAX_PAIRS). If pairCount > 0, overrides single fields.
  OraclePricePair pairs[XAHAU_ORACLE_MAX_PAIRS];
  uint8_t pairCount;
  // Required on create (max 256 ASCII / 16 ASCII for class). Optional on update.
  char provider[64];
  char assetClass[32];
  // Optional URI - UTF-8 string; on wire VL blob, max 256 bytes.
  char uri[256];
  OracleSetParams()
      : oracleDocumentID(0),
        lastUpdateTime(0),
        assetPrice(0),
        scale(0),
        hasPrice(false),
        pairCount(0) {
    baseAsset[0] = quoteAsset[0] = provider[0] = assetClass[0] = uri[0] = '\0';
  }
};

Status fillOracleSet(const OracleSetParams&, const uint8_t*, const uint8_t*, TxFieldBag&);


}  // namespace xahau
