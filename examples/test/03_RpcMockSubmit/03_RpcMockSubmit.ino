/**
 * XahauClient Example - 03_RpcMockSubmit
 *
 * Mock RPC submit flow
 *
 * Demonstrates: Exercises mock transport submit path offline.
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
  Serial.println(F("=== XahauClient RPC Mock Submit ==="));

  xahau::MockXahauTransport mock;
  mock.push("account_info",
            "{\"result\":{\"account_data\":{\"Sequence\":1,\"Balance\":\"100000000\"},"
            "\"ledger_current_index\":50}}");
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":\"MOCKHASH\"}}}");

  xahau::XahauRpcClient client(mock);

  // Demo keys would come from seed in a real sketch
  Serial.println(F("Mock scripts loaded (account_info + submit)."));
  Serial.println(F("Live WSS: examples/wss/Transport/EndToEndPrepareSubmit"));
  Serial.println(F("Offline builders: examples/rpc/"));
}

void loop() {}
