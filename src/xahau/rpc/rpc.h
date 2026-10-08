// rpc.h — pure JSON-RPC request builders + response parsers (no I/O)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

// ---------------------------------------------------------------------------
// Request builders (null-terminated JSON into out)
// ---------------------------------------------------------------------------

Status rpcBuildAccountInfo(const char* account, char* out, size_t outCap);
/** ledger_index: "current" | "validated" | "closed" or decimal string. */
Status rpcBuildAccountInfoEx(const char* account, const char* ledgerIndex, char* out,
                             size_t outCap);

Status rpcBuildSubmit(const char* txBlobHex, char* out, size_t outCap);
/** fail_hard=true asks rippled not to queue soft failures when possible. */
Status rpcBuildSubmitEx(const char* txBlobHex, bool failHard, char* out, size_t outCap);

Status rpcBuildServerInfo(char* out, size_t outCap);
Status rpcBuildServerState(char* out, size_t outCap);
Status rpcBuildFee(char* out, size_t outCap);
/** Unsigned/dummy Fee=0 tx hex for exact Hooks fee (CreateCode + params + execution). */
Status rpcBuildFeeWithTxBlob(const char* txBlobHex, char* out, size_t outCap);
Status rpcBuildLedger(const char* ledgerIndex, bool transactions, char* out, size_t outCap);
Status rpcBuildLedgerClosed(char* out, size_t outCap);
Status rpcBuildPing(char* out, size_t outCap);

/**
 * Subscribe / unsubscribe (Xahau subscription methods).
 *
 * Stream names allowlisted: ledger, validations, transactions,
 * transactions_proposed, consensus, server, peer_status, manifests, book_changes.
 * - peer_status: admin-only on rippled (public clusters reject).
 * - server / consensus / peer_status: not available on Clio (reportingUnsupported).
 * Accounts / accounts_proposed: classic r-addresses (Base58Check).
 * Admin url callbacks are not supported (non-goal for embedded clients).
 *
 * Prefer wallet path: streams=["ledger"] + accounts=[r...] (not global transactions).
 */
struct SubscribeParams {
  const char* const* streams;
  size_t streamCount;
  const char* const* accounts;
  size_t accountCount;
  const char* const* accountsProposed;
  size_t accountsProposedCount;
};

Status rpcBuildSubscribeStreams(const char* const* streams, size_t streamCount, char* out,
                                size_t outCap);
Status rpcBuildSubscribeAccounts(const char* const* accounts, size_t accountCount, char* out,
                                 size_t outCap);
Status rpcBuildSubscribeAccountsProposed(const char* const* accounts, size_t accountCount, char* out,
                                         size_t outCap);
/** Streams and/or accounts (at least one side non-empty). */
Status rpcBuildSubscribe(const char* const* streams, size_t streamCount,
                         const char* const* accounts, size_t accountCount, char* out,
                         size_t outCap);
/** Full public fields: streams + accounts + accounts_proposed (books deferred). */
Status rpcBuildSubscribeEx(const SubscribeParams& p, char* out, size_t outCap);
Status rpcBuildUnsubscribeStreams(const char* const* streams, size_t streamCount, char* out,
                                  size_t outCap);
Status rpcBuildUnsubscribeAccounts(const char* const* accounts, size_t accountCount, char* out,
                                   size_t outCap);
Status rpcBuildUnsubscribeAccountsProposed(const char* const* accounts, size_t accountCount,
                                           char* out, size_t outCap);
Status rpcBuildUnsubscribe(const char* const* streams, size_t streamCount,
                           const char* const* accounts, size_t accountCount, char* out,
                           size_t outCap);
Status rpcBuildUnsubscribeEx(const SubscribeParams& p, char* out, size_t outCap);
/** Convenience: subscribe streams=["ledger"] only. */
Status rpcBuildSubscribeLedger(char* out, size_t outCap);
Status rpcBuildUnsubscribeLedger(char* out, size_t outCap);

Status rpcBuildAccountLines(const char* account, char* out, size_t outCap);
Status rpcBuildAccountTx(const char* account, uint32_t limit, char* out, size_t outCap);
Status rpcBuildAccountObjects(const char* account, const char* typeFilter, char* out,
                              size_t outCap);

// --- Wallet-priority public methods (WebSocket API catalog subset) ---
// Account inventory / settings used by wallet UI (lists, tokens, NFTs, offers).
Status rpcBuildAccountChannels(const char* account, char* out, size_t outCap);
Status rpcBuildAccountCurrencies(const char* account, char* out, size_t outCap);
Status rpcBuildAccountNfts(const char* account, uint32_t limit, char* out, size_t outCap);
Status rpcBuildAccountOffers(const char* account, uint32_t limit, char* out, size_t outCap);
Status rpcBuildGatewayBalances(const char* account, char* out, size_t outCap);
/** role: "send" or "receive" (noripple_check). */
Status rpcBuildNoRippleCheck(const char* account, const char* role, char* out, size_t outCap);

// Ledger lookups
Status rpcBuildLedgerCurrent(char* out, size_t outCap);
Status rpcBuildLedgerData(const char* ledgerIndex, uint32_t limit, char* out, size_t outCap);
/** ledger_entry by 64-hex object index. */
Status rpcBuildLedgerEntryByIndex(const char* indexHex64, char* out, size_t outCap);
/** ledger_entry AccountRoot for classic address. */
Status rpcBuildLedgerEntryAccountRoot(const char* account, char* out, size_t outCap);

// Transactions
Status rpcBuildTx(const char* txHashHex, bool binary, char* out, size_t outCap);
Status rpcBuildSubmitMultisigned(const char* txJson, char* out, size_t outCap);
/** transaction_entry — tx must be in the specified ledger. */
Status rpcBuildTransactionEntry(const char* txHashHex, const char* ledgerIndex, char* out,
                                size_t outCap);

// Path / DEX / deposit (minimal params for wallet checks)
Status rpcBuildDepositAuthorized(const char* sourceAccount, const char* destinationAccount,
                                 char* out, size_t outCap);
/* AMM info JSON builder removed 2026-09-28 (EncodeOnly cut). */
/**
 * book_offers: XAH/XRP-style vs issued currency (empty issuer => native on that side is invalid —
 * pass currency "XAH" (or "XRP" alias) with null/empty issuer for pure native side).
 * gets/pays: ISO code or 40-hex currency; issuer classic address or empty for native (XAH).
 */
Status rpcBuildBookOffers(const char* takerGetsCurrency, const char* takerGetsIssuer,
                          const char* takerPaysCurrency, const char* takerPaysIssuer,
                          uint32_t limit, char* out, size_t outCap);
Status rpcBuildNftBuyOffers(const char* nftIdHex, uint32_t limit, char* out, size_t outCap);
Status rpcBuildNftSellOffers(const char* nftIdHex, uint32_t limit, char* out, size_t outCap);

// Payment channels
Status rpcBuildChannelVerify(const char* channelIdHex, const char* amountDrops,
                             const char* publicKeyHex, const char* signatureHex, char* out,
                             size_t outCap);

// Server / utility
Status rpcBuildFeature(char* out, size_t outCap);
Status rpcBuildServerDefinitions(char* out, size_t outCap);
Status rpcBuildManifest(const char* publicKeyBase58, char* out, size_t outCap);
Status rpcBuildRandom(char* out, size_t outCap);

// Clio NFT (wallet NFT studio)
Status rpcBuildNftInfo(const char* nftIdHex, char* out, size_t outCap);
Status rpcBuildNftHistory(const char* nftIdHex, uint32_t limit, char* out, size_t outCap);
Status rpcBuildNftsByIssuer(const char* issuer, uint32_t limit, char* out, size_t outCap);

// ---------------------------------------------------------------------------
// Response parsers (tolerant substring / lightweight)
// ---------------------------------------------------------------------------

Status rpcParseAccountSequence(const char* json, uint32_t& sequence, uint32_t& ledgerIndex);

/** Balance in drops from account_data.Balance (string). */
Status rpcParseAccountBalanceDrops(const char* json, uint64_t& drops);

Status rpcParseSubmitEngineResult(const char* json, char* resultOut, size_t resultCap,
                                  char* hashOut, size_t hashCap);

/** true if engine_result starts with "tes". */
bool rpcIsTesSuccess(const char* engineResult);

/** true if engine_result is terQUEUED (provisional open-ledger accept). */
bool rpcIsTerQueued(const char* engineResult);

/** Engine class: tes / ter / tec / tef / tem / other. */
enum class EngineClass : uint8_t {
  Unknown = 0,
  Tes,  // success
  Ter,  // temporary / retryable (includes QUEUED)
  Tec,  // claimed fee, failed
  Tef,  // failure, no fee (e.g. PAST_SEQ)
  Tem,  // malformed
};

EngineClass rpcEngineClass(const char* engineResult);

/**
 * Parse final TransactionResult / engine_result from a `tx` RPC response.
 * Prefers meta.TransactionResult when present.
 */
Status rpcParseTxEngineResult(const char* json, char* resultOut, size_t resultCap,
                              bool* outValidated);

/**
 * Fee drops from fee RPC: max(open_ledger_fee, base_fee, minimum_fee);
 * if median_fee is higher and &lt; 100000, use median (load hint).
 */
Status rpcParseFeeDrops(const char* json, uint64_t& feeDrops);

/** validated ledger index from server_info or ledger response. */
Status rpcParseLedgerIndex(const char* json, uint32_t& ledgerIndex);

/**
 * Parse ledger close_time (Ripple Epoch seconds) from a `ledger` response.
 * Tries "close_time" first, then "ledger_time".
 */
Status rpcParseLedgerCloseTime(const char* json, uint32_t& closeTime);

/** Generic: extract first quoted string value for a JSON key. */
Status rpcParseStringField(const char* json, const char* keyQuoted, char* out, size_t outCap);

/** Generic: extract first integer/u64 value after a key. */
Status rpcParseUIntField(const char* json, const char* keyQuoted, uint64_t& out);

/** Check top-level "error" string if present. */
Status rpcParseError(const char* json, char* errOut, size_t errCap);

// ---------------------------------------------------------------------------
// High-level sequence + LLS helper values
// ---------------------------------------------------------------------------

struct AccountInfoSnapshot {
  uint32_t sequence;
  uint32_t ledgerIndex;
  uint64_t balanceDrops;
  uint32_t ownerCount;
  bool found;
};

Status rpcParseAccountInfoSnapshot(const char* json, AccountInfoSnapshot& out);

// ---------------------------------------------------------------------------
// account_objects helpers (generic object ID scrape)
// ---------------------------------------------------------------------------

#ifndef XAHAU_MAX_OBJECT_IDS
#define XAHAU_MAX_OBJECT_IDS 8
#endif

/**
 * Collect up to maxIds 32-byte indexes from "index":"<64 hex>" occurrences in JSON.
 * Optionally require a nearby type marker (e.g. "\"LedgerEntryType\":\"Escrow\"") —
 * pass typeMarker=nullptr to accept all indexes.
 */
Status rpcParseObjectIds(const char* json, const char* typeMarker, uint8_t outIds[][32],
                         uint8_t maxIds, uint8_t& outCount);

/** First object index only (convenience). */
Status rpcParseFirstObjectId(const char* json, const char* typeMarker, uint8_t outId[32]);


/** Decode 64-char hex into 32 bytes. */
Status rpcHexToHash256(const char* hex64, uint8_t out[32]);

// ---------------------------------------------------------------------------
// Stream (subscription push) parsers — HandySockets / ledger-monitor style
// ---------------------------------------------------------------------------

enum class StreamMessageType : uint8_t {
  Unknown = 0,
  LedgerClosed,
  Transaction,
  Validation,
  ConsensusPhase,
  ServerStatus,
  BookChanges,
  ManifestReceived,
  PeerStatusChange,
  Response,  // JSON-RPC response (has result/status), not a stream push
};

/** Classify a WS TEXT frame by "type" / response shape. */
Status rpcParseStreamMessageType(const char* json, StreamMessageType& out);

/**
 * ledgerClosed stream fields (subset used by monitors).
 * Struct may grow — zero-initialize whole struct before parse.
 * hash is 64 hex chars + NUL when present.
 */
struct LedgerClosedEvent {
  uint32_t ledgerIndex;
  char ledgerHash[65];
  uint32_t txnCount;
  uint32_t ledgerTime;  // Xahau epoch seconds when present
  bool valid;
  // Appended fields (optional when missing in JSON):
  uint32_t feeBase;
  uint32_t reserveBase;
  uint32_t reserveInc;
  uint32_t networkId;
};

Status rpcParseLedgerClosedEvent(const char* json, LedgerClosedEvent& out);

/** Optional long field — caller supplies buffer (range string can be large). */
Status rpcParseLedgerClosedValidatedLedgers(const char* json, char* out, size_t outCap);

/**
 * Transaction stream: type "transaction" (accounts / books / global streams).
 * Prefers API v2 top-level hash/engine_result; also scans nested tx_json fields.
 * Struct may grow — zero-initialize whole struct before parse.
 */
struct TransactionStreamEvent {
  char hash[65];
  char txType[32];
  char account[36];
  char destination[36];
  bool validated;
  bool valid;
  // Appended:
  char engineResult[32];
  uint32_t ledgerIndex;  // 0 if unvalidated / missing
};

Status rpcParseTransactionStreamEvent(const char* json, TransactionStreamEvent& out);

/**
 * True if JSON-RPC body reports error reportingUnsupported (Clio vs rippled streams).
 */
bool rpcIsReportingUnsupported(const char* json);

}  // namespace xahau

