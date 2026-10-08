// account_keys.h - TicketCreate, SetRegularKey, SignerListSet
#pragma once

#include "../codec/st_helpers.h"
#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

struct TicketCreateParams {
  TxCommon common;
  uint32_t ticketCount;

  TicketCreateParams() : ticketCount(0) {}
};

struct SetRegularKeyParams {
  TxCommon common;
  /** Empty string = remove regular key. */
  char regularKey[36];
  bool hasRegularKeyId;
  uint8_t regularKeyId[20];

  SetRegularKeyParams() : hasRegularKeyId(false) {
    regularKey[0] = '\0';
    secureZero(regularKeyId, sizeof(regularKeyId));
  }
};

#ifndef XAHAU_MAX_SIGNER_ENTRIES
#define XAHAU_MAX_SIGNER_ENTRIES 8
#endif

struct SignerListSetParams {
  TxCommon common;
  uint32_t signerQuorum;  // 0 + empty entries = delete list
  uint8_t entryCount;
  SignerEntryWire entries[XAHAU_MAX_SIGNER_ENTRIES];

  SignerListSetParams() : signerQuorum(0), entryCount(0) {}
};

Status fillTicketCreate(const TicketCreateParams& params, const uint8_t accountId[20],
                        const uint8_t pubKey[33], TxFieldBag& bag);
Status fillSetRegularKey(const SetRegularKeyParams& params, const uint8_t accountId[20],
                         const uint8_t pubKey[33], TxFieldBag& bag);
Status fillSignerListSet(const SignerListSetParams& params, const uint8_t accountId[20],
                         const uint8_t pubKey[33], TxFieldBag& bag);

Status buildAndSignTicketCreate(const KeyPair& keys, const TicketCreateParams& params,
                                char* outHex, size_t outHexCap);
Status buildAndSignSetRegularKey(const KeyPair& keys, const SetRegularKeyParams& params,
                                 char* outHex, size_t outHexCap);
Status buildAndSignSignerListSet(const KeyPair& keys, const SignerListSetParams& params,
                                 char* outHex, size_t outHexCap);

}  // namespace xahau
