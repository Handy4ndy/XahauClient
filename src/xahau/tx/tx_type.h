// tx_type.h - complete Xahau transaction type catalog

// Codes match live xahaud server_definitions (mainnet 21337, 2026.6.21-release+3350).

#pragma once



#include <stddef.h>

#include <stdint.h>



namespace xahau {



enum class TxType : uint16_t {

  // --- Classic / shared lineage ---

  Payment = 0,

  EscrowCreate = 1,

  EscrowFinish = 2,

  AccountSet = 3,

  EscrowCancel = 4,

  SetRegularKey = 5,

  NicknameSet = 6,  // legacy

  OfferCreate = 7,

  OfferCancel = 8,

  Contract = 9,  // legacy

  TicketCreate = 10,

  SpinalTap = 11,  // legacy

  SignerListSet = 12,

  PaymentChannelCreate = 13,

  PaymentChannelFund = 14,

  PaymentChannelClaim = 15,

  CheckCreate = 16,

  CheckCash = 17,

  CheckCancel = 18,

  DepositPreauth = 19,

  TrustSet = 20,

  AccountDelete = 21,



  // --- Hooks (Xahau) ---

  SetHook = 22,



  // NFToken codes retained in definitions (URIToken is the live NFT model)

  NFTokenMint = 25,

  NFTokenBurn = 26,

  NFTokenCreateOffer = 27,

  NFTokenCancelOffer = 28,

  NFTokenAcceptOffer = 29,

  Clawback = 30,

  AMMClawback = 31,

  AMMCreate = 35,

  AMMDeposit = 36,

  AMMWithdraw = 37,

  AMMVote = 38,

  AMMBid = 39,

  AMMDelete = 40,



  // --- URIToken (Xahau native NFT) ---

  URITokenMint = 45,

  URITokenBurn = 46,

  URITokenBuy = 47,

  URITokenCreateSellOffer = 48,

  URITokenCancelSellOffer = 49,



  // --- XChain (Xahau codes differ from XRPL) ---

  XChainCreateClaimID = 50,

  XChainCommit = 51,

  XChainClaim = 52,

  XChainAccountCreateCommit = 53,

  XChainAddClaimAttestation = 54,

  XChainAddAccountCreateAttestation = 55,

  XChainModifyBridge = 56,

  XChainCreateBridge = 57,



  DIDSet = 58,

  DIDDelete = 59,



  // --- PriceOracle (enabled on Xahau mainnet) ---

  OracleSet = 60,

  OracleDelete = 61,



  LedgerStateFix = 62,

  MPTokenIssuanceCreate = 63,

  MPTokenIssuanceDestroy = 64,

  MPTokenIssuanceSet = 65,

  MPTokenAuthorize = 66,

  CredentialCreate = 67,

  CredentialAccept = 68,

  CredentialDelete = 69,

  NFTokenModify = 70,

  PermissionedDomainSet = 71,

  PermissionedDomainDelete = 72,



  // --- XRPL-lineage catalog codes (NOT live on Xahau; encode/sign helpers removed 2026-09-28) ---

  // Retained for decode/catalog name lookup only; do not submit.

  Batch = 200,

  DelegateSet = 201,

  VaultCreate = 202,

  VaultSet = 203,

  VaultDelete = 204,

  VaultDeposit = 205,

  VaultWithdraw = 206,

  VaultClawback = 207,

  LoanBrokerSet = 208,

  LoanBrokerDelete = 209,

  LoanBrokerCoverDeposit = 210,

  LoanBrokerCoverWithdraw = 211,

  LoanBrokerCoverClawback = 212,

  LoanSet = 213,

  LoanDelete = 214,

  LoanManage = 215,

  LoanPay = 216,



  // --- Xahau-native extensions ---

  Cron = 92,

  CronSet = 93,

  SetRemarks = 94,

  Remit = 95,

  GenesisMint = 96,

  Import = 97,

  ClaimReward = 98,

  Invoke = 99,



  // --- Pseudo / network ---

  EnableAmendment = 100,

  SetFee = 101,

  UNLModify = 102,

  EmitFailure = 103,

  UNLReport = 104,



  Invalid = 0xFFFF,

};



/** Implementation maturity for a registered type. */

enum class TxMaturity : uint8_t {

  Catalog = 0,   // name + type code only

  Encode = 1,    // can place fields in bag / serialize

  Validate = 2,  // typed param validation

  Golden = 3,    // host vector matches reference

};



struct TxTypeInfo {

  TxType type;

  uint16_t typeCode;

  const char* name;

  bool userSubmittable;

  TxMaturity maturity;

};



const TxTypeInfo* txTypeCatalog();

size_t txTypeCatalogCount();



const TxTypeInfo* txTypeLookup(TxType t);

const TxTypeInfo* txTypeLookupByCode(uint16_t typeCode);

const TxTypeInfo* txTypeLookupByName(const char* name);



const char* txTypeName(TxType t);



/**

 * Special transaction cost floor (drops of XAH).

 * AccountDelete must destroy at least the incremental owner reserve.

 * Returns 0 when the type uses the ordinary reference fee.

 *

 * SetHook: soft floor only. Novel CreateCode installs need ~500 drops/byte

 * (see setHookCreateFeeDrops). Prefer fee RPC with tx_blob for exact fee

 * (execution + params + create/install). prepareCommonForTx applies this

 * soft floor; sketches must also ensureFeeAtLeast(setHookCreateFeeDrops(len)).

 */

inline uint64_t txMinFeeDrops(TxType t) {

  switch (t) {

    case TxType::AccountDelete:

      return 200000ull;

    case TxType::SetHook:

      // Soft floor; size CreateCode via setHookCreateFeeDrops / fee+tx_blob.

      return 1000ull;

    default:

      return 0;

  }

}



/** Novel SetHook CreateCode storage fee: 500 drops per WASM byte (Xahau Hooks). */

constexpr uint64_t kSetHookCreateCodeDropsPerByte = 500ull;

/** Network reference/base fee included in fee+tx_blob base_fee (Xahau testnet = 10). */

constexpr uint64_t kSetHookBaseFeeDrops = 10ull;



/**

 * Local CreateCode fee floor: createCodeLen * 500 + base(10).

 * Matches fee RPC base_fee/minimum_fee/open_ledger_fee for starter BIP SetHook

 * (830510 for 1661-byte CreateCode). Still not a substitute for fee+tx_blob when

 * params/execution differ, but avoids underpay by 10 that left txs unvalidated.

 */

inline uint64_t setHookCreateFeeDrops(uint16_t createCodeLen) {

  return static_cast<uint64_t>(createCodeLen) * kSetHookCreateCodeDropsPerByte + kSetHookBaseFeeDrops;

}



inline bool txHasSpecialFee(TxType t) { return txMinFeeDrops(t) > 0; }



}  // namespace xahau

