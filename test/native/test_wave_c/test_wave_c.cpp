// Wave C unit tests: multi-sign helpers, issued arena, escrow condition
#include <string.h>
#include <unity.h>

#include <XahauClient.h>

#if defined(__has_include)
#  if __has_include("test_seeds.local.h")
#    include "test_seeds.local.h"
#  else
#    include "test_seeds.example.h"
#  endif
#else
#  include "test_seeds.example.h"
#endif


static const char* SECP = XAHAU_TEST_SEED_SECP_GENESIS;  // fixture seed from test_seeds.local.h (never fund)
static const char* DEST = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

void setUp() {}
void tearDown() {}

void test_issued_amount_arena_owned() {
  xahau::TxFieldBag bag;
  uint8_t iss[20];
  memset(iss, 1, 20);
  {
    char value[16];
    char cur[8];
    strcpy(value, "12.5");
    strcpy(cur, "USD");
    TEST_ASSERT_EQUAL(xahau::Status::Ok, bag.setAmountIssued(xahau::fc::Amount, value, cur, iss));
    // Destroy stack temps -- bag must still serialize.
  }
  uint8_t out[128];
  size_t outLen = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, out, sizeof(out), outLen));
  TEST_ASSERT_TRUE(outLen > 20);
}


void test_escrow_finish_condition_fields() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::EscrowFinishParams p;
  p.common.flags = 0x80000000u;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  p.common.feeDrops = 12;
  strncpy(p.owner, DEST, sizeof(p.owner) - 1);
  p.offerSequence = 5;
  // Dummy 32-byte condition + fulfillment (not valid crypto-condition, encode only)
  p.conditionLen = 32;
  p.fulfillmentLen = 32;
  memset(p.condition, 0xAB, 32);
  memset(p.fulfillment, 0xCD, 32);

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillEscrowFinish(p, keys.accountId, keys.publicKey, bag));
  TEST_ASSERT_TRUE(bag.count() >= 5);
}


void test_multi_sign_build_and_sign() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));

  xahau::PaymentParams pay;
  pay.common.flags = 0x80000000u;
  pay.common.sequence = 1;
  pay.common.lastLedgerSequence = 100;
  pay.common.feeDrops = 12;
  strncpy(pay.destination, DEST, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;

  xahau::TxFieldBag bag;
  // Multi-sign envelope: empty SigningPubKey
  TEST_ASSERT_EQUAL(
      xahau::Status::Ok,
      xahau::bagSetCommon(bag, xahau::TxType::Payment, pay.common, keys.accountId, nullptr, nullptr,
                         0, false));
  // Minimal payment fields via fill would reset bag -- set destination/amount manually path:
  bag.clear();
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bagPrepareMultiSign(bag));

  xahau::MultiSigner ms;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::multiSignAs(bag, keys, ms));
  TEST_ASSERT_TRUE(ms.signatureLen > 0);

  char hex[4096];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::multiSignFinalize(bag, &ms, 1, hex, sizeof(hex)));
  TEST_ASSERT_TRUE(strlen(hex) > 40);
}


int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_issued_amount_arena_owned);
  RUN_TEST(test_escrow_finish_condition_fields);
  RUN_TEST(test_multi_sign_build_and_sign);
  return UNITY_END();
}
