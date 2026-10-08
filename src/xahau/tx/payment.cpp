// payment.cpp
#include "payment.h"

#include "../codec/st_helpers.h"
#include "../crypto/keys.h"
#include "../sign/sign.h"

#include <string.h>

namespace xahau {

Status fillPayment(const PaymentParams& params, const uint8_t accountId[20],
                   const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  if (!params.hasDestinationId && params.destination[0] == '\0') {
    return Status::MissingField;
  }
  if (params.amount.kind == AmountKind::None) return Status::MissingField;

  bag.clear();

  Status st = bagSetCommon(bag, TxType::Payment, params.common, accountId, pubKey, nullptr, 0,
                           false);
  if (st != Status::Ok) return st;

  if (params.amount.kind == AmountKind::XrpDrops) {
    st = bag.setAmountXrp(fc::Amount, params.amount.drops);
  } else if (params.amount.kind == AmountKind::Issued) {
    uint8_t issuer[20];
    st = decodeClassicAddress(params.amount.issued.issuer, issuer);
    if (st != Status::Ok) return st;
    st = bag.setAmountIssued(fc::Amount, params.amount.issued.value, params.amount.issued.currency,
                             issuer);
  } else if (params.amount.kind == AmountKind::Mpt) {
    st = bag.setAmountMpt(fc::Amount, params.amount.mpt.value, params.amount.mpt.issuanceId);
  } else {
    return Status::InvalidField;
  }
  if (st != Status::Ok) return st;

  if (params.sendMax.kind == AmountKind::XrpDrops) {
    st = bag.setAmountXrp(fc::SendMax, params.sendMax.drops);
    if (st != Status::Ok) return st;
  } else if (params.sendMax.kind == AmountKind::Issued) {
    uint8_t issuer[20];
    st = decodeClassicAddress(params.sendMax.issued.issuer, issuer);
    if (st != Status::Ok) return st;
    st = bag.setAmountIssued(fc::SendMax, params.sendMax.issued.value,
                             params.sendMax.issued.currency, issuer);
    if (st != Status::Ok) return st;
  }

  uint8_t destId[20];
  if (params.hasDestinationId) {
    memcpy(destId, params.destinationId, 20);
  } else {
    st = decodeClassicAddress(params.destination, destId);
    if (st != Status::Ok) return st;
  }
  st = bag.setAccountID(fc::Destination, destId);
  if (st != Status::Ok) return st;

  if (params.memoData[0] != '\0') {
    uint8_t memos[280];
    size_t memosLen = 0;
    st = buildMemosArray(params.memoData, memos, sizeof(memos), memosLen);
    if (st != Status::Ok) return st;
    st = bag.setEmbedded(fc::Memos, memos, static_cast<uint16_t>(memosLen));
    if (st != Status::Ok) return st;
  }

  if (params.hookName[0] != '\0') {
    st = bag.setBlob(fc::HookName, reinterpret_cast<const uint8_t*>(params.hookName),
                     static_cast<uint16_t>(strlen(params.hookName)));
    if (st != Status::Ok) return st;
  }
  if (params.hookParameters && params.hookParametersLen > 0) {
    st = bag.setEmbedded(fc::HookParameters, params.hookParameters, params.hookParametersLen);
    if (st != Status::Ok) return st;
  }

  return Status::Ok;
}

Status buildAndSignPayment(const KeyPair& keys, const PaymentParams& params, char* outHex,
                           size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillPayment(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

}  // namespace xahau
