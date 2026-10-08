// account_set.cpp
#include "account_set.h"

#include "../sign/sign.h"

#include <string.h>

namespace xahau {

Status fillAccountSet(const AccountSetParams& params, const uint8_t accountId[20],
                      const uint8_t pubKey[33], TxFieldBag& bag) {
  if (!accountId || !pubKey) return Status::InvalidArgument;

  bag.clear();
  Status st = bagSetCommon(bag, TxType::AccountSet, params.common, accountId, pubKey, nullptr, 0,
                           false);
  if (st != Status::Ok) return st;

  if (params.hasSetFlag) {
    st = bag.setUInt32(fc::SetFlag, params.setFlag);
    if (st != Status::Ok) return st;
  }
  if (params.hasClearFlag) {
    st = bag.setUInt32(fc::ClearFlag, params.clearFlag);
    if (st != Status::Ok) return st;
  }
  if (params.transferRate != 0) {
    st = bag.setUInt32(fc::TransferRate, params.transferRate);
    if (st != Status::Ok) return st;
  }
  if (params.domain[0] != '\0') {
    st = bag.setBlob(fc::Domain, reinterpret_cast<const uint8_t*>(params.domain),
                     static_cast<uint16_t>(strlen(params.domain)));
    if (st != Status::Ok) return st;
  }
  if (params.forceTickSize || params.tickSize != 0) {
    st = bag.setUInt8(fc::TickSize, params.tickSize);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status buildAndSignAccountSet(const KeyPair& keys, const AccountSetParams& params, char* outHex,
                              size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillAccountSet(params, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}

}  // namespace xahau
