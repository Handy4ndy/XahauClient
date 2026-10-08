/**
 * XahauClient Example - PathNftRpcLive
 *
 * Live path/NFT RPC catalog
 *
 * Demonstrates: Runs live path and NFT-related WebSocket RPCs.
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
const char* XAHAU_SEED_B   = "sEdPLACEHOLDER_B_REPLACE_ME";
const bool  PRINT_RAW_JSON = false;

static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_req[640];
static char s_addrA[48];
static char s_addrB[48];
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
  Serial.println(F("=== XahauClient WSS live | Path/NFT RPC catalog ==="));
  Serial.println(XAHAU_CLIENT_VERSION);
  Serial.println(F("Xahau note: nft_* often server-error (NFToken); deposit_authorized+book_offers are primary."));

  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println(F("Done."));
    return;
  }
  if (!resolveAddr(XAHAU_SEED, s_addrA, sizeof(s_addrA)) ||
      !resolveAddr(XAHAU_SEED_B, s_addrB, sizeof(s_addrB))) {
    Serial.println(F("derive A/B failed"));
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("accountA: "));
  Serial.println(s_addrA);
  Serial.print(F("accountB: "));
  Serial.println(s_addrB);

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.println(net.name);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(25000);

  live("deposit_authorized A->B",
       xahau::rpcBuildDepositAuthorized(s_addrA, s_addrB, s_req, sizeof(s_req)));
  // XAH / IOU book (issuer = B is arbitrary; may be empty book)
  live("book_offers XAH/USD@B",
       xahau::rpcBuildBookOffers("XAH", nullptr, "USD", s_addrB, 5, s_req, sizeof(s_req)));
  // amm_info JSON builder removed (AMM EncodeOnly cut); probe via rawRequest if needed.
  static const char* kNft =
      "00080000B4F4AFC5FBCBD76873F18006173D213FDC0084B40000099B00000000";
  live("nft_info", xahau::rpcBuildNftInfo(kNft, s_req, sizeof(s_req)));
  live("nft_history", xahau::rpcBuildNftHistory(kNft, 5, s_req, sizeof(s_req)));
  live("nft_buy_offers", xahau::rpcBuildNftBuyOffers(kNft, 5, s_req, sizeof(s_req)));
  live("nft_sell_offers", xahau::rpcBuildNftSellOffers(kNft, 5, s_req, sizeof(s_req)));
  live("nfts_by_issuer A",
       xahau::rpcBuildNftsByIssuer(s_addrA, 5, s_req, sizeof(s_req)));

  // channel_verify with dummy values - server will reject; build+transport still exercised
  live("channel_verify",
       xahau::rpcBuildChannelVerify(
           "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF", "1000",
           "ED5F5AC8B98974A3CA843326D9B88CEBD0560177B973EE0B149F782CFAA06DC66A",
           "3045022100AABBCCDDEEFF00112233445566778899AABBCCDDEEFF00112233445566778899"
           "022100AABBCCDDEEFF00112233445566778899AABBCCDDEEFF00112233445566778899",
           s_req, sizeof(s_req)));

  Serial.print(F("live_ok="));
  Serial.print(s_ok);
  Serial.print(F(" live_fail="));
  Serial.println(s_fail);
  if (s_fail == 0) Serial.println(F("catalog: Ok"));
  Serial.println(F("Done."));
}

void loop() {}
