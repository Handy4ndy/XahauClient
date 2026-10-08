// client.h - high-level RPC client over IXahauTransport
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"
#include "../core/xahau_types.h"
#include "rpc.h"
#include "transport.h"

namespace xahau {

struct SubmitResult {
  char engineResult[32];
  char txHash[80];
  bool tesSuccess;
  /** true when engine_result is terQUEUED (provisional accept). */
  bool queued;
  /** true when `tx` RPC reported validated:true (if known). */
  bool validated;
};

struct FeeAndLedger {
  uint64_t feeDrops;
  uint32_t ledgerIndex;
  /**
   * Suggested LastLedgerSequence = ledgerIndex + effectiveLedgerOffset().
   * effective offset is max(ledgerOffset, kMinLedgerOffset) - see setLedgerOffset.
   */
  uint32_t lastLedgerSequence;
};

/** Floor applied to LLS offset so public-cluster lag does not expire txs early. */
constexpr uint32_t kMinLedgerOffset = 40;

/**
 * High-level client: build request -> transport -> parse.
 * Does not own the transport.
 */
class XahauRpcClient {
 public:
  explicit XahauRpcClient(IXahauTransport& transport);

  void setTimeoutMs(uint32_t ms) { timeoutMs_ = ms; }
  /**
   * Base ledger offset for LastLedgerSequence (default 40).
   * Effective offset is max(offset, kMinLedgerOffset) so values below 40 still
   * get a 40-ledger window (public Testnet lag).
   */
  void setLedgerOffset(uint32_t offset) { ledgerOffset_ = offset; }
  /** Effective LLS offset: max(ledgerOffset_, kMinLedgerOffset). */
  uint32_t effectiveLedgerOffset() const {
    return ledgerOffset_ < kMinLedgerOffset ? kMinLedgerOffset : ledgerOffset_;
  }
  void setResponseBuffer(char* buf, size_t cap);

  Status ping();
  Status fetchAccountInfo(const char* account, AccountInfoSnapshot& out);
  /**
   * Sequence + LastLedgerSequence for signing.
   * LLS = ledgerIndex + ledgerOffset_ (or sequence + offset if ledger unknown).
   */
  Status fetchSequenceAndLls(const char* account, uint32_t& sequence, uint32_t& lastLedgerSequence);
  Status fetchFeeDrops(uint64_t& feeDrops);
  Status fetchFeeAndLedger(FeeAndLedger& out);
  /**
   * Validated ledger close_time in Ripple Epoch seconds (for Oracle LastUpdateTime).
   * Prefer this over device NTP - NTP often runs ahead of UNL clocks -> tecINVALID_UPDATE_TIME.
   */
  Status fetchLedgerCloseTime(uint32_t& closeTimeRipple);
  Status submitBlob(const char* txBlobHex, SubmitResult& out);
  Status submitBlobFailHard(const char* txBlobHex, SubmitResult& out);

  /**
   * Look up a transaction by hash (`tx` RPC). Fills engineResult (prefers
   * meta.TransactionResult) and tesSuccess. txHash is copied from the request
   * when the server omits it. Returns Ok when a result code is present;
   * Timeout/DecodeFailed/TransportFailed otherwise.
   *
   * Use after Status::Queued or submit Timeout before re-signing.
   */
  Status fetchTxResult(const char* txHashHex, SubmitResult& out);

  /**
   * Poll `tx` until a final engine result, LastLedgerSequence is exceeded, or
   * timeoutMs elapses.
   *
   * Returns:
   * - Ok if final tesSUCCESS
   * - Rejected if final tec/tef/tem (or other non-success final)
   * - Queued if still provisional (ter*) when the budget ends
   * - Timeout if never found / only transport failures until budget ends
   *
   * @param lastLedgerSequence  0 disables the "ledger expired" check.
   * @param pollIntervalMs      sleep between polls (clamped to >= 50 on device).
   *
   * Policy: never re-sign a new Sequence while this is running; the original
   * signed blob may still be in the open ledger queue.
   */
  Status waitForOutcome(const char* txHashHex, uint32_t lastLedgerSequence, uint32_t timeoutMs,
                        SubmitResult& out, uint32_t pollIntervalMs = 500);

  /**
   * account_objects -> up to maxIds ledger indexes (32-byte).
   * typeFilter e.g. "vault", "state", or nullptr for all.
   */
  Status fetchAccountObjectIds(const char* account, const char* typeFilter, uint8_t outIds[][32],
                               uint8_t maxIds, uint8_t& outCount);


  /** Raw: send prebuilt JSON, copy response into out. */
  Status rawRequest(const char* jsonReq, char* out, size_t outCap);

  /**
   * Subscription helpers (HandySockets-style streams).
   * Transport must support push frames (WsXahauTransport::setStreamHandler).
   * subscribe* sends the command and waits for the JSON-RPC ack only;
   * subsequent stream messages arrive via the stream handler + poll().
   * Do not call request/subscribe from inside the stream handler.
   */
  Status subscribeStreams(const char* const* streams, size_t streamCount);
  Status subscribeAccounts(const char* const* accounts, size_t accountCount);
  Status subscribeAccountsProposed(const char* const* accounts, size_t accountCount);
  Status subscribe(const char* const* streams, size_t streamCount, const char* const* accounts,
                   size_t accountCount);
  Status subscribeEx(const SubscribeParams& p);
  Status subscribeLedger();
  Status unsubscribeStreams(const char* const* streams, size_t streamCount);
  Status unsubscribeAccounts(const char* const* accounts, size_t accountCount);
  Status unsubscribeAccountsProposed(const char* const* accounts, size_t accountCount);
  Status unsubscribe(const char* const* streams, size_t streamCount, const char* const* accounts,
                     size_t accountCount);
  Status unsubscribeEx(const SubscribeParams& p);
  Status unsubscribeLedger();

  /** Drain WebSocket (call often while subscribed). */
  void poll() { transport_.poll(); }

 private:
  Status ensureBuf();
  Status doRequest(const char* req);

  IXahauTransport& transport_;
  uint32_t timeoutMs_;
  uint32_t ledgerOffset_;
  char* respBuf_;
  size_t respCap_;
  // Default response buffer is static BSS (see client.cpp) so placing
  // XahauRpcClient on the Arduino task stack does not cost 4 KiB.
};

}  // namespace xahau
