// sign.cpp - STX-prefix signing (Ed25519 raw + secp256k1 RFC6979 DER)
//
// Host: stack-local buffers (re-entrant).
// Arduino/ESP: static BSS buffers - WebSocketsClient + TxFieldBag already
// blow the default ~8 KiB task stack if signTx also uses ~3.5 KiB stack.
#include "sign.h"

#include "../codec/serialize.h"
#include "../core/hex.h"
#include "../crypto/bip66.h"
#include "../crypto/hash.h"
#include "../crypto/secp256k1_math.h"
#include "../platform/xahau_platform.h"

#include <string.h>

#include "../crypto/vendor/Ed25519.h"

extern "C" {
#include "../crypto/vendor/uECC.h"
}

namespace xahau {

#ifndef XAHAU_SIGN_UNSIGNED_CAP
// Tier A SetHook: unsigned/signed STObject with CreateCode <= 8 KiB.
#define XAHAU_SIGN_UNSIGNED_CAP 8704
#endif
#ifndef XAHAU_SIGN_SIGNED_CAP
#define XAHAU_SIGN_SIGNED_CAP 8704
#endif
#ifndef XAHAU_SIGN_MSG_CAP
#define XAHAU_SIGN_MSG_CAP 8712
#endif
#ifndef XAHAU_SIGN_SIG_CAP
#define XAHAU_SIGN_SIG_CAP 128
#endif

// Arduino/ESP: prefer static sign buffers (single-threaded sketches / one signer task).
// Host tests: stack buffers for re-entrancy.
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
#ifndef XAHAU_SIGN_USE_STATIC_BUFS
#define XAHAU_SIGN_USE_STATIC_BUFS 1
#endif
#else
#ifndef XAHAU_SIGN_USE_STATIC_BUFS
#define XAHAU_SIGN_USE_STATIC_BUFS 0
#endif
#endif

// --- SHA-256 context bridge for uECC_sign_deterministic (RFC 6979) ---

struct UeccSha256Ctx {
  uECC_HashContext uECC;
  Sha256Ctx sha;
  uint8_t tmp[32 + 32 + 64];
};

static void uecc_sha256_init(const uECC_HashContext* base) {
  auto* ctx = (UeccSha256Ctx*)base;
  sha256Init(ctx->sha);
}

static void uecc_sha256_update(const uECC_HashContext* base, const uint8_t* message,
                               unsigned message_size) {
  auto* ctx = (UeccSha256Ctx*)base;
  sha256Update(ctx->sha, message, message_size);
}

static void uecc_sha256_finish(const uECC_HashContext* base, uint8_t* hash_result) {
  auto* ctx = (UeccSha256Ctx*)base;
  sha256Finish(ctx->sha, hash_result);
}

Status signRaw(const KeyPair& keys, const uint8_t* msg, size_t msgLen, uint8_t* outSig,
               size_t outSigCap, size_t& outSigLen) {
  outSigLen = 0;
  if (!msg || !outSig) return Status::InvalidArgument;

  if (keys.curve == Curve::Ed25519) {
#if XAHAU_FEATURE_ED25519
    if (outSigCap < 64) return Status::BufferTooSmall;
    if (keys.publicKey[0] != 0xED) return Status::InvalidArgument;
    Ed25519::sign(outSig, keys.privateKey, keys.publicKey + 1, msg, msgLen);
    outSigLen = 64;
    return Status::Ok;
#else
    return Status::Unsupported;
#endif
  }

  if (keys.curve == Curve::Secp256k1) {
#if XAHAU_FEATURE_SECP256K1
    uint8_t hash32[32];
    sha512Half(msg, msgLen, hash32);

    UeccSha256Ctx hctx;
    hctx.uECC.init_hash = &uecc_sha256_init;
    hctx.uECC.update_hash = &uecc_sha256_update;
    hctx.uECC.finish_hash = &uecc_sha256_finish;
    hctx.uECC.block_size = 64;
    hctx.uECC.result_size = 32;
    hctx.uECC.tmp = hctx.tmp;

    uint8_t sig64[64];
    if (!uECC_sign_deterministic(keys.privateKey, hash32, 32, &hctx.uECC, sig64,
                                 uECC_secp256k1())) {
      secureZero(hash32, sizeof(hash32));
      return Status::CryptoFailed;
    }
    secureZero(hash32, sizeof(hash32));

    uint8_t r[32], s[32];
    memcpy(r, sig64, 32);
    memcpy(s, sig64 + 32, 32);
    secureZero(sig64, sizeof(sig64));
    if (!secp256k1CanonicalizeS(s)) {
      secureZero(r, sizeof(r));
      secureZero(s, sizeof(s));
      return Status::CryptoFailed;
    }
    size_t derLen = bip66DerEncode(r, s, outSig, outSigCap);
    secureZero(r, sizeof(r));
    secureZero(s, sizeof(s));
    if (derLen == 0) return Status::BufferTooSmall;
    outSigLen = derLen;
    return Status::Ok;
#else
    return Status::Unsupported;
#endif
  }

  return Status::InvalidArgument;
}

Status signTx(const TxFieldBag& bag, const KeyPair& keys, char* outHex, size_t outHexCap) {
  if (!outHex || outHexCap < 3) return Status::InvalidArgument;
  if (keys.curve != Curve::Ed25519 && keys.curve != Curve::Secp256k1) {
    return Status::InvalidArgument;
  }

#if XAHAU_SIGN_USE_STATIC_BUFS
  static uint8_t unsignedBuf[XAHAU_SIGN_UNSIGNED_CAP];
  static uint8_t signedBuf[XAHAU_SIGN_SIGNED_CAP];
  static uint8_t signMsg[XAHAU_SIGN_MSG_CAP];
  static uint8_t sig[XAHAU_SIGN_SIG_CAP];
  static TxFieldBag signedBag;
#else
  uint8_t unsignedBuf[XAHAU_SIGN_UNSIGNED_CAP];
  uint8_t signedBuf[XAHAU_SIGN_SIGNED_CAP];
  uint8_t signMsg[XAHAU_SIGN_MSG_CAP];
  uint8_t sig[XAHAU_SIGN_SIG_CAP];
  TxFieldBag signedBag;
#endif

  size_t unsignedLen = 0;
  Status st = xahauSerialize(bag, /*includeSigningFields=*/false, unsignedBuf, sizeof(unsignedBuf),
                            unsignedLen);
  if (st != Status::Ok) {
    secureZero(unsignedBuf, sizeof(unsignedBuf));
    return st;
  }

  static const uint8_t kPrefix[4] = {0x53, 0x54, 0x58, 0x00};
  if (4 + unsignedLen > sizeof(signMsg)) {
    secureZero(unsignedBuf, sizeof(unsignedBuf));
    return Status::BufferTooSmall;
  }
  memcpy(signMsg, kPrefix, 4);
  memcpy(signMsg + 4, unsignedBuf, unsignedLen);

  size_t sigLen = 0;
  st = signRaw(keys, signMsg, 4 + unsignedLen, sig, sizeof(sig), sigLen);
  secureZero(signMsg, sizeof(signMsg));
  secureZero(unsignedBuf, sizeof(unsignedBuf));
  if (st != Status::Ok) {
    secureZero(sig, sizeof(sig));
    return st;
  }

  signedBag = bag;
  st = signedBag.setBlob(fc::TxnSignature, sig, static_cast<uint16_t>(sigLen));
  secureZero(sig, sizeof(sig));
  if (st != Status::Ok) return st;

  size_t signedLen = 0;
  st = xahauSerialize(signedBag, true, signedBuf, sizeof(signedBuf), signedLen);
  signedBag.clear();
  if (st != Status::Ok) {
    secureZero(signedBuf, sizeof(signedBuf));
    return st;
  }

  st = bytesToHex(signedBuf, signedLen, outHex, outHexCap);
  secureZero(signedBuf, sizeof(signedBuf));
  return st;
}

Status bagApplyTxnSignature(TxFieldBag& bag, const KeyPair& keys) {
  if (keys.curve != Curve::Ed25519 && keys.curve != Curve::Secp256k1) {
    return Status::InvalidArgument;
  }

#if XAHAU_SIGN_USE_STATIC_BUFS
  static uint8_t unsignedBuf[XAHAU_SIGN_UNSIGNED_CAP];
  static uint8_t signMsg[XAHAU_SIGN_MSG_CAP];
  static uint8_t sig[XAHAU_SIGN_SIG_CAP];
#else
  uint8_t unsignedBuf[XAHAU_SIGN_UNSIGNED_CAP];
  uint8_t signMsg[XAHAU_SIGN_MSG_CAP];
  uint8_t sig[XAHAU_SIGN_SIG_CAP];
#endif

  size_t unsignedLen = 0;
  Status st = xahauSerialize(bag, /*includeSigningFields=*/false, unsignedBuf, sizeof(unsignedBuf),
                            unsignedLen);
  if (st != Status::Ok) {
    secureZero(unsignedBuf, sizeof(unsignedBuf));
    return st;
  }

  static const uint8_t kPrefix[4] = {0x53, 0x54, 0x58, 0x00};  // STX\0
  if (4 + unsignedLen > sizeof(signMsg)) {
    secureZero(unsignedBuf, sizeof(unsignedBuf));
    return Status::BufferTooSmall;
  }
  memcpy(signMsg, kPrefix, 4);
  memcpy(signMsg + 4, unsignedBuf, unsignedLen);
  secureZero(unsignedBuf, sizeof(unsignedBuf));

  size_t sigLen = 0;
  st = signRaw(keys, signMsg, 4 + unsignedLen, sig, sizeof(sig), sigLen);
  secureZero(signMsg, sizeof(signMsg));
  if (st != Status::Ok) {
    secureZero(sig, sizeof(sig));
    return st;
  }

  st = bag.setBlob(fc::TxnSignature, sig, static_cast<uint16_t>(sigLen));
  secureZero(sig, sizeof(sig));
  return st;
}

Status bagToSignedHex(const TxFieldBag& bag, char* outHex, size_t outHexCap) {
  if (!outHex || outHexCap < 3) return Status::InvalidArgument;
#if XAHAU_SIGN_USE_STATIC_BUFS
  static uint8_t signedBuf[XAHAU_SIGN_SIGNED_CAP];
#else
  uint8_t signedBuf[XAHAU_SIGN_SIGNED_CAP];
#endif
  size_t signedLen = 0;
  Status st = xahauSerialize(bag, true, signedBuf, sizeof(signedBuf), signedLen);
  if (st != Status::Ok) {
    secureZero(signedBuf, sizeof(signedBuf));
    return st;
  }
  st = bytesToHex(signedBuf, signedLen, outHex, outHexCap);
  secureZero(signedBuf, sizeof(signedBuf));
  return st;
}

}  // namespace xahau
