/**
 * XahauClient Example - URITokenMint
 *
 * URITokenMint transaction
 *
 * Demonstrates: Builds and optionally submits URITokenMint.
 * Hardware: ESP32 / ESP32-S3 (Wi-Fi + WebSocketsClient when XAHAU_FEATURE_NET=1)
 * Network: Xahau Testnet by default (NetworkID 21338); set XAHAU_NETWORK=0 for Mainnet (21337).
 * Config: Set WIFI_SSID / WIFI_PASSWORD and XAHAU_SEED placeholders (sEdPLACEHOLDER_REPLACE_ME). Prefer DO_SUBMIT=false until ready. Never commit real credentials.
 *
 * SPDX-License-Identifier: MIT
 */
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* XAHAU_SEED    = "sEdPLACEHOLDER_REPLACE_ME";
// 0 = Mainnet (NetworkID 21337), 1 = Testnet (NetworkID 21338)
const int   XAHAU_NETWORK = 1;
const bool  DO_SUBMIT     = false;  // offline by default for library example
const bool  PRINT_BLOB    = true;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient - URITokenMint ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  xahau::KeyPair keys;
  xahau::Status st = xahau::deriveKeysFromSeed(XAHAU_SEED, keys);
  if (st != xahau::Status::Ok) {
    Serial.print(F("deriveKeysFromSeed failed: "));
    Serial.println(xahau::statusName(st));
    Serial.println(F("Paste a valid family seed into XAHAU_SEED."));
    return;
  }
  Serial.print(F("Address: "));
  Serial.println(keys.address);
  Serial.print(F("Curve:   "));
  Serial.println(keys.curve == xahau::Curve::Ed25519 ? F("Ed25519") : F("secp256k1"));

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.print(net.name);
  Serial.print(F("  NetworkID="));
  Serial.println(net.protocolNetworkId);

  xahau::URITokenMintParams p;
  strncpy(p.uri, "https://example.com/uritoken/1", sizeof(p.uri) - 1);
  // Optional: p.hasDigest = true; strncpy(p.digestHex, "...64 hex...", sizeof(p.digestHex)-1);
  // Optional sale listing: p.amount.kind = xahau::AmountKind::XrpDrops; p.amount.drops = 1000000ULL;
  p.common.feeDrops = 12;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  xahau::sketch::applyNetworkId(p.common, net);
  xahau::sketch::applyCanonicalFlag(p.common, keys.curve);

  static char s_blob[2048];
  auto doSign = [&]() -> xahau::Status {
    return xahau::buildAndSignURITokenMint(keys, p, s_blob, sizeof(s_blob));
  };

  st = doSign();
  Serial.print(F("sign: "));
  Serial.println(xahau::statusName(st));
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keys);
    return;
  }
  if (PRINT_BLOB) {
    Serial.println(F("Signed blob (hex):"));
    Serial.println(s_blob);
  }
  if (!DO_SUBMIT) {
    Serial.println(F("DO_SUBMIT=false - offline only. Done."));
    xahau::secureClear(keys);
    return;
  }

#if XAHAU_FEATURE_NET
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    xahau::secureClear(keys);
    return;
  }
  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  s_ws.setEndpoint(net);
  st = xahau::sketch::prepareCommon(s_client, keys, p.common);
  if (st != xahau::Status::Ok) {
    Serial.print(F("prepareCommon failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    return;
  }
  xahau::sketch::applyNetworkId(p.common, net);
  Serial.print(F("Sequence: "));
  Serial.println(p.common.sequence);
  Serial.print(F("LLS:      "));
  Serial.println(p.common.lastLedgerSequence);
  Serial.print(F("Fee:      "));
  Serial.println((unsigned long)p.common.feeDrops);
  Serial.print(F("NetworkID:"));
  Serial.println(p.common.networkId);
  st = doSign();
  if (st != xahau::Status::Ok) {
    Serial.print(F("re-sign failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    return;
  }
  xahau::sketch::submitAndLog(s_client, s_blob);
#else
  Serial.println(F("XAHAU_FEATURE_NET disabled - cannot submit."));
#endif
  xahau::secureClear(keys);
  Serial.println(F("Done."));
}

void loop() {}
