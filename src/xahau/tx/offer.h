// offer.h - OfferCreate / OfferCancel
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

struct OfferCreateParams {
  TxCommon common;
  Amount takerPays;
  Amount takerGets;
  uint32_t expiration;      // 0 = omit
  uint32_t offerSequence;   // 0 = omit (OfferSequence for replace)

  OfferCreateParams() : expiration(0), offerSequence(0) {}
};

struct OfferCancelParams {
  TxCommon common;
  uint32_t offerSequence;

  OfferCancelParams() : offerSequence(0) {}
};

Status fillOfferCreate(const OfferCreateParams& params, const uint8_t accountId[20],
                       const uint8_t pubKey[33], TxFieldBag& bag);
Status fillOfferCancel(const OfferCancelParams& params, const uint8_t accountId[20],
                       const uint8_t pubKey[33], TxFieldBag& bag);

Status buildAndSignOfferCreate(const KeyPair& keys, const OfferCreateParams& params, char* outHex,
                               size_t outHexCap);
Status buildAndSignOfferCancel(const KeyPair& keys, const OfferCancelParams& params, char* outHex,
                               size_t outHexCap);

}  // namespace xahau
