// Host golden vectors: CronSet / SetRemarks / Import / Payment(HookName+FLS)
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

static const char* kCronSetCreate =
    "12005D22800000002400000001201B00000064205D00000000205E00000003205F0000003C68400000"
    "000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681"
    "146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static const char* kCronSetUnset =
    "12005D22800000012400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC"
    "379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5"
    "E10C7C";

static const char* kSetRemarks =
    "12005E22800000002400000001201B000000645E979797979797979797979797979797979797979797"
    "979797979797979797979768400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD"
    "827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7CF061E06170"
    "620568656C6C6F7063046E6F7465E1F1";

static const char* kImportFake =
    "12006122800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC"
    "379C68F970A0FBAD827646E41F8346FC515D64CFFEE6701A08DEADBEEF0102030481146DE8B4EADE2E"
    "2306E80543259BD5DE11A5E10C7C";

static const char* kPaymentHookFls =
    "12000022800000002400000001201A00000032201B0000006461400000000000000168400000000000"
    "000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE670610342"
    "495081146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D"
    "7537CB4637";

static void common(xahau::TxCommon& c) {
  c.flags = 0x80000000u;
  c.sequence = 1;
  c.lastLedgerSequence = 100;
  c.feeDrops = 12;
}

static void assertU(const xahau::TxFieldBag& bag, const char* exp) {
  uint8_t bin[768];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[1600];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(exp, hex);
}

void test_maturity_p0_encode() {
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::CronSet)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::SetRemarks)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Encode, xahau::txTypeLookup(TxType::Import)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::Payment)->maturity);
}

void test_cronset_create() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::CronSetParams p;
  common(p.common);
  p.unset = false;
  p.startTime = 0;
  p.repeatCount = 3;
  p.delaySeconds = 60;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillCronSet(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kCronSetCreate);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignCronSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignCronSet(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "12005D"));
  xahau::secureClear(keys);
}

void test_cronset_unset() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::CronSetParams p;
  common(p.common);
  p.unset = true;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillCronSet(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kCronSetUnset);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignCronSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignCronSet(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "2280000001"));
  xahau::secureClear(keys);
}

void test_setremarks() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SetRemarksParams p;
  common(p.common);
  memset(p.objectId, 0x97, 32);
  strncpy(p.remarkName, "note", sizeof(p.remarkName) - 1);
  strncpy(p.remarkValue, "hello", sizeof(p.remarkValue) - 1);
  p.immutable = false;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSetRemarks(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kSetRemarks);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRemarks(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRemarks(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "12005E"));
  xahau::secureClear(keys);
}

void test_import_fake_blob() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  static const uint8_t fake[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
  xahau::ImportParams p;
  common(p.common);
  p.blob = fake;
  p.blobLen = sizeof(fake);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillImport(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kImportFake);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignImport(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignImport(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "120061"));
  xahau::secureClear(keys);
}

void test_payment_hookname_fls() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::PaymentParams pay;
  common(pay.common);
  pay.common.firstLedgerSequence = 50;
  strncpy(pay.destination, DEST, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;
  strncpy(pay.hookName, "BIP", sizeof(pay.hookName) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));
  assertU(bag, kPaymentHookFls);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "201A00000032"));
  TEST_ASSERT_NOT_NULL(strstr(a, "706103424950"));
  xahau::secureClear(keys);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_maturity_p0_encode);
  RUN_TEST(test_cronset_create);
  RUN_TEST(test_cronset_unset);
  RUN_TEST(test_setremarks);
  RUN_TEST(test_import_fake_blob);
  RUN_TEST(test_payment_hookname_fls);
  return UNITY_END();
}
