/**
 * XahauClient Example — 06_GenerateSeed
 *
 * Generate family seed from entropy
 *
 * Demonstrates: Encodes a family seed from caller-supplied test entropy (not production RNG).
 * Hardware: ESP32 / ESP32-S3 (offline; no Wi-Fi required)
 * Network: N/A (offline). Use Testnet seeds only if you adapt this sketch for live use.
 * Config: Replace any seed placeholders (e.g. sEdPLACEHOLDER_REPLACE_ME) before adapting for live use. Never commit real secrets.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

// Fixed public vector (ClientTest / ripple-keypairs) — test only.
static const uint8_t kTestEntropy[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println(F("=== XahauClient Generate Seed (test entropy) ==="));
  Serial.println(F("WARNING: Copy this seed offline. After vault encryption,"));
  Serial.println(F("         it is NOT recoverable from this device."));

  char seed[xahau::kFamilySeedBufSize];
  XahauKeyPair keys;

  XahauStatus st = xahauGenerateKeyPairFromEntropy(kTestEntropy, XahauCurve::Ed25519, seed,
                                                 sizeof(seed), keys);
  Serial.print(F("Ed25519 status: "));
  Serial.println(xahauStatusName(st));
  if (st == xahau::Status::Ok) {
    Serial.print(F("Seed: "));
    Serial.println(seed);
    Serial.print(F("Address: "));
    Serial.println(keys.address);
  }
  xahauSecureClear(keys);
  memset(seed, 0, sizeof(seed));

  st = xahauGenerateKeyPairFromEntropy(kTestEntropy, XahauCurve::Secp256k1, seed, sizeof(seed),
                                      keys);
  Serial.print(F("secp256k1 status: "));
  Serial.println(xahauStatusName(st));
  if (st == xahau::Status::Ok) {
    Serial.print(F("Seed: "));
    Serial.println(seed);
    Serial.print(F("Address: "));
    Serial.println(keys.address);
  }
  xahauSecureClear(keys);
  memset(seed, 0, sizeof(seed));

  Serial.println(F("Done."));
}

void loop() {}
