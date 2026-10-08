// tx_field_bag.cpp
#include "tx_field_bag.h"

#include "st_number.h"

#include <stdlib.h>
#include <string.h>

namespace xahau {

TxFieldBag::TxFieldBag() { clear(); }

void TxFieldBag::clear() {
  count_ = 0;
  arenaUsed_ = 0;
  memset(slots_, 0, sizeof(slots_));
  memset(arena_, 0, sizeof(arena_));
}

Status TxFieldBag::storeBytes(const uint8_t* data, uint16_t len, const uint8_t*& outPtr) {
  if (!data && len > 0) return Status::InvalidArgument;
  if (arenaUsed_ + len > XAHAU_TX_FIELD_BAG_ARENA) return Status::BufferTooSmall;
  if (len == 0) {
    outPtr = nullptr;
    return Status::Ok;
  }
  memcpy(arena_ + arenaUsed_, data, len);
  outPtr = arena_ + arenaUsed_;
  arenaUsed_ += len;
  return Status::Ok;
}

Status TxFieldBag::upsert(const FieldSlot& s) {
  for (size_t i = 0; i < count_; ++i) {
    if (slots_[i].fieldCode == s.fieldCode) {
      slots_[i] = s;
      return Status::Ok;
    }
  }
  if (count_ >= XAHAU_TX_FIELD_BAG_MAX_SLOTS) return Status::BufferTooSmall;
  slots_[count_++] = s;
  return Status::Ok;
}

Status TxFieldBag::setUInt8(uint32_t code, uint8_t val) {
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::UInt8;
  s.v.u8 = val;
  return upsert(s);
}

Status TxFieldBag::setUInt16(uint32_t code, uint16_t val) {
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::UInt16;
  s.v.u16 = val;
  return upsert(s);
}

Status TxFieldBag::setUInt32(uint32_t code, uint32_t val) {
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::UInt32;
  s.v.u32 = val;
  return upsert(s);
}

Status TxFieldBag::setUInt64(uint32_t code, uint64_t val) {
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::UInt64;
  s.v.u64 = val;
  return upsert(s);
}

Status TxFieldBag::setHash256(uint32_t code, const uint8_t hash[32]) {
  if (!hash) return Status::InvalidArgument;
  const uint8_t* p = nullptr;
  Status st = storeBytes(hash, 32, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Hash256;
  s.v.blob.data = p;
  s.v.blob.len = 32;
  return upsert(s);
}

Status TxFieldBag::setHash192(uint32_t code, const uint8_t hash[24]) {
  if (!hash) return Status::InvalidArgument;
  const uint8_t* p = nullptr;
  Status st = storeBytes(hash, 24, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Hash192;
  s.v.blob.data = p;
  s.v.blob.len = 24;
  return upsert(s);
}

Status TxFieldBag::setIssueXrp(uint32_t code) {
  uint8_t cur[20];
  memset(cur, 0, 20);
  const uint8_t* p = nullptr;
  Status st = storeBytes(cur, 20, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Issue;
  s.v.blob.data = p;
  s.v.blob.len = 20;
  return upsert(s);
}

Status TxFieldBag::setIssueIou(uint32_t code, const char* currency, const uint8_t issuer[20]) {
  if (!currency || !issuer) return Status::InvalidArgument;
  uint8_t raw[40];
  memset(raw, 0, 40);
  size_t clen = strlen(currency);
  if (clen == 3) {
    raw[12] = (uint8_t)currency[0];
    raw[13] = (uint8_t)currency[1];
    raw[14] = (uint8_t)currency[2];
  } else if (clen == 40) {
    for (int i = 0; i < 20; ++i) {
      char hb[3] = {currency[i * 2], currency[i * 2 + 1], 0};
      raw[i] = (uint8_t)strtoul(hb, nullptr, 16);
    }
  } else {
    return Status::InvalidField;
  }
  memcpy(raw + 20, issuer, 20);
  const uint8_t* p = nullptr;
  Status st = storeBytes(raw, 40, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Issue;
  s.v.blob.data = p;
  s.v.blob.len = 40;
  return upsert(s);
}

Status TxFieldBag::setIssueMpt(uint32_t code, const uint8_t mptIssuanceId[24]) {
  if (!mptIssuanceId) return Status::InvalidArgument;
  // mpt_issuance_id: Sequence (4 BE) || Issuer AccountID (20)
  // Issue wire (44): Issuer AccountID (20) || 0x00..01 (20) || Sequence (4 LE)
  uint8_t raw[44];
  memset(raw, 0, 44);
  memcpy(raw, mptIssuanceId + 4, 20);  // issuer account
  raw[39] = 0x01;                      // NO_ACCOUNT marker (...0001)
  // Sequence big-endian -> little-endian
  raw[40] = mptIssuanceId[3];
  raw[41] = mptIssuanceId[2];
  raw[42] = mptIssuanceId[1];
  raw[43] = mptIssuanceId[0];
  const uint8_t* p = nullptr;
  Status st = storeBytes(raw, 44, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Issue;
  s.v.blob.data = p;
  s.v.blob.len = 44;
  return upsert(s);
}

Status TxFieldBag::setHash128(uint32_t code, const uint8_t hash[16]) {
  if (!hash) return Status::InvalidArgument;
  const uint8_t* p = nullptr;
  Status st = storeBytes(hash, 16, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Hash128;
  s.v.blob.data = p;
  s.v.blob.len = 16;
  return upsert(s);
}

Status TxFieldBag::setAccountID(uint32_t code, const uint8_t id[20]) {
  if (!id) return Status::InvalidArgument;
  const uint8_t* p = nullptr;
  Status st = storeBytes(id, 20, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::AccountID;
  s.v.blob.data = p;
  s.v.blob.len = 20;
  return upsert(s);
}

Status TxFieldBag::setBlob(uint32_t code, const uint8_t* data, uint16_t len) {
  const uint8_t* p = nullptr;
  Status st = storeBytes(data, len, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Blob;
  s.v.blob.data = p;
  s.v.blob.len = len;
  return upsert(s);
}

Status TxFieldBag::setEmbedded(uint32_t code, const uint8_t* data, uint16_t len) {
  const uint8_t* p = nullptr;
  Status st = storeBytes(data, len, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Embedded;
  s.v.blob.data = p;
  s.v.blob.len = len;
  return upsert(s);
}

Status TxFieldBag::setNumber(uint32_t code, const char* decimal) {
  uint8_t body[12];
  Status st = encodeStNumber(decimal, body);
  if (st != Status::Ok) return st;
  const uint8_t* p = nullptr;
  st = storeBytes(body, 12, p);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::Embedded;
  s.v.blob.data = p;
  s.v.blob.len = 12;
  return upsert(s);
}

Status TxFieldBag::setAmountXrp(uint32_t code, uint64_t drops) {
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::AmountXrp;
  s.v.xrpDrops = drops;
  return upsert(s);
}

Status TxFieldBag::setAmountIssued(uint32_t code, const char* value, const char* currency,
                                    const uint8_t issuer[20]) {
  if (!value || !currency || !issuer) return Status::InvalidArgument;
  // Arena-own value + currency strings so callers can use stack temps safely.
  const size_t vlen = strlen(value);
  const size_t clen = strlen(currency);
  if (vlen > 64 || clen > 40) return Status::InvalidField;
  const uint8_t* vPtr = nullptr;
  const uint8_t* cPtr = nullptr;
  const uint8_t* iss = nullptr;
  Status st = storeBytes(reinterpret_cast<const uint8_t*>(value), static_cast<uint16_t>(vlen + 1),
                         vPtr);
  if (st != Status::Ok) return st;
  st = storeBytes(reinterpret_cast<const uint8_t*>(currency), static_cast<uint16_t>(clen + 1), cPtr);
  if (st != Status::Ok) return st;
  st = storeBytes(issuer, 20, iss);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::AmountIssued;
  s.v.issued.value = reinterpret_cast<const char*>(vPtr);
  s.v.issued.currency = reinterpret_cast<const char*>(cPtr);
  s.v.issued.issuer = iss;
  return upsert(s);
}

Status TxFieldBag::setAmountMpt(uint32_t code, uint64_t value, const uint8_t issuanceId[24]) {
  if (!issuanceId) return Status::InvalidArgument;
  if (value > 0x7FFFFFFFFFFFFFFFULL) return Status::InvalidField;
  const uint8_t* id = nullptr;
  Status st = storeBytes(issuanceId, 24, id);
  if (st != Status::Ok) return st;
  FieldSlot s{};
  s.fieldCode = code;
  s.kind = FieldValueKind::AmountMpt;
  s.v.mpt.value = value;
  s.v.mpt.issuanceId = id;
  return upsert(s);
}

size_t TxFieldBag::sortedIndices(size_t* outIdx, size_t maxIdx) const {
  size_t n = count_ < maxIdx ? count_ : maxIdx;
  for (size_t i = 0; i < n; ++i) outIdx[i] = i;
  // Insertion sort by fieldCode
  for (size_t i = 1; i < n; ++i) {
    size_t key = outIdx[i];
    uint32_t keyCode = slots_[key].fieldCode;
    size_t j = i;
    while (j > 0 && slots_[outIdx[j - 1]].fieldCode > keyCode) {
      outIdx[j] = outIdx[j - 1];
      --j;
    }
    outIdx[j] = key;
  }
  return n;
}

Status bagSetCommon(TxFieldBag& bag, TxType type, const TxCommon& common,
                    const uint8_t accountId[20], const uint8_t pubKey[33],
                    const uint8_t* signature, uint16_t sigLen, bool includeSig) {
  Status st;
  st = bag.setUInt16(fc::TransactionType, static_cast<uint16_t>(type));
  if (st != Status::Ok) return st;

  // NetworkID is part of the signed payload on Xahau (IDs >= 1025).
  // Omit when 0 for offline experiments; live mainnet/testnet must set 21337/21338.
  if (common.networkId >= 1025u) {
    st = bag.setUInt32(fc::NetworkID, common.networkId);
    if (st != Status::Ok) return st;
  }

  // Always emit Flags (Xahau commonly includes Flags=0).
  st = bag.setUInt32(fc::Flags, common.flags);
  if (st != Status::Ok) return st;
  if (common.sourceTag != 0) {
    st = bag.setUInt32(fc::SourceTag, common.sourceTag);
    if (st != Status::Ok) return st;
  }
  st = bag.setUInt32(fc::Sequence, common.sequence);
  if (st != Status::Ok) return st;

  if (common.destinationTag != 0) {
    st = bag.setUInt32(fc::DestinationTag, common.destinationTag);
    if (st != Status::Ok) return st;
  }
  if (common.firstLedgerSequence != 0) {
    st = bag.setUInt32(fc::FirstLedgerSequence, common.firstLedgerSequence);
    if (st != Status::Ok) return st;
  }
  if (common.lastLedgerSequence != 0) {
    st = bag.setUInt32(fc::LastLedgerSequence, common.lastLedgerSequence);
    if (st != Status::Ok) return st;
  }
  if (common.ticketSequence != 0) {
    st = bag.setUInt32(fc::TicketSequence, common.ticketSequence);
    if (st != Status::Ok) return st;
  }

  // Enforce protocol special-cost floors at encode time so AMMCreate / VaultCreate /
  // AccountDelete never ship with ordinary open-ledger fees (telINSUF_FEE_P).
  uint64_t feeDrops = common.feeDrops;
  const uint64_t feeFloor = txMinFeeDrops(type);
  if (feeFloor > 0 && feeDrops < feeFloor) {
    feeDrops = feeFloor;
  }
  st = bag.setAmountXrp(fc::Fee, feeDrops);
  if (st != Status::Ok) return st;

  // Multi-sign: empty SigningPubKey (VL length 0). Single-sign: 33-byte key.
  // Pass pubKey=nullptr for multi-sign envelopes (Signers filled separately).
  if (pubKey) {
    st = bag.setBlob(fc::SigningPubKey, pubKey, 33);
  } else {
    st = bag.setBlob(fc::SigningPubKey, nullptr, 0);
  }
  if (st != Status::Ok) return st;

  if (includeSig && signature && sigLen > 0) {
    st = bag.setBlob(fc::TxnSignature, signature, sigLen);
    if (st != Status::Ok) return st;
  }

  st = bag.setAccountID(fc::Account, accountId);
  if (st != Status::Ok) return st;

  return Status::Ok;
}

}  // namespace xahau
