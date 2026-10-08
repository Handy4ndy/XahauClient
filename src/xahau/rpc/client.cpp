// client.cpp
#include "client.h"

#include "../platform/xahau_platform.h"

#include <string.h>

#if defined(XAHAU_PLATFORM_HOST) && !defined(ARDUINO)
#include <chrono>
#include <thread>
#endif

namespace xahau {

// Shared default response buffer — keeps XahauRpcClient small for stack placement.
// Override with setResponseBuffer() for concurrent clients or larger payloads.
// Tier A SetHook: larger submit/meta responses.
static char s_defaultRpcResp[16384];  // match WebSockets ESP max (~15KiB) + NUL margin

static void sleepMs(uint32_t ms) {
  if (ms == 0) return;
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
  delay(ms);
#elif defined(XAHAU_PLATFORM_HOST)
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#else
  (void)ms;
#endif
}

XahauRpcClient::XahauRpcClient(IXahauTransport& transport)
    : transport_(transport),
      timeoutMs_(90000),  // SetHook WASM submit/meta can exceed 30s
      ledgerOffset_(kMinLedgerOffset),
      respBuf_(s_defaultRpcResp),
      respCap_(sizeof(s_defaultRpcResp)) {}

void XahauRpcClient::setResponseBuffer(char* buf, size_t cap) {
  if (buf && cap > 0) {
    respBuf_ = buf;
    respCap_ = cap;
  } else {
    respBuf_ = s_defaultRpcResp;
    respCap_ = sizeof(s_defaultRpcResp);
  }
}

Status XahauRpcClient::ensureBuf() {
  if (!respBuf_ || respCap_ < 64) return Status::BufferTooSmall;
  return Status::Ok;
}

Status XahauRpcClient::doRequest(const char* req) {
  Status st = ensureBuf();
  if (st != Status::Ok) return st;
  respBuf_[0] = '\0';
  return transport_.request(req, respBuf_, respCap_, timeoutMs_);
}

Status XahauRpcClient::rawRequest(const char* jsonReq, char* out, size_t outCap) {
  if (!jsonReq || !out || outCap == 0) return Status::InvalidArgument;
  Status st = transport_.request(jsonReq, out, outCap, timeoutMs_);
  return st;
}

Status XahauRpcClient::ping() {
  char req[64];
  Status st = rpcBuildPing(req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  // rippled returns result.status "success" or empty error
  char err[32];
  if (rpcParseError(respBuf_, err, sizeof(err)) == Status::Ok) return Status::Rejected;
  return Status::Ok;
}

Status XahauRpcClient::fetchAccountInfo(const char* account, AccountInfoSnapshot& out) {
  char req[192];
  Status st = rpcBuildAccountInfo(account, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  st = rpcParseAccountInfoSnapshot(respBuf_, out);
  // Truncated / empty WSS payloads sometimes surface as DecodeFailed — surface as
  // Rejected only when parse explicitly set error; keep DecodeFailed for retries.
  return st;
}

Status XahauRpcClient::fetchSequenceAndLls(const char* account, uint32_t& sequence,
                                          uint32_t& lastLedgerSequence) {
  sequence = 0;
  lastLedgerSequence = 0;
  AccountInfoSnapshot snap;
  Status st = fetchAccountInfo(account, snap);
  if (st != Status::Ok) return st;
  if (!snap.found) return Status::Rejected;
  sequence = snap.sequence;
  uint32_t led = snap.ledgerIndex;
  if (led == 0) {
    // fallback: try ledger_closed
    char req[48];
    if (rpcBuildLedgerClosed(req, sizeof(req)) == Status::Ok && doRequest(req) == Status::Ok) {
      rpcParseLedgerIndex(respBuf_, led);
    }
  }
  if (led == 0) led = sequence;  // last resort
  lastLedgerSequence = led + effectiveLedgerOffset();
  return Status::Ok;
}

Status XahauRpcClient::fetchFeeDrops(uint64_t& feeDrops) {
  feeDrops = 12;
  char req[32];
  Status st = rpcBuildFee(req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  st = rpcParseFeeDrops(respBuf_, feeDrops);
  if (st != Status::Ok) {
    feeDrops = 12;
    return Status::Ok;  // soft default
  }
  return Status::Ok;
}

Status XahauRpcClient::fetchFeeAndLedger(FeeAndLedger& out) {
  memset(&out, 0, sizeof(out));
  out.feeDrops = 12;
  Status st = fetchFeeDrops(out.feeDrops);
  if (st != Status::Ok) return st;

  char req[48];
  st = rpcBuildLedgerClosed(req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st == Status::Ok) {
    rpcParseLedgerIndex(respBuf_, out.ledgerIndex);
  }
  if (out.ledgerIndex == 0) {
    st = rpcBuildServerInfo(req, sizeof(req));
    if (st == Status::Ok && doRequest(req) == Status::Ok) {
      rpcParseLedgerIndex(respBuf_, out.ledgerIndex);
    }
  }
  out.lastLedgerSequence = out.ledgerIndex + effectiveLedgerOffset();
  return Status::Ok;
}

Status XahauRpcClient::fetchLedgerCloseTime(uint32_t& closeTimeRipple) {
  closeTimeRipple = 0;
  Status st = ensureBuf();
  if (st != Status::Ok) return st;
  char req[96];
  st = rpcBuildLedger("validated", false, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  st = rpcParseLedgerCloseTime(respBuf_, closeTimeRipple);
  if (st != Status::Ok || closeTimeRipple == 0) return Status::DecodeFailed;
  return Status::Ok;
}

Status XahauRpcClient::submitBlob(const char* txBlobHex, SubmitResult& out) {
  memset(&out, 0, sizeof(out));
  // Static BSS — large enough for long signed blobs without stack pressure.
  // Tier A SetHook: submit JSON wrapping ~8 KiB CreateCode hex (~17 KiB).
  static char s_submitReq[17408];
  Status st = rpcBuildSubmit(txBlobHex, s_submitReq, sizeof(s_submitReq));
  if (st != Status::Ok) return st;
  // One reconnect retry on Timeout only. Re-sends the *same* signed blob (same
  // Sequence/hash) — rippled treats identical re-submit as idempotent. Never
  // re-sign with a new sequence here (that would double-apply on-ledger).
  for (int attempt = 0; attempt < 2; ++attempt) {
    st = doRequest(s_submitReq);
    if (st == Status::Ok) break;
    if (st != Status::Timeout || attempt == 1) return st;
  }
  st = rpcParseSubmitEngineResult(respBuf_, out.engineResult, sizeof(out.engineResult), out.txHash,
                                  sizeof(out.txHash));
  if (st != Status::Ok) return st;
  out.tesSuccess = rpcIsTesSuccess(out.engineResult);
  out.queued = rpcIsTerQueued(out.engineResult);
  // Provisional queue accept: distinct from Rejected so callers do not re-sign.
  if (out.tesSuccess) return Status::Ok;
  if (out.queued) return Status::Queued;
  return Status::Rejected;
}

Status XahauRpcClient::submitBlobFailHard(const char* txBlobHex, SubmitResult& out) {
  memset(&out, 0, sizeof(out));
  char req[2200];
  Status st = rpcBuildSubmitEx(txBlobHex, true, req, sizeof(req));
  if (st != Status::Ok) return st;
  // Same-blob retry on Timeout (parity with submitBlob).
  for (int attempt = 0; attempt < 2; ++attempt) {
    st = doRequest(req);
    if (st == Status::Ok) break;
    if (st != Status::Timeout || attempt == 1) return st;
  }
  st = rpcParseSubmitEngineResult(respBuf_, out.engineResult, sizeof(out.engineResult), out.txHash,
                                  sizeof(out.txHash));
  if (st != Status::Ok) return st;
  out.tesSuccess = rpcIsTesSuccess(out.engineResult);
  out.queued = rpcIsTerQueued(out.engineResult);
  if (out.tesSuccess) return Status::Ok;
  if (out.queued) return Status::Queued;
  return Status::Rejected;
}

Status XahauRpcClient::fetchTxResult(const char* txHashHex, SubmitResult& out) {
  memset(&out, 0, sizeof(out));
  if (!txHashHex || !txHashHex[0]) return Status::InvalidArgument;
  static char s_txReq[192];
  Status st = rpcBuildTx(txHashHex, false, s_txReq, sizeof(s_txReq));
  if (st != Status::Ok) return st;
  st = doRequest(s_txReq);
  if (st != Status::Ok) return st;
  bool validated = false;
  st = rpcParseTxEngineResult(respBuf_, out.engineResult, sizeof(out.engineResult), &validated);
  if (st != Status::Ok) {
    // Fall back to submit-style engine_result / hash fields.
    st = rpcParseSubmitEngineResult(respBuf_, out.engineResult, sizeof(out.engineResult), out.txHash,
                                    sizeof(out.txHash));
    if (st != Status::Ok) return st;
  } else {
    out.validated = validated;
  }
  if (!out.txHash[0]) {
    // Copy request hash for caller convenience.
    size_t i = 0;
    for (; txHashHex[i] && i + 1 < sizeof(out.txHash); ++i) out.txHash[i] = txHashHex[i];
    out.txHash[i] = '\0';
  }
  out.tesSuccess = rpcIsTesSuccess(out.engineResult);
  out.queued = rpcIsTerQueued(out.engineResult);
  return Status::Ok;
}

Status XahauRpcClient::waitForOutcome(const char* txHashHex, uint32_t lastLedgerSequence,
                                     uint32_t timeoutMs, SubmitResult& out,
                                     uint32_t pollIntervalMs) {
  memset(&out, 0, sizeof(out));
  if (!txHashHex || !txHashHex[0]) return Status::InvalidArgument;
  if (pollIntervalMs < 50) pollIntervalMs = 50;
  if (timeoutMs == 0) timeoutMs = timeoutMs_;

  // Copy hash early so callers always have it even on Timeout.
  {
    size_t i = 0;
    for (; txHashHex[i] && i + 1 < sizeof(out.txHash); ++i) out.txHash[i] = txHashHex[i];
    out.txHash[i] = '\0';
  }

  uint32_t elapsed = 0;
  Status lastTransport = Status::Timeout;
  bool sawProvisional = false;

  while (elapsed <= timeoutMs) {
    SubmitResult step;
    Status st = fetchTxResult(txHashHex, step);
    lastTransport = st;

    if (st == Status::Ok && step.engineResult[0]) {
      out = step;
      if (!out.txHash[0]) {
        size_t i = 0;
        for (; txHashHex[i] && i + 1 < sizeof(out.txHash); ++i) out.txHash[i] = txHashHex[i];
        out.txHash[i] = '\0';
      }

      const EngineClass cls = rpcEngineClass(out.engineResult);
      if (cls == EngineClass::Tes) {
        out.tesSuccess = true;
        out.queued = false;
        return Status::Ok;
      }
      // Final failures (applied or not): tec / tef / tem
      if (cls == EngineClass::Tec || cls == EngineClass::Tef || cls == EngineClass::Tem) {
        out.tesSuccess = false;
        out.queued = false;
        return Status::Rejected;
      }
      // ter* including QUEUED — keep polling
      if (cls == EngineClass::Ter) {
        sawProvisional = true;
        out.queued = rpcIsTerQueued(out.engineResult);
      }
    }

    // Ledger expiry: if current ledger is past LLS, stop (tx cannot apply later).
    if (lastLedgerSequence != 0) {
      char req[48];
      uint32_t led = 0;
      if (rpcBuildLedgerClosed(req, sizeof(req)) == Status::Ok && doRequest(req) == Status::Ok) {
        rpcParseLedgerIndex(respBuf_, led);
      }
      if (led != 0 && led > lastLedgerSequence) {
        // Prefer last known engine result; otherwise mark expired as Timeout.
        if (out.engineResult[0] && rpcEngineClass(out.engineResult) == EngineClass::Tes) {
          return Status::Ok;
        }
        if (out.engineResult[0] && rpcEngineClass(out.engineResult) != EngineClass::Ter &&
            rpcEngineClass(out.engineResult) != EngineClass::Unknown) {
          return Status::Rejected;
        }
        // Past LLS without final success.
        if (!out.engineResult[0]) {
          // "tefMAX_LEDGER" is the usual rippled code; set for harness visibility.
          const char* exp = "tefMAX_LEDGER";
          size_t i = 0;
          for (; exp[i] && i + 1 < sizeof(out.engineResult); ++i) out.engineResult[i] = exp[i];
          out.engineResult[i] = '\0';
        }
        out.tesSuccess = false;
        out.queued = false;
        return Status::Rejected;
      }
    }

    if (elapsed + pollIntervalMs > timeoutMs) break;
    sleepMs(pollIntervalMs);
    elapsed += pollIntervalMs;
    transport_.poll();
  }

  if (out.tesSuccess) return Status::Ok;
  if (sawProvisional || out.queued || rpcIsTerQueued(out.engineResult)) {
    out.queued = true;
    return Status::Queued;
  }
  if (out.engineResult[0] && !out.tesSuccess) return Status::Rejected;
  return (lastTransport == Status::Ok) ? Status::Timeout : lastTransport;
}

Status XahauRpcClient::fetchAccountObjectIds(const char* account, const char* typeFilter,
                                            uint8_t outIds[][32], uint8_t maxIds,
                                            uint8_t& outCount) {
  outCount = 0;
  char req[256];
  Status st = rpcBuildAccountObjects(account, typeFilter, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return rpcParseObjectIds(respBuf_, nullptr, outIds, maxIds, outCount);
}




static Status subscribeAckOk(const char* resp) {
  if (!resp || !resp[0]) return Status::DecodeFailed;
  // Clio: server/consensus/peer_status streams → reportingUnsupported
  if (rpcIsReportingUnsupported(resp)) return Status::Rejected;
  char err[48];
  if (rpcParseError(resp, err, sizeof(err)) == Status::Ok && err[0]) return Status::Rejected;
  // Accept status success or result present
  if (strstr(resp, "\"status\":\"success\"") || strstr(resp, "\"status\": \"success\"") ||
      strstr(resp, "\"result\"")) {
    return Status::Ok;
  }
  return Status::Ok;  // rippled often returns result with empty object
}

Status XahauRpcClient::subscribeStreams(const char* const* streams, size_t streamCount) {
  char req[384];
  Status st = rpcBuildSubscribeStreams(streams, streamCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::subscribeAccounts(const char* const* accounts, size_t accountCount) {
  char req[512];
  Status st = rpcBuildSubscribeAccounts(accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::subscribeAccountsProposed(const char* const* accounts, size_t accountCount) {
  char req[512];
  Status st = rpcBuildSubscribeAccountsProposed(accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::subscribe(const char* const* streams, size_t streamCount,
                                const char* const* accounts, size_t accountCount) {
  char req[640];
  Status st = rpcBuildSubscribe(streams, streamCount, accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::subscribeEx(const SubscribeParams& p) {
  char req[768];
  Status st = rpcBuildSubscribeEx(p, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::subscribeLedger() {
  char req[96];
  Status st = rpcBuildSubscribeLedger(req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribeStreams(const char* const* streams, size_t streamCount) {
  char req[384];
  Status st = rpcBuildUnsubscribeStreams(streams, streamCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribeAccounts(const char* const* accounts, size_t accountCount) {
  char req[512];
  Status st = rpcBuildUnsubscribeAccounts(accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribeAccountsProposed(const char* const* accounts,
                                                  size_t accountCount) {
  char req[512];
  Status st = rpcBuildUnsubscribeAccountsProposed(accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribe(const char* const* streams, size_t streamCount,
                                  const char* const* accounts, size_t accountCount) {
  char req[640];
  Status st = rpcBuildUnsubscribe(streams, streamCount, accounts, accountCount, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribeEx(const SubscribeParams& p) {
  char req[768];
  Status st = rpcBuildUnsubscribeEx(p, req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

Status XahauRpcClient::unsubscribeLedger() {
  char req[96];
  Status st = rpcBuildUnsubscribeLedger(req, sizeof(req));
  if (st != Status::Ok) return st;
  st = doRequest(req);
  if (st != Status::Ok) return st;
  return subscribeAckOk(respBuf_);
}

}  // namespace xahau

