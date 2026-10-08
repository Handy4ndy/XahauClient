/**
 * XahauClient — full Xahau client library for Arduino / ESP32
 *
 * Single include for sketches:
 *   #include <XahauClient.h>
 *
 * Port of XRPLClient aligned to live Xahau mainnet (NetworkID 21337):
 * Hooks, URIToken, Remit, Import, ClaimReward, Invoke, Cron, Remarks,
 * PriceOracle (OracleSet/OracleDelete), IOU escrow channels, etc.
 *
 * Core is portable (host tests without Arduino). Network is optional.
 */
#pragma once

#define XAHAU_CLIENT_VERSION_MAJOR 0
#define XAHAU_CLIENT_VERSION_MINOR 3
#define XAHAU_CLIENT_VERSION_PATCH 1
#define XAHAU_CLIENT_VERSION "0.3.1"

#include "xahau/platform/xahau_platform.h"
#include "xahau/core/xahau_error.h"
#include "xahau/core/xahau_types.h"
#include "xahau/core/hex.h"
#include "xahau/tx/tx_type.h"
#include "xahau/codec/tx_field_bag.h"
#include "xahau/codec/binary_write.h"
#include "xahau/codec/serialize.h"
#include "xahau/crypto/keys.h"
#include "xahau/sign/sign.h"
#include "xahau/sign/multi_sign.h"
#include "xahau/tx/payment.h"
#include "xahau/tx/account_set.h"
#include "xahau/tx/trust_set.h"
#include "xahau/tx/offer.h"
#include "xahau/tx/account_keys.h"
#include "xahau/tx/phase2_extra.h"
#include "xahau/tx/phase4_extra.h"
#include "xahau/tx/xahau_extra.h"
#include "xahau/codec/st_number.h"
#include "xahau/generated/protocol_fields.h"
#include "xahau/net/networks.h"
#include "xahau/rpc/transport.h"
#include "xahau/rpc/rpc.h"
#include "xahau/rpc/client.h"
#include "xahau/rpc/mock_transport.h"
#if XAHAU_FEATURE_NET
#include "xahau/net/ws_transport.h"
#endif

// ---------------------------------------------------------------------------
// C-friendly aliases matching README examples
// ---------------------------------------------------------------------------

using XahauKeyPair = xahau::KeyPair;
using XahauCurve = xahau::Curve;
using XahauStatus = xahau::Status;
using XahauAmount = xahau::Amount;
using PaymentParams = xahau::PaymentParams;
using AccountSetParams = xahau::AccountSetParams;
using TrustSetParams = xahau::TrustSetParams;
using TxFieldBag = xahau::TxFieldBag;
using TxType = xahau::TxType;

inline XahauStatus xahauDeriveKeysFromSeed(const char* seed, XahauKeyPair& keys) {
  return xahau::deriveKeysFromSeed(seed, keys);
}

/**
 * Build family seed from 16-byte CSPRNG entropy (library does not generate RNG).
 * Always tell users to copy the seed offline — vault encryption is not recoverable.
 */
inline XahauStatus xahauSeedFromEntropy(const uint8_t entropy[16], XahauCurve curve, char* out,
                                     size_t outCap) {
  return xahau::seedFromEntropy(entropy, curve, out, outCap);
}

inline XahauStatus xahauGenerateKeyPairFromEntropy(const uint8_t entropy[16], XahauCurve curve,
                                                 char* seedOut, size_t seedOutCap,
                                                 XahauKeyPair& keys) {
  return xahau::generateKeyPairFromEntropy(entropy, curve, seedOut, seedOutCap, keys);
}

inline void xahauSecureClear(XahauKeyPair& keys) { xahau::secureClear(keys); }

inline XahauStatus xahauBuildAndSignPayment(const XahauKeyPair& keys, const PaymentParams& pay,
                                          char* outHex, size_t outHexCap) {
  return xahau::buildAndSignPayment(keys, pay, outHex, outHexCap);
}

inline XahauStatus xahauBuildAndSignAccountSet(const XahauKeyPair& keys, const AccountSetParams& p,
                                             char* outHex, size_t outHexCap) {
  return xahau::buildAndSignAccountSet(keys, p, outHex, outHexCap);
}

inline XahauStatus xahauBuildAndSignTrustSet(const XahauKeyPair& keys, const TrustSetParams& p,
                                           char* outHex, size_t outHexCap) {
  return xahau::buildAndSignTrustSet(keys, p, outHex, outHexCap);
}

inline XahauStatus xahauSignTx(const TxFieldBag& bag, const XahauKeyPair& keys, char* outHex,
                             size_t outHexCap) {
  return xahau::signTx(bag, keys, outHex, outHexCap);
}

inline const char* xahauStatusName(XahauStatus s) { return xahau::statusName(s); }
/** Alias for sketches that still call XahauStatusName (rename artifact). */
inline const char* XahauStatusName(XahauStatus s) { return xahau::statusName(s); }

inline size_t xahauTxTypeCount() { return xahau::txTypeCatalogCount(); }

inline const char* xahauTxTypeName(TxType t) { return xahau::txTypeName(t); }
