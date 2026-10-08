// xahau_types.h — shared value types (portable)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../platform/xahau_platform.h"

namespace xahau {

// ---------------------------------------------------------------------------
// Curves / keys
// ---------------------------------------------------------------------------

enum class Curve : uint8_t {
  Unknown = 0,
  Ed25519 = 1,
  Secp256k1 = 2,
};

struct KeyPair {
  uint8_t privateKey[32];
  uint8_t publicKey[33];  // 0xED || pub  or  compressed secp
  uint8_t accountId[20];
  char address[36];       // classic r-address, null-terminated
  Curve curve;

  KeyPair() : curve(Curve::Unknown) {
    secureZero(privateKey, sizeof(privateKey));
    secureZero(publicKey, sizeof(publicKey));
    secureZero(accountId, sizeof(accountId));
    secureZero(address, sizeof(address));
  }
};

// ---------------------------------------------------------------------------
// Amounts
// ---------------------------------------------------------------------------

enum class AmountKind : uint8_t {
  None = 0,
  XrpDrops = 1,
  Issued = 2,
  Mpt = 3,  // Multi-purpose token (value + 192-bit issuance id)
};

struct IssuedAmount {
  char value[32];      // decimal string, e.g. "1000" or "1.5"
  char currency[41];   // 3-char ASCII or 40-char hex currency code
  char issuer[36];     // r-address
};

/** MPT amount: integer value (base units) + 24-byte MPTokenIssuanceID. */
struct MptAmount {
  uint64_t value;       // 0 .. 2^63-1
  uint8_t issuanceId[24];
  MptAmount() : value(0) { secureZero(issuanceId, 24); }
};

struct Amount {
  AmountKind kind;
  uint64_t drops;      // when XrpDrops
  IssuedAmount issued; // when Issued
  MptAmount mpt;       // when Mpt

  Amount() : kind(AmountKind::None), drops(0) {
    secureZero(&issued, sizeof(issued));
  }
};

// ---------------------------------------------------------------------------
// Common tx envelope fields (filled by builders / client)
// ---------------------------------------------------------------------------

struct TxCommon {
  uint32_t flags;
  uint32_t sequence;
  uint32_t firstLedgerSequence;
  uint32_t lastLedgerSequence;
  uint64_t feeDrops;
  uint32_t sourceTag;
  uint32_t destinationTag;
  uint32_t ticketSequence;
  /**
   * On-wire NetworkID (UInt32 type=2 field=1).
   * Required for Xahau (mainnet 21337, testnet 21338) — network IDs >= 1025.
   * Set 0 to omit (invalid on Xahau public nets).
   */
  uint32_t networkId;

  TxCommon()
      : flags(0),
        sequence(0),
        firstLedgerSequence(0),
        lastLedgerSequence(0),
        feeDrops(12),
        sourceTag(0),
        destinationTag(0),
        ticketSequence(0),
        networkId(0) {}
};

// ---------------------------------------------------------------------------
// Field identity (type << 16 | nth) — matches Xahau binary fieldCode
// ---------------------------------------------------------------------------

constexpr uint32_t fieldCode(uint16_t type, uint16_t nth) {
  return (static_cast<uint32_t>(type) << 16) | nth;
}

// ---------------------------------------------------------------------------
// Time epochs
// ---------------------------------------------------------------------------

/** Seconds from Unix epoch (1970) to Ripple epoch (2000-01-01 UTC). */
constexpr uint32_t kRippleEpochOffset = 946684800u;

/** Ledger close_time is Ripple Epoch; Oracle LastUpdateTime is Unix Epoch (XLS-47). */
inline uint32_t unixToRippleTime(uint32_t unixSec) {
  if (unixSec < kRippleEpochOffset) return 0;
  return unixSec - kRippleEpochOffset;
}

inline uint32_t rippleToUnixTime(uint32_t rippleSec) {
  return rippleSec + kRippleEpochOffset;
}

/**
 * Normalize OracleSet LastUpdateTime to Unix seconds.
 * Accepts Unix (~1.7e9) or accidental Ripple (~8e8) inputs.
 * Must be within ±300s of ledger close (as Unix) or → tecINVALID_UPDATE_TIME.
 */
inline uint32_t normalizeOracleUpdateTime(uint32_t t) {
  // Ripple-sized values in the 2020s are ~7e8–9e8; treat as ledger close units.
  if (t > 0 && t < 1500000000u) return rippleToUnixTime(t);
  return t;
}

/**
 * Build a safe Oracle LastUpdateTime (Unix) from validated ledger close_time (Ripple).
 * Uses closeTime-1 in Unix domain so we are never ahead of the network clock.
 */
inline uint32_t oracleUpdateTimeFromLedgerClose(uint32_t closeTimeRipple) {
  if (closeTimeRipple == 0) return 0;
  uint32_t closeUnix = rippleToUnixTime(closeTimeRipple);
  return (closeUnix > 1) ? (closeUnix - 1) : closeUnix;
}

// Common field codes (subset; full set lives in generated tables)
namespace fc {
  constexpr uint32_t TransactionType     = fieldCode(1, 2);
  /** NetworkID — type=2 field=1 header 0x21; required on Xahau (IDs >= 1025). */
  constexpr uint32_t NetworkID           = fieldCode(2, 1);
  constexpr uint32_t Flags               = fieldCode(2, 2);
  constexpr uint32_t SourceTag           = fieldCode(2, 3);
  constexpr uint32_t Sequence            = fieldCode(2, 4);
  constexpr uint32_t DestinationTag      = fieldCode(2, 14);
  constexpr uint32_t FirstLedgerSequence = fieldCode(2, 26);
  constexpr uint32_t LastLedgerSequence  = fieldCode(2, 27);
  constexpr uint32_t Expiration          = fieldCode(2, 10);
  constexpr uint32_t TransferRate        = fieldCode(2, 11);
  constexpr uint32_t WalletSize          = fieldCode(2, 12);
  constexpr uint32_t QualityIn           = fieldCode(2, 20);
  constexpr uint32_t QualityOut          = fieldCode(2, 21);
  constexpr uint32_t OfferSequence       = fieldCode(2, 25);
  constexpr uint32_t SetFlag             = fieldCode(2, 33);
  constexpr uint32_t ClearFlag           = fieldCode(2, 34);
  constexpr uint32_t SignerQuorum        = fieldCode(2, 35);
  constexpr uint32_t CancelAfter         = fieldCode(2, 36);
  constexpr uint32_t FinishAfter         = fieldCode(2, 37);
  constexpr uint32_t SettleDelay         = fieldCode(2, 39);
  constexpr uint32_t TicketCount         = fieldCode(2, 40);
  constexpr uint32_t TicketSequence      = fieldCode(2, 41);
  constexpr uint32_t NFTokenTaxon        = fieldCode(2, 42);
  constexpr uint32_t LastUpdateTime      = fieldCode(2, 15);  // 0x2F
  constexpr uint32_t OracleDocumentID    = fieldCode(2, 51);
  constexpr uint32_t PermissionValue     = fieldCode(2, 52);
  constexpr uint32_t TransferFee         = fieldCode(1, 4);   // UInt16 (NFToken royalty bps)
  constexpr uint32_t TradingFee          = fieldCode(1, 5);   // UInt16
  constexpr uint32_t XChainClaimID       = fieldCode(3, 20);  // UInt64
  constexpr uint32_t MaximumAmount       = fieldCode(3, 24);  // UInt64
  constexpr uint32_t Amount              = fieldCode(6, 1);
  constexpr uint32_t Balance             = fieldCode(6, 2);
  constexpr uint32_t LimitAmount         = fieldCode(6, 3);
  constexpr uint32_t TakerPays           = fieldCode(6, 4);
  constexpr uint32_t TakerGets           = fieldCode(6, 5);
  constexpr uint32_t Fee                 = fieldCode(6, 8);
  constexpr uint32_t SendMax             = fieldCode(6, 9);
  constexpr uint32_t Amount2             = fieldCode(6, 11);
  constexpr uint32_t LPTokenOut          = fieldCode(6, 25);
  constexpr uint32_t LPTokenIn           = fieldCode(6, 26);
  constexpr uint32_t EPrice              = fieldCode(6, 27);
  constexpr uint32_t SignatureReward     = fieldCode(6, 29);
  constexpr uint32_t MinAccountCreateAmount = fieldCode(6, 30);
  constexpr uint32_t EmailHash           = fieldCode(4, 1);
  constexpr uint32_t NFTokenID           = fieldCode(5, 10);
  constexpr uint32_t Channel             = fieldCode(5, 22);
  constexpr uint32_t CheckID             = fieldCode(5, 24);
  constexpr uint32_t NFTokenBuyOffer     = fieldCode(5, 28);
  constexpr uint32_t NFTokenSellOffer    = fieldCode(5, 29);
  constexpr uint32_t DomainID            = fieldCode(5, 34);
  constexpr uint32_t VaultID             = fieldCode(5, 35);
  constexpr uint32_t LoanBrokerID        = fieldCode(5, 37);
  constexpr uint32_t LoanID              = fieldCode(5, 38);
  constexpr uint32_t PublicKey           = fieldCode(7, 1);
  constexpr uint32_t SigningPubKey       = fieldCode(7, 3);
  constexpr uint32_t TxnSignature        = fieldCode(7, 4);
  constexpr uint32_t URI                 = fieldCode(7, 5);
  constexpr uint32_t Signature           = fieldCode(7, 6);
  constexpr uint32_t Fulfillment         = fieldCode(7, 16);
  constexpr uint32_t Condition           = fieldCode(7, 17);
  constexpr uint32_t Domain              = fieldCode(7, 7);
  constexpr uint32_t MessageKey          = fieldCode(7, 8);
  constexpr uint32_t DIDDocument         = fieldCode(7, 26);
  constexpr uint32_t Data                = fieldCode(7, 27);
  // Xahau PriceOracle (XLS-47d port): AssetClass nth=29, Provider nth=30
  constexpr uint32_t AssetClass          = fieldCode(7, 29);  // Blob (OracleSet create)
  constexpr uint32_t Provider            = fieldCode(7, 30);  // Blob (OracleSet create)
  constexpr uint32_t CreateCode          = fieldCode(7, 11);  // Blob (SetHook WASM)
  constexpr uint32_t Blob                = fieldCode(7, 26);  // Import xPOP blob
  constexpr uint32_t HookName            = fieldCode(7, 97);  // Blob
  constexpr uint32_t RemarkName          = fieldCode(7, 99);
  constexpr uint32_t RemarkValue         = fieldCode(7, 98);
  constexpr uint32_t URITokenID          = fieldCode(5, 36);  // Hash256
  constexpr uint32_t Digest              = fieldCode(5, 21);  // Hash256
  constexpr uint32_t ObjectID            = fieldCode(5, 14);  // Hash256 (SetRemarks target)
  constexpr uint32_t OperationLimit      = fieldCode(2, 29);
  constexpr uint32_t StartTime           = fieldCode(2, 93);
  constexpr uint32_t RepeatCount         = fieldCode(2, 94);
  constexpr uint32_t DelaySeconds        = fieldCode(2, 95);
  constexpr uint32_t HookOn              = fieldCode(5, 20);  // Hash256
  constexpr uint32_t HookNamespace       = fieldCode(5, 32);  // Hash256
  constexpr uint32_t HookApiVersion      = fieldCode(1, 20);  // UInt16
  constexpr uint32_t Hooks               = fieldCode(15, 11); // STArray
  constexpr uint32_t HookParameters      = fieldCode(15, 19);
  constexpr uint32_t Remarks             = fieldCode(15, 97);
  constexpr uint32_t Amounts             = fieldCode(15, 92); // Remit STArray
  constexpr uint32_t MintURIToken        = fieldCode(14, 92); // Remit STObject
  constexpr uint32_t URITokenIDs         = fieldCode(19, 99); // Vector256
  constexpr uint32_t CredentialType      = fieldCode(7, 31);
  constexpr uint32_t Account             = fieldCode(8, 1);
  constexpr uint32_t Owner               = fieldCode(8, 2);
  constexpr uint32_t Destination         = fieldCode(8, 3);
  constexpr uint32_t Issuer              = fieldCode(8, 4);
  constexpr uint32_t Authorize           = fieldCode(8, 5);
  constexpr uint32_t RegularKey          = fieldCode(8, 8);
  constexpr uint32_t Holder              = fieldCode(8, 11);
  constexpr uint32_t NFTokenMinter       = fieldCode(8, 16);
  constexpr uint32_t OtherChainSource    = fieldCode(8, 18);
  constexpr uint32_t Subject             = fieldCode(8, 24);
  constexpr uint32_t OtherChainDestination = fieldCode(8, 19);
  constexpr uint32_t AttestationSignerAccount = fieldCode(8, 20);
  constexpr uint32_t AttestationRewardAccount = fieldCode(8, 21);
  constexpr uint32_t Counterparty        = fieldCode(8, 26);
  constexpr uint32_t WasLockingChainSend = fieldCode(16, 19);
  constexpr uint32_t AssetsMaximum       = fieldCode(9, 3);   // NUMBER (use Embedded body)
  constexpr uint32_t PrincipalRequested  = fieldCode(9, 14);  // NUMBER (LoanSet)
  constexpr uint32_t PaymentInterval     = fieldCode(2, 55);
  constexpr uint32_t GracePeriod         = fieldCode(2, 56);
  constexpr uint32_t PaymentTotal        = fieldCode(2, 60);
  constexpr uint32_t InterestRate        = fieldCode(2, 65);
  constexpr uint32_t TickSize            = fieldCode(16, 16);
  constexpr uint32_t AssetScale          = fieldCode(16, 5);
  constexpr uint32_t Signers             = fieldCode(15, 3);
  constexpr uint32_t SignerEntries       = fieldCode(15, 4);
  constexpr uint32_t Memos               = fieldCode(15, 9);
  constexpr uint32_t PriceDataSeries     = fieldCode(15, 24);
  constexpr uint32_t AcceptedCredentials = fieldCode(15, 28);
  constexpr uint32_t Permissions         = fieldCode(15, 29);
  constexpr uint32_t RawTransactions     = fieldCode(15, 30);
  constexpr uint32_t BatchSigners        = fieldCode(15, 31);
  constexpr uint32_t NFTokenOffers       = fieldCode(19, 4);
  // STObject fields
  constexpr uint32_t Signer              = fieldCode(14, 16);  // inner multi-sign Signer
  constexpr uint32_t RawTransaction      = fieldCode(14, 34);
  constexpr uint32_t CounterpartySignature = fieldCode(14, 37);
  constexpr uint32_t MPTokenIssuanceID   = fieldCode(21, 1);  // UInt192
  constexpr uint32_t Asset               = fieldCode(24, 3);  // ISSUE
  constexpr uint32_t Asset2              = fieldCode(24, 4);
  constexpr uint32_t ClaimCurrency       = fieldCode(24, 5);  // ISSUE (IOURewardClaim)
  constexpr uint32_t XChainBridge        = fieldCode(25, 1);
}

// ST type codes used in field headers
namespace st {
  constexpr uint8_t UInt16 = 1;
  constexpr uint8_t UInt32 = 2;
  constexpr uint8_t UInt64 = 3;
  constexpr uint8_t Hash128 = 4;
  constexpr uint8_t Hash256 = 5;
  constexpr uint8_t Amount = 6;
  constexpr uint8_t Blob = 7;
  constexpr uint8_t AccountID = 8;
  constexpr uint8_t STArray = 15;
  constexpr uint8_t UInt8 = 16;
  constexpr uint8_t STObject = 14;
  constexpr uint8_t PathSet = 18;
  constexpr uint8_t Vector256 = 19;
}

}  // namespace xahau
