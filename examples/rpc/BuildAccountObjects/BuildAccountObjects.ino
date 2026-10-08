/**
 * XahauClient Example - BuildAccountObjects
 *
 * RPC builder: account_objects
 *
 * Demonstrates: Builds an account_objects JSON-RPC request offline.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

const char* ACCOUNT = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient RPC offline | BuildAccountObjects ==="));

  char req[256];
  xahau::Status st = xahau::rpcBuildAccountObjects(ACCOUNT, nullptr, req, sizeof(req));
  Serial.print(F("all objects: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) Serial.println(req);

  st = xahau::rpcBuildAccountObjects(ACCOUNT, "vault", req, sizeof(req));
  Serial.print(F("type=vault: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) Serial.println(req);

  st = xahau::rpcBuildAccountObjects(ACCOUNT, "bad;drop", req, sizeof(req));
  Serial.print(F("bad filter rejected: "));
  Serial.println(xahau::statusName(st));
  Serial.println(F("Done."));
}

void loop() {}
