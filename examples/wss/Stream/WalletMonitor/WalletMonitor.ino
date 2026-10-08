/**
 * XahauClient Example — WalletMonitor
 *
 * WSS wallet monitor
 *
 * Demonstrates: Subscribes to ledger + account streams for a wallet monitor pattern.
 * Hardware: ESP32 / ESP32-S3 (Wi-Fi + WebSocketsClient)
 * Network: Xahau Testnet by default (NetworkID 21338); set XAHAU_NETWORK=0 for Mainnet (21337).
 * Config: Set WIFI_SSID / WIFI_PASSWORD and any XAHAU_SEED placeholders before live use. Never commit real credentials.
 *
 * SPDX-License-Identifier: MIT
 */
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(32 * 1024);
#endif

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int   XAHAU_NETWORK  = 1;
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";
const char* ACCOUNT       = "";
// Also watch proposed (open ledger) txs for this account
const bool  INCLUDE_PROPOSED = true;
const uint32_t LISTEN_MS = 20000;
const int TARGET_LEDGERS = 2;

static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_account[48];

struct Ctx {
  volatile int ledgers;
  volatile int txs;
  uint32_t lastLedger;
};

static Ctx s_ctx;

static void onStream(const char* json, size_t, void* user) {
  Ctx* ctx = static_cast<Ctx*>(user);
  if (!json || !ctx) return;
  // Do not call request()/subscribe* from this handler.
  xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
  xahau::rpcParseStreamMessageType(json, t);
  if (t == xahau::StreamMessageType::LedgerClosed) {
    xahau::LedgerClosedEvent ev;
    memset(&ev, 0, sizeof(ev));
    if (xahau::rpcParseLedgerClosedEvent(json, ev) != xahau::Status::Ok || !ev.valid) return;
    ctx->ledgers++;
    ctx->lastLedger = ev.ledgerIndex;
    Serial.print(F("ledger index="));
    Serial.print(ev.ledgerIndex);
    Serial.print(F(" fee_base="));
    Serial.print(ev.feeBase);
    Serial.print(F(" txn_count="));
    Serial.println(ev.txnCount);
    return;
  }
  if (t == xahau::StreamMessageType::Transaction) {
    xahau::TransactionStreamEvent ev;
    memset(&ev, 0, sizeof(ev));
    if (xahau::rpcParseTransactionStreamEvent(json, ev) != xahau::Status::Ok) return;
    ctx->txs++;
    Serial.print(F("tx type="));
    Serial.print(ev.txType[0] ? ev.txType : "?");
    Serial.print(F(" eng="));
    Serial.print(ev.engineResult[0] ? ev.engineResult : "?");
    Serial.print(F(" validated="));
    Serial.println(ev.validated ? F("true") : F("false"));
  }
}

static bool resolveAccount(char* out, size_t outCap) {
  if (ACCOUNT && ACCOUNT[0]) {
    strncpy(out, ACCOUNT, outCap - 1);
    out[outCap - 1] = '\0';
    return true;
  }
  if (!XAHAU_SEED || !XAHAU_SEED[0] || strstr(XAHAU_SEED, "PLACEHOLDER")) {
    Serial.println(F("Set ACCOUNT or XAHAU_SEED."));
    return false;
  }
  xahau::KeyPair keys;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, keys) != xahau::Status::Ok) return false;
  strncpy(out, keys.address, outCap - 1);
  out[outCap - 1] = '\0';
  xahau::secureClear(keys);
  return true;
}

static void printStatus(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient WSS - wallet monitor ==="));
  Serial.println(XAHAU_CLIENT_VERSION);
  s_ctx.ledgers = 0;
  s_ctx.txs = 0;
  s_ctx.lastLedger = 0;

  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) return;
  if (!resolveAccount(s_account, sizeof(s_account))) {
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("Watching: "));
  Serial.println(s_account);

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(20000);
  s_ws.setStreamHandler(onStream, &s_ctx);

  xahau::Status st = s_ws.connect(20000);
  printStatus("connect", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  const char* streams[] = {"ledger"};
  const char* accts[] = {s_account};
  xahau::SubscribeParams p;
  memset(&p, 0, sizeof(p));
  p.streams = streams;
  p.streamCount = 1;
  p.accounts = accts;
  p.accountCount = 1;
  if (INCLUDE_PROPOSED) {
    p.accountsProposed = accts;
    p.accountsProposedCount = 1;
  }

  st = s_client.subscribeEx(p);
  printStatus("subscribeEx", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  const uint32_t t0 = millis();
  while ((millis() - t0) < LISTEN_MS) {
    if (s_ctx.ledgers >= TARGET_LEDGERS) break;
    s_client.poll();
    delay(10);
  }

  st = s_client.unsubscribeEx(p);
  printStatus("unsubscribeEx", st);
  Serial.print(F("ledgers="));
  Serial.print(s_ctx.ledgers);
  Serial.print(F(" txs="));
  Serial.println(s_ctx.txs);
  Serial.println(F("stream: Ok"));
  s_ws.setStreamHandler(nullptr, nullptr);
  s_ws.close();
  Serial.println(F("Done."));
}

void loop() {}
