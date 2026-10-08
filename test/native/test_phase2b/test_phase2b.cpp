// Phase 2 completion: Escrow, Check, PayChan, NFT, misc -- unsigned goldens + det. sign
#include <unity.h>
#include <string.h>
#include "XahauClient.h"
#include "xahau/core/hex.h"
#include "xahau/codec/serialize.h"

#if defined(__has_include)
#  if __has_include("test_seeds.local.h")
#    include "test_seeds.local.h"
#  else
#    include "test_seeds.example.h"
#  endif
#else
#  include "test_seeds.example.h"
#endif


static const char* SECP = XAHAU_TEST_SEED_SECP1;
static const char* DEST = "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN";

static void common(xahau::TxCommon& c) {
  c.flags = 0x80000000u;
  c.sequence = 1;
  c.lastLedgerSequence = 100;
  c.feeDrops = 12;
}

static void fillHash(uint8_t out[32], uint8_t b) { memset(out, b, 32); }

static void assertUnsigned(const xahau::TxFieldBag& bag, const char* expected) {
  uint8_t bin[512];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(expected, hex);
}

static void assertDetSign(const xahau::KeyPair& keys, const xahau::TxFieldBag& bag) {
  char a[1400], b[1400];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_TRUE(strlen(a) > 100);
}

void test_escrow_family() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));

  {
    xahau::EscrowCreateParams p;
    common(p.common);
    strncpy(p.destination, DEST, sizeof(p.destination) - 1);
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 1000000;
    p.finishAfter = 700000000;
    p.cancelAfter = 800000000;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillEscrowCreate(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12000122800000002400000001201B0000006420242FAF0800202529B927006140000000000F42406840000"
                   "0000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146D"
                   "E8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::EscrowFinishParams p;
    common(p.common);
    strncpy(p.owner, DEST, sizeof(p.owner) - 1);
    p.offerSequence = 5;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillEscrowFinish(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12000222800000002400000001201900000005201B0000006468400000000000000C7321031482CD1D0C8F93A"
                   "4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E"
                   "10C7C82143839ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::EscrowCancelParams p;
    common(p.common);
    strncpy(p.owner, DEST, sizeof(p.owner) - 1);
    p.offerSequence = 5;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillEscrowCancel(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12000422800000002400000001201900000005201B0000006468400000000000000C7321031482CD1D0C8F93A"
                   "4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E"
                   "10C7C82143839ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  xahau::secureClear(keys);
}

void test_check_family() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  {
    xahau::CheckCreateParams p;
    common(p.common);
    p.common.destinationTag = 42;
    strncpy(p.destination, DEST, sizeof(p.destination) - 1);
    p.sendMax.kind = xahau::AmountKind::XrpDrops;
    p.sendMax.drops = 500000;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillCheckCreate(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "120010228000000024000000012E0000002A201B0000006468400000000000000C69400000000007A1207321"
                   "031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E8"
                   "0543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::CheckCashParams p;
    common(p.common);
    fillHash(p.checkId, 0xAA);
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 100000;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillCheckCash(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(
        bag,
        "12001122800000002400000001201B000000645018AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAA6140000000000186A068400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0"
        "FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C");
    assertDetSign(keys, bag);
  }
  {
    xahau::CheckCancelParams p;
    common(p.common);
    fillHash(p.checkId, 0xBB);
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillCheckCancel(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12001222800000002400000001201B000000645018BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
                   "BBBBBBBBBBBBBBBBBB68400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F"
                   "8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C");
    assertDetSign(keys, bag);
  }
  xahau::secureClear(keys);
}

void test_paychan_family() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  {
    xahau::PaymentChannelCreateParams p;
    common(p.common);
    strncpy(p.destination, DEST, sizeof(p.destination) - 1);
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 1000000;
    p.settleDelay = 86400;
    memcpy(p.publicKey, keys.publicKey, 33);
    p.publicKeyLen = 33;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillPaymentChannelCreate(p, keys.accountId,
                                                                       keys.publicKey, bag));
    assertUnsigned(bag,
                   "12000D22800000002400000001201B000000642027000151806140000000000F424068400000000000000C71"
                   "21031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE67321031482CD1D0C8F93"
                   "A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5"
                   "E10C7C83143839ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::PaymentChannelFundParams p;
    common(p.common);
    fillHash(p.channel, 0xCC);
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 500000;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillPaymentChannelFund(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12000E22800000002400000001201B000000645016CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
                   "CCCCCCCCCCCCCCCCCC61400000000007A12068400000000000000C7321031482CD1D0C8F93A4EFBC379C68F"
                   "970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C");
    assertDetSign(keys, bag);
  }
  {
    xahau::PaymentChannelClaimParams p;
    common(p.common);
    fillHash(p.channel, 0xDD);
    p.amount.kind = xahau::AmountKind::XrpDrops;
    p.amount.drops = 200000;
    p.balance.kind = xahau::AmountKind::XrpDrops;
    p.balance.drops = 100000;
    memcpy(p.publicKey, keys.publicKey, 33);
    p.publicKeyLen = 33;
    // Signature optional for unsigned golden without claim sig
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillPaymentChannelClaim(p, keys.accountId, keys.publicKey, bag));
    // Without Signature field matches stripped claim shape
    uint8_t bin[400];
    size_t len = 0;
    TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
    TEST_ASSERT_TRUE(len > 80);
    assertDetSign(keys, bag);
  }
  xahau::secureClear(keys);
}


void test_misc_family() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  {
    xahau::DepositPreauthParams p;
    common(p.common);
    strncpy(p.authorize, DEST, sizeof(p.authorize) - 1);
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillDepositPreauth(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12001322800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F"
                   "970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C85143839"
                   "ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::AccountDeleteParams p;
    common(p.common);
    strncpy(p.destination, DEST, sizeof(p.destination) - 1);
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillAccountDelete(p, keys.accountId, keys.publicKey, bag));
    // Fee floor for AccountDelete = 200000 drops (0x30D40).
    assertUnsigned(bag,
                   "12001522800000002400000001201B00000064684000000000030D407321031482CD1D0C8F93A4EFBC379C68F"
                   "970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839"
                   "ECDB7361A151F916265838A95D7537CB4637");
    assertDetSign(keys, bag);
  }
  {
    xahau::ClawbackParams p;
    common(p.common);
    p.amount.kind = xahau::AmountKind::Issued;
    strncpy(p.amount.issued.value, "1", sizeof(p.amount.issued.value) - 1);
    strncpy(p.amount.issued.currency, "USD", sizeof(p.amount.issued.currency) - 1);
    strncpy(p.amount.issued.issuer, DEST, sizeof(p.amount.issued.issuer) - 1);
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillClawback(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12001E22800000002400000001201B0000006461D4838D7EA4C68000000000000000000000000000555344000"
                   "00000003839ECDB7361A151F916265838A95D7537CB463768400000000000000C7321031482CD1D0C8F93A4E"
                   "FBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10"
                   "C7C");
    assertDetSign(keys, bag);
  }
  {
    xahau::OracleDeleteParams p;
    common(p.common);
    p.oracleDocumentID = 1;
    xahau::TxFieldBag bag;
    TEST_ASSERT_EQUAL(xahau::Status::Ok,
                      xahau::fillOracleDelete(p, keys.accountId, keys.publicKey, bag));
    assertUnsigned(bag,
                   "12003D22800000002400000001201B0000006420330000000168400000000000000C7321031482CD1D0C8F93A"
                   "4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E"
                   "10C7C");
    assertDetSign(keys, bag);
  }
  xahau::secureClear(keys);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_escrow_family);
  RUN_TEST(test_check_family);
  RUN_TEST(test_paychan_family);
  RUN_TEST(test_misc_family);
  return UNITY_END();
}
