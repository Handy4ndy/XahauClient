// offer.cpp
#include "offer.h"

#include "../crypto/base58.h"
#include "../sign/sign.h"

namespace xahau {

static Status setAmountField(TxFieldBag& bag, uint32_t code, const Amount& a) {
  if (a.kind == AmountKind::XrpDrops) {
    return bag.setAmountXrp(code, a.drops);
  }
  if (a.kind == AmountKind::Issued) {
    uint8_t issuer[20];
    Status st = decodeClassicAddress(a.issued.issuer, issuer);
    if (st != Status::Ok) return st;
    return bag.setAmountIssued(code, a.issued.value, a.issued.currency, issuer);
  }
  return Status::MissingField;
}

Status fillOfferCreate(const OfferCreateParams& params, const uint8_t accountId[20],
                       const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::OfferCreate, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;
  if (params.expiration != 0) {
    st = bag.setUInt32(fc::Expiration, params.expiration);
    if (st != Status::Ok) return st;
  }
  if (params.offerSequence != 0) {
    st = bag.setUInt32(fc::OfferSequence, params.offerSequence);
    if (st != Status::Ok) return st;
  }
  st = setAmountField(bag, fc::TakerPays, params.takerPays);
  if (st != Status::Ok) return st;
  st = setAmountField(bag, fc::TakerGets, params.takerGets);
  if (st != Status::Ok) return st;
  return Status::Ok;
}

Status fillOfferCancel(const OfferCancelParams& params, const uint8_t accountId[20],
                       const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  if (params.offerSequence == 0) return Status::MissingField;
  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::OfferCancel, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;
  return bag.setUInt32(fc::OfferSequence, params.offerSequence);
}

Status buildAndSignOfferCreate(const KeyPair& keys, const OfferCreateParams& params, char* outHex,
                               size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillOfferCreate(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

Status buildAndSignOfferCancel(const KeyPair& keys, const OfferCancelParams& params, char* outHex,
                               size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillOfferCancel(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

}  // namespace xahau
