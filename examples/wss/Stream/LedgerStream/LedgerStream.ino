/**
 * XahauClient Example - LedgerStream
 *
 * WSS ledger stream subscribe
 *
 * Demonstrates: Subscribes to the ledgerClosed stream.
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

// ---------------------------------------------------------------------------
// CONFIG - edit like a .js / .py script
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 0 = Mainnet, 1 = Testnet (default); 2+ also maps to Testnet (no public Devnet)
const int   XAHAU_NETWORK  = 1;

// How many ledgerClosed events before exit (finite - suite-friendly).
const int   TARGET_LEDGERS = 2;

// Max wait for TARGET_LEDGERS (ms). Testnet ~3-4s/ledger.
const uint32_t STREAM_TIMEOUT_MS = 45000;

// Print full stream JSON snippets (can flood Serial)
const bool  PRINT_RAW_JSON = false;

// BSS - never put large buffers / WebSocketsClient on setup() stack
static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_printTmp[768];

struct StreamCtx {
  volatile int ledgerCount;
  uint32_t lastIndex;
};

static StreamCtx s_ctx;

static void onStream(const char* json, size_t /*len*/, void* user) {
  StreamCtx* ctx = static_cast<StreamCtx*>(user);
  if (!json || !ctx) return;

  xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
  xahau::rpcParseStreamMessageType(json, t);
  if (t != xahau::StreamMessageType::LedgerClosed && !strstr(json, "ledgerClosed")) {
    return;
  }

  xahau::LedgerClosedEvent ev;
  if (xahau::rpcParseLedgerClosedEvent(json, ev) != xahau::Status::Ok || !ev.valid) {
    return;
  }

  ctx->ledgerCount++;
  ctx->lastIndex = ev.ledgerIndex;

  Serial.print(F("ledgerClosed #"));
  Serial.print(ctx->ledgerCount);
  Serial.print(F(" index="));
  Serial.print(ev.ledgerIndex);
  Serial.print(F(" txn_count="));
  Serial.println(ev.txnCount);
  if (ev.ledgerHash[0]) {
    Serial.print(F("  hash="));
    Serial.println(ev.ledgerHash);
  }

  if (PRINT_RAW_JSON) {
    size_t n = strlen(json);
    if (n > 700) {
      memcpy(s_printTmp, json, 700);
      s_printTmp[700] = '\0';
      Serial.println(s_printTmp);
      Serial.println(F("... (truncated)"));
    } else {
      Serial.println(json);
    }
  }
}

static bool connectWs() {
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) return false;
  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.print(net.name);
  Serial.print(F("  wss://"));
  Serial.print(net.wsHost);
  Serial.print(F(":"));
  Serial.println(net.wsPort);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(20000);
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
  Serial.println();
  Serial.println(F("=== XahauClient WSS - ledger stream (subscribe) ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  s_ctx.ledgerCount = 0;
  s_ctx.lastIndex = 0;

  if (!connectWs()) return;

  s_ws.setStreamHandler(onStream, &s_ctx);
  xahau::Status st = s_ws.connect(20000);
  printStatus("connect", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  st = s_client.subscribeLedger();
  printStatus("subscribeLedger", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  Serial.print(F("Waiting for "));
  Serial.print(TARGET_LEDGERS);
  Serial.println(F(" ledgerClosed event(s)..."));

  const uint32_t t0 = millis();
  while (s_ctx.ledgerCount < TARGET_LEDGERS &&
         (millis() - t0) < STREAM_TIMEOUT_MS) {
    s_client.poll();
    delay(10);
  }

  st = s_client.unsubscribeLedger();
  printStatus("unsubscribeLedger", st);

  Serial.print(F("ledgers_received="));
  Serial.println(s_ctx.ledgerCount);
  if (s_ctx.ledgerCount > 0) {
    Serial.print(F("last_ledger_index="));
    Serial.println(s_ctx.lastIndex);
    Serial.println(F("stream: Ok"));
  } else {
    Serial.println(F("stream: Timeout (no ledgerClosed)"));
  }

  s_ws.setStreamHandler(nullptr, nullptr);
  s_ws.close();
  Serial.println(F("Done."));
}

void loop() {}
