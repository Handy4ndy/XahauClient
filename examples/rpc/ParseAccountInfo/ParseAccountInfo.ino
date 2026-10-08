/**
 * XahauClient Example - ParseAccountInfo
 *
 * RPC parser: account_info
 *
 * Demonstrates: Parses a sample account_info response offline.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient RPC offline | ParseAccountInfo ==="));

  const char* json =
      "{\"result\":{\"account_data\":{\"Account\":\"rABC\",\"Balance\":\"25000000\","
      "\"Sequence\":7,\"OwnerCount\":2},\"ledger_current_index\":100}}";

  xahau::AccountInfoSnapshot snap;
  xahau::Status st = xahau::rpcParseAccountInfoSnapshot(json, snap);
  Serial.print(F("parse: "));
  Serial.println(xahau::statusName(st));
  Serial.print(F("found="));
  Serial.println(snap.found ? F("yes") : F("no"));
  Serial.print(F("sequence="));
  Serial.println(snap.sequence);
  Serial.print(F("balance drops="));
  Serial.println((unsigned long)snap.balanceDrops);
  Serial.print(F("ownerCount="));
  Serial.println(snap.ownerCount);
  Serial.print(F("ledgerIndex="));
  Serial.println(snap.ledgerIndex);

  const char* missing = "{\"error\":\"actNotFound\",\"status\":\"error\"}";
  st = xahau::rpcParseAccountInfoSnapshot(missing, snap);
  Serial.print(F("actNotFound: found="));
  Serial.println(snap.found ? F("yes") : F("no"));
  Serial.println(F("Done."));
}

void loop() {}
