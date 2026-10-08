// phase2_extra.cpp - bulk Phase 2 transaction fillers
#include "phase2_extra.h"

#include "../crypto/base58.h"
#include "../sign/sign.h"

#include <string.h>

namespace xahau {

static Status putAmt(TxFieldBag& bag, uint32_t code, const Amount& a) {
  if (a.kind == AmountKind::XrpDrops) return bag.setAmountXrp(code, a.drops);
  if (a.kind == AmountKind::Issued) {
    uint8_t iss[20];
    Status st = decodeClassicAddress(a.issued.issuer, iss);
    if (st != Status::Ok) return st;
    return bag.setAmountIssued(code, a.issued.value, a.issued.currency, iss);
  }
  if (a.kind == AmountKind::Mpt) {
    return bag.setAmountMpt(code, a.mpt.value, a.mpt.issuanceId);
  }
  return Status::MissingField;
}

static Status putAddr(TxFieldBag& bag, uint32_t code, const char* r) {
  uint8_t id[20];
  Status st = decodeClassicAddress(r, id);
  if (st != Status::Ok) return st;
  return bag.setAccountID(code, id);
}

#define BEGIN_TX(tt, params)                                                                  \
  if (!acc || !pub) return Status::InvalidArgument;                                           \
  bag.clear();                                                                                \
  Status st = bagSetCommon(bag, tt, (params).common, acc, pub, nullptr, 0, false);            \
  if (st != Status::Ok) return st

#define SIGN_HELPER(Name, Params, Fill)                                                       \
  Status buildAndSign##Name(const KeyPair& keys, const Params& p, char* out, size_t cap) {    \
    TxFieldBag& bag = scratchSignBag();                                                     \
    Status st = fill##Name(p, keys.accountId, keys.publicKey, bag);                           \
    if (st != Status::Ok) return st;                                                          \
    return signTx(bag, keys, out, cap);                                                       \
  }

// ---- Escrow ----
Status fillEscrowCreate(const EscrowCreateParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  BEGIN_TX(TxType::EscrowCreate, p);
  if (p.finishAfter) {
    st = bag.setUInt32(fc::FinishAfter, p.finishAfter);
    if (st != Status::Ok) return st;
  }
  if (p.cancelAfter) {
    st = bag.setUInt32(fc::CancelAfter, p.cancelAfter);
    if (st != Status::Ok) return st;
  }
  if (p.common.destinationTag) { /* already in bagSetCommon if set on common */ }
  st = putAmt(bag, fc::Amount, p.amount);
  if (st != Status::Ok) return st;
  return putAddr(bag, fc::Destination, p.destination);
}

Status fillEscrowFinish(const EscrowFinishParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  BEGIN_TX(TxType::EscrowFinish, p);
  st = bag.setUInt32(fc::OfferSequence, p.offerSequence);
  if (st != Status::Ok) return st;
  st = putAddr(bag, fc::Owner, p.owner);
  if (st != Status::Ok) return st;
  // Crypto-condition finish: both Condition and Fulfillment (optional for time-based).
  if (p.conditionLen > 0) {
    if (p.conditionLen > sizeof(p.condition)) return Status::InvalidField;
    st = bag.setBlob(fc::Condition, p.condition, p.conditionLen);
    if (st != Status::Ok) return st;
  }
  if (p.fulfillmentLen > 0) {
    if (p.fulfillmentLen > sizeof(p.fulfillment)) return Status::InvalidField;
    st = bag.setBlob(fc::Fulfillment, p.fulfillment, p.fulfillmentLen);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillEscrowCancel(const EscrowCancelParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  BEGIN_TX(TxType::EscrowCancel, p);
  st = bag.setUInt32(fc::OfferSequence, p.offerSequence);
  if (st != Status::Ok) return st;
  return putAddr(bag, fc::Owner, p.owner);
}

// ---- Check ----
Status fillCheckCreate(const CheckCreateParams& p, const uint8_t* acc, const uint8_t* pub,
                       TxFieldBag& bag) {
  BEGIN_TX(TxType::CheckCreate, p);
  if (p.expiration) {
    st = bag.setUInt32(fc::Expiration, p.expiration);
    if (st != Status::Ok) return st;
  }
  st = putAmt(bag, fc::SendMax, p.sendMax);
  if (st != Status::Ok) return st;
  return putAddr(bag, fc::Destination, p.destination);
}

Status fillCheckCash(const CheckCashParams& p, const uint8_t* acc, const uint8_t* pub,
                     TxFieldBag& bag) {
  BEGIN_TX(TxType::CheckCash, p);
  st = bag.setHash256(fc::CheckID, p.checkId);
  if (st != Status::Ok) return st;
  return putAmt(bag, fc::Amount, p.amount);
}

Status fillCheckCancel(const CheckCancelParams& p, const uint8_t* acc, const uint8_t* pub,
                       TxFieldBag& bag) {
  BEGIN_TX(TxType::CheckCancel, p);
  return bag.setHash256(fc::CheckID, p.checkId);
}

// ---- PayChan ----
Status fillPaymentChannelCreate(const PaymentChannelCreateParams& p, const uint8_t* acc,
                                const uint8_t* pub, TxFieldBag& bag) {
  BEGIN_TX(TxType::PaymentChannelCreate, p);
  st = bag.setUInt32(fc::SettleDelay, p.settleDelay);
  if (st != Status::Ok) return st;
  if (p.cancelAfter) {
    st = bag.setUInt32(fc::CancelAfter, p.cancelAfter);
    if (st != Status::Ok) return st;
  }
  st = putAmt(bag, fc::Amount, p.amount);
  if (st != Status::Ok) return st;
  if (!p.publicKeyLen) return Status::MissingField;
  st = bag.setBlob(fc::PublicKey, p.publicKey, p.publicKeyLen);
  if (st != Status::Ok) return st;
  return putAddr(bag, fc::Destination, p.destination);
}

Status fillPaymentChannelFund(const PaymentChannelFundParams& p, const uint8_t* acc,
                              const uint8_t* pub, TxFieldBag& bag) {
  BEGIN_TX(TxType::PaymentChannelFund, p);
  st = bag.setHash256(fc::Channel, p.channel);
  if (st != Status::Ok) return st;
  if (p.expiration) {
    st = bag.setUInt32(fc::Expiration, p.expiration);
    if (st != Status::Ok) return st;
  }
  return putAmt(bag, fc::Amount, p.amount);
}

Status fillPaymentChannelClaim(const PaymentChannelClaimParams& p, const uint8_t* acc,
                               const uint8_t* pub, TxFieldBag& bag) {
  BEGIN_TX(TxType::PaymentChannelClaim, p);
  st = bag.setHash256(fc::Channel, p.channel);
  if (st != Status::Ok) return st;
  if (p.amount.kind != AmountKind::None) {
    st = putAmt(bag, fc::Amount, p.amount);
    if (st != Status::Ok) return st;
  }
  if (p.balance.kind != AmountKind::None) {
    st = putAmt(bag, fc::Balance, p.balance);
    if (st != Status::Ok) return st;
  }
  if (p.publicKeyLen) {
    st = bag.setBlob(fc::PublicKey, p.publicKey, p.publicKeyLen);
    if (st != Status::Ok) return st;
  }
  if (p.signatureLen) {
    st = bag.setBlob(fc::Signature, p.signature, p.signatureLen);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

// ---- Misc ----
Status fillDepositPreauth(const DepositPreauthParams& p, const uint8_t* acc, const uint8_t* pub,
                          TxFieldBag& bag) {
  BEGIN_TX(TxType::DepositPreauth, p);
  return putAddr(bag, fc::Authorize, p.authorize);
}

Status fillAccountDelete(const AccountDeleteParams& p, const uint8_t* acc, const uint8_t* pub,
                         TxFieldBag& bag) {
  BEGIN_TX(TxType::AccountDelete, p);
  return putAddr(bag, fc::Destination, p.destination);
}

Status fillClawback(const ClawbackParams& p, const uint8_t* acc, const uint8_t* pub,
                    TxFieldBag& bag) {
  BEGIN_TX(TxType::Clawback, p);
  // MPT clawback requires Holder; trust-line clawback must omit Holder.
  if (p.amount.kind == AmountKind::Mpt) {
    if (!p.holder[0]) return Status::MissingField;
    st = putAddr(bag, fc::Holder, p.holder);
    if (st != Status::Ok) return st;
  } else if (p.holder[0]) {
    // Optional Holder only valid for MPT; ignore for IOU path if empty, reject if set with IOU?
    // Protocol: must omit Holder for trust lines - only set when non-empty and MPT.
  }
  return putAmt(bag, fc::Amount, p.amount);
}

Status fillOracleDelete(const OracleDeleteParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  BEGIN_TX(TxType::OracleDelete, p);
  return bag.setUInt32(fc::OracleDocumentID, p.oracleDocumentID);
}

SIGN_HELPER(EscrowCreate, EscrowCreateParams, fillEscrowCreate)
SIGN_HELPER(EscrowFinish, EscrowFinishParams, fillEscrowFinish)
SIGN_HELPER(EscrowCancel, EscrowCancelParams, fillEscrowCancel)
SIGN_HELPER(CheckCreate, CheckCreateParams, fillCheckCreate)
SIGN_HELPER(CheckCash, CheckCashParams, fillCheckCash)
SIGN_HELPER(CheckCancel, CheckCancelParams, fillCheckCancel)
SIGN_HELPER(PaymentChannelCreate, PaymentChannelCreateParams, fillPaymentChannelCreate)
SIGN_HELPER(PaymentChannelFund, PaymentChannelFundParams, fillPaymentChannelFund)
SIGN_HELPER(PaymentChannelClaim, PaymentChannelClaimParams, fillPaymentChannelClaim)
SIGN_HELPER(DepositPreauth, DepositPreauthParams, fillDepositPreauth)
SIGN_HELPER(AccountDelete, AccountDeleteParams, fillAccountDelete)
SIGN_HELPER(Clawback, ClawbackParams, fillClawback)
SIGN_HELPER(OracleDelete, OracleDeleteParams, fillOracleDelete)

}  // namespace xahau

