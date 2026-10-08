/**
 * XahauClient Example - AccountStream
 *
 * WSS account stream subscribe
 *
 * Demonstrates: Subscribes to account transaction streams.
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
// CONFIG
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int   XAHAU_NETWORK  = 1;

// Classic family seed sEd... or s...  (never commit mainnet keys)
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";

// Classic address to watch. Empty => derive from XAHAU_SEED when possible.
const char* ACCOUNT       = "";

// How long to listen for account txs (ms). Idle accounts often see zero events.
const uint32_t LISTEN_MS = 12000;

// Stop early after this many transaction events (0 = listen full LISTEN_MS).
const int TARGET_TXS = 0;

const bool PRINT_RAW_JSON = false;

static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_printTmp[768];
static char s_account[48];

struct StreamCtx {
  volatile int txCount;
};

static StreamCtx s_ctx;

static void onStream(const char* json, size_t /*len*/, void* user) {
  StreamCtx* ctx = static_cast<StreamCtx*>(user);
  if (!json || !ctx) return;

  xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
  xahau::rpcParseStreamMessageType(json, t);
  if (t != xahau::StreamMessageType::Transaction && !strstr(json, "\"transaction\"")) {
    return;
  }

  xahau::TransactionStreamEvent ev;
  if (xahau::rpcParseTransactionStreamEvent(json, ev) != xahau::Status::Ok) {
    return;
  }

  ctx->txCount++;
  Serial.print(F("tx #"));
  Serial.print(ctx->txCount);
  if (ev.txType[0]) {
    Serial.print(F(" type="));
    Serial.print(ev.txType);
  }
  if (ev.hash[0]) {
    Serial.print(F(" hash="));
    Serial.print(ev.hash);
  }
  if (ev.account[0]) {
    Serial.print(F(" Account="));
    Serial.print(ev.account);
  }
  Serial.print(F(" validated="));
  Serial.println(ev.validated ? F("true") : F("false"));

  if (PRINT_RAW_JSON) {
    size_t n = strlen(json);
    if (n > 700) {
      memcpy(s_printTmp, json, 700);
      s_printTmp[700] = '\0';
      Serial.println(s_printTmp);
    } else {
      Serial.println(json);
    }
  }
}

static bool resolveAccount(char* out, size_t outCap) {
  if (ACCOUNT && ACCOUNT[0]) {
    strncpy(out, ACCOUNT, outCap - 1);
    out[outCap - 1] = '\0';
    return true;
  }
  if (!XAHAU_SEED || !XAHAU_SEED[0] || strstr(XAHAU_SEED, "PLACEHOLDER")) {
    Serial.println(F("Set ACCOUNT or a valid XAHAU_SEED in CONFIG."));
    return false;
  }
  xahau::KeyPair keys;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, keys) != xahau::Status::Ok) {
    Serial.println(F("deriveKeysFromSeed failed."));
    return false;
  }
  strncpy(out, keys.address, outCap - 1);
  out[outCap - 1] = '\0';
  xahau::secureClear(keys);
  return true;
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
  Serial.println(F("=== XahauClient WSS - account stream (subscribe) ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  s_ctx.txCount = 0;
  s_account[0] = '\0';

  if (!connectWs()) return;
  if (!resolveAccount(s_account, sizeof(s_account))) {
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("Watching: "));
  Serial.println(s_account);

  s_ws.setStreamHandler(onStream, &s_ctx);
  xahau::Status st = s_ws.connect(20000);
  printStatus("connect", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  const char* accts[] = {s_account};
  st = s_client.subscribeAccounts(accts, 1);
  printStatus("subscribeAccounts", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }

  Serial.print(F("Listening "));
  Serial.print(LISTEN_MS);
  Serial.println(F(" ms for account transactions..."));

  const uint32_t t0 = millis();
  while ((millis() - t0) < LISTEN_MS) {
    if (TARGET_TXS > 0 && s_ctx.txCount >= TARGET_TXS) break;
    s_client.poll();
    delay(10);
  }

  st = s_client.unsubscribeAccounts(accts, 1);
  printStatus("unsubscribeAccounts", st);

  Serial.print(F("txs_received="));
  Serial.println(s_ctx.txCount);
  // Subscribe path exercised even if account is idle
  Serial.println(F("stream: Ok"));

  s_ws.setStreamHandler(nullptr, nullptr);
  s_ws.close();
  Serial.println(F("Done."));
}

void loop() {}
