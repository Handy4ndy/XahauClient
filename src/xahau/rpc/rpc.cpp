// rpc.cpp — JSON-RPC builders/parsers without ArduinoJson
#include "rpc.h"

#include "../crypto/base58.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace xahau {

// ---------------------------------------------------------------------------
// Input validation + JSON string escaping (Medium audit: injection hardening)
// ---------------------------------------------------------------------------

static bool isBase58Char(char c) {
  static const char* alpha = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
  for (const char* p = alpha; *p; ++p) {
    if (*p == c) return true;
  }
  return false;
}

static bool isHexChar(char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/**
 * Classic r-address: Base58Check via decodeClassicAddress (version + checksum).
 * Rejects injection payloads and wrong-checksum addresses.
 */
static bool isSafeAccountId(const char* s) {
  if (!s || s[0] != 'r') return false;
  // Fast reject: alphabet + length before checksum work.
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isBase58Char(*p)) return false;
    if (n >= 40) return false;
  }
  if (n < 25) return false;  // typical classic addresses ~25–35 chars
  uint8_t accountId[20];
  return decodeClassicAddress(s, accountId) == Status::Ok;
}

static bool isSafeHex(const char* s, bool requireEven) {
  if (!s || !s[0]) return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isHexChar(*p)) return false;
    if (n >= 4096) return false;
  }
  if (requireEven && (n & 1u)) return false;
  return n > 0;
}

static bool isSafeLedgerIndex(const char* s) {
  if (!s || !s[0]) return false;
  if (strcmp(s, "current") == 0 || strcmp(s, "validated") == 0 || strcmp(s, "closed") == 0) {
    return true;
  }
  for (const char* p = s; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
  }
  return true;
}

static bool isSafeTypeFilter(const char* s) {
  if (!s || !s[0]) return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    char c = *p;
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    if (!ok || n >= 64) return false;
  }
  return true;
}

static Status jsonEscapeContent(const char* s, char* out, size_t outCap) {
  if (!s || !out || outCap == 0) return Status::InvalidArgument;
  size_t o = 0;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p; ++p) {
    unsigned char c = *p;
    auto need = [&](size_t k) -> bool { return o + k < outCap; };
    if (c == '"' || c == '\\') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = static_cast<char>(c);
    } else if (c == '\b') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = 'b';
    } else if (c == '\f') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = 'f';
    } else if (c == '\n') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = 'n';
    } else if (c == '\r') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = 'r';
    } else if (c == '\t') {
      if (!need(2)) return Status::BufferTooSmall;
      out[o++] = '\\';
      out[o++] = 't';
    } else if (c < 0x20) {
      if (!need(6)) return Status::BufferTooSmall;
      static const char* hexd = "0123456789abcdef";
      out[o++] = '\\';
      out[o++] = 'u';
      out[o++] = '0';
      out[o++] = '0';
      out[o++] = hexd[(c >> 4) & 0xF];
      out[o++] = hexd[c & 0xF];
    } else {
      if (!need(1)) return Status::BufferTooSmall;
      out[o++] = static_cast<char>(c);
    }
  }
  if (o >= outCap) return Status::BufferTooSmall;
  out[o] = '\0';
  return Status::Ok;
}

static Status fmtOut(char* out, size_t cap, const char* fmt, ...) {
  if (!out || !fmt || cap == 0) return Status::InvalidArgument;
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(out, cap, fmt, ap);
  va_end(ap);
  if (n < 0 || static_cast<size_t>(n) >= cap) return Status::BufferTooSmall;
  return Status::Ok;
}

Status rpcBuildAccountInfo(const char* account, char* out, size_t outCap) {
  return rpcBuildAccountInfoEx(account, "current", out, outCap);
}

Status rpcBuildAccountInfoEx(const char* account, const char* ledgerIndex, char* out,
                             size_t outCap) {
  if (!account || !ledgerIndex) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  if (!isSafeLedgerIndex(ledgerIndex)) return Status::InvalidArgument;
  char accEsc[96];
  char ledEsc[48];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  st = jsonEscapeContent(ledgerIndex, ledEsc, sizeof(ledEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"account_info\",\"account\":\"%s\",\"ledger_index\":\"%s\"}", accEsc,
                ledEsc);
}

Status rpcBuildSubmit(const char* txBlobHex, char* out, size_t outCap) {
  return rpcBuildSubmitEx(txBlobHex, false, out, outCap);
}

Status rpcBuildSubmitEx(const char* txBlobHex, bool failHard, char* out, size_t outCap) {
  if (!txBlobHex) return Status::InvalidArgument;
  if (!isSafeHex(txBlobHex, true)) return Status::InvalidArgument;
  if (failHard) {
    return fmtOut(out, outCap,
                  "{\"command\":\"submit\",\"tx_blob\":\"%s\",\"fail_hard\":true}", txBlobHex);
  }
  return fmtOut(out, outCap, "{\"command\":\"submit\",\"tx_blob\":\"%s\"}", txBlobHex);
}

Status rpcBuildServerInfo(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"server_info\"}");
}

Status rpcBuildServerState(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"server_state\"}");
}

Status rpcBuildFee(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"fee\"}");
}

Status rpcBuildFeeWithTxBlob(const char* txBlobHex, char* out, size_t outCap) {
  if (!txBlobHex) return Status::InvalidArgument;
  if (!isSafeHex(txBlobHex, true)) return Status::InvalidArgument;
  return fmtOut(out, outCap, "{\"command\":\"fee\",\"tx_blob\":\"%s\"}", txBlobHex);
}

Status rpcBuildLedger(const char* ledgerIndex, bool transactions, char* out, size_t outCap) {
  if (!ledgerIndex) return Status::InvalidArgument;
  if (!isSafeLedgerIndex(ledgerIndex)) return Status::InvalidArgument;
  char ledEsc[48];
  Status st = jsonEscapeContent(ledgerIndex, ledEsc, sizeof(ledEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"ledger\",\"ledger_index\":\"%s\",\"transactions\":%s}", ledEsc,
                transactions ? "true" : "false");
}

Status rpcBuildLedgerClosed(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"ledger_closed\"}");
}

Status rpcBuildPing(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"ping\"}");
}

// Xahau subscribe stream names (public API). Reject unknown to avoid injection.
static bool isSafeStreamName(const char* s) {
  if (!s || !s[0]) return false;
  static const char* kOk[] = {"ledger",
                              "validations",
                              "transactions",
                              "transactions_proposed",
                              "consensus",
                              "server",
                              "peer_status",
                              "manifests",
                              "book_changes",
                              nullptr};
  for (int i = 0; kOk[i]; ++i) {
    if (strcmp(s, kOk[i]) == 0) return true;
  }
  return false;
}

static Status appendJsonStringArray(char* out, size_t cap, size_t& o, const char* const* items,
                                    size_t count, bool accountMode) {
  if (o + 1 >= cap) return Status::BufferTooSmall;
  out[o++] = '[';
  for (size_t i = 0; i < count; ++i) {
    if (!items || !items[i]) return Status::InvalidArgument;
    if (accountMode) {
      if (!isSafeAccountId(items[i])) return Status::InvalidAddress;
    } else {
      if (!isSafeStreamName(items[i])) return Status::InvalidArgument;
    }
    char esc[96];
    Status st = jsonEscapeContent(items[i], esc, sizeof(esc));
    if (st != Status::Ok) return st;
    int n = snprintf(out + o, cap - o, "%s\"%s\"", (i ? "," : ""), esc);
    if (n < 0 || static_cast<size_t>(n) >= cap - o) return Status::BufferTooSmall;
    o += static_cast<size_t>(n);
  }
  if (o + 1 >= cap) return Status::BufferTooSmall;
  out[o++] = ']';
  out[o] = '\0';
  return Status::Ok;
}

static Status buildSubUnsubEx(const char* command, const SubscribeParams& p, char* out,
                              size_t outCap) {
  if (!out || !command || outCap < 32) return Status::InvalidArgument;
  const size_t sc = p.streamCount;
  const size_t ac = p.accountCount;
  const size_t pc = p.accountsProposedCount;
  if (sc == 0 && ac == 0 && pc == 0) return Status::InvalidArgument;
  if (sc > 8 || ac > 8 || pc > 8) return Status::InvalidArgument;

  size_t o = 0;
  int n = snprintf(out, outCap, "{\"command\":\"%s\"", command);
  if (n < 0 || static_cast<size_t>(n) >= outCap) return Status::BufferTooSmall;
  o = static_cast<size_t>(n);

  if (sc > 0) {
    n = snprintf(out + o, outCap - o, ",\"streams\":");
    if (n < 0 || static_cast<size_t>(n) >= outCap - o) return Status::BufferTooSmall;
    o += static_cast<size_t>(n);
    Status st = appendJsonStringArray(out, outCap, o, p.streams, sc, false);
    if (st != Status::Ok) return st;
  }
  if (ac > 0) {
    n = snprintf(out + o, outCap - o, ",\"accounts\":");
    if (n < 0 || static_cast<size_t>(n) >= outCap - o) return Status::BufferTooSmall;
    o += static_cast<size_t>(n);
    Status st = appendJsonStringArray(out, outCap, o, p.accounts, ac, true);
    if (st != Status::Ok) return st;
  }
  if (pc > 0) {
    n = snprintf(out + o, outCap - o, ",\"accounts_proposed\":");
    if (n < 0 || static_cast<size_t>(n) >= outCap - o) return Status::BufferTooSmall;
    o += static_cast<size_t>(n);
    Status st = appendJsonStringArray(out, outCap, o, p.accountsProposed, pc, true);
    if (st != Status::Ok) return st;
  }
  if (o + 2 >= outCap) return Status::BufferTooSmall;
  out[o++] = '}';
  out[o] = '\0';
  return Status::Ok;
}

static SubscribeParams makeParams(const char* const* streams, size_t streamCount,
                                  const char* const* accounts, size_t accountCount,
                                  const char* const* proposed, size_t proposedCount) {
  SubscribeParams p;
  p.streams = streams;
  p.streamCount = streamCount;
  p.accounts = accounts;
  p.accountCount = accountCount;
  p.accountsProposed = proposed;
  p.accountsProposedCount = proposedCount;
  return p;
}

Status rpcBuildSubscribeStreams(const char* const* streams, size_t streamCount, char* out,
                                size_t outCap) {
  return rpcBuildSubscribeEx(makeParams(streams, streamCount, nullptr, 0, nullptr, 0), out, outCap);
}

Status rpcBuildSubscribeAccounts(const char* const* accounts, size_t accountCount, char* out,
                                 size_t outCap) {
  return rpcBuildSubscribeEx(makeParams(nullptr, 0, accounts, accountCount, nullptr, 0), out,
                             outCap);
}

Status rpcBuildSubscribeAccountsProposed(const char* const* accounts, size_t accountCount,
                                         char* out, size_t outCap) {
  return rpcBuildSubscribeEx(makeParams(nullptr, 0, nullptr, 0, accounts, accountCount), out,
                             outCap);
}

Status rpcBuildSubscribe(const char* const* streams, size_t streamCount,
                         const char* const* accounts, size_t accountCount, char* out,
                         size_t outCap) {
  return rpcBuildSubscribeEx(makeParams(streams, streamCount, accounts, accountCount, nullptr, 0),
                             out, outCap);
}

Status rpcBuildSubscribeEx(const SubscribeParams& p, char* out, size_t outCap) {
  return buildSubUnsubEx("subscribe", p, out, outCap);
}

Status rpcBuildUnsubscribeStreams(const char* const* streams, size_t streamCount, char* out,
                                  size_t outCap) {
  return rpcBuildUnsubscribeEx(makeParams(streams, streamCount, nullptr, 0, nullptr, 0), out,
                               outCap);
}

Status rpcBuildUnsubscribeAccounts(const char* const* accounts, size_t accountCount, char* out,
                                   size_t outCap) {
  return rpcBuildUnsubscribeEx(makeParams(nullptr, 0, accounts, accountCount, nullptr, 0), out,
                               outCap);
}

Status rpcBuildUnsubscribeAccountsProposed(const char* const* accounts, size_t accountCount,
                                           char* out, size_t outCap) {
  return rpcBuildUnsubscribeEx(makeParams(nullptr, 0, nullptr, 0, accounts, accountCount), out,
                               outCap);
}

Status rpcBuildUnsubscribe(const char* const* streams, size_t streamCount,
                           const char* const* accounts, size_t accountCount, char* out,
                           size_t outCap) {
  return rpcBuildUnsubscribeEx(makeParams(streams, streamCount, accounts, accountCount, nullptr, 0),
                               out, outCap);
}

Status rpcBuildUnsubscribeEx(const SubscribeParams& p, char* out, size_t outCap) {
  return buildSubUnsubEx("unsubscribe", p, out, outCap);
}

Status rpcBuildSubscribeLedger(char* out, size_t outCap) {
  const char* streams[] = {"ledger"};
  return rpcBuildSubscribeStreams(streams, 1, out, outCap);
}

Status rpcBuildUnsubscribeLedger(char* out, size_t outCap) {
  const char* streams[] = {"ledger"};
  return rpcBuildUnsubscribeStreams(streams, 1, out, outCap);
}

Status rpcBuildAccountLines(const char* account, char* out, size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"account_lines\",\"account\":\"%s\",\"ledger_index\":\"validated\"}",
                accEsc);
}

Status rpcBuildAccountTx(const char* account, uint32_t limit, char* out, size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  if (limit == 0) limit = 10;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"account_tx\",\"account\":\"%s\",\"limit\":%u,\"ledger_index_min\":-1,"
                "\"ledger_index_max\":-1}",
                accEsc, static_cast<unsigned>(limit));
}

Status rpcBuildAccountObjects(const char* account, const char* typeFilter, char* out,
                              size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  if (typeFilter && typeFilter[0]) {
    if (!isSafeTypeFilter(typeFilter)) return Status::InvalidArgument;
    char typeEsc[80];
    st = jsonEscapeContent(typeFilter, typeEsc, sizeof(typeEsc));
    if (st != Status::Ok) return st;
    return fmtOut(out, outCap,
                  "{\"command\":\"account_objects\",\"account\":\"%s\",\"type\":\"%s\","
                  "\"ledger_index\":\"validated\"}",
                  accEsc, typeEsc);
  }
  return fmtOut(out, outCap,
                "{\"command\":\"account_objects\",\"account\":\"%s\",\"ledger_index\":\"validated\"}",
                accEsc);
}

Status rpcBuildTx(const char* txHashHex, bool binary, char* out, size_t outCap) {
  if (!txHashHex) return Status::InvalidArgument;
  if (!isSafeHex(txHashHex, true)) return Status::InvalidArgument;
  return fmtOut(out, outCap, "{\"command\":\"tx\",\"transaction\":\"%s\",\"binary\":%s}", txHashHex,
                binary ? "true" : "false");
}

Status rpcBuildSubmitMultisigned(const char* txJson, char* out, size_t outCap) {
  if (!txJson || !txJson[0]) return Status::InvalidArgument;
  while (*txJson == ' ' || *txJson == '\t' || *txJson == '\r' || *txJson == '\n') ++txJson;
  if (*txJson != '{') return Status::InvalidArgument;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(txJson); *p; ++p) {
    if (*p < 0x20 && *p != '\t' && *p != '\n' && *p != '\r') return Status::InvalidArgument;
  }
  return fmtOut(out, outCap, "{\"command\":\"submit_multisigned\",\"tx_json\":%s}", txJson);
}

// ---------------------------------------------------------------------------
// Wallet-priority public method builders
// ---------------------------------------------------------------------------

static bool isSafeCurrencyCode(const char* s) {
  if (!s || !s[0]) return false;
  // ISO 3-letter or 40-char hex currency code
  size_t n = 0;
  bool allHex = true;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isHexChar(*p)) allHex = false;
    char c = *p;
    bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (!ok || n >= 40) return false;
  }
  if (n == 3) return true;
  if (n == 40 && allHex) return true;
  // allow "XRP"
  if (n == 3) return true;
  return false;
}

static bool isSafeHash64(const char* s) {
  if (!s) return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isHexChar(*p)) return false;
  }
  return n == 64;
}

static bool isSafeNftId(const char* s) {
  // NFTokenID is 64 hex chars
  return isSafeHash64(s);
}

static bool isSafeDropsAmount(const char* s) {
  if (!s || !s[0]) return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (*p < '0' || *p > '9' || n >= 20) return false;
  }
  return n > 0;
}

static bool isSafeBase58Pub(const char* s) {
  if (!s || !s[0]) return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isBase58Char(*p) || n >= 80) return false;
  }
  return n >= 16;
}

static Status buildAccountCmdValidated(const char* command, const char* account, char* out,
                                       size_t outCap) {
  if (!command || !account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"%s\",\"account\":\"%s\",\"ledger_index\":\"validated\"}", command,
                accEsc);
}

Status rpcBuildAccountChannels(const char* account, char* out, size_t outCap) {
  return buildAccountCmdValidated("account_channels", account, out, outCap);
}

Status rpcBuildAccountCurrencies(const char* account, char* out, size_t outCap) {
  return buildAccountCmdValidated("account_currencies", account, out, outCap);
}

Status rpcBuildAccountNfts(const char* account, uint32_t limit, char* out, size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"account_nfts\",\"account\":\"%s\",\"limit\":%u,"
                "\"ledger_index\":\"validated\"}",
                accEsc, static_cast<unsigned>(limit));
}

Status rpcBuildAccountOffers(const char* account, uint32_t limit, char* out, size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"account_offers\",\"account\":\"%s\",\"limit\":%u,"
                "\"ledger_index\":\"validated\"}",
                accEsc, static_cast<unsigned>(limit));
}

Status rpcBuildGatewayBalances(const char* account, char* out, size_t outCap) {
  // strict account, ledger validated — hot/cold wallet inventory for issuers
  return buildAccountCmdValidated("gateway_balances", account, out, outCap);
}

Status rpcBuildNoRippleCheck(const char* account, const char* role, char* out, size_t outCap) {
  if (!account || !role) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  if (strcmp(role, "send") != 0 && strcmp(role, "receive") != 0) return Status::InvalidArgument;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"noripple_check\",\"account\":\"%s\",\"role\":\"%s\","
                "\"ledger_index\":\"validated\"}",
                accEsc, role);
}

Status rpcBuildLedgerCurrent(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"ledger_current\"}");
}

Status rpcBuildLedgerData(const char* ledgerIndex, uint32_t limit, char* out, size_t outCap) {
  if (!ledgerIndex) return Status::InvalidArgument;
  if (!isSafeLedgerIndex(ledgerIndex)) return Status::InvalidArgument;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  char ledEsc[48];
  Status st = jsonEscapeContent(ledgerIndex, ledEsc, sizeof(ledEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"ledger_data\",\"ledger_index\":\"%s\",\"limit\":%u,\"binary\":false}",
                ledEsc, static_cast<unsigned>(limit));
}

Status rpcBuildLedgerEntryByIndex(const char* indexHex64, char* out, size_t outCap) {
  if (!indexHex64) return Status::InvalidArgument;
  if (!isSafeHash64(indexHex64)) return Status::InvalidArgument;
  return fmtOut(out, outCap,
                "{\"command\":\"ledger_entry\",\"index\":\"%s\",\"ledger_index\":\"validated\"}",
                indexHex64);
}

Status rpcBuildLedgerEntryAccountRoot(const char* account, char* out, size_t outCap) {
  if (!account) return Status::InvalidArgument;
  if (!isSafeAccountId(account)) return Status::InvalidAddress;
  char accEsc[96];
  Status st = jsonEscapeContent(account, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"ledger_entry\",\"account_root\":\"%s\","
                "\"ledger_index\":\"validated\"}",
                accEsc);
}

Status rpcBuildTransactionEntry(const char* txHashHex, const char* ledgerIndex, char* out,
                                size_t outCap) {
  if (!txHashHex || !ledgerIndex) return Status::InvalidArgument;
  if (!isSafeHex(txHashHex, true) || strlen(txHashHex) != 64) return Status::InvalidArgument;
  if (!isSafeLedgerIndex(ledgerIndex)) return Status::InvalidArgument;
  char ledEsc[48];
  Status st = jsonEscapeContent(ledgerIndex, ledEsc, sizeof(ledEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"transaction_entry\",\"tx_hash\":\"%s\",\"ledger_index\":\"%s\"}",
                txHashHex, ledEsc);
}

Status rpcBuildDepositAuthorized(const char* sourceAccount, const char* destinationAccount,
                                 char* out, size_t outCap) {
  if (!sourceAccount || !destinationAccount) return Status::InvalidArgument;
  if (!isSafeAccountId(sourceAccount) || !isSafeAccountId(destinationAccount)) {
    return Status::InvalidAddress;
  }
  char src[96], dst[96];
  Status st = jsonEscapeContent(sourceAccount, src, sizeof(src));
  if (st != Status::Ok) return st;
  st = jsonEscapeContent(destinationAccount, dst, sizeof(dst));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"deposit_authorized\",\"source_account\":\"%s\","
                "\"destination_account\":\"%s\",\"ledger_index\":\"validated\"}",
                src, dst);
}


static Status appendCurrencyJson(char* out, size_t cap, size_t& o, const char* currency,
                                 const char* issuer) {
  if (!currency || !isSafeCurrencyCode(currency)) return Status::InvalidArgument;
  char curEsc[48];
  Status st = jsonEscapeContent(currency, curEsc, sizeof(curEsc));
  if (st != Status::Ok) return st;
  // XRP side: {"currency":"XRP"} only
  if ((strcmp(currency, "XRP") == 0 || strcmp(currency, "xrp") == 0) &&
      (!issuer || !issuer[0])) {
    int n = snprintf(out + o, cap - o, "{\"currency\":\"XRP\"}");
    if (n < 0 || static_cast<size_t>(n) >= cap - o) return Status::BufferTooSmall;
    o += static_cast<size_t>(n);
    return Status::Ok;
  }
  if (!issuer || !isSafeAccountId(issuer)) return Status::InvalidAddress;
  char issEsc[96];
  st = jsonEscapeContent(issuer, issEsc, sizeof(issEsc));
  if (st != Status::Ok) return st;
  int n = snprintf(out + o, cap - o, "{\"currency\":\"%s\",\"issuer\":\"%s\"}", curEsc, issEsc);
  if (n < 0 || static_cast<size_t>(n) >= cap - o) return Status::BufferTooSmall;
  o += static_cast<size_t>(n);
  return Status::Ok;
}

Status rpcBuildBookOffers(const char* takerGetsCurrency, const char* takerGetsIssuer,
                          const char* takerPaysCurrency, const char* takerPaysIssuer,
                          uint32_t limit, char* out, size_t outCap) {
  if (!out || outCap < 64) return Status::InvalidArgument;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  size_t o = 0;
  int n = snprintf(out, outCap, "{\"command\":\"book_offers\",\"taker_gets\":");
  if (n < 0 || static_cast<size_t>(n) >= outCap) return Status::BufferTooSmall;
  o = static_cast<size_t>(n);
  Status st = appendCurrencyJson(out, outCap, o, takerGetsCurrency, takerGetsIssuer);
  if (st != Status::Ok) return st;
  n = snprintf(out + o, outCap - o, ",\"taker_pays\":");
  if (n < 0 || static_cast<size_t>(n) >= outCap - o) return Status::BufferTooSmall;
  o += static_cast<size_t>(n);
  st = appendCurrencyJson(out, outCap, o, takerPaysCurrency, takerPaysIssuer);
  if (st != Status::Ok) return st;
  n = snprintf(out + o, outCap - o, ",\"limit\":%u,\"ledger_index\":\"validated\"}",
               static_cast<unsigned>(limit));
  if (n < 0 || static_cast<size_t>(n) >= outCap - o) return Status::BufferTooSmall;
  return Status::Ok;
}

Status rpcBuildNftBuyOffers(const char* nftIdHex, uint32_t limit, char* out, size_t outCap) {
  if (!nftIdHex || !isSafeNftId(nftIdHex)) return Status::InvalidArgument;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  return fmtOut(out, outCap,
                "{\"command\":\"nft_buy_offers\",\"nft_id\":\"%s\",\"limit\":%u}", nftIdHex,
                static_cast<unsigned>(limit));
}

Status rpcBuildNftSellOffers(const char* nftIdHex, uint32_t limit, char* out, size_t outCap) {
  if (!nftIdHex || !isSafeNftId(nftIdHex)) return Status::InvalidArgument;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  return fmtOut(out, outCap,
                "{\"command\":\"nft_sell_offers\",\"nft_id\":\"%s\",\"limit\":%u}", nftIdHex,
                static_cast<unsigned>(limit));
}

Status rpcBuildChannelVerify(const char* channelIdHex, const char* amountDrops,
                             const char* publicKeyHex, const char* signatureHex, char* out,
                             size_t outCap) {
  if (!channelIdHex || !amountDrops || !publicKeyHex || !signatureHex) {
    return Status::InvalidArgument;
  }
  if (!isSafeHash64(channelIdHex)) return Status::InvalidArgument;
  if (!isSafeDropsAmount(amountDrops)) return Status::InvalidArgument;
  if (!isSafeHex(publicKeyHex, true) || !isSafeHex(signatureHex, true)) {
    return Status::InvalidArgument;
  }
  return fmtOut(out, outCap,
                "{\"command\":\"channel_verify\",\"channel_id\":\"%s\",\"amount\":\"%s\","
                "\"public_key\":\"%s\",\"signature\":\"%s\"}",
                channelIdHex, amountDrops, publicKeyHex, signatureHex);
}

Status rpcBuildFeature(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"feature\"}");
}

Status rpcBuildServerDefinitions(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"server_definitions\"}");
}

Status rpcBuildManifest(const char* publicKeyBase58, char* out, size_t outCap) {
  if (!publicKeyBase58 || !isSafeBase58Pub(publicKeyBase58)) return Status::InvalidArgument;
  char esc[96];
  Status st = jsonEscapeContent(publicKeyBase58, esc, sizeof(esc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap, "{\"command\":\"manifest\",\"public_key\":\"%s\"}", esc);
}

Status rpcBuildRandom(char* out, size_t outCap) {
  return fmtOut(out, outCap, "{\"command\":\"random\"}");
}


Status rpcBuildNftInfo(const char* nftIdHex, char* out, size_t outCap) {
  if (!nftIdHex || !isSafeNftId(nftIdHex)) return Status::InvalidArgument;
  return fmtOut(out, outCap, "{\"command\":\"nft_info\",\"nft_id\":\"%s\"}", nftIdHex);
}

Status rpcBuildNftHistory(const char* nftIdHex, uint32_t limit, char* out, size_t outCap) {
  if (!nftIdHex || !isSafeNftId(nftIdHex)) return Status::InvalidArgument;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  return fmtOut(out, outCap,
                "{\"command\":\"nft_history\",\"nft_id\":\"%s\",\"limit\":%u}", nftIdHex,
                static_cast<unsigned>(limit));
}

Status rpcBuildNftsByIssuer(const char* issuer, uint32_t limit, char* out, size_t outCap) {
  if (!issuer) return Status::InvalidArgument;
  if (!isSafeAccountId(issuer)) return Status::InvalidAddress;
  if (limit == 0) limit = 20;
  if (limit > 400) limit = 400;
  char accEsc[96];
  Status st = jsonEscapeContent(issuer, accEsc, sizeof(accEsc));
  if (st != Status::Ok) return st;
  return fmtOut(out, outCap,
                "{\"command\":\"nfts_by_issuer\",\"issuer\":\"%s\",\"limit\":%u}", accEsc,
                static_cast<unsigned>(limit));
}

static const char* findKey(const char* json, const char* key) {
  if (!json || !key) return nullptr;
  return strstr(json, key);
}

static const char* skipWsColon(const char* p) {
  if (!p) return nullptr;
  p = strchr(p, ':');
  if (!p) return nullptr;
  ++p;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
  return p;
}

Status rpcParseStringField(const char* json, const char* keyQuoted, char* out, size_t outCap) {
  if (!json || !keyQuoted || !out || outCap == 0) return Status::InvalidArgument;
  out[0] = '\0';
  const char* p = findKey(json, keyQuoted);
  if (!p) return Status::DecodeFailed;
  p = skipWsColon(p);
  if (!p || *p != '"') return Status::DecodeFailed;
  ++p;
  size_t i = 0;
  while (*p && *p != '"' && i + 1 < outCap) {
    if (*p == '\\' && p[1]) ++p;
    out[i++] = *p++;
  }
  out[i] = '\0';
  return i > 0 ? Status::Ok : Status::DecodeFailed;
}

Status rpcParseUIntField(const char* json, const char* keyQuoted, uint64_t& out) {
  out = 0;
  if (!json || !keyQuoted) return Status::InvalidArgument;
  const char* p = findKey(json, keyQuoted);
  if (!p) return Status::DecodeFailed;
  p = skipWsColon(p);
  if (!p) return Status::DecodeFailed;
  if (*p == '"') ++p;
  char* end = nullptr;
  out = strtoull(p, &end, 10);
  if (end == p) return Status::DecodeFailed;
  return Status::Ok;
}

Status rpcParseError(const char* json, char* errOut, size_t errCap) {
  if (!errOut || errCap == 0) return Status::InvalidArgument;
  errOut[0] = '\0';
  Status st = rpcParseStringField(json, "\"error\"", errOut, errCap);
  if (st == Status::Ok && errOut[0]) return Status::Ok;
  return Status::DecodeFailed;
}

Status rpcParseAccountSequence(const char* json, uint32_t& sequence, uint32_t& ledgerIndex) {
  sequence = 0;
  ledgerIndex = 0;
  if (!json) return Status::InvalidArgument;

  uint64_t seq = 0;
  if (rpcParseUIntField(json, "\"Sequence\"", seq) != Status::Ok) return Status::DecodeFailed;
  sequence = static_cast<uint32_t>(seq);

  uint64_t led = 0;
  if (rpcParseUIntField(json, "\"ledger_current_index\"", led) != Status::Ok) {
    if (rpcParseUIntField(json, "\"ledger_index\"", led) != Status::Ok) led = 0;
  }
  ledgerIndex = static_cast<uint32_t>(led);
  return sequence > 0 ? Status::Ok : Status::DecodeFailed;
}

Status rpcParseAccountBalanceDrops(const char* json, uint64_t& drops) {
  drops = 0;
  char buf[32];
  if (rpcParseStringField(json, "\"Balance\"", buf, sizeof(buf)) == Status::Ok) {
    drops = strtoull(buf, nullptr, 10);
    return Status::Ok;
  }
  return rpcParseUIntField(json, "\"Balance\"", drops);
}

Status rpcParseSubmitEngineResult(const char* json, char* resultOut, size_t resultCap,
                                  char* hashOut, size_t hashCap) {
  if (!json || !resultOut || resultCap == 0) return Status::InvalidArgument;
  resultOut[0] = '\0';
  if (hashOut && hashCap) hashOut[0] = '\0';

  Status st = rpcParseStringField(json, "\"engine_result\"", resultOut, resultCap);
  if (st != Status::Ok) {
    st = rpcParseStringField(json, "\"error\"", resultOut, resultCap);
    if (st != Status::Ok) return Status::DecodeFailed;
  }
  if (hashOut && hashCap > 1) {
    rpcParseStringField(json, "\"hash\"", hashOut, hashCap);
  }
  return Status::Ok;
}

bool rpcIsTesSuccess(const char* engineResult) {
  return engineResult && engineResult[0] == 't' && engineResult[1] == 'e' && engineResult[2] == 's';
}

bool rpcIsTerQueued(const char* engineResult) {
  if (!engineResult || engineResult[0] != 't' || engineResult[1] != 'e' || engineResult[2] != 'r') {
    return false;
  }
  // terQUEUED
  return strstr(engineResult, "QUEUED") != nullptr;
}

EngineClass rpcEngineClass(const char* engineResult) {
  if (!engineResult || engineResult[0] != 't' || engineResult[1] != 'e') return EngineClass::Unknown;
  switch (engineResult[2]) {
    case 's':
      return EngineClass::Tes;
    case 'r':
      return EngineClass::Ter;
    case 'c':
      return EngineClass::Tec;
    case 'f':
      return EngineClass::Tef;
    case 'm':
      return EngineClass::Tem;
    default:
      return EngineClass::Unknown;
  }
}

Status rpcParseTxEngineResult(const char* json, char* resultOut, size_t resultCap,
                              bool* outValidated) {
  if (!json || !resultOut || resultCap == 0) return Status::InvalidArgument;
  resultOut[0] = '\0';
  if (outValidated) *outValidated = false;

  // Prefer final meta result when validated.
  char metaKey[] = "\"TransactionResult\"";
  Status st = rpcParseStringField(json, metaKey, resultOut, resultCap);
  if (st != Status::Ok) {
    st = rpcParseStringField(json, "\"engine_result\"", resultOut, resultCap);
  }
  if (st != Status::Ok) return st;

  // validated: true
  const char* v = strstr(json, "\"validated\"");
  if (v && outValidated) {
    const char* colon = strchr(v, ':');
    if (colon) {
      const char* p = colon + 1;
      while (*p == ' ' || *p == '\t') ++p;
      if (strncmp(p, "true", 4) == 0) *outValidated = true;
    }
  }
  return Status::Ok;
}

Status rpcParseFeeDrops(const char* json, uint64_t& feeDrops) {
  // Prefer open_ledger_fee (what gets you into the open ledger), then median, then base.
  // Some public nets report base_fee/open_ledger_fee = "1" (valid for that network).
  feeDrops = 0;
  if (!json) return Status::InvalidArgument;
  char buf[32];
  uint64_t openFee = 0, medianFee = 0, baseFee = 0, minFee = 0;
  if (rpcParseStringField(json, "\"open_ledger_fee\"", buf, sizeof(buf)) == Status::Ok) {
    openFee = strtoull(buf, nullptr, 10);
  }
  if (rpcParseStringField(json, "\"median_fee\"", buf, sizeof(buf)) == Status::Ok) {
    medianFee = strtoull(buf, nullptr, 10);
  }
  if (rpcParseStringField(json, "\"base_fee\"", buf, sizeof(buf)) == Status::Ok) {
    baseFee = strtoull(buf, nullptr, 10);
  } else {
    rpcParseUIntField(json, "\"base_fee\"", baseFee);
  }
  if (rpcParseStringField(json, "\"minimum_fee\"", buf, sizeof(buf)) == Status::Ok) {
    minFee = strtoull(buf, nullptr, 10);
  }
  // Use the highest of open/base/min so we clear open-ledger + load floors.
  feeDrops = openFee;
  if (baseFee > feeDrops) feeDrops = baseFee;
  if (minFee > feeDrops) feeDrops = minFee;
  // Cap median contribution: useful under load but avoid overpaying when median is huge.
  if (medianFee > feeDrops && medianFee < 100000ULL) feeDrops = medianFee;
  if (feeDrops > 0) return Status::Ok;
  return Status::DecodeFailed;
}

Status rpcParseLedgerCloseTime(const char* json, uint32_t& closeTime) {
  closeTime = 0;
  if (!json) return Status::InvalidArgument;
  uint64_t v = 0;
  // Prefer close_time (ledger RPC); some streams use ledger_time.
  if (rpcParseUIntField(json, "\"close_time\"", v) == Status::Ok && v > 0) {
    closeTime = static_cast<uint32_t>(v);
    return Status::Ok;
  }
  if (rpcParseUIntField(json, "\"ledger_time\"", v) == Status::Ok && v > 0) {
    closeTime = static_cast<uint32_t>(v);
    return Status::Ok;
  }
  return Status::DecodeFailed;
}

Status rpcParseLedgerIndex(const char* json, uint32_t& ledgerIndex) {
  ledgerIndex = 0;
  uint64_t v = 0;
  if (rpcParseUIntField(json, "\"ledger_index\"", v) == Status::Ok && v > 0) {
    ledgerIndex = static_cast<uint32_t>(v);
    return Status::Ok;
  }
  if (rpcParseUIntField(json, "\"seq\"", v) == Status::Ok && v > 0) {
    ledgerIndex = static_cast<uint32_t>(v);
    return Status::Ok;
  }
  if (rpcParseUIntField(json, "\"ledger_current_index\"", v) == Status::Ok && v > 0) {
    ledgerIndex = static_cast<uint32_t>(v);
    return Status::Ok;
  }
  return Status::DecodeFailed;
}

Status rpcParseAccountInfoSnapshot(const char* json, AccountInfoSnapshot& out) {
  memset(&out, 0, sizeof(out));
  if (!json) return Status::InvalidArgument;

  char err[64];
  if (rpcParseError(json, err, sizeof(err)) == Status::Ok) {
    if (strcmp(err, "actNotFound") == 0) {
      out.found = false;
      return Status::Ok;
    }
    return Status::Rejected;
  }

  uint32_t seq = 0, led = 0;
  if (rpcParseAccountSequence(json, seq, led) != Status::Ok) return Status::DecodeFailed;
  out.sequence = seq;
  out.ledgerIndex = led;
  out.found = true;

  uint64_t drops = 0;
  if (rpcParseAccountBalanceDrops(json, drops) == Status::Ok) out.balanceDrops = drops;

  uint64_t oc = 0;
  if (rpcParseUIntField(json, "\"OwnerCount\"", oc) == Status::Ok) {
    out.ownerCount = static_cast<uint32_t>(oc);
  }
  return Status::Ok;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

Status rpcHexToHash256(const char* hex64, uint8_t out[32]) {
  if (!hex64 || !out) return Status::InvalidArgument;
  if (strlen(hex64) != 64) return Status::InvalidField;
  for (int i = 0; i < 32; ++i) {
    int hi = hexNibble(hex64[i * 2]);
    int lo = hexNibble(hex64[i * 2 + 1]);
    if (hi < 0 || lo < 0) return Status::DecodeFailed;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return Status::Ok;
}

Status rpcParseObjectIds(const char* json, const char* typeMarker, uint8_t outIds[][32],
                         uint8_t maxIds, uint8_t& outCount) {
  outCount = 0;
  if (!json || !outIds || maxIds == 0) return Status::InvalidArgument;

  const char* p = json;
  while (outCount < maxIds && (p = strstr(p, "\"index\"")) != nullptr) {
    // If a type marker is required, ensure it appears in the preceding ~400 chars
    // of this object (between previous '{' and this index).
    if (typeMarker && typeMarker[0]) {
      const char* windowStart = p;
      size_t back = 0;
      while (windowStart > json && back < 400) {
        --windowStart;
        ++back;
        if (*windowStart == '{') break;
      }
      size_t wlen = (size_t)(p - windowStart);
      bool found = false;
      for (size_t i = 0; i + strlen(typeMarker) <= wlen; ++i) {
        if (strncmp(windowStart + i, typeMarker, strlen(typeMarker)) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        p += 7;
        continue;
      }
    }

    const char* q = skipWsColon(p);
    if (!q || *q != '"') {
      p += 7;
      continue;
    }
    ++q;
    char hex[65];
    size_t i = 0;
    while (*q && *q != '"' && i < 64) hex[i++] = *q++;
    hex[i] = '\0';
    if (i == 64) {
      if (rpcHexToHash256(hex, outIds[outCount]) == Status::Ok) ++outCount;
    }
    p = q;
  }
  return Status::Ok;
}

Status rpcParseFirstObjectId(const char* json, const char* typeMarker, uint8_t outId[32]) {
  uint8_t ids[1][32];
  uint8_t n = 0;
  Status st = rpcParseObjectIds(json, typeMarker, ids, 1, n);
  if (st != Status::Ok) return st;
  if (n == 0) return Status::DecodeFailed;
  memcpy(outId, ids[0], 32);
  return Status::Ok;
}




Status rpcParseStreamMessageType(const char* json, StreamMessageType& out) {
  out = StreamMessageType::Unknown;
  if (!json) return Status::InvalidArgument;
  char typeBuf[40];
  if (rpcParseStringField(json, "\"type\"", typeBuf, sizeof(typeBuf)) == Status::Ok) {
    if (strcmp(typeBuf, "ledgerClosed") == 0) {
      out = StreamMessageType::LedgerClosed;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "transaction") == 0) {
      out = StreamMessageType::Transaction;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "validationReceived") == 0 || strcmp(typeBuf, "validation") == 0) {
      out = StreamMessageType::Validation;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "consensusPhase") == 0) {
      out = StreamMessageType::ConsensusPhase;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "serverStatus") == 0) {
      out = StreamMessageType::ServerStatus;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "bookChanges") == 0) {
      out = StreamMessageType::BookChanges;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "manifestReceived") == 0) {
      out = StreamMessageType::ManifestReceived;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "peerStatusChange") == 0) {
      out = StreamMessageType::PeerStatusChange;
      return Status::Ok;
    }
    if (strcmp(typeBuf, "response") == 0) {
      out = StreamMessageType::Response;
      return Status::Ok;
    }
  }
  // JSON-RPC replies often omit type and use result/status
  if (strstr(json, "\"result\"") && strstr(json, "\"status\"")) {
    out = StreamMessageType::Response;
    return Status::Ok;
  }
  return Status::Ok;
}

Status rpcParseLedgerClosedEvent(const char* json, LedgerClosedEvent& out) {
  memset(&out, 0, sizeof(out));
  if (!json) return Status::InvalidArgument;
  StreamMessageType t = StreamMessageType::Unknown;
  rpcParseStreamMessageType(json, t);
  (void)t;
  uint64_t idx = 0;
  if (rpcParseUIntField(json, "\"ledger_index\"", idx) != Status::Ok) {
    return Status::DecodeFailed;
  }
  out.ledgerIndex = static_cast<uint32_t>(idx);
  out.ledgerHash[0] = '\0';
  rpcParseStringField(json, "\"ledger_hash\"", out.ledgerHash, sizeof(out.ledgerHash));
  uint64_t txn = 0;
  if (rpcParseUIntField(json, "\"txn_count\"", txn) == Status::Ok) {
    out.txnCount = static_cast<uint32_t>(txn);
  }
  uint64_t lt = 0;
  if (rpcParseUIntField(json, "\"ledger_time\"", lt) == Status::Ok) {
    out.ledgerTime = static_cast<uint32_t>(lt);
  }
  uint64_t u = 0;
  if (rpcParseUIntField(json, "\"fee_base\"", u) == Status::Ok) {
    out.feeBase = static_cast<uint32_t>(u);
  }
  u = 0;
  if (rpcParseUIntField(json, "\"reserve_base\"", u) == Status::Ok) {
    out.reserveBase = static_cast<uint32_t>(u);
  }
  u = 0;
  if (rpcParseUIntField(json, "\"reserve_inc\"", u) == Status::Ok) {
    out.reserveInc = static_cast<uint32_t>(u);
  }
  u = 0;
  if (rpcParseUIntField(json, "\"network_id\"", u) == Status::Ok) {
    out.networkId = static_cast<uint32_t>(u);
  }
  out.valid = true;
  return Status::Ok;
}

Status rpcParseLedgerClosedValidatedLedgers(const char* json, char* out, size_t outCap) {
  if (!json || !out || outCap == 0) return Status::InvalidArgument;
  out[0] = '\0';
  return rpcParseStringField(json, "\"validated_ledgers\"", out, outCap);
}

Status rpcParseTransactionStreamEvent(const char* json, TransactionStreamEvent& out) {
  memset(&out, 0, sizeof(out));
  if (!json) return Status::InvalidArgument;
  // Prefer top-level hash (API v2 stream); also nested
  if (rpcParseStringField(json, "\"hash\"", out.hash, sizeof(out.hash)) != Status::Ok) {
    out.hash[0] = '\0';
  }
  if (rpcParseStringField(json, "\"engine_result\"", out.engineResult, sizeof(out.engineResult)) !=
      Status::Ok) {
    out.engineResult[0] = '\0';
  }
  // TransactionType / Account / Destination — first match (often under tx_json)
  if (rpcParseStringField(json, "\"TransactionType\"", out.txType, sizeof(out.txType)) !=
      Status::Ok) {
    out.txType[0] = '\0';
  }
  if (rpcParseStringField(json, "\"Account\"", out.account, sizeof(out.account)) != Status::Ok) {
    out.account[0] = '\0';
  }
  if (rpcParseStringField(json, "\"Destination\"", out.destination, sizeof(out.destination)) !=
      Status::Ok) {
    out.destination[0] = '\0';
  }
  uint64_t led = 0;
  if (rpcParseUIntField(json, "\"ledger_index\"", led) == Status::Ok) {
    out.ledgerIndex = static_cast<uint32_t>(led);
  }
  // validated may be bool true without quotes — simple scan
  out.validated = (strstr(json, "\"validated\":true") != nullptr) ||
                  (strstr(json, "\"validated\": true") != nullptr);
  out.valid = (out.hash[0] != '\0' || out.txType[0] != '\0' || out.engineResult[0] != '\0');
  return out.valid ? Status::Ok : Status::DecodeFailed;
}

bool rpcIsReportingUnsupported(const char* json) {
  if (!json) return false;
  return strstr(json, "reportingUnsupported") != nullptr;
}

}  // namespace xahau

