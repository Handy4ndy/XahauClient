// sign.h — STX-prefix signing over serialized bag
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

/**
 * Serialize bag without signature, sign with keys.curve algorithm,
 * re-serialize with TxnSignature, write uppercase hex to outHex.
 *
 * Requires SigningPubKey + Account + all tx fields already in bag
 * (use bagSetCommon + type-specific fillers). Do not pre-set TxnSignature.
 *
 * Host: stack-local buffers (re-entrant).
 * Arduino/ESP: static BSS buffers (avoids task stack overflow with WebSockets).
 * Caps: XAHAU_SIGN_*_CAP in sign.cpp. Override XAHAU_SIGN_USE_STATIC_BUFS if needed.
 */
Status signTx(const TxFieldBag& bag, const KeyPair& keys, char* outHex, size_t outHexCap);

/**
 * STX-sign bag and set TxnSignature on the bag in place (does not produce hex).
 * Used when applying TxnSignature in-place before multi-sign Signers or hex finalize.
 */
Status bagApplyTxnSignature(TxFieldBag& bag, const KeyPair& keys);

/**
 * Serialize bag including signing fields (TxnSignature / Signers)
 * to uppercase hex. Bag must already be fully signed.
 */
Status bagToSignedHex(const TxFieldBag& bag, char* outHex, size_t outHexCap);

/**
 * Low-level: sign arbitrary message (already includes STX prefix if needed).
 * outSig: 64 bytes Ed25519 raw, or DER for secp (caller provides cap).
 */
Status signRaw(const KeyPair& keys, const uint8_t* msg, size_t msgLen, uint8_t* outSig,
               size_t outSigCap, size_t& outSigLen);

}  // namespace xahau
