/**
 * XahauClient Example - MockClientFlow
 *
 * Mock client Payment flow
 *
 * Demonstrates: Mock transport: sequence, fee, sign Payment, submitBlob, wait for outcome.
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>
#include <string.h>

// Set XAHAU_SEED (A) and XAHAU_SEED_B (B) to Testnet family seeds
const char* XAHAU_SEED   = "sEdPLACEHOLDER_REPLACE_ME";
const char* XAHAU_SEED_B = "sEdPLACEHOLDER_B_REPLACE_ME";

static void printSt(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient RPC offline | MockClientFlow (A->B) ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  xahau::KeyPair keysA;
  xahau::KeyPair keysB;
  xahau::Status st = xahau::deriveKeysFromSeed(XAHAU_SEED, keysA);
  printSt("derive A", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Done."));
    return;
  }
  st = xahau::deriveKeysFromSeed(XAHAU_SEED_B, keysB);
  printSt("derive B", st);
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keysA);
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("accountA: "));
  Serial.println(keysA.address);
  Serial.print(F("accountB: "));
  Serial.println(keysB.address);

  // Scripted cluster responses (FIFO by command substring match)
  xahau::MockXahauTransport mock;
  mock.push("account_info",
            "{\"result\":{\"account_data\":{\"Sequence\":42,\"Balance\":\"100000000\","
            "\"OwnerCount\":0},\"ledger_current_index\":900}}");
  mock.push("fee",
            "{\"result\":{\"drops\":{\"base_fee\":\"12\",\"open_ledger_fee\":\"12\","
            "\"median_fee\":\"12\",\"minimum_fee\":\"10\"}}}");
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":"
            "\"EEFF00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDD\"}}}");
  mock.push("tx",
            "{\"result\":{\"validated\":true,\"meta\":{\"TransactionResult\":\"tesSUCCESS\"},"
            "\"hash\":\"EEFF00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDD\"}}");
  // Second payment: terQUEUED then final tx + optional ledger polls
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"terQUEUED\",\"tx_json\":{\"hash\":"
            "\"11223344556677889900AABBCCDDEEFF11223344556677889900AABBCCDDEEFF\"}}}");
  mock.push("tx",
            "{\"result\":{\"validated\":true,\"meta\":{\"TransactionResult\":\"tesSUCCESS\"}}}");
  mock.push("ledger_closed", "{\"result\":{\"ledger_index\":901}}");
  mock.push("ledger_closed", "{\"result\":{\"ledger_index\":901}}");
  mock.push("subscribe",
            "{\"result\":{},\"status\":\"success\",\"type\":\"response\"}");
  mock.push("unsubscribe",
            "{\"result\":{},\"status\":\"success\",\"type\":\"response\"}");
  mock.push("subscribe",
            "{\"result\":{},\"status\":\"success\",\"type\":\"response\"}");

  xahau::XahauRpcClient client(mock);

  uint32_t seq = 0, lls = 0;
  st = client.fetchSequenceAndLls(keysA.address, seq, lls);
  printSt("fetchSequenceAndLls", st);
  Serial.print(F("seq="));
  Serial.print(seq);
  Serial.print(F(" lls="));
  Serial.println(lls);

  uint64_t fee = 0;
  st = client.fetchFeeDrops(fee);
  printSt("fetchFeeDrops", st);
  if (st != xahau::Status::Ok || fee == 0) fee = 12;

  xahau::PaymentParams pay;
  memset(&pay, 0, sizeof(pay));
  strncpy(pay.destination, keysB.address, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;
  pay.common.sequence = seq;
  pay.common.lastLedgerSequence = lls;
  pay.common.feeDrops = fee;
  if (keysA.curve == xahau::Curve::Secp256k1) {
    pay.common.flags |= 0x80000000u;
  }

  char blob[2048];
  st = xahau::buildAndSignPayment(keysA, pay, blob, sizeof(blob));
  printSt("buildAndSignPayment", st);
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keysA);
    xahau::secureClear(keysB);
    Serial.println(F("Done."));
    return;
  }

  xahau::SubmitResult sr;
  memset(&sr, 0, sizeof(sr));
  st = client.submitBlob(blob, sr);
  printSt("submitBlob", st);
  Serial.print(F("engine_result: "));
  Serial.println(sr.engineResult);
  Serial.print(F("hash: "));
  Serial.println(sr.txHash);
  Serial.println(sr.tesSuccess ? F("tesSUCCESS") : F("not tesSUCCESS"));

  if (sr.txHash[0]) {
    xahau::SubmitResult looked;
    memset(&looked, 0, sizeof(looked));
    st = client.fetchTxResult(sr.txHash, looked);
    printSt("fetchTxResult", st);
    Serial.print(F("tx_engine: "));
    Serial.println(looked.engineResult);
  }

  // Second payment: queued submit then waitForOutcome (scripts pre-pushed above)
  pay.common.sequence = seq + 1;
  pay.common.lastLedgerSequence = lls + 1;
  st = xahau::buildAndSignPayment(keysA, pay, blob, sizeof(blob));
  printSt("buildAndSignPayment2", st);
  if (st == xahau::Status::Ok) {
    memset(&sr, 0, sizeof(sr));
    st = client.submitBlob(blob, sr);
    printSt("submitBlob queued", st);
    Serial.print(F("queued_hash: "));
    Serial.println(sr.txHash);
    memset(&sr, 0, sizeof(sr));
    // LLS=0 disables ledger expiry for fast mock wait
    st = client.waitForOutcome(
        "11223344556677889900AABBCCDDEEFF11223344556677889900AABBCCDDEEFF", 0, 3000, sr, 50);
    printSt("waitForOutcome", st);
    Serial.print(F("wait_engine: "));
    Serial.println(sr.engineResult);
  }

  st = client.subscribeLedger();
  printSt("subscribeLedger", st);
  st = client.unsubscribeLedger();
  printSt("unsubscribeLedger", st);

  const char* accts[] = {keysA.address, keysB.address};
  st = client.subscribeAccounts(accts, 2);
  printSt("subscribeAccounts A+B", st);

  xahau::secureClear(keysA);
  xahau::secureClear(keysB);
  Serial.println(F("mock flow: Ok"));
  Serial.println(F("Done."));
}

void loop() {}
