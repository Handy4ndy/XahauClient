/**
 * XahauClient Example — BuildCatalog
 *
 * RPC request-builder catalog
 *
 * Demonstrates: Exercises public rpcBuild* helpers offline (no Wi-Fi).
 * Hardware: ESP32 / ESP32-S3 (offline mock/builders; no Wi-Fi required)
 * Network: N/A (offline). Live I/O lives under examples/wss/.
 * Config: Set XAHAU_SEED / XAHAU_SEED_B placeholders (sEdPLACEHOLDER_REPLACE_ME). Never commit real seeds.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>
#include <string.h>

const char* XAHAU_SEED   = "sEdPLACEHOLDER_REPLACE_ME";
const char* XAHAU_SEED_B = "sEdPLACEHOLDER_B_REPLACE_ME";

static char s_req[768];
static char s_addrA[48];
static char s_addrB[48];
static int s_ok = 0;
static int s_fail = 0;

static void line(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
  if (st == xahau::Status::Ok) s_ok++;
  else s_fail++;
}

static void expect(const char* label, xahau::Status st, xahau::Status want) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
  if (st == want) s_ok++;
  else s_fail++;
}

static bool resolveKeys(const char* seed, char* addrOut, size_t addrCap) {
  xahau::KeyPair kp;
  if (xahau::deriveKeysFromSeed(seed, kp) != xahau::Status::Ok) return false;
  strncpy(addrOut, kp.address, addrCap - 1);
  addrOut[addrCap - 1] = '\0';
  xahau::secureClear(kp);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient RPC offline · BuildCatalog ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  if (!resolveKeys(XAHAU_SEED, s_addrA, sizeof(s_addrA)) ||
      !resolveKeys(XAHAU_SEED_B, s_addrB, sizeof(s_addrB))) {
    Serial.println(F("deriveKeysFromSeed failed"));
    Serial.println(F("Done."));
    return;
  }
  Serial.print(F("accountA: "));
  Serial.println(s_addrA);
  Serial.print(F("accountB: "));
  Serial.println(s_addrB);

  // Account
  line("rpcBuildAccountInfo", xahau::rpcBuildAccountInfo(s_addrA, s_req, sizeof(s_req)));
  line("rpcBuildAccountInfoEx",
       xahau::rpcBuildAccountInfoEx(s_addrA, "validated", s_req, sizeof(s_req)));
  line("rpcBuildAccountLines", xahau::rpcBuildAccountLines(s_addrA, s_req, sizeof(s_req)));
  line("rpcBuildAccountTx", xahau::rpcBuildAccountTx(s_addrA, 10, s_req, sizeof(s_req)));
  line("rpcBuildAccountObjects",
       xahau::rpcBuildAccountObjects(s_addrA, nullptr, s_req, sizeof(s_req)));
  line("rpcBuildAccountObjects vault",
       xahau::rpcBuildAccountObjects(s_addrA, "vault", s_req, sizeof(s_req)));
  // Wallet-priority account inventory
  line("rpcBuildAccountChannels", xahau::rpcBuildAccountChannels(s_addrA, s_req, sizeof(s_req)));
  line("rpcBuildAccountCurrencies",
       xahau::rpcBuildAccountCurrencies(s_addrA, s_req, sizeof(s_req)));
  line("rpcBuildAccountNfts", xahau::rpcBuildAccountNfts(s_addrA, 20, s_req, sizeof(s_req)));
  line("rpcBuildAccountOffers", xahau::rpcBuildAccountOffers(s_addrA, 20, s_req, sizeof(s_req)));
  line("rpcBuildGatewayBalances", xahau::rpcBuildGatewayBalances(s_addrA, s_req, sizeof(s_req)));
  line("rpcBuildNoRippleCheck",
       xahau::rpcBuildNoRippleCheck(s_addrA, "send", s_req, sizeof(s_req)));

  // Server / ledger
  line("rpcBuildPing", xahau::rpcBuildPing(s_req, sizeof(s_req)));
  line("rpcBuildFee", xahau::rpcBuildFee(s_req, sizeof(s_req)));
  line("rpcBuildFeature", xahau::rpcBuildFeature(s_req, sizeof(s_req)));
  line("rpcBuildRandom", xahau::rpcBuildRandom(s_req, sizeof(s_req)));
  line("rpcBuildServerInfo", xahau::rpcBuildServerInfo(s_req, sizeof(s_req)));
  line("rpcBuildServerState", xahau::rpcBuildServerState(s_req, sizeof(s_req)));
  line("rpcBuildServerDefinitions", xahau::rpcBuildServerDefinitions(s_req, sizeof(s_req)));
  line("rpcBuildLedgerClosed", xahau::rpcBuildLedgerClosed(s_req, sizeof(s_req)));
  line("rpcBuildLedgerCurrent", xahau::rpcBuildLedgerCurrent(s_req, sizeof(s_req)));
  line("rpcBuildLedger", xahau::rpcBuildLedger("validated", false, s_req, sizeof(s_req)));
  line("rpcBuildLedgerData", xahau::rpcBuildLedgerData("validated", 20, s_req, sizeof(s_req)));
  line("rpcBuildLedgerEntryAccountRoot",
       xahau::rpcBuildLedgerEntryAccountRoot(s_addrA, s_req, sizeof(s_req)));

  // Submit / tx / path
  static const char* kBlob = "1200002280000000240000000161400000000000000168400000000000000C";
  line("rpcBuildSubmit", xahau::rpcBuildSubmit(kBlob, s_req, sizeof(s_req)));
  line("rpcBuildSubmitEx", xahau::rpcBuildSubmitEx(kBlob, true, s_req, sizeof(s_req)));
  static const char* kHash =
      "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
  line("rpcBuildTx", xahau::rpcBuildTx(kHash, false, s_req, sizeof(s_req)));
  line("rpcBuildTransactionEntry",
       xahau::rpcBuildTransactionEntry(kHash, "validated", s_req, sizeof(s_req)));
  line("rpcBuildSubmitMultisigned",
       xahau::rpcBuildSubmitMultisigned("{\"TransactionType\":\"Payment\"}", s_req, sizeof(s_req)));
  line("rpcBuildDepositAuthorized",
       xahau::rpcBuildDepositAuthorized(s_addrA, s_addrB, s_req, sizeof(s_req)));
  line("rpcBuildBookOffers XAH/USD",
       xahau::rpcBuildBookOffers("XAH", nullptr, "USD", s_addrB, 10, s_req, sizeof(s_req)));
  // AMM info builder removed 2026-09-28 (EncodeOnly cut).
  line("rpcBuildLedgerEntryByIndex",
       xahau::rpcBuildLedgerEntryByIndex(kHash, s_req, sizeof(s_req)));
  // vault_info builder removed 2026-09-28 (EncodeOnly cut).
  line("rpcBuildNftInfo", xahau::rpcBuildNftInfo(kHash, s_req, sizeof(s_req)));
  line("rpcBuildNftBuyOffers", xahau::rpcBuildNftBuyOffers(kHash, 10, s_req, sizeof(s_req)));
  line("rpcBuildNftSellOffers", xahau::rpcBuildNftSellOffers(kHash, 10, s_req, sizeof(s_req)));
  line("rpcBuildNftsByIssuer",
       xahau::rpcBuildNftsByIssuer(s_addrA, 10, s_req, sizeof(s_req)));

  // Subscribe
  const char* streamsLed[] = {"ledger"};
  const char* acctsA[] = {s_addrA};
  const char* acctsAB[] = {s_addrA, s_addrB};
  line("rpcBuildSubscribeLedger", xahau::rpcBuildSubscribeLedger(s_req, sizeof(s_req)));
  line("rpcBuildUnsubscribeLedger", xahau::rpcBuildUnsubscribeLedger(s_req, sizeof(s_req)));
  line("rpcBuildSubscribeStreams",
       xahau::rpcBuildSubscribeStreams(streamsLed, 1, s_req, sizeof(s_req)));
  line("rpcBuildSubscribeAccounts A",
       xahau::rpcBuildSubscribeAccounts(acctsA, 1, s_req, sizeof(s_req)));
  line("rpcBuildSubscribeAccounts A+B",
       xahau::rpcBuildSubscribeAccounts(acctsAB, 2, s_req, sizeof(s_req)));
  line("rpcBuildSubscribeAccountsProposed",
       xahau::rpcBuildSubscribeAccountsProposed(acctsA, 1, s_req, sizeof(s_req)));
  line("rpcBuildSubscribe combined",
       xahau::rpcBuildSubscribe(streamsLed, 1, acctsA, 1, s_req, sizeof(s_req)));

  xahau::SubscribeParams p;
  memset(&p, 0, sizeof(p));
  p.streams = streamsLed;
  p.streamCount = 1;
  p.accounts = acctsA;
  p.accountCount = 1;
  p.accountsProposed = acctsAB;
  p.accountsProposedCount = 2;
  line("rpcBuildSubscribeEx", xahau::rpcBuildSubscribeEx(p, s_req, sizeof(s_req)));
  line("rpcBuildUnsubscribeEx", xahau::rpcBuildUnsubscribeEx(p, s_req, sizeof(s_req)));
  line("rpcBuildUnsubscribeAccountsProposed",
       xahau::rpcBuildUnsubscribeAccountsProposed(acctsA, 1, s_req, sizeof(s_req)));

  // Rejections (security)
  expect("reject injection account",
         xahau::rpcBuildAccountInfo("rEvil\",\"cmd\":\"x", s_req, sizeof(s_req)),
         xahau::Status::InvalidAddress);
  expect("reject odd blob", xahau::rpcBuildSubmit("ODD", s_req, sizeof(s_req)),
         xahau::Status::InvalidArgument);
  const char* badStream[] = {"not_a_stream"};
  expect("reject unknown stream",
         xahau::rpcBuildSubscribeStreams(badStream, 1, s_req, sizeof(s_req)),
         xahau::Status::InvalidArgument);
  expect("reject bad type filter",
         xahau::rpcBuildAccountObjects(s_addrA, "vault\";drop", s_req, sizeof(s_req)),
         xahau::Status::InvalidArgument);

  Serial.print(F("build_ok="));
  Serial.print(s_ok);
  Serial.print(F(" build_fail="));
  Serial.println(s_fail);
  if (s_fail == 0) Serial.println(F("catalog: Ok"));
  else Serial.println(F("catalog: FAIL"));
  Serial.println(F("Done."));
}

void loop() {}
