// multi_sign.h - multi-sign (Signers array) + helpers for dual-party envelopes
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"

namespace xahau {

#ifndef XAHAU_MAX_MULTI_SIGNERS
#define XAHAU_MAX_MULTI_SIGNERS 8
#endif

/** One multi-sign contribution (Account + SigningPubKey + TxnSignature). */
struct MultiSigner {
  uint8_t accountId[20];
  uint8_t publicKey[33];  // Ed25519 0xED-pub or compressed secp
  uint8_t signature[80];
  uint16_t signatureLen;
  MultiSigner() : signatureLen(0) {
    secureZero(accountId, 20);
    secureZero(publicKey, 33);
    secureZero(signature, 80);
  }
};

/**
 * Prepare a filled bag for multi-sign: empty SigningPubKey, no TxnSignature.
 * Call after fill* with a normal single-sign bag (or bagSetCommon with pubKey=nullptr).
 */
Status bagPrepareMultiSign(TxFieldBag& bag);

/**
 * Encode Signers STArray body (no outer F0 header - setEmbedded adds field header).
 * Each Signer: Account, SigningPubKey, TxnSignature.
 */
Status buildSignersArray(const MultiSigner* signers, uint8_t count, uint8_t* out, size_t outCap,
                         size_t& outLen);

/**
 * Multi-signing message (HashPrefix::transactionMultiSig):
 *   SMT\0 || serialized_tx (SigningPubKey empty; no TxnSignature/Signers) || AccountID(20)
 * AccountID is a **suffix** (matches ripple-binary-codec multiSigningData).
 * Signs with signRaw. Does not attach to bag.
 *
 * @param bag  Must already have empty SigningPubKey and NO TxnSignature / Signers.
 */
Status multiSignMessage(const TxFieldBag& bag, const uint8_t signerAccountId[20],
                        uint8_t* outMsg, size_t outMsgCap, size_t& outMsgLen);

/** Sign bag as multi-signer; fills MultiSigner signature fields (account/pub from keys). */
Status multiSignAs(const TxFieldBag& bag, const KeyPair& keys, MultiSigner& outSigner);

/**
 * Attach Signers array to bag (replaces any existing Signers field).
 * Call after all multiSignAs contributions are collected.
 */
Status bagSetSigners(TxFieldBag& bag, const MultiSigner* signers, uint8_t count);

/**
 * Full multi-sign finalize: set Signers on bag copy and serialize with includeSigningFields.
 * SigningPubKey must already be empty; no single TxnSignature.
 */
Status multiSignFinalize(const TxFieldBag& bag, const MultiSigner* signers, uint8_t count,
                         char* outHex, size_t outHexCap);


}  // namespace xahau
