/**
 * XahauClient Example - ParseCatalog
 *
 * RPC response-parser catalog
 *
 * Demonstrates: Exercises public rpcParse* helpers offline.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>
#include <string.h>

static int s_ok = 0;
static int s_fail = 0;

static void line(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) s_ok++;
  else s_fail++;
}

static void expectTrue(const char* label, bool v) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(v ? F("yes") : F("no"));
  if (v) s_ok++;
  else s_fail++;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient RPC offline | ParseCatalog ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  // account_info snapshot
  {
    const char* json =
        "{\"result\":{\"account_data\":{\"Account\":\"rABC\",\"Balance\":\"25000000\","
        "\"Sequence\":7,\"OwnerCount\":2},\"ledger_current_index\":100}}";
    xahau::AccountInfoSnapshot snap;
    line("rpcParseAccountInfoSnapshot", xahau::rpcParseAccountInfoSnapshot(json, snap));
    expectTrue("found=yes", snap.found);
    Serial.print(F("sequence="));
    Serial.println(snap.sequence);
  }
  {
    const char* missing = "{\"error\":\"actNotFound\",\"status\":\"error\"}";
    xahau::AccountInfoSnapshot snap;
    line("rpcParseAccountInfoSnapshot actNotFound",
         xahau::rpcParseAccountInfoSnapshot(missing, snap));
    expectTrue("found=no", !snap.found);
  }

  // fee
  {
    const char* feeJson =
        "{\"result\":{\"drops\":{\"base_fee\":\"10\",\"median_fee\":\"12\","
        "\"minimum_fee\":\"10\",\"open_ledger_fee\":\"12\"}}}";
    uint64_t drops = 0;
    line("rpcParseFeeDrops", xahau::rpcParseFeeDrops(feeJson, drops));
    Serial.print(F("feeDrops="));
    Serial.println((unsigned long)drops);
  }

  // submit engine
  {
    const char* sub =
        "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":"
        "\"AABBCCDDEEFF00112233445566778899AABBCCDDEEFF00112233445566778899\"}}}";
    char eng[32], hash[65];
    line("rpcParseSubmitEngineResult",
         xahau::rpcParseSubmitEngineResult(sub, eng, sizeof(eng), hash, sizeof(hash)));
    expectTrue("rpcIsTesSuccess", xahau::rpcIsTesSuccess(eng));
    Serial.print(F("engine="));
    Serial.println(eng);
  }
  {
    expectTrue("rpcIsTerQueued", xahau::rpcIsTerQueued("terQUEUED"));
    expectTrue("rpcEngineClass Tes",
               xahau::rpcEngineClass("tesSUCCESS") == xahau::EngineClass::Tes);
  }

  // ledger index
  {
    const char* led = "{\"result\":{\"ledger_index\":12345}}";
    uint32_t idx = 0;
    line("rpcParseLedgerIndex", xahau::rpcParseLedgerIndex(led, idx));
    Serial.print(F("ledger="));
    Serial.println(idx);
  }

  // object ids (generic account_objects scrape)
  {
    const char* objs =
        "{\"result\":{\"account_objects\":[{\"LedgerEntryType\":\"Escrow\",\"index\":"
        "\"1111111111111111111111111111111111111111111111111111111111111111\"}]}}";
    uint8_t ids[2][32];
    uint8_t n = 0;
    line("rpcParseObjectIds",
         xahau::rpcParseObjectIds(objs, "\"Escrow\"", ids, 2, n));
    Serial.print(F("objectCount="));
    Serial.println(n);
  }

  // stream parsers
  {
    xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
    line("rpcParseStreamMessageType ledgerClosed",
         xahau::rpcParseStreamMessageType("{\"type\":\"ledgerClosed\"}", t));
    expectTrue("type LedgerClosed", t == xahau::StreamMessageType::LedgerClosed);

    const char* lc =
        "{\"type\":\"ledgerClosed\",\"ledger_index\":99,\"ledger_hash\":"
        "\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\","
        "\"txn_count\":3,\"fee_base\":10,\"reserve_base\":1000000,\"reserve_inc\":200000,"
        "\"network_id\":1,\"validated_ledgers\":\"1-99\"}";
    xahau::LedgerClosedEvent ev;
    memset(&ev, 0, sizeof(ev));
    line("rpcParseLedgerClosedEvent", xahau::rpcParseLedgerClosedEvent(lc, ev));
    expectTrue("ledger valid", ev.valid && ev.ledgerIndex == 99);
    char range[32];
    line("rpcParseLedgerClosedValidatedLedgers",
         xahau::rpcParseLedgerClosedValidatedLedgers(lc, range, sizeof(range)));
  }
  {
    const char* txj =
        "{\"type\":\"transaction\",\"validated\":true,\"ledger_index\":50,"
        "\"engine_result\":\"tesSUCCESS\","
        "\"hash\":\"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\","
        "\"tx_json\":{\"TransactionType\":\"Payment\","
        "\"Account\":\"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh\","
        "\"Destination\":\"rYOUR_DESTINATION_ADDRESS\"}}";  // replace with your destination r-address
    xahau::TransactionStreamEvent tev;
    memset(&tev, 0, sizeof(tev));
    line("rpcParseTransactionStreamEvent", xahau::rpcParseTransactionStreamEvent(txj, tev));
    expectTrue("tx Payment", tev.valid && strcmp(tev.txType, "Payment") == 0);
    expectTrue("engine tesSUCCESS", strcmp(tev.engineResult, "tesSUCCESS") == 0);
  }
  {
    expectTrue("rpcIsReportingUnsupported",
               xahau::rpcIsReportingUnsupported(
                   "{\"error\":\"reportingUnsupported\",\"status\":\"error\"}"));
  }

  Serial.print(F("parse_ok="));
  Serial.print(s_ok);
  Serial.print(F(" parse_fail="));
  Serial.println(s_fail);
  if (s_fail == 0) Serial.println(F("catalog: Ok"));
  else Serial.println(F("catalog: FAIL"));
  Serial.println(F("Done."));
}

void loop() {}
