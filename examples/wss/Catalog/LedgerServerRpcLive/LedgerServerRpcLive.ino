/**
 * XahauClient Example — LedgerServerRpcLive
 *
 * Live ledger/server RPC catalog
 *
 * Demonstrates: Runs live ledger and server WebSocket RPCs.
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
const int   XAHAU_NETWORK  = 2;
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";
const bool  PRINT_RAW_JSON = false;

static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_req[512];
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

static void live(const char* label, xahau::Status buildSt) {
  printSt(label, buildSt);
  if (buildSt != xahau::Status::Ok) return;
  xahau::Status st = s_client.rawRequest(s_req, s_respBig, sizeof(s_respBig));
  printSt("  rawRequest", st);
  if (st == xahau::Status::Ok && strstr(s_respBig, "\"error\"")) {
    char err[48];
    if (xahau::rpcParseError(s_respBig, err, sizeof(err)) == xahau::Status::Ok && err[0]) {
      Serial.print(F("  server_error="));
      Serial.println(err);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient WSS live · Ledger/Server RPC catalog ==="));
  Serial.println(XAHAU_CLIENT_VERSION);

  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println(F("Done."));
    return;
  }
  xahau::KeyPair kp;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, kp) != xahau::Status::Ok) {
    Serial.println(F("derive failed"));
    Serial.println(F("Done."));
    return;
  }
  strncpy(s_addrA, kp.address, sizeof(s_addrA) - 1);
  s_addrA[sizeof(s_addrA) - 1] = '\0';
  xahau::secureClear(kp);
  Serial.print(F("accountA: "));
  Serial.println(s_addrA);

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.println(net.name);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(25000);

  live("ping", xahau::rpcBuildPing(s_req, sizeof(s_req)));
  live("fee", xahau::rpcBuildFee(s_req, sizeof(s_req)));
  live("feature", xahau::rpcBuildFeature(s_req, sizeof(s_req)));
  live("random", xahau::rpcBuildRandom(s_req, sizeof(s_req)));
  live("server_info", xahau::rpcBuildServerInfo(s_req, sizeof(s_req)));
  live("server_state", xahau::rpcBuildServerState(s_req, sizeof(s_req)));
  // server_definitions can be huge — still exercise command (may truncate resp)
  live("server_definitions", xahau::rpcBuildServerDefinitions(s_req, sizeof(s_req)));
  live("ledger_closed", xahau::rpcBuildLedgerClosed(s_req, sizeof(s_req)));
  live("ledger_current", xahau::rpcBuildLedgerCurrent(s_req, sizeof(s_req)));
  live("ledger validated", xahau::rpcBuildLedger("validated", false, s_req, sizeof(s_req)));
  live("ledger_data", xahau::rpcBuildLedgerData("validated", 5, s_req, sizeof(s_req)));
  live("ledger_entry AccountRoot",
       xahau::rpcBuildLedgerEntryAccountRoot(s_addrA, s_req, sizeof(s_req)));

  static const char* kDummy =
      "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
  live("ledger_entry by index",
       xahau::rpcBuildLedgerEntryByIndex(kDummy, s_req, sizeof(s_req)));
  // vault_info JSON builder removed 2026-09-28 (Vault EncodeOnly cut).

  Serial.print(F("live_ok="));
  Serial.print(s_ok);
  Serial.print(F(" live_fail="));
  Serial.println(s_fail);
  if (s_fail == 0) Serial.println(F("catalog: Ok"));
  Serial.println(F("Done."));
}

void loop() {}
