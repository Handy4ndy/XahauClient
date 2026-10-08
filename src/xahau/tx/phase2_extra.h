// phase2_extra.h - Escrow, Check, PayChan, misc fills (Phase 2; NFToken encode stubs removed 2026-09-28)
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

// ---- Escrow ----
struct EscrowCreateParams {
  TxCommon common;
  char destination[36];
  Amount amount;
  uint32_t finishAfter;  // 0 omit
  uint32_t cancelAfter;  // 0 omit
  uint32_t destinationTag;
  EscrowCreateParams() : finishAfter(0), cancelAfter(0), destinationTag(0) {
    destination[0] = '\0';
  }
};
struct EscrowFinishParams {
  TxCommon common;
  char owner[36];
  uint32_t offerSequence;
  /** Optional crypto-condition PREIMAGE-SHA-256 (raw condition bytes, max 128). */
  uint8_t condition[128];
  uint16_t conditionLen;
  /** Optional fulfillment matching Condition (raw bytes, max 128). */
  uint8_t fulfillment[128];
  uint16_t fulfillmentLen;
  EscrowFinishParams() : offerSequence(0), conditionLen(0), fulfillmentLen(0) {
    owner[0] = '\0';
    secureZero(condition, sizeof(condition));
    secureZero(fulfillment, sizeof(fulfillment));
  }
};
struct EscrowCancelParams {
  TxCommon common;
  char owner[36];
  uint32_t offerSequence;
  EscrowCancelParams() : offerSequence(0) { owner[0] = '\0'; }
};

// ---- Check ----
struct CheckCreateParams {
  TxCommon common;
  char destination[36];
  Amount sendMax;
  uint32_t expiration;  // 0 omit
  CheckCreateParams() : expiration(0) { destination[0] = '\0'; }
};
struct CheckCashParams {
  TxCommon common;
  uint8_t checkId[32];
  Amount amount;  // or deliverMin later
  CheckCashParams() { secureZero(checkId, 32); }
};
struct CheckCancelParams {
  TxCommon common;
  uint8_t checkId[32];
  CheckCancelParams() { secureZero(checkId, 32); }
};

// ---- PaymentChannel ----
struct PaymentChannelCreateParams {
  TxCommon common;
  char destination[36];
  Amount amount;
  uint32_t settleDelay;
  uint8_t publicKey[33];
  uint16_t publicKeyLen;
  uint32_t cancelAfter;  // 0 omit
  PaymentChannelCreateParams() : settleDelay(0), publicKeyLen(0), cancelAfter(0) {
    destination[0] = '\0';
    secureZero(publicKey, 33);
  }
};
struct PaymentChannelFundParams {
  TxCommon common;
  uint8_t channel[32];
  Amount amount;
  uint32_t expiration;  // 0 omit
  PaymentChannelFundParams() : expiration(0) { secureZero(channel, 32); }
};
struct PaymentChannelClaimParams {
  TxCommon common;
  uint8_t channel[32];
  Amount amount;   // optional
  Amount balance;  // optional
  uint8_t publicKey[33];
  uint16_t publicKeyLen;
  uint8_t signature[80];
  uint16_t signatureLen;
  PaymentChannelClaimParams() : publicKeyLen(0), signatureLen(0) {
    secureZero(channel, 32);
    secureZero(publicKey, 33);
    secureZero(signature, 80);
  }
};

// ---- Misc ----
struct DepositPreauthParams {
  TxCommon common;
  char authorize[36];  // or unauthorize later
  DepositPreauthParams() { authorize[0] = '\0'; }
};
struct AccountDeleteParams {
  TxCommon common;
  char destination[36];
  AccountDeleteParams() { destination[0] = '\0'; }
};
/**
 * Clawback - trust-line IOU or MPT.
 * IOU: Amount = Issued (issuer field = holder), omit Holder.
 * MPT: Amount = Mpt (issuance + value), Holder required; create needs tfMPTCanClawback.
 */
struct ClawbackParams {
  TxCommon common;
  Amount amount;
  char holder[36];  // required for MPT; omit for trust-line clawback
  ClawbackParams() { holder[0] = '\0'; }
};
struct OracleDeleteParams {
  TxCommon common;
  uint32_t oracleDocumentID;
  OracleDeleteParams() : oracleDocumentID(0) {}
};

Status fillEscrowCreate(const EscrowCreateParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);
Status fillEscrowFinish(const EscrowFinishParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);
Status fillEscrowCancel(const EscrowCancelParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);
Status fillCheckCreate(const CheckCreateParams&, const uint8_t acc[20], const uint8_t pub[33],
                       TxFieldBag&);
Status fillCheckCash(const CheckCashParams&, const uint8_t acc[20], const uint8_t pub[33],
                     TxFieldBag&);
Status fillCheckCancel(const CheckCancelParams&, const uint8_t acc[20], const uint8_t pub[33],
                       TxFieldBag&);
Status fillPaymentChannelCreate(const PaymentChannelCreateParams&, const uint8_t acc[20],
                                const uint8_t pub[33], TxFieldBag&);
Status fillPaymentChannelFund(const PaymentChannelFundParams&, const uint8_t acc[20],
                              const uint8_t pub[33], TxFieldBag&);
Status fillPaymentChannelClaim(const PaymentChannelClaimParams&, const uint8_t acc[20],
                               const uint8_t pub[33], TxFieldBag&);
Status fillDepositPreauth(const DepositPreauthParams&, const uint8_t acc[20], const uint8_t pub[33],
                          TxFieldBag&);
Status fillAccountDelete(const AccountDeleteParams&, const uint8_t acc[20], const uint8_t pub[33],
                         TxFieldBag&);
Status fillClawback(const ClawbackParams&, const uint8_t acc[20], const uint8_t pub[33],
                    TxFieldBag&);
Status fillOracleDelete(const OracleDeleteParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);

// Convenience sign helpers
Status buildAndSignEscrowCreate(const KeyPair&, const EscrowCreateParams&, char*, size_t);
Status buildAndSignEscrowFinish(const KeyPair&, const EscrowFinishParams&, char*, size_t);
Status buildAndSignEscrowCancel(const KeyPair&, const EscrowCancelParams&, char*, size_t);
Status buildAndSignCheckCreate(const KeyPair&, const CheckCreateParams&, char*, size_t);
Status buildAndSignCheckCash(const KeyPair&, const CheckCashParams&, char*, size_t);
Status buildAndSignCheckCancel(const KeyPair&, const CheckCancelParams&, char*, size_t);
Status buildAndSignPaymentChannelCreate(const KeyPair&, const PaymentChannelCreateParams&, char*,
                                        size_t);
Status buildAndSignPaymentChannelFund(const KeyPair&, const PaymentChannelFundParams&, char*,
                                      size_t);
Status buildAndSignPaymentChannelClaim(const KeyPair&, const PaymentChannelClaimParams&, char*,
                                       size_t);
Status buildAndSignDepositPreauth(const KeyPair&, const DepositPreauthParams&, char*, size_t);
Status buildAndSignAccountDelete(const KeyPair&, const AccountDeleteParams&, char*, size_t);
Status buildAndSignClawback(const KeyPair&, const ClawbackParams&, char*, size_t);
Status buildAndSignOracleDelete(const KeyPair&, const OracleDeleteParams&, char*, size_t);

}  // namespace xahau

