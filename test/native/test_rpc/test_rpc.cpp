// Phase 3: RPC builders, parsers, MockXahauTransport, XahauRpcClient
#include <unity.h>
#include <string.h>
#include "XahauClient.h"

#if defined(__has_include)
#  if __has_include("test_seeds.local.h")
#    include "test_seeds.local.h"
#  else
#    include "test_seeds.example.h"
#  endif
#else
#  include "test_seeds.example.h"
#endif


// Well-known genesis classic address (valid Base58Check).
static const char* kValidAcct = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

void test_rpc_build_account_info() {
  char buf[160];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildAccountInfo(kValidAcct, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_info"));
  TEST_ASSERT_NOT_NULL(strstr(buf, kValidAcct));
  TEST_ASSERT_NOT_NULL(strstr(buf, "current"));
}

void test_rpc_build_rejects_injection() {
  char buf[256];
  // Quote / control injection in account must not produce open JSON string
  TEST_ASSERT_EQUAL(xahau::Status::InvalidAddress,
                    xahau::rpcBuildAccountInfo("rEvil\",\"command\":\"admin", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidAddress,
                    xahau::rpcBuildAccountInfo("not-an-address", buf, sizeof(buf)));
  // Wrong checksum / short fake r-address rejected by Base58Check
  TEST_ASSERT_EQUAL(xahau::Status::InvalidAddress,
                    xahau::rpcBuildAccountInfo("rTestAddrNotValidChecksumXX", buf, sizeof(buf)));
  // Odd-length / non-hex blob rejected
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubmit("ABC", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubmit("ZZZZ", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildAccountObjects(kValidAcct, "vault\";drop", buf, sizeof(buf)));
  // Multisign body must be a JSON object
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubmitMultisigned("not-json", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubmitMultisigned("{\"TransactionType\":\"Payment\"}", buf,
                                                    sizeof(buf)));
}

void test_rpc_build_submit_fail_hard() {
  char buf[128];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubmitEx("ABCD", true, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "submit"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ABCD"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "fail_hard"));
}

void test_rpc_build_fee_ledger_ping() {
  char buf[128];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildFee(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "fee"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildLedgerClosed(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ledger_closed"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildPing(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ping"));
}

void test_rpc_parse_account_info_snapshot() {
  const char* json =
      "{\"result\":{\"account_data\":{\"Account\":\"rABC\",\"Balance\":\"25000000\","
      "\"Sequence\":7,\"OwnerCount\":2},\"ledger_current_index\":100}}";
  xahau::AccountInfoSnapshot snap;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseAccountInfoSnapshot(json, snap));
  TEST_ASSERT_TRUE(snap.found);
  TEST_ASSERT_EQUAL_UINT32(7, snap.sequence);
  TEST_ASSERT_EQUAL_UINT32(100, snap.ledgerIndex);
  TEST_ASSERT_EQUAL_UINT64(25000000ULL, snap.balanceDrops);
  TEST_ASSERT_EQUAL_UINT32(2, snap.ownerCount);
}

void test_rpc_parse_act_not_found() {
  const char* json = "{\"result\":{},\"error\":\"actNotFound\",\"status\":\"error\"}";
  xahau::AccountInfoSnapshot snap;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseAccountInfoSnapshot(json, snap));
  TEST_ASSERT_FALSE(snap.found);
}

void test_rpc_parse_submit() {
  const char* json =
      "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":"
      "\"DEADBEEFCAFE\"}}}";
  char res[32], hash[80];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseSubmitEngineResult(json, res, sizeof(res), hash, sizeof(hash)));
  TEST_ASSERT_EQUAL_STRING("tesSUCCESS", res);
  TEST_ASSERT_EQUAL_STRING("DEADBEEFCAFE", hash);
  TEST_ASSERT_TRUE(xahau::rpcIsTesSuccess(res));
}

void test_rpc_parse_fee() {
  // Policy: max(open, base, min); median used only if higher and < 100k.
  const char* json =
      "{\"result\":{\"drops\":{\"base_fee\":\"10\",\"open_ledger_fee\":\"12\",\"median_fee\":\"11\"}"
      "}}";
  uint64_t fee = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseFeeDrops(json, fee));
  TEST_ASSERT_EQUAL_UINT64(12ULL, fee);  // open_ledger_fee wins over base 10
}

void test_mock_transport_and_client_fetch_seq() {
  xahau::MockXahauTransport mock;
  mock.push("account_info",
            "{\"result\":{\"account_data\":{\"Sequence\":42,\"Balance\":\"1000000\"},"
            "\"ledger_current_index\":900}}");
  mock.push("fee", "{\"result\":{\"drops\":{\"base_fee\":\"12\"}}}");
  mock.push("ledger_closed", "{\"result\":{\"ledger_index\":900}}");

  xahau::XahauRpcClient client(mock);
  // Values below kMinLedgerOffset (40) still floor to 40.
  client.setLedgerOffset(30);

  uint32_t seq = 0, lls = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.fetchSequenceAndLls(kValidAcct, seq, lls));
  TEST_ASSERT_EQUAL_UINT32(42, seq);
  TEST_ASSERT_EQUAL_UINT32(940, lls);  // 900 + max(30, 40)
  TEST_ASSERT_EQUAL_UINT32(40, client.effectiveLedgerOffset());
  TEST_ASSERT_NOT_NULL(strstr(mock.lastRequest(), "account_info"));
}

void test_lls_offset_above_floor() {
  xahau::MockXahauTransport mock;
  mock.push("account_info",
            "{\"result\":{\"account_data\":{\"Sequence\":1,\"Balance\":\"1\"},"
            "\"ledger_current_index\":1000}}");
  xahau::XahauRpcClient client(mock);
  client.setLedgerOffset(80);
  uint32_t seq = 0, lls = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.fetchSequenceAndLls(kValidAcct, seq, lls));
  TEST_ASSERT_EQUAL_UINT32(1080, lls);  // 1000 + 80
}

void test_mock_client_submit() {
  xahau::MockXahauTransport mock;
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":\"ABC123\"}}}");

  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.submitBlob("120000", sr));
  TEST_ASSERT_TRUE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("tesSUCCESS", sr.engineResult);
  TEST_ASSERT_EQUAL_STRING("ABC123", sr.txHash);
}

void test_mock_submit_tec_rejected() {
  xahau::MockXahauTransport mock;
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"tecUNFUNDED_PAYMENT\",\"tx_json\":{\"hash\":\"X\"}}}");
  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Rejected, client.submitBlob("120000", sr));
  TEST_ASSERT_FALSE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("tecUNFUNDED_PAYMENT", sr.engineResult);
}

void test_engine_class_and_queued() {
  TEST_ASSERT_TRUE(xahau::rpcIsTesSuccess("tesSUCCESS"));
  TEST_ASSERT_TRUE(xahau::rpcIsTerQueued("terQUEUED"));
  TEST_ASSERT_FALSE(xahau::rpcIsTerQueued("tesSUCCESS"));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::EngineClass::Tes),
                    static_cast<int>(xahau::rpcEngineClass("tesSUCCESS")));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::EngineClass::Ter),
                    static_cast<int>(xahau::rpcEngineClass("terQUEUED")));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::EngineClass::Tef),
                    static_cast<int>(xahau::rpcEngineClass("tefPAST_SEQ")));
}

void test_mock_submit_ter_queued() {
  xahau::MockXahauTransport mock;
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"terQUEUED\",\"tx_json\":{\"hash\":\"Q1\"}}}");
  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Queued, client.submitBlob("120000", sr));
  TEST_ASSERT_TRUE(sr.queued);
  TEST_ASSERT_FALSE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("terQUEUED", sr.engineResult);
  TEST_ASSERT_EQUAL_STRING("Q1", sr.txHash);
}

void test_mock_fetch_tx_result() {
  xahau::MockXahauTransport mock;
  mock.push("tx",
            "{\"result\":{\"validated\":true,\"meta\":{\"TransactionResult\":\"tesSUCCESS\"},"
            "\"hash\":\"AABB\"}}");
  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.fetchTxResult("AABB", sr));
  TEST_ASSERT_TRUE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("tesSUCCESS", sr.engineResult);
}

void test_submit_timeout_same_blob_retry() {
  // First submit attempt times out; second succeeds with same blob (idempotent path).
  xahau::MockXahauTransport mock;
  TEST_ASSERT_TRUE(mock.pushTimeout("submit"));
  TEST_ASSERT_TRUE(mock.push(
      "submit",
      "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":\"RETRY1\"}}}"));
  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.submitBlob("120000AABB", sr));
  TEST_ASSERT_TRUE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("RETRY1", sr.txHash);
}

void test_wait_for_outcome_success() {
  // First tx lookup not found / no meta; second returns validated tesSUCCESS.
  xahau::MockXahauTransport mock;
  TEST_ASSERT_TRUE(mock.push("tx", "{\"result\":{\"status\":\"not found\"}}"));
  TEST_ASSERT_TRUE(mock.push(
      "tx",
      "{\"result\":{\"validated\":true,\"meta\":{\"TransactionResult\":\"tesSUCCESS\"}}}"));
  // waitForOutcome may also query ledger_closed for LLS check
  TEST_ASSERT_TRUE(mock.push("ledger_closed", "{\"result\":{\"ledger_index\":100}}"));
  TEST_ASSERT_TRUE(mock.push("ledger_closed", "{\"result\":{\"ledger_index\":100}}"));

  xahau::XahauRpcClient client(mock);
  client.setTimeoutMs(5000);
  xahau::SubmitResult sr;
  // LLS=0 disables ledger expiry; short poll interval for host speed.
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.waitForOutcome("DEADBEEF", 0, 3000, sr, 50));
  TEST_ASSERT_TRUE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("tesSUCCESS", sr.engineResult);
}

void test_wait_for_outcome_tec_final() {
  xahau::MockXahauTransport mock;
  TEST_ASSERT_TRUE(mock.push(
      "tx",
      "{\"result\":{\"validated\":true,\"meta\":{\"TransactionResult\":\"tecUNFUNDED_PAYMENT\"}}}"));
  xahau::XahauRpcClient client(mock);
  xahau::SubmitResult sr;
  // Hash must be valid even-length hex (rpcBuildTx rejects non-hex like "H1").
  static const char* kHash =
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  TEST_ASSERT_EQUAL(xahau::Status::Rejected, client.waitForOutcome(kHash, 0, 2000, sr, 50));
  TEST_ASSERT_FALSE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("tecUNFUNDED_PAYMENT", sr.engineResult);
}

void test_mock_unmatched_fails() {
  xahau::MockXahauTransport mock;
  mock.push("ping", "{\"result\":{\"status\":\"success\"}}");
  char resp[64];
  TEST_ASSERT_EQUAL(xahau::Status::TransportFailed,
                    mock.request("{\"command\":\"fee\"}", resp, sizeof(resp), 1000));
}

void test_end_to_end_prepare_sign_submit_mock() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  // Full offline sign + mock submit path used by product apps
  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));

  xahau::MockXahauTransport mock;
  mock.push("account_info",
            "{\"result\":{\"account_data\":{\"Sequence\":5,\"Balance\":\"50000000\"},"
            "\"ledger_current_index\":1000}}");
  mock.push("submit",
            "{\"result\":{\"engine_result\":\"tesSUCCESS\",\"tx_json\":{\"hash\":\"EEFF\"}}}");

  xahau::XahauRpcClient client(mock);
  uint32_t seq = 0, lls = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.fetchSequenceAndLls(keys.address, seq, lls));

  xahau::PaymentParams pay;
  pay.common.sequence = seq;
  pay.common.lastLedgerSequence = lls;
  pay.common.feeDrops = 12;
  pay.common.flags = 0;
  strncpy(pay.destination, XAHAU_TEST_ADDR_SECP1, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1000;

  char blob[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, blob, sizeof(blob)));

  xahau::SubmitResult sr;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.submitBlob(blob, sr));
  TEST_ASSERT_TRUE(sr.tesSuccess);
  TEST_ASSERT_EQUAL_STRING("EEFF", sr.txHash);
  xahau::secureClear(keys);
}

void test_rpc_build_subscribe_ledger() {
  char buf[128];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildSubscribeLedger(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"command\":\"subscribe\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"streams\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ledger"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildUnsubscribeLedger(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"command\":\"unsubscribe\""));
}

void test_rpc_build_subscribe_streams_allowlist() {
  char buf[256];
  const char* good[] = {"ledger", "transactions"};
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubscribeStreams(good, 2, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "transactions"));

  const char* bad[] = {"ledger\";drop"};
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubscribeStreams(bad, 1, buf, sizeof(buf)));

  const char* unknown[] = {"not_a_stream"};
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubscribeStreams(unknown, 1, buf, sizeof(buf)));
}

void test_rpc_build_subscribe_accounts() {
  char buf[256];
  const char* accts[] = {kValidAcct};
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubscribeAccounts(accts, 1, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"command\":\"subscribe\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"accounts\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, kValidAcct));

  const char* evil[] = {"rEvil\",\"command\":\"admin"};
  TEST_ASSERT_EQUAL(xahau::Status::InvalidAddress,
                    xahau::rpcBuildSubscribeAccounts(evil, 1, buf, sizeof(buf)));
}

void test_rpc_build_subscribe_combined() {
  char buf[320];
  const char* streams[] = {"ledger"};
  const char* accts[] = {kValidAcct};
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubscribe(streams, 1, accts, 1, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "streams"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "accounts"));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubscribe(nullptr, 0, nullptr, 0, buf, sizeof(buf)));
}

void test_rpc_build_subscribe_accounts_proposed() {
  char buf[256];
  const char* accts[] = {kValidAcct};
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildSubscribeAccountsProposed(accts, 1, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"command\":\"subscribe\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"accounts_proposed\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, kValidAcct));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildUnsubscribeAccountsProposed(accts, 1, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"command\":\"unsubscribe\""));
}

void test_rpc_build_subscribe_ex() {
  char buf[400];
  const char* streams[] = {"ledger"};
  const char* accts[] = {kValidAcct};
  const char* prop[] = {kValidAcct};
  xahau::SubscribeParams p;
  memset(&p, 0, sizeof(p));
  p.streams = streams;
  p.streamCount = 1;
  p.accounts = accts;
  p.accountCount = 1;
  p.accountsProposed = prop;
  p.accountsProposedCount = 1;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildSubscribeEx(p, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"streams\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"accounts\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"accounts_proposed\""));
  xahau::SubscribeParams empty;
  memset(&empty, 0, sizeof(empty));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildSubscribeEx(empty, buf, sizeof(buf)));
}

void test_rpc_parse_stream_message_type() {
  xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType(
                        "{\"type\":\"ledgerClosed\",\"ledger_index\":1}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::LedgerClosed), static_cast<int>(t));

  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType(
                        "{\"type\":\"transaction\",\"transaction\":{}}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::Transaction), static_cast<int>(t));

  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType(
                        "{\"id\":1,\"result\":{},\"status\":\"success\"}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::Response), static_cast<int>(t));
}

void test_rpc_parse_ledger_closed_event() {
  const char* json =
      "{\"type\":\"ledgerClosed\",\"ledger_index\":12345,"
      "\"ledger_hash\":\"ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789\","
      "\"txn_count\":7,\"ledger_time\":700000000,\"fee_base\":10,\"reserve_base\":1000000,"
      "\"reserve_inc\":200000,\"network_id\":1,"
      "\"validated_ledgers\":\"32570-12345\"}";
  xahau::LedgerClosedEvent ev;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseLedgerClosedEvent(json, ev));
  TEST_ASSERT_TRUE(ev.valid);
  TEST_ASSERT_EQUAL_UINT32(12345, ev.ledgerIndex);
  TEST_ASSERT_EQUAL_UINT32(7, ev.txnCount);
  TEST_ASSERT_EQUAL_UINT32(700000000, ev.ledgerTime);
  TEST_ASSERT_EQUAL_UINT32(10, ev.feeBase);
  TEST_ASSERT_EQUAL_UINT32(1000000, ev.reserveBase);
  TEST_ASSERT_EQUAL_UINT32(200000, ev.reserveInc);
  TEST_ASSERT_EQUAL_UINT32(1, ev.networkId);
  TEST_ASSERT_EQUAL_STRING(
      "ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789", ev.ledgerHash);
  char range[48];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseLedgerClosedValidatedLedgers(json, range, sizeof(range)));
  TEST_ASSERT_EQUAL_STRING("32570-12345", range);
}

void test_rpc_parse_transaction_stream_event() {
  const char* json =
      "{\"type\":\"transaction\",\"validated\":true,\"ledger_index\":99,"
      "\"engine_result\":\"tesSUCCESS\","
      "\"hash\":\"1111111111111111111111111111111111111111111111111111111111111111\","
      "\"tx_json\":{\"TransactionType\":\"Payment\","
      "\"Account\":\"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh\","
      "\"Destination\":\"rYOUR_DESTINATION_ADDRESS\"}}";
  xahau::TransactionStreamEvent ev;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseTransactionStreamEvent(json, ev));
  TEST_ASSERT_TRUE(ev.valid);
  TEST_ASSERT_TRUE(ev.validated);
  TEST_ASSERT_EQUAL_STRING("Payment", ev.txType);
  TEST_ASSERT_EQUAL_STRING("rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", ev.account);
  TEST_ASSERT_EQUAL_STRING("rYOUR_DESTINATION_ADDRESS", ev.destination);
  TEST_ASSERT_EQUAL_STRING("tesSUCCESS", ev.engineResult);
  TEST_ASSERT_EQUAL_UINT32(99, ev.ledgerIndex);
}

void test_rpc_parse_stream_types_extra() {
  xahau::StreamMessageType t = xahau::StreamMessageType::Unknown;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType("{\"type\":\"bookChanges\"}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::BookChanges), static_cast<int>(t));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType("{\"type\":\"manifestReceived\"}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::ManifestReceived),
                    static_cast<int>(t));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseStreamMessageType("{\"type\":\"peerStatusChange\"}", t));
  TEST_ASSERT_EQUAL(static_cast<int>(xahau::StreamMessageType::PeerStatusChange),
                    static_cast<int>(t));
  TEST_ASSERT_TRUE(xahau::rpcIsReportingUnsupported(
      "{\"error\":\"reportingUnsupported\",\"status\":\"error\"}"));
  TEST_ASSERT_FALSE(xahau::rpcIsReportingUnsupported("{\"status\":\"success\"}"));
}

void test_mock_client_subscribe_ledger() {
  xahau::MockXahauTransport mock;
  TEST_ASSERT_TRUE(mock.push(
      "subscribe", "{\"id\":1,\"result\":{},\"status\":\"success\",\"type\":\"response\"}"));
  TEST_ASSERT_TRUE(mock.push(
      "unsubscribe", "{\"id\":2,\"result\":{},\"status\":\"success\",\"type\":\"response\"}"));
  xahau::XahauRpcClient client(mock);
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.subscribeLedger());
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.unsubscribeLedger());
}

void test_rpc_build_wallet_priority_account() {
  char buf[320];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildAccountChannels(kValidAcct, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_channels"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildAccountCurrencies(kValidAcct, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_currencies"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildAccountNfts(kValidAcct, 20, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_nfts"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildAccountOffers(kValidAcct, 20, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_offers"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildGatewayBalances(kValidAcct, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildNoRippleCheck(kValidAcct, "send", buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "noripple_check"));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildNoRippleCheck(kValidAcct, "evil", buf, sizeof(buf)));
}

void test_rpc_build_wallet_priority_ledger_tx() {
  char buf[320];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildLedgerCurrent(buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ledger_current"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildLedgerData("validated", 20, buf, sizeof(buf)));
  static const char* kIdx =
      "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildLedgerEntryByIndex(kIdx, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "ledger_entry"));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildLedgerEntryAccountRoot(kValidAcct, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildTransactionEntry(kIdx, "validated", buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "transaction_entry"));
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument,
                    xahau::rpcBuildLedgerEntryByIndex("deadbeef", buf, sizeof(buf)));
}

void test_rpc_build_wallet_priority_path_nft_server() {
  char buf[480];
  TEST_ASSERT_EQUAL(
      xahau::Status::Ok,
      xahau::rpcBuildDepositAuthorized(kValidAcct, kValidAcct, buf,
                                      sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "deposit_authorized"));
  TEST_ASSERT_EQUAL(
      xahau::Status::Ok,
      xahau::rpcBuildBookOffers("XRP", nullptr, "USD", kValidAcct, 20, buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "book_offers"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "taker_gets"));
  static const char* kNft =
      "00080000B4F4AFC5FBCBD76873F18006173D213FDC0084B40000099B00000000";
  // pad to 64 if needed - use full 64 hex
  static const char* kNft64 =
      "00080000B4F4AFC5FBCBD76873F18006173D213FDC0084B40000099B00000000";
  // Ensure 64 chars - the above is 64?
  TEST_ASSERT_EQUAL(64, (int)strlen(kNft64));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildNftBuyOffers(kNft64, 10, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildNftSellOffers(kNft64, 10, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildNftInfo(kNft64, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildNftHistory(kNft64, 10, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildNftsByIssuer(kValidAcct, 10, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildFeature(buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildServerDefinitions(buf, sizeof(buf)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcBuildRandom(buf, sizeof(buf)));
  (void)kNft;
}

void test_mock_client_subscribe_ex_and_reporting_unsupported() {
  xahau::MockXahauTransport mock;
  TEST_ASSERT_TRUE(mock.push(
      "subscribe", "{\"id\":1,\"result\":{},\"status\":\"success\",\"type\":\"response\"}"));
  TEST_ASSERT_TRUE(mock.push(
      "subscribe",
      "{\"id\":2,\"error\":\"reportingUnsupported\",\"status\":\"error\",\"type\":\"response\"}"));
  xahau::XahauRpcClient client(mock);
  const char* streams[] = {"ledger"};
  const char* accts[] = {kValidAcct};
  xahau::SubscribeParams p;
  memset(&p, 0, sizeof(p));
  p.streams = streams;
  p.streamCount = 1;
  p.accounts = accts;
  p.accountCount = 1;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, client.subscribeEx(p));
  const char* bad[] = {"server"};
  TEST_ASSERT_EQUAL(xahau::Status::Rejected, client.subscribeStreams(bad, 1));
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_rpc_build_account_info);
  RUN_TEST(test_rpc_build_rejects_injection);
  RUN_TEST(test_rpc_build_submit_fail_hard);
  RUN_TEST(test_rpc_build_fee_ledger_ping);
  RUN_TEST(test_rpc_parse_account_info_snapshot);
  RUN_TEST(test_rpc_parse_act_not_found);
  RUN_TEST(test_rpc_parse_submit);
  RUN_TEST(test_rpc_parse_fee);
  RUN_TEST(test_mock_transport_and_client_fetch_seq);
  RUN_TEST(test_mock_client_submit);
  RUN_TEST(test_mock_submit_tec_rejected);
  RUN_TEST(test_engine_class_and_queued);
  RUN_TEST(test_mock_submit_ter_queued);
  RUN_TEST(test_mock_fetch_tx_result);
  RUN_TEST(test_lls_offset_above_floor);
  RUN_TEST(test_submit_timeout_same_blob_retry);
  RUN_TEST(test_wait_for_outcome_success);
  RUN_TEST(test_wait_for_outcome_tec_final);
  RUN_TEST(test_mock_unmatched_fails);
  RUN_TEST(test_end_to_end_prepare_sign_submit_mock);
  RUN_TEST(test_rpc_build_subscribe_ledger);
  RUN_TEST(test_rpc_build_subscribe_streams_allowlist);
  RUN_TEST(test_rpc_build_subscribe_accounts);
  RUN_TEST(test_rpc_build_subscribe_combined);
  RUN_TEST(test_rpc_build_subscribe_accounts_proposed);
  RUN_TEST(test_rpc_build_subscribe_ex);
  RUN_TEST(test_rpc_parse_stream_message_type);
  RUN_TEST(test_rpc_parse_ledger_closed_event);
  RUN_TEST(test_rpc_parse_transaction_stream_event);
  RUN_TEST(test_rpc_parse_stream_types_extra);
  RUN_TEST(test_mock_client_subscribe_ledger);
  RUN_TEST(test_mock_client_subscribe_ex_and_reporting_unsupported);
  RUN_TEST(test_rpc_build_wallet_priority_account);
  RUN_TEST(test_rpc_build_wallet_priority_ledger_tx);
  RUN_TEST(test_rpc_build_wallet_priority_path_nft_server);
  return UNITY_END();
}
