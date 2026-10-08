/**
 * XahauClient Example - 05_MultiSignOffline
 *
 * Offline multi-sign
 *
 * Demonstrates: Builds a multi-signed transaction offline.
 * Hardware: ESP32 / ESP32-S3 (offline; no Wi-Fi required)
 * Network: N/A (offline). Use Testnet seeds only if you adapt this sketch for live use.
 * Config: Replace any seed placeholders (e.g. sEdPLACEHOLDER_REPLACE_ME) before adapting for live use. Never commit real secrets.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>
#include <string.h>

const char* XAHAU_SEED = "sEdPLACEHOLDER_REPLACE_ME";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient | MultiSign Offline ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  xahau::KeyPair keys;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, keys) != xahau::Status::Ok) {
    Serial.println(F("Bad seed - paste a faucet sEd... seed for a real encode."));
    return;
  }
  Serial.print(F("Account: "));
  Serial.println(keys.address);

  xahau::PaymentParams pay;
  pay.common.sequence = 1;
  pay.common.lastLedgerSequence = 100;
  pay.common.feeDrops = 12;
  if (keys.curve == xahau::Curve::Secp256k1) {
    pay.common.flags |= 0x80000000u;  // tfFullyCanonicalSig
  }
  strncpy(pay.destination, "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1000000ULL;

  static xahau::TxFieldBag bag;
  if (xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag) != xahau::Status::Ok) {
    Serial.println(F("fillPayment failed"));
    return;
  }
  if (xahau::bagPrepareMultiSign(bag) != xahau::Status::Ok) {
    Serial.println(F("bagPrepareMultiSign failed"));
    return;
  }

  xahau::MultiSigner ms;
  if (xahau::multiSignAs(bag, keys, ms) != xahau::Status::Ok) {
    Serial.println(F("multiSignAs failed"));
    return;
  }

  static char hex[4096];
  if (xahau::multiSignFinalize(bag, &ms, 1, hex, sizeof(hex)) != xahau::Status::Ok) {
    Serial.println(F("multiSignFinalize failed"));
    return;
  }
  Serial.println(F("Multi-sign blob (hex):"));
  Serial.println(hex);
  Serial.println(F("Done."));
  xahau::secureClear(keys);
}

void loop() {}
