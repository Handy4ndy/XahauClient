// account_keys.cpp
#include "account_keys.h"

#include "../crypto/base58.h"
#include "../sign/sign.h"

#include <string.h>

namespace xahau {

Status fillTicketCreate(const TicketCreateParams& params, const uint8_t accountId[20],
                        const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  if (params.ticketCount == 0 || params.ticketCount > 250) return Status::InvalidField;
  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::TicketCreate, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;
  return bag.setUInt32(fc::TicketCount, params.ticketCount);
}

Status fillSetRegularKey(const SetRegularKeyParams& params, const uint8_t accountId[20],
                         const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::SetRegularKey, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;

  uint8_t rk[20];
  bool have = false;
  if (params.hasRegularKeyId) {
    memcpy(rk, params.regularKeyId, 20);
    have = true;
  } else if (params.regularKey[0] != '\0') {
    st = decodeClassicAddress(params.regularKey, rk);
    if (st != Status::Ok) return st;
    have = true;
  }
  if (have) {
    st = bag.setAccountID(fc::RegularKey, rk);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillSignerListSet(const SignerListSetParams& params, const uint8_t accountId[20],
                         const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;
  bag.clear();
  Status st =
      bagSetCommon(bag, TxType::SignerListSet, params.common, accountId, pubKey, nullptr, 0, false);
  if (st != Status::Ok) return st;

  st = bag.setUInt32(fc::SignerQuorum, params.signerQuorum);
  if (st != Status::Ok) return st;

  if (params.entryCount > 0) {
    if (params.entryCount > XAHAU_MAX_SIGNER_ENTRIES) return Status::InvalidField;
    uint8_t arr[512];
    size_t arrLen = 0;
    st = buildSignerEntriesArray(params.entries, params.entryCount, arr, sizeof(arr), arrLen);
    if (st != Status::Ok) return st;
    st = bag.setEmbedded(fc::SignerEntries, arr, static_cast<uint16_t>(arrLen));
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status buildAndSignTicketCreate(const KeyPair& keys, const TicketCreateParams& params,
                                char* outHex, size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillTicketCreate(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

Status buildAndSignSetRegularKey(const KeyPair& keys, const SetRegularKeyParams& params,
                                 char* outHex, size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillSetRegularKey(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

Status buildAndSignSignerListSet(const KeyPair& keys, const SignerListSetParams& params,
                                 char* outHex, size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillSignerListSet(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

}  // namespace xahau
