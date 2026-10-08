// trust_set.cpp
#include "trust_set.h"

#include "../crypto/base58.h"
#include "../sign/sign.h"

#include <string.h>

namespace xahau {

Status fillTrustSet(const TrustSetParams& params, const uint8_t accountId[20],
                    const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  if (params.limit.value[0] == '\0' || params.limit.currency[0] == '\0' ||
      params.limit.issuer[0] == '\0') {
    return Status::MissingField;
  }

  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::TrustSet, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;

  uint8_t issuer[20];
  st = decodeClassicAddress(params.limit.issuer, issuer);
  if (st != Status::Ok) return st;

  st = bag.setAmountIssued(fc::LimitAmount, params.limit.value, params.limit.currency, issuer);
  if (st != Status::Ok) return st;

  if (params.hasQualityIn) {
    st = bag.setUInt32(fc::QualityIn, params.qualityIn);
    if (st != Status::Ok) return st;
  }
  if (params.hasQualityOut) {
    st = bag.setUInt32(fc::QualityOut, params.qualityOut);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status buildAndSignTrustSet(const KeyPair& keys, const TrustSetParams& params, char* outHex,
                            size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillTrustSet(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

}  // namespace xahau
