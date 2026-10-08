/**
 * XahauClient Example - EscrowCreate
 *
 * EscrowCreate transaction
 *
 * Demonstrates: Builds and optionally submits EscrowCreate.
 * Hardware: ESP32 / ESP32-S3 (Wi-Fi + WebSocketsClient when XAHAU_FEATURE_NET=1)
 * Network: Xahau Testnet by default (NetworkID 21338); set XAHAU_NETWORK=0 for Mainnet (21337).
 * Config: Set WIFI_SSID / WIFI_PASSWORD and XAHAU_SEED placeholders (sEdPLACEHOLDER_REPLACE_ME). Prefer DO_SUBMIT=false until ready. Never commit real credentials.
 *
 * SPDX-License-Identifier: MIT
 */
// Enable optional ESP32 WebSocket transport before the umbrella include.
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
// ESP32 loopTask default stack is ~8 KiB; raise it for WiFi + WSS + crypto.
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif

// ---------------------------------------------------------------------------
// CONFIG - edit like a .js / .py script (seed + WiFi only)
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Classic family seed: "sEd..." (Ed25519) or "s..." (secp256k1).
// Fund via Testnet faucet, then paste here (same idea as Wallet.from_seed).
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";

// 0 = Mainnet (real value!), 1 = Testnet (default); 2+ also maps to Testnet (no public Devnet)
const int   XAHAU_NETWORK  = 1;

// true = WiFi + submit; false = offline build/sign only (prints blob hex)
const bool  DO_SUBMIT     = true;

// Print full signed blob hex to Serial (can be long)
const bool  PRINT_BLOB    = true;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient | EscrowCreate ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  // ---- keys from seed (like Xahau-py / Xahau.js Wallet.from_seed) ----
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

  // ---- build typed params ----
  // Default: XAH escrow. For TokenEscrow:
  //   IOU  -> amount.kind=Issued (+ issuer asfAllowTrustLineLocking)
  //   MPT  -> amount.kind=Mpt + issuance id (48 hex)
  xahau::EscrowCreateParams p;
  strncpy(p.destination, "rYOUR_DESTINATION_ADDRESS", sizeof(p.destination) - 1);  // replace with your destination r-address
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  strncpy(p.amount.issued.value, "1", sizeof(p.amount.issued.value) - 1);
  strncpy(p.amount.issued.currency, "USD", sizeof(p.amount.issued.currency) - 1);
  strncpy(p.amount.issued.issuer, "rYOUR_ISSUER_ADDRESS",  // replace with your issuer r-address
          sizeof(p.amount.issued.issuer) - 1);
  p.amount.mpt.value = 10ULL;
  if (!xahau::sketch::parseIssuanceId("000000000000000000000000000000000000000000000000",
                                     p.amount.mpt.issuanceId)) {
    Serial.println(F("Bad MPT issuance placeholder"));
    return;
  }
  // FinishAfter must be AFTER ledger close at create (else tecNO_PERMISSION).
  // Live sketches should set finishAfter = fetchLedgerCloseTime() + N.
  p.finishAfter = 900000000UL;  // offline placeholder (far future)
  p.cancelAfter = 0;            // omit
  p.common.feeDrops = 12;
  p.common.sequence = 1;              // overwritten when DO_SUBMIT
  p.common.lastLedgerSequence = 100;  // overwritten when DO_SUBMIT
  xahau::sketch::applyCanonicalFlag(p.common, keys.curve);

  // ---- sign helper (rebuild after network seq/fee refresh) ----

  static char s_blob[2048];
  auto doSign = [&]() -> xahau::Status {
    return xahau::buildAndSignEscrowCreate(keys, p, s_blob, sizeof(s_blob));
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

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.print(net.name);
  Serial.print(F("  wss://"));
  Serial.print(net.wsHost);
  Serial.print(F(":"));
  Serial.println(net.wsPort);

  // CRITICAL: WebSocketsClient is large - never put WsXahauTransport on setup() stack.
  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  s_ws.setEndpoint(net);

  st = xahau::sketch::prepareCommon(s_client, keys, p.common);
  if (st != xahau::Status::Ok) {
    Serial.print(F("prepareCommon failed: "));
    Serial.println(xahau::statusName(st));
    Serial.println(F("Is the account funded on this network?"));
    xahau::secureClear(keys);
    return;
  }
  Serial.print(F("Sequence: "));
  Serial.println(p.common.sequence);
  Serial.print(F("LLS:      "));
  Serial.println(p.common.lastLedgerSequence);
  Serial.print(F("Fee:      "));
  Serial.println((unsigned long)p.common.feeDrops);

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
