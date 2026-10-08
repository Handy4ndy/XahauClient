/**
 * XahauClient Example - OracleSet
 *
 * OracleSet transaction
 *
 * Demonstrates: Builds and optionally submits OracleSet.
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
#include <time.h>
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif

// ---------------------------------------------------------------------------
// CONFIG - edit like a .js / .py script
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Classic family seed: "sEd..." (Ed25519) or "s..." (secp256k1).
const char* XAHAU_SEED    = "sEdPLACEHOLDER_REPLACE_ME";

// 0 = Mainnet (NetworkID 21337), 1 = Testnet (NetworkID 21338)
const int   XAHAU_NETWORK = 1;

// true = WiFi + submit; false = offline build/sign only
const bool  DO_SUBMIT     = true;
const bool  PRINT_BLOB    = true;

// Oracle document fields (Provider + AssetClass required on create)
const uint32_t ORACLE_DOC_ID = 1;
const char* BASE_ASSET  = "XAH";
const char* QUOTE_ASSET = "USD";
// price = assetPrice * 10^(-scale)  e.g. 500000 / 10^6 = 0.5
const uint64_t ASSET_PRICE = 500000ULL;
const uint8_t  PRICE_SCALE = 6;
const char* PROVIDER    = "XahauClient";
const char* ASSET_CLASS = "currency";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient | OracleSet ==="));
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
  Serial.print(F("  protocol NetworkID="));
  Serial.println(net.protocolNetworkId);

  xahau::OracleSetParams p;
  p.oracleDocumentID = ORACLE_DOC_ID;
  // Placeholder; before submit we set UNIX LastUpdateTime from ledger close_time.
  p.lastUpdateTime = (uint32_t)time(nullptr);
  if (p.lastUpdateTime < 1700000000UL) p.lastUpdateTime = 1700000000UL;
  strncpy(p.baseAsset, BASE_ASSET, sizeof(p.baseAsset) - 1);
  strncpy(p.quoteAsset, QUOTE_ASSET, sizeof(p.quoteAsset) - 1);
  p.hasPrice = true;
  p.assetPrice = ASSET_PRICE;
  p.scale = PRICE_SCALE;
  strncpy(p.provider, PROVIDER, sizeof(p.provider) - 1);
  strncpy(p.assetClass, ASSET_CLASS, sizeof(p.assetClass) - 1);
  p.common.feeDrops = 12;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  xahau::sketch::applyNetworkId(p.common, net);
  xahau::sketch::applyCanonicalFlag(p.common, keys.curve);

  static char s_blob[2048];
  static xahau::TxFieldBag s_bag;
  auto doSign = [&]() -> xahau::Status {
    s_bag.clear();
    xahau::Status s = xahau::fillOracleSet(p, keys.accountId, keys.publicKey, s_bag);
    if (s != xahau::Status::Ok) return s;
    return xahau::signTx(s_bag, keys, s_blob, sizeof(s_blob));
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

  // Best-effort NTP so LastUpdateTime is accepted by the ledger
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  for (int i = 0; i < 30 && time(nullptr) < 1700000000UL; ++i) delay(200);
  Serial.print(F("wss://"));
  Serial.print(net.wsHost);
  Serial.print(F(":"));
  Serial.println(net.wsPort);

  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  xahau::sketch::applyNetworkTransport(s_ws, net);
  // Demo convenience: allow unpinned TLS. For production mainnet, pin the cert fingerprint.
  s_ws.setRequireSslPin(false);
  s_ws.setInsecureSsl(true);

  st = xahau::sketch::prepareCommon(s_client, keys, p.common);
  if (st != xahau::Status::Ok) {
    Serial.print(F("prepareCommon failed: "));
    Serial.println(xahau::statusName(st));
    Serial.println(F("Is the account funded on this network?"));
    xahau::secureClear(keys);
    return;
  }
  // Re-apply NetworkID in case prepareCommon defaulted (it preserves if already set).
  xahau::sketch::applyNetworkId(p.common, net);

  // LastUpdateTime is UNIX epoch (XLS-47). Derive from ledger close_time (Ripple->Unix).
  {
    uint32_t closeRipple = 0;
    if (s_client.fetchLedgerCloseTime(closeRipple) == xahau::Status::Ok && closeRipple > 0) {
      p.lastUpdateTime = xahau::oracleUpdateTimeFromLedgerClose(closeRipple);
    } else {
      uint32_t ntp = (uint32_t)time(nullptr);
      p.lastUpdateTime = (ntp > 30) ? (ntp - 30) : ntp;
    }
  }
  Serial.print(F("LastUpdateTime (unix): "));
  Serial.println(p.lastUpdateTime);

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
