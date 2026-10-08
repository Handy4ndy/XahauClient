/**
 * XahauClient Example — AccountRpcLive
 *
 * Live account_* RPC catalog
 *
 * Demonstrates: Runs a live set of account_* WebSocket RPCs.
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
const int   XAHAU_NETWORK  = 1;  // 0=Mainnet, 1=Testnet (2+ also Testnet; no public Devnet)
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";
const char* XAHAU_SEED_B   = "sEdPLACEHOLDER_B_REPLACE_ME";
const bool  PRINT_RAW_JSON = false;

static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_req[512];
static char s_printTmp[768];
static char s_addrA[48];
static int s_ok = 0;
static int s_fail = 0;

static void printSt(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) s_ok++;
  else s_fail++;
}

static void snippet(const char* json) {
  if (!PRINT_RAW_JSON || !json) return;
  size_t n = strlen(json);
  if (n > 700) {
    memcpy(s_printTmp, json, 700);
    s_printTmp[700] = '\0';
    Serial.println(s_printTmp);
    Serial.println(F("..."));
  } else {
    Serial.println(json);
  }
}

/** Build + live rawRequest. Transport Ok counts even if result has error. */
static void live(const char* label, xahau::Status buildSt) {
  printSt(label, buildSt);
  if (buildSt != xahau::Status::Ok) return;
  xahau::Status st = s_client.rawRequest(s_req, s_respBig, sizeof(s_respBig));
  printSt("  rawRequest", st);
  if (st == xahau::Status::Ok) {
    // Soft note if server error (still a successful round-trip)
    if (strstr(s_respBig, "\"error\"")) {
      char err[48];
      if (xahau::rpcParseError(s_respBig, err, sizeof(err)) == xahau::Status::Ok && err[0]) {
        Serial.print(F("  server_error="));
        Serial.println(err);
      }
    }
    snippet(s_respBig);
  }
}

static bool resolveAddr(const char* seed, char* out, size_t cap) {
  if (!seed || !seed[0] || strstr(seed, "PLACEHOLDER")) return false;
  xahau::KeyPair kp;
  if (xahau::deriveKeysFromSeed(seed, kp) != xahau::Status::Ok) return false;
  strncpy(out, kp.address, cap - 1);
  out[cap - 1] = '\0';
  xahau::secureClear(kp);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient WSS live · AccountRpc catalog ==="));
  Serial.println(XAHAU_CLIENT_VERSION);

  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println(F("Done."));
    return;
  }
  if (!resolveAddr(XAHAU_SEED, s_addrA, sizeof(s_addrA))) {
    Serial.println(F("derive A failed"));
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("accountA: "));
  Serial.println(s_addrA);
  (void)XAHAU_SEED_B;

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.println(net.name);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(25000);

  live("account_channels", xahau::rpcBuildAccountChannels(s_addrA, s_req, sizeof(s_req)));
  live("account_currencies", xahau::rpcBuildAccountCurrencies(s_addrA, s_req, sizeof(s_req)));
  live("account_nfts", xahau::rpcBuildAccountNfts(s_addrA, 20, s_req, sizeof(s_req)));
  live("account_offers", xahau::rpcBuildAccountOffers(s_addrA, 20, s_req, sizeof(s_req)));
  live("gateway_balances", xahau::rpcBuildGatewayBalances(s_addrA, s_req, sizeof(s_req)));
  live("noripple_check send",
       xahau::rpcBuildNoRippleCheck(s_addrA, "send", s_req, sizeof(s_req)));
  live("account_lines", xahau::rpcBuildAccountLines(s_addrA, s_req, sizeof(s_req)));
  live("account_objects",
       xahau::rpcBuildAccountObjects(s_addrA, nullptr, s_req, sizeof(s_req)));
  live("account_tx", xahau::rpcBuildAccountTx(s_addrA, 10, s_req, sizeof(s_req)));
  live("account_info", xahau::rpcBuildAccountInfo(s_addrA, s_req, sizeof(s_req)));

  Serial.print(F("live_ok="));
  Serial.print(s_ok);
  Serial.print(F(" live_fail="));
  Serial.println(s_fail);
  if (s_fail == 0) Serial.println(F("catalog: Ok"));
  Serial.println(F("Done."));
}

void loop() {}
