/**
 * XahauClient Example — PaymentMultiSign
 *
 * Payment multi-sign
 *
 * Demonstrates: Live multi-sign Payment (account A, signer B).
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
SET_LOOP_TASK_STACK_SIZE(20 * 1024);
#endif

// ---------------------------------------------------------------------------
// CONFIG
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Account that owns the transaction (must have SignerList with B).
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";
// Multi-signer key (must be on A's SignerList).
const char* XAHAU_SEED_B   = "sEdPLACEHOLDER_SIGNER_B";

const int   XAHAU_NETWORK  = 2;
const bool  DO_SUBMIT     = true;
const bool  PRINT_BLOB    = true;

// Drops paid from A (small smoke amount).
const uint64_t PAY_DROPS  = 1000000ULL;  // 1 XAH

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient · PaymentMultiSign ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  // Primary account uses name `keys` so harness inject (submitAndLog run_id) matches.
  xahau::KeyPair keys;
  xahau::KeyPair keysB;
  xahau::Status st = xahau::deriveKeysFromSeed(XAHAU_SEED, keys);
  if (st != xahau::Status::Ok) {
    Serial.println(F("deriveKeysFromSeed(A) failed — set XAHAU_SEED"));
    return;
  }
  st = xahau::deriveKeysFromSeed(XAHAU_SEED_B, keysB);
  if (st != xahau::Status::Ok) {
    Serial.println(F("deriveKeysFromSeed(B) failed — set XAHAU_SEED_B"));
    xahau::secureClear(keys);
    return;
  }
  if (memcmp(keys.accountId, keysB.accountId, 20) == 0) {
    Serial.println(F("A and B seeds must be different accounts"));
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }

  Serial.print(F("Account A: "));
  Serial.println(keys.address);
  Serial.print(F("Signer  B: "));
  Serial.println(keysB.address);

  xahau::PaymentParams p;
  // Send to B (counterparty); harness may overwrite p.destination.
  strncpy(p.destination, keysB.address, sizeof(p.destination) - 1);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = PAY_DROPS;
  p.common.feeDrops = 24;  // (1 sig + 1) * ~12 drops; raised after prepare
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  // Multi-sign uses empty SigningPubKey — no single-sig FullyCanonical needed for B.
  // Still fine to set on common; bagPrepareMultiSign clears SigningPubKey.

  static char s_blob[4096];
  auto doMultiSign = [&]() -> xahau::Status {
    static xahau::TxFieldBag bag;
    bag.clear();
    xahau::Status s = xahau::fillPayment(p, keys.accountId, keys.publicKey, bag);
    if (s != xahau::Status::Ok) return s;
    s = xahau::bagPrepareMultiSign(bag);  // empty SigningPubKey
    if (s != xahau::Status::Ok) return s;
    xahau::MultiSigner ms;
    s = xahau::multiSignAs(bag, keysB, ms);
    if (s != xahau::Status::Ok) return s;
    return xahau::multiSignFinalize(bag, &ms, 1, s_blob, sizeof(s_blob));
  };

  st = doMultiSign();
  Serial.print(F("multi-sign: "));
  Serial.println(xahau::statusName(st));
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }
  if (PRINT_BLOB) {
    Serial.println(F("Multi-sign blob (hex):"));
    Serial.println(s_blob);
  }

  if (!DO_SUBMIT) {
    Serial.println(F("DO_SUBMIT=false - offline only. Done."));
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }

#if XAHAU_FEATURE_NET
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.println(net.name);

  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  xahau::sketch::applyNetworkTransport(s_ws, net);

  // Sequence/LLS/fee for account A (the Account field).
  st = xahau::sketch::prepareCommon(s_client, keys, p.common);
  if (st != xahau::Status::Ok) {
    Serial.print(F("prepareCommon failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }
  // Multi-sign fee = (N+1) * base; N=1 → 2×.
  {
    const uint64_t base = p.common.feeDrops > 0 ? p.common.feeDrops : 12ULL;
    xahau::sketch::ensureFeeAtLeast(p.common, base * 2ULL);
  }
  Serial.print(F("Sequence: "));
  Serial.println(p.common.sequence);
  Serial.print(F("LLS:      "));
  Serial.println(p.common.lastLedgerSequence);
  Serial.print(F("Fee:      "));
  Serial.println((unsigned long)p.common.feeDrops);

  st = doMultiSign();
  if (st != xahau::Status::Ok) {
    Serial.print(F("re multi-sign failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    xahau::secureClear(keysB);
    return;
  }
  if (PRINT_BLOB) {
    Serial.println(F("Re-signed multi-sign blob (hex):"));
    Serial.println(s_blob);
  }
  xahau::sketch::submitAndLog(s_client, s_blob);
#else
  Serial.println(F("XAHAU_FEATURE_NET disabled - cannot submit."));
#endif

  xahau::secureClear(keys);
  xahau::secureClear(keysB);
  Serial.println(F("Done."));
}

void loop() {}
