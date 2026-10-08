/**
 * XahauClient Example — BuildFeeAndSubmit
 *
 * RPC builders: fee + submit
 *
 * Demonstrates: Builds fee and submit-related JSON-RPC requests offline.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

// Even-length hex only (builder enforces)
const char* TX_BLOB_HEX = "12000022800000002400000001";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient RPC offline · BuildFeeAndSubmit ==="));

  char req[256];
  xahau::Status st = xahau::rpcBuildFee(req, sizeof(req));
  Serial.print(F("fee: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) Serial.println(req);

  st = xahau::rpcBuildSubmit(TX_BLOB_HEX, req, sizeof(req));
  Serial.print(F("submit: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) Serial.println(req);

  st = xahau::rpcBuildSubmitEx(TX_BLOB_HEX, true, req, sizeof(req));
  Serial.print(F("submit fail_hard: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) Serial.println(req);

  st = xahau::rpcBuildSubmit("ODD", req, sizeof(req));
  Serial.print(F("odd hex rejected: "));
  Serial.println(xahau::statusName(st));
  Serial.println(F("Done."));
}

void loop() {}
