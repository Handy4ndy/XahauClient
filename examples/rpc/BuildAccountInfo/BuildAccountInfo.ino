/**
 * XahauClient Example — BuildAccountInfo
 *
 * RPC builder: account_info
 *
 * Demonstrates: Builds an account_info JSON-RPC request offline.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

// Classic address shape (builder validates base58-ish r-address)
const char* ACCOUNT = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F("=== XahauClient RPC offline · BuildAccountInfo ==="));

  char req[192];
  xahau::Status st = xahau::rpcBuildAccountInfo(ACCOUNT, req, sizeof(req));
  Serial.print(F("rpcBuildAccountInfo: "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) {
    Serial.println(req);
  }

  // Injection is rejected
  st = xahau::rpcBuildAccountInfo("rEvil\",\"cmd\":\"x", req, sizeof(req));
  Serial.print(F("injection attempt: "));
  Serial.println(xahau::statusName(st));  // InvalidAddress
  Serial.println(F("Done."));
}

void loop() {}
