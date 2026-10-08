/**
 * XahauClient Example - Remit
 *
 * Remit transaction
 *
 * Demonstrates: Builds and optionally submits Remit (multi-pay / URIToken modes).
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

// 0=XAH  1=+MintURIToken  2=XAH+USD  3=URITokenIDs  4=XAH+USD+URITokenIDs
const int   REMIT_MODE    = 2;  // showcase multi-IOU by default in this example
const char* MINT_URI      = "https://example.com/remit-mint/1";
// IOU issuer for modes 2/4 - replace with a real issuer before live submit
const char* USD_ISSUER    = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
const char* DEST_ACCOUNT  = "rYOUR_DESTINATION_ADDRESS";  // replace with your destination r-address
// Placeholder URITokenID for modes 3/4 (64 hex) - replace before live
const char* URI_TOKEN_ID_HEX =
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient | Remit ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);
  Serial.print(F("REMIT_MODE="));
  Serial.println(REMIT_MODE);

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

  xahau::RemitParams p;
  strncpy(p.destination, DEST_ACCOUNT, sizeof(p.destination) - 1);

  if (REMIT_MODE == 0 || REMIT_MODE == 1) {
    // Convenience single amount (encoded as Remit.Amounts[0] when amountCount==0)
    p.amount.kind = xahau::AmountKind::XrpDrops;  // native XAH drops (enum name historical)
    p.amount.drops = 10000ULL;  // 0.01 XAH
    if (REMIT_MODE == 1) {
      strncpy(p.mintUri, MINT_URI, sizeof(p.mintUri) - 1);
      p.mintHasDigest = false;
      Serial.print(F("MintURIToken URI: "));
      Serial.println(p.mintUri);
    }
  } else if (REMIT_MODE == 2 || REMIT_MODE == 4) {
    // Multi Amounts: XAH + USD IOU (host golden remit_xah_usd / remit_xah_usd_uri)
    p.amountCount = 2;
    p.amounts[0].kind = xahau::AmountKind::XrpDrops;
    p.amounts[0].drops = 10000ULL;  // 0.01 XAH
    p.amounts[1].kind = xahau::AmountKind::Issued;
    strncpy(p.amounts[1].issued.value, "5", sizeof(p.amounts[1].issued.value) - 1);
    strncpy(p.amounts[1].issued.currency, "USD", sizeof(p.amounts[1].issued.currency) - 1);
    strncpy(p.amounts[1].issued.issuer, USD_ISSUER, sizeof(p.amounts[1].issued.issuer) - 1);
    Serial.println(F("Amounts: XAH drops + 5 USD IOU"));
  } else if (REMIT_MODE == 3) {
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 1000ULL;
  } else {
    Serial.println(F("REMIT_MODE out of range (0..4)"));
    xahau::secureClear(keys);
    return;
  }

  if (REMIT_MODE == 3 || REMIT_MODE == 4) {
    if (!xahau::sketch::parseHash256(URI_TOKEN_ID_HEX, p.uriTokenIds[0])) {
      Serial.println(F("URI_TOKEN_ID_HEX invalid (need 64 hex chars)"));
      xahau::secureClear(keys);
      return;
    }
    p.uriTokenIdCount = 1;
    Serial.println(F("URITokenIDs[0] set"));
  }

  p.common.feeDrops = 12;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  xahau::sketch::applyNetworkId(p.common, net);
  xahau::sketch::applyCanonicalFlag(p.common, keys.curve);

  static char s_blob[4096];
  auto doSign = [&]() -> xahau::Status {
    return xahau::buildAndSignRemit(keys, p, s_blob, sizeof(s_blob));
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
  // Mint / multi-asset Remits often need a higher fee floor on live nets
  if (REMIT_MODE == 1) xahau::sketch::ensureFeeAtLeast(p.common, 100000ULL);
  if (REMIT_MODE >= 2) xahau::sketch::ensureFeeAtLeast(p.common, 20000ULL);
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
