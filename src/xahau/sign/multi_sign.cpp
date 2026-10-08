// multi_sign.cpp
#include "multi_sign.h"

#include "sign.h"

#include "../codec/binary_write.h"
#include "../codec/serialize.h"
#include "../core/hex.h"
#include "../platform/xahau_platform.h"

#include <string.h>

namespace xahau {

Status bagPrepareMultiSign(TxFieldBag& bag) {
  // Empty SigningPubKey (VL 0). TxnSignature omitted if not set.
  return bag.setBlob(fc::SigningPubKey, nullptr, 0);
}

Status buildSignersArray(const MultiSigner* signers, uint8_t count, uint8_t* out, size_t outCap,
                         size_t& outLen) {
  outLen = 0;
  if (!signers || count == 0 || !out) return Status::InvalidArgument;
  if (count > XAHAU_MAX_MULTI_SIGNERS) return Status::InvalidField;

  size_t pos = 0;
  for (uint8_t i = 0; i < count; ++i) {
    if (signers[i].signatureLen == 0) return Status::MissingField;
    // Signer STObject header: type 14 nth 16 → 0xE0, 0x10 (type<16, field>=16)
    Status st = writeByte(out, pos, outCap, 0xE0);
    if (st != Status::Ok) return st;
    st = writeByte(out, pos, outCap, 0x10);
    if (st != Status::Ok) return st;

    // Canonical field order inside Signer (ascending type/nth):
    //   SigningPubKey (7,3) → TxnSignature (7,4) → Account (8,1)
    // (Account-first wire order is non-canonical and yields invalidTransaction.)

    // SigningPubKey (7,3)
    st = writeFieldHeader(out, pos, outCap, 7, 3);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, 33);
    if (st != Status::Ok) return st;
    st = writeBytes(out, pos, outCap, signers[i].publicKey, 33);
    if (st != Status::Ok) return st;

    // TxnSignature (7,4)
    st = writeFieldHeader(out, pos, outCap, 7, 4);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, signers[i].signatureLen);
    if (st != Status::Ok) return st;
    st = writeBytes(out, pos, outCap, signers[i].signature, signers[i].signatureLen);
    if (st != Status::Ok) return st;

    // Account (8,1)
    st = writeFieldHeader(out, pos, outCap, 8, 1);
    if (st != Status::Ok) return st;
    st = writeVLLength(out, pos, outCap, 20);
    if (st != Status::Ok) return st;
    st = writeBytes(out, pos, outCap, signers[i].accountId, 20);
    if (st != Status::Ok) return st;

    // Object end
    st = writeByte(out, pos, outCap, 0xE1);
    if (st != Status::Ok) return st;
  }
  // Array end
  Status st = writeByte(out, pos, outCap, 0xF1);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

Status multiSignMessage(const TxFieldBag& bag, const uint8_t signerAccountId[20], uint8_t* outMsg,
                        size_t outMsgCap, size_t& outMsgLen) {
  outMsgLen = 0;
  if (!signerAccountId || !outMsg) return Status::InvalidArgument;

  uint8_t localUnsigned[1024];
  size_t unsignedLen = 0;
  Status st = xahauSerialize(bag, /*includeSigningFields=*/false, localUnsigned, sizeof(localUnsigned),
                            unsignedLen);
  if (st != Status::Ok) return st;

  // HashPrefix::transactionMultiSig = "SMT\0"
  // Wire for multi-sign (ripple-binary-codec multiSigningData):
  //   SMT\0 || serialized_tx (no TxnSignature/Signers) || AccountID(20) as SUFFIX
  // (Account is after the tx body — not between prefix and body.)
  static const uint8_t kPrefix[4] = {0x53, 0x4D, 0x54, 0x00};
  const size_t need = 4 + unsignedLen + 20;
  if (need > outMsgCap) return Status::BufferTooSmall;
  memcpy(outMsg, kPrefix, 4);
  memcpy(outMsg + 4, localUnsigned, unsignedLen);
  memcpy(outMsg + 4 + unsignedLen, signerAccountId, 20);
  outMsgLen = need;
  secureZero(localUnsigned, sizeof(localUnsigned));
  return Status::Ok;
}

Status multiSignAs(const TxFieldBag& bag, const KeyPair& keys, MultiSigner& outSigner) {
  memcpy(outSigner.accountId, keys.accountId, 20);
  memcpy(outSigner.publicKey, keys.publicKey, 33);

  uint8_t msg[4 + 20 + 1024];
  size_t msgLen = 0;
  Status st = multiSignMessage(bag, keys.accountId, msg, sizeof(msg), msgLen);
  if (st != Status::Ok) return st;

  size_t sigLen = 0;
  st = signRaw(keys, msg, msgLen, outSigner.signature, sizeof(outSigner.signature), sigLen);
  secureZero(msg, sizeof(msg));
  if (st != Status::Ok) return st;
  outSigner.signatureLen = static_cast<uint16_t>(sigLen);
  return Status::Ok;
}

Status bagSetSigners(TxFieldBag& bag, const MultiSigner* signers, uint8_t count) {
  uint8_t arr[768];
  size_t arrLen = 0;
  Status st = buildSignersArray(signers, count, arr, sizeof(arr), arrLen);
  if (st != Status::Ok) return st;
  return bag.setEmbedded(fc::Signers, arr, static_cast<uint16_t>(arrLen));
}

Status multiSignFinalize(const TxFieldBag& bag, const MultiSigner* signers, uint8_t count,
                         char* outHex, size_t outHexCap) {
  if (!outHex || outHexCap == 0) return Status::InvalidArgument;
  // Shallow copy is OK: we only add Signers (arena-owned embedded body).
  TxFieldBag signedBag = bag;
  Status st = bagSetSigners(signedBag, signers, count);
  if (st != Status::Ok) return st;

  uint8_t signedBuf[1280];
  size_t signedLen = 0;
  st = xahauSerialize(signedBag, true, signedBuf, sizeof(signedBuf), signedLen);
  signedBag.clear();
  if (st != Status::Ok) return st;
  st = bytesToHex(signedBuf, signedLen, outHex, outHexCap);
  secureZero(signedBuf, sizeof(signedBuf));
  return st;
}


}  // namespace xahau
