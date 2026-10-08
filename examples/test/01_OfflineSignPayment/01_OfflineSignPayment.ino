/**
 * XahauClient Example - 01_OfflineSignPayment
 *
 * Offline Payment encode and sign
 *
 * Demonstrates: Signs a Payment offline and prints the tx blob; no Wi-Fi.
 * Hardware: ESP32 / ESP32-S3 (offline; no Wi-Fi required)
 * Network: N/A (offline). Use Testnet seeds only if you adapt this sketch for live use.
 * Config: Replace any seed placeholders (e.g. sEdPLACEHOLDER_REPLACE_ME) before adapting for live use. Never commit real secrets.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

// Local only - use a faucet-funded testnet seed when experimenting.
// Leave as empty / placeholder in committed examples.
#ifndef XAHAU_DEMO_SEED
#define XAHAU_DEMO_SEED "sEdPLACEHOLDER_REPLACE_ME"
#endif

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println(F("=== XahauClient Offline Payment ==="));
  Serial.print(F("Version: "));
  Serial.println(XAHAU_CLIENT_VERSION);
  Serial.print(F("Tx catalog: "));
  Serial.println(static_cast<unsigned>(xahauTxTypeCount()));

  XahauKeyPair keys;
  XahauStatus st = xahauDeriveKeysFromSeed(XAHAU_DEMO_SEED, keys);
  Serial.print(F("deriveKeys: "));
  Serial.println(xahauStatusName(st));
  if (st != XahauStatus::Ok) {
    Serial.println(F("Set XAHAU_DEMO_SEED to a valid sEd... or s... seed to sign."));
    return;
  }

  Serial.print(F("Address: "));
  Serial.println(keys.address);

  PaymentParams pay;
  strncpy(pay.destination, "rYOUR_DESTINATION_ADDRESS", sizeof(pay.destination) - 1);  // replace with your destination r-address
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1000000;
  pay.common.feeDrops = 12;
  pay.common.sequence = 1;              // replace with live sequence before submit
  pay.common.lastLedgerSequence = 100;  // replace with live LLS
  pay.common.flags = (keys.curve == XahauCurve::Secp256k1) ? 0x80000000u : 0;

  char blob[2048];
  st = xahauBuildAndSignPayment(keys, pay, blob, sizeof(blob));
  Serial.print(F("buildAndSign: "));
  Serial.println(xahauStatusName(st));
  if (st == XahauStatus::Ok) {
    Serial.println(blob);
  }
  xahauSecureClear(keys);
}

void loop() {}
