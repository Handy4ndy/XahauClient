// account_set.h - AccountSet builder (fills TxFieldBag only)
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

struct AccountSetParams {
  TxCommon common;

  bool hasSetFlag;
  uint32_t setFlag;
  bool hasClearFlag;
  uint32_t clearFlag;

  /** Optional domain as raw ASCII (encoded as Blob). Empty = omit. */
  char domain[256];
  /** Optional transfer rate (0 = omit; 1000000000 = 0% fee). */
  uint32_t transferRate;
  /** Optional tick size 0-15; 0 = omit unless forceTickSize. */
  uint8_t tickSize;
  bool forceTickSize;

  AccountSetParams()
      : hasSetFlag(false),
        setFlag(0),
        hasClearFlag(false),
        clearFlag(0),
        transferRate(0),
        tickSize(0),
        forceTickSize(false) {
    domain[0] = '\0';
  }
};

Status fillAccountSet(const AccountSetParams& params, const uint8_t accountId[20],
                      const uint8_t pubKey[33], TxFieldBag& bag);

Status buildAndSignAccountSet(const KeyPair& keys, const AccountSetParams& params, char* outHex,
                              size_t outHexCap);

}  // namespace xahau
