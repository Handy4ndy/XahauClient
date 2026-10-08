// trust_set.h - TrustSet builder (fills TxFieldBag only)
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

struct TrustSetParams {
  TxCommon common;

  /** Issued currency limit (required). value "0" clears the line. */
  IssuedAmount limit;

  bool hasQualityIn;
  uint32_t qualityIn;
  bool hasQualityOut;
  uint32_t qualityOut;

  TrustSetParams()
      : hasQualityIn(false), qualityIn(0), hasQualityOut(false), qualityOut(0) {
    limit.value[0] = '\0';
    limit.currency[0] = '\0';
    limit.issuer[0] = '\0';
  }
};

Status fillTrustSet(const TrustSetParams& params, const uint8_t accountId[20],
                    const uint8_t pubKey[33], TxFieldBag& bag);

Status buildAndSignTrustSet(const KeyPair& keys, const TrustSetParams& params, char* outHex,
                            size_t outHexCap);

}  // namespace xahau
