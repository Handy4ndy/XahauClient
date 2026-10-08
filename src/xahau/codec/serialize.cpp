// serialize.cpp
#include "serialize.h"

#include "binary_write.h"

namespace xahau {

static Status emitSlot(uint8_t* buf, size_t& pos, size_t max, const FieldSlot& s) {
  Status st = writeFieldHeaderCode(buf, pos, max, s.fieldCode);
  if (st != Status::Ok) return st;

  switch (s.kind) {
    case FieldValueKind::UInt8:
      return writeByte(buf, pos, max, s.v.u8);
    case FieldValueKind::UInt16:
      return writeUInt16BE(buf, pos, max, s.v.u16);
    case FieldValueKind::UInt32:
      return writeUInt32BE(buf, pos, max, s.v.u32);
    case FieldValueKind::UInt64:
      return writeUInt64BE(buf, pos, max, s.v.u64);
    case FieldValueKind::Hash128:
    case FieldValueKind::Hash192:
    case FieldValueKind::Hash256:
    case FieldValueKind::Issue:
      return writeBytes(buf, pos, max, s.v.blob.data, s.v.blob.len);
    case FieldValueKind::AccountID:
      st = writeVLLength(buf, pos, max, 20);
      if (st != Status::Ok) return st;
      return writeBytes(buf, pos, max, s.v.blob.data, 20);
    case FieldValueKind::Blob:
      st = writeVLLength(buf, pos, max, s.v.blob.len);
      if (st != Status::Ok) return st;
      return writeBytes(buf, pos, max, s.v.blob.data, s.v.blob.len);
    case FieldValueKind::Embedded:
      // Field header already written; raw STArray/STObject bytes (no VL).
      return writeBytes(buf, pos, max, s.v.blob.data, s.v.blob.len);
    case FieldValueKind::AmountXrp:
      return writeXrpAmount(buf, pos, max, s.v.xrpDrops);
    case FieldValueKind::AmountIssued:
      return writeIssuedAmount(buf, pos, max, s.v.issued.value, s.v.issued.currency,
                               s.v.issued.issuer);
    case FieldValueKind::AmountMpt:
      return writeMptAmount(buf, pos, max, s.v.mpt.value, s.v.mpt.issuanceId);
    default:
      return Status::InvalidField;
  }
}

Status xahauSerialize(const TxFieldBag& bag, bool includeSigningFields, uint8_t* outBuf,
                     size_t outCap, size_t& outLen) {
  if (!outBuf || outCap == 0) return Status::InvalidArgument;

  size_t idx[XAHAU_TX_FIELD_BAG_MAX_SLOTS];
  size_t n = bag.sortedIndices(idx, XAHAU_TX_FIELD_BAG_MAX_SLOTS);
  size_t pos = 0;

  for (size_t i = 0; i < n; ++i) {
    const FieldSlot& s = bag.slot(idx[i]);
    // TxnSignature and claim/channel Signature are isSigningField=false on Xahau/XRPL.
    if (!includeSigningFields &&
        (s.fieldCode == fc::TxnSignature || s.fieldCode == fc::Signature)) {
      continue;
    }
    Status st = emitSlot(outBuf, pos, outCap, s);
    if (st != Status::Ok) return st;
  }

  outLen = pos;
  return Status::Ok;
}

}  // namespace xahau
