// payment.h - Payment builder (fills TxFieldBag only)
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

struct PaymentParams {
  TxCommon common;
  char destination[36];
  bool hasDestinationId;
  uint8_t destinationId[20];
  Amount amount;
  Amount sendMax;  // optional (kind None = omit)
  /** Optional UTF-8 memo (MemoData only). Empty = omit. */
  char memoData[256];
  /** Optional HookName blob. */
  char hookName[64];
  /** Prebuilt HookParameters STArray body (buildHookParameters); nullptr = omit. */
  const uint8_t* hookParameters;
  uint16_t hookParametersLen;

  PaymentParams()
      : hasDestinationId(false), hookParameters(nullptr), hookParametersLen(0) {
    destination[0] = memoData[0] = hookName[0] = '\0';
    secureZero(destinationId, sizeof(destinationId));
  }
};

Status fillPayment(const PaymentParams& params, const uint8_t accountId[20],
                   const uint8_t pubKey[33], TxFieldBag& bag);

Status buildAndSignPayment(const KeyPair& keys, const PaymentParams& params, char* outHex,
                           size_t outHexCap);

}  // namespace xahau
