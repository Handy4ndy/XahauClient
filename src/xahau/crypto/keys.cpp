// keys.cpp — dual-curve seed derivation
#include "keys.h"

#include "hash.h"
#include "secp256k1_math.h"
#include "../platform/xahau_platform.h"

#include <string.h>

// Vendored Ed25519 (Arduino Crypto) + micro-ecc
#include "vendor/Ed25519.h"

extern "C" {
#include "vendor/uECC.h"
}

namespace xahau {

void secureClear(KeyPair& keys) {
  secureZero(keys.privateKey, sizeof(keys.privateKey));
  secureZero(keys.publicKey, sizeof(keys.publicKey));
  secureZero(keys.accountId, sizeof(keys.accountId));
  secureZero(keys.address, sizeof(keys.address));
  keys.curve = Curve::Unknown;
}

Status accountIdFromPublicKey(const uint8_t pubKey[33], uint8_t accountId[20]) {
  if (!pubKey || !accountId) return Status::InvalidArgument;
  accountIdFromPubKey33(pubKey, accountId);
  return Status::Ok;
}

static Status finishAccount(KeyPair& keys) {
  Status st = accountIdFromPublicKey(keys.publicKey, keys.accountId);
  if (st != Status::Ok) return st;
  return encodeClassicAddress(keys.accountId, keys.address);
}

static Status deriveEd25519(const char* seedBase58, KeyPair& out) {
#if !XAHAU_FEATURE_ED25519
  (void)seedBase58;
  (void)out;
  return Status::Unsupported;
#else
  uint8_t entropy[16];
  Status st = decodeEd25519Seed(seedBase58, entropy);
  if (st != Status::Ok) return st;

  uint8_t digest[64];
  sha512(entropy, 16, digest);
  memcpy(out.privateKey, digest, 32);
  secureZero(entropy, sizeof(entropy));
  secureZero(digest, sizeof(digest));

  uint8_t rawPub[32];
  Ed25519::derivePublicKey(rawPub, out.privateKey);
  out.publicKey[0] = 0xED;
  memcpy(out.publicKey + 1, rawPub, 32);
  secureZero(rawPub, sizeof(rawPub));
  out.curve = Curve::Ed25519;
  return finishAccount(out);
#endif
}

static bool secpCompressedPub(const uint8_t priv[32], uint8_t out33[33]) {
  uint8_t pubxy[64];
  if (!uECC_compute_public_key(priv, pubxy, uECC_secp256k1())) return false;
  // big-endian API (default): pubxy = X(32) || Y(32)
  out33[0] = (pubxy[63] & 1) ? 0x03 : 0x02;
  memcpy(out33 + 1, pubxy, 32);
  secureZero(pubxy, sizeof(pubxy));
  return true;
}

static Status deriveSecp256k1(const char* seedBase58, KeyPair& out) {
#if !XAHAU_FEATURE_SECP256K1
  (void)seedBase58;
  (void)out;
  return Status::Unsupported;
#else
  uint8_t entropy[16];
  Status st = decodeSecp256k1Seed(seedBase58, entropy);
  if (st != Status::Ok) return st;

  uint8_t privateGen[32];
  if (!secp256k1DeriveScalar(entropy, 16, false, 0, privateGen)) {
    secureZero(entropy, sizeof(entropy));
    return Status::CryptoFailed;
  }

  uint8_t publicGen[33];
  if (!secpCompressedPub(privateGen, publicGen)) {
    secureZero(entropy, sizeof(entropy));
    secureZero(privateGen, sizeof(privateGen));
    return Status::CryptoFailed;
  }

  uint8_t familyScalar[32];
  if (!secp256k1DeriveScalar(publicGen, 33, true, 0, familyScalar)) {
    secureZero(entropy, sizeof(entropy));
    secureZero(privateGen, sizeof(privateGen));
    secureZero(publicGen, sizeof(publicGen));
    return Status::CryptoFailed;
  }

  if (!secp256k1ModAdd(familyScalar, privateGen, out.privateKey)) {
    secureZero(entropy, sizeof(entropy));
    secureZero(privateGen, sizeof(privateGen));
    secureZero(publicGen, sizeof(publicGen));
    secureZero(familyScalar, sizeof(familyScalar));
    return Status::CryptoFailed;
  }

  if (!secpCompressedPub(out.privateKey, out.publicKey)) {
    secureClear(out);
    return Status::CryptoFailed;
  }

  secureZero(entropy, sizeof(entropy));
  secureZero(privateGen, sizeof(privateGen));
  secureZero(publicGen, sizeof(publicGen));
  secureZero(familyScalar, sizeof(familyScalar));
  out.curve = Curve::Secp256k1;
  return finishAccount(out);
#endif
}

Status deriveKeysFromSeed(const char* seedBase58, Curve curve, KeyPair& out) {
  secureClear(out);
  if (!seedBase58 || !seedBase58[0]) return Status::InvalidSeed;
  if (curve == Curve::Ed25519) return deriveEd25519(seedBase58, out);
  if (curve == Curve::Secp256k1) return deriveSecp256k1(seedBase58, out);
  return Status::InvalidArgument;
}

Status deriveKeysFromSeed(const char* seedBase58, KeyPair& out) {
  if (!seedBase58 || !seedBase58[0]) return Status::InvalidSeed;
  if (seedBase58[0] == 's' && seedBase58[1] == 'E' && seedBase58[2] == 'd') {
    return deriveKeysFromSeed(seedBase58, Curve::Ed25519, out);
  }
  return deriveKeysFromSeed(seedBase58, Curve::Secp256k1, out);
}

Status seedFromEntropy(const uint8_t entropy[16], Curve curve, char* out, size_t outCap) {
  if (!entropy || !out) return Status::InvalidArgument;
  if (curve == Curve::Ed25519) {
#if !XAHAU_FEATURE_ED25519
    return Status::Unsupported;
#else
    return encodeEd25519Seed(entropy, out, outCap);
#endif
  }
  if (curve == Curve::Secp256k1) {
#if !XAHAU_FEATURE_SECP256K1
    return Status::Unsupported;
#else
    return encodeSecp256k1Seed(entropy, out, outCap);
#endif
  }
  return Status::InvalidArgument;
}

Status generateKeyPairFromEntropy(const uint8_t entropy[16], Curve curve, char* seedOut,
                                  size_t seedOutCap, KeyPair& out) {
  if (!entropy) return Status::InvalidArgument;
  char localSeed[kFamilySeedBufSize];
  char* seedBuf = seedOut ? seedOut : localSeed;
  size_t seedCap = seedOut ? seedOutCap : sizeof(localSeed);

  Status st = seedFromEntropy(entropy, curve, seedBuf, seedCap);
  if (st != Status::Ok) {
    if (!seedOut) secureZero(localSeed, sizeof(localSeed));
    return st;
  }

  st = deriveKeysFromSeed(seedBuf, curve, out);
  if (!seedOut) secureZero(localSeed, sizeof(localSeed));
  return st;
}

}  // namespace xahau
