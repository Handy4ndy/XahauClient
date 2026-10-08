#!/usr/bin/env python3
"""Patch rpc.cpp builders with validation + JSON escaping."""
from pathlib import Path

RPC = Path(__file__).resolve().parents[2] / "src" / "Xahau" / "rpc" / "rpc.cpp"

NEW_PREFIX = r'''// rpc.cpp - JSON-RPC builders/parsers without ArduinoJson
#include "rpc.h"

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

/** Classic r-address shape: starts with 'r', base58, length 3..40 (tests + real). */
static bool isSafeAccountId(const char* s) {
  if (!s || s[0] != 'r') return false;
  size_t n = 0;
  for (const char* p = s; *p; ++p, ++n) {
    if (!isBase58Char(*p)) return false;
    if (n >= 40) return false;
  }
  return n >= 3;
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

'''


def main():
    text = RPC.read_text(encoding="utf-8")
    marker = "static const char* findKey"
    idx = text.find(marker)
    if idx < 0:
        raise SystemExit("marker not found")
    RPC.write_text(NEW_PREFIX + text[idx:], encoding="utf-8")
    print("patched", RPC)


if __name__ == "__main__":
    main()
