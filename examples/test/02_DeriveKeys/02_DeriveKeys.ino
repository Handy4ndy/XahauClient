/**
 * XahauClient Example - 02_DeriveKeys
 *
 * Key derivation from family seed
 *
 * Demonstrates: Derives Ed25519 and secp256k1 key pairs from placeholder seeds.
 * Hardware: ESP32 / ESP32-S3 (offline; no Wi-Fi required)
 * Network: N/A (offline). Use Testnet seeds only if you adapt this sketch for live use.
 * Config: Replace any seed placeholders (e.g. sEdPLACEHOLDER_REPLACE_ME) before adapting for live use. Never commit real secrets.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println(F("=== XahauClient Derive Keys ==="));

  const char* edSeed = "sEdPLACEHOLDER";
  const char* secpSeed = "sPLACEHOLDER";

  XahauKeyPair edKeys;
  XahauStatus st = xahau::deriveKeysFromSeed(edSeed, XahauCurve::Ed25519, edKeys);
  Serial.print(F("Ed25519: "));
  Serial.println(xahauStatusName(st));
  xahauSecureClear(edKeys);

  XahauKeyPair secpKeys;
  st = xahau::deriveKeysFromSeed(secpSeed, XahauCurve::Secp256k1, secpKeys);
  Serial.print(F("secp256k1: "));
  Serial.println(xahauStatusName(st));
  xahauSecureClear(secpKeys);
}

void loop() {}
