// xahau_extra.h — Xahau-native transaction builders (Hooks, URIToken, Remit, …)
#pragma once

#include "../codec/tx_field_bag.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"
#include "../crypto/keys.h"
#include "phase4_extra.h"  // OracleSetParams

namespace xahau {

// ---------------------------------------------------------------------------
// Invoke — call hooks on an account (type 99)
// ---------------------------------------------------------------------------
struct InvokeParams {
  TxCommon common;
  char destination[36];  // optional; empty = self
  InvokeParams() { destination[0] = '\0'; }
};

// ---------------------------------------------------------------------------
// ClaimReward — balance / IOU rewards (type 98)
//
// XAH genesis: set issuer = genesis (rHb9…), leave claimCurrency empty.
//   Requires Issuer Hook that fires on ClaimReward (mainnet genesis has it;
//   testnet genesis currently does NOT → tecNO_TARGET).
// IOU: set issuer = IOU issuer AND claimCurrency = {currency, issuer}.
//   Requires trustline + issuer Hook on ClaimReward (IOURewardClaim).
// Opt-out: set optOut=true (Flags tfOptOut=1); omit issuer / claimCurrency.
// Empty issuer without optOut is invalid for a claim (not “default genesis”).
// ---------------------------------------------------------------------------
struct ClaimRewardParams {
  TxCommon common;
  char issuer[36];           // genesis for XAH; IOU issuer for ClaimCurrency path
  char claimCurrency[41];    // empty = XAH path; 3-char ASCII or 40-hex for IOU
  char claimCurrencyIssuer[36];  // usually same as issuer; required if claimCurrency set
  bool optOut;               // Flags = tfOptOut (1)
  ClaimRewardParams() : optOut(false) {
    issuer[0] = claimCurrency[0] = claimCurrencyIssuer[0] = '\0';
  }
};

// ---------------------------------------------------------------------------
// Import — xPOP import / key sync (type 97)
// ---------------------------------------------------------------------------
struct ImportParams {
  TxCommon common;
  const uint8_t* blob;  // hex-decoded xPOP bytes (required)
  uint16_t blobLen;
  char issuer[36];      // optional Burner/Issuer
  ImportParams() : blob(nullptr), blobLen(0) { issuer[0] = '\0'; }
};

// ---------------------------------------------------------------------------
// Remit — multi-asset push payment (type 95) — XAH amount + destination
// Encodes Remit.Amounts STArray (one AmountEntry), not top-level Amount.
// ---------------------------------------------------------------------------
struct RemitParams {
  TxCommon common;
  char destination[36];
  Amount amount;              // convenience single amount (used if amountCount==0)
  Amount amounts[4];          // multi-asset Amounts STArray
  uint8_t amountCount;        // 0..4; 0 => use `amount` if set
  uint8_t uriTokenIds[4][32]; // URITokenIDs Vector256
  uint8_t uriTokenIdCount;    // 0..4
  char mintUri[256];          // optional MintURIToken.URI
  char mintDigestHex[65];     // optional 32-byte digest as 64 hex
  bool mintHasDigest;
  uint32_t mintFlags;         // MintURIToken Flags (0 = omit)
  RemitParams()
      : amountCount(0), uriTokenIdCount(0), mintHasDigest(false), mintFlags(0) {
    destination[0] = mintUri[0] = mintDigestHex[0] = '\0';
    for (int i = 0; i < 4; ++i) secureZero(uriTokenIds[i], 32);
  }
};

// ---------------------------------------------------------------------------
// CronSet — schedule hook self-invocations (type 93)
// ---------------------------------------------------------------------------
struct CronSetParams {
  TxCommon common;
  uint32_t startTime;     // ripple epoch; REQUIRED on create (0 = ASAP)
  uint32_t repeatCount;   // 1..256
  uint32_t delaySeconds;
  bool unset;             // Flags tfCronUnset=1; omit scheduling fields
  CronSetParams() : startTime(0), repeatCount(1), delaySeconds(60), unset(false) {}
};

// ---------------------------------------------------------------------------
// SetRemarks — key/value remarks on an owned object (type 94)
// ---------------------------------------------------------------------------
struct SetRemarksParams {
  TxCommon common;
  uint8_t objectId[32];   // required Hash256 of target ledger object
  char remarkName[64];    // raw blob bytes (1..256)
  char remarkValue[128];  // empty = delete remark
  bool immutable;         // Remark Flags tfImmutable=1
  SetRemarksParams() : immutable(false) {
    secureZero(objectId, 32);
    remarkName[0] = remarkValue[0] = '\0';
  }
};

// ---------------------------------------------------------------------------
// URITokenMint (type 45) — simplified mint
// ---------------------------------------------------------------------------
struct URITokenMintParams {
  TxCommon common;
  char uri[256];
  char digestHex[65];  // optional 32-byte digest as 64 hex chars
  bool hasDigest;
  Amount amount;  // optional sale amount (empty = not for sale)
  URITokenMintParams() : hasDigest(false) {
    uri[0] = digestHex[0] = '\0';
  }
};

struct URITokenBurnParams {
  TxCommon common;
  uint8_t uriTokenId[32];
  URITokenBurnParams() { secureZero(uriTokenId, 32); }
};

// ---------------------------------------------------------------------------
// URITokenBuy (type 47) — buy a listed URIToken
// ---------------------------------------------------------------------------
struct URITokenBuyParams {
  TxCommon common;
  uint8_t uriTokenId[32];
  Amount amount;  // payment (must meet/exceed sell offer)
  URITokenBuyParams() { secureZero(uriTokenId, 32); }
};

// ---------------------------------------------------------------------------
// URITokenCreateSellOffer (type 48)
// ---------------------------------------------------------------------------
struct URITokenCreateSellOfferParams {
  TxCommon common;
  uint8_t uriTokenId[32];
  Amount amount;          // ask price
  char destination[36];   // optional buyer restriction
  URITokenCreateSellOfferParams() {
    secureZero(uriTokenId, 32);
    destination[0] = '\0';
  }
};

// ---------------------------------------------------------------------------
// URITokenCancelSellOffer (type 49)
// ---------------------------------------------------------------------------
struct URITokenCancelSellOfferParams {
  TxCommon common;
  uint8_t uriTokenId[32];
  URITokenCancelSellOfferParams() { secureZero(uriTokenId, 32); }
};

// ---------------------------------------------------------------------------
// SetHook (type 22) — install a single hook WASM blob (Tier A: CreateCode <= 8 KiB)
// ---------------------------------------------------------------------------
struct SetHookParams {
  TxCommon common;
  const uint8_t* createCode;  // WASM bytes
  uint16_t createCodeLen;  // Tier A max 8192
  uint8_t hookNamespace[32];  // required namespace hash
  uint8_t hookOn[32];         // HookOn bitmask as 32-byte hash field
  uint16_t hookApiVersion;    // usually 0
  uint32_t hookFlags;         // e.g. 1 = hsfOVERRIDE
  SetHookParams()
      : createCode(nullptr),
        createCodeLen(0),
        hookApiVersion(0),
        hookFlags(1) {
    secureZero(hookNamespace, 32);
    // default HookOn: all bits set — caller should set explicitly for production
    for (int i = 0; i < 32; ++i) hookOn[i] = 0xFF;
  }
};

Status fillInvoke(const InvokeParams&, const uint8_t acc[20], const uint8_t pub[33], TxFieldBag&);
Status fillClaimReward(const ClaimRewardParams&, const uint8_t acc[20], const uint8_t pub[33],
                       TxFieldBag&);
Status fillImport(const ImportParams&, const uint8_t acc[20], const uint8_t pub[33], TxFieldBag&);
Status fillRemit(const RemitParams&, const uint8_t acc[20], const uint8_t pub[33], TxFieldBag&);
Status fillCronSet(const CronSetParams&, const uint8_t acc[20], const uint8_t pub[33],
                   TxFieldBag&);
Status fillSetRemarks(const SetRemarksParams&, const uint8_t acc[20], const uint8_t pub[33],
                      TxFieldBag&);
Status fillURITokenMint(const URITokenMintParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);
Status fillURITokenBurn(const URITokenBurnParams&, const uint8_t acc[20], const uint8_t pub[33],
                        TxFieldBag&);
Status fillURITokenBuy(const URITokenBuyParams&, const uint8_t acc[20], const uint8_t pub[33],
                       TxFieldBag&);
Status fillURITokenCreateSellOffer(const URITokenCreateSellOfferParams&, const uint8_t acc[20],
                                   const uint8_t pub[33], TxFieldBag&);
Status fillURITokenCancelSellOffer(const URITokenCancelSellOfferParams&, const uint8_t acc[20],
                                   const uint8_t pub[33], TxFieldBag&);
Status fillSetHook(const SetHookParams&, const uint8_t acc[20], const uint8_t pub[33],
                   TxFieldBag&);

Status buildAndSignInvoke(const KeyPair&, const InvokeParams&, char* outHex, size_t outHexCap);
Status buildAndSignClaimReward(const KeyPair&, const ClaimRewardParams&, char*, size_t);
Status buildAndSignRemit(const KeyPair&, const RemitParams&, char*, size_t);
Status buildAndSignImport(const KeyPair&, const ImportParams&, char*, size_t);
Status buildAndSignCronSet(const KeyPair&, const CronSetParams&, char*, size_t);
Status buildAndSignSetRemarks(const KeyPair&, const SetRemarksParams&, char*, size_t);
Status buildAndSignURITokenMint(const KeyPair&, const URITokenMintParams&, char*, size_t);
Status buildAndSignURITokenBurn(const KeyPair&, const URITokenBurnParams&, char*, size_t);
Status buildAndSignURITokenBuy(const KeyPair&, const URITokenBuyParams&, char*, size_t);
Status buildAndSignURITokenCreateSellOffer(const KeyPair&, const URITokenCreateSellOfferParams&,
                                           char*, size_t);
Status buildAndSignURITokenCancelSellOffer(const KeyPair&, const URITokenCancelSellOfferParams&,
                                           char*, size_t);
Status buildAndSignOracleSet(const KeyPair&, const OracleSetParams&, char*, size_t);

// Re-export OracleSetParams from phase4 — build helper lives here for umbrella ease.
// (fillOracleSet is in phase4_extra.h)


// ---------------------------------------------------------------------------
// Cron (type 92) — network-emitted pseudo-tx (userSubmittable=false)
// No fill*/buildAndSign*. Minimal field-walker decode for known Cron fields
// (UInt16/UInt32/Amount-XRP/Blob/AccountID). Not a general STObject reader.
// ---------------------------------------------------------------------------
struct CronEmittedView {
  uint32_t sequence;
  uint32_t parentSequence;   // best-effort; 0 if absent (no sfParentSequence in defs)
  uint32_t ledgerSequence;   // sfLedgerSequence when present
  char owner[36];            // classic address; empty if absent
  CronEmittedView() : sequence(0), parentSequence(0), ledgerSequence(0) { owner[0] = '\0'; }
};
/** Decode emitted Cron blob. Requires TransactionType==92. */
Status decodeEmittedCron(const uint8_t* blob, size_t len, CronEmittedView& out);
}  // namespace xahau
