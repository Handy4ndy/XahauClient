// Host goldens: Invoke + SetHook (tiny) + ClaimReward encode shapes
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
static const char* GENESIS = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

static const char* kInvokeSelf =
    "12006322800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";
static const char* kInvokeDest =
    "12006322800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839EC"
    "DB7361A151F916265838A95D7537CB4637";
static const char* kSetHookTiny =
    "12001622800000002400000001201B000000646840000000000003E87321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7CFBEE101400002200000001501400000000000000000000000000000000000000000000"
    "00000000000000000000502011111111111111111111111111111111111111111111111111111111"
    "111111117B040061736DE1F1";
static const char* kClaimGenesis =
    "12006222800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C8414B5F762"
    "798A53D543A014CAF8B297CFF8F2F937E8";
static const char* kClaimOptOut =
    "12006222800000012400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static void common(xahau::TxCommon& c) {
  c.flags = 0x80000000u;
  c.sequence = 1;
  c.lastLedgerSequence = 100;
  c.feeDrops = 12;
}

static void assertU(const xahau::TxFieldBag& bag, const char* exp) {
  uint8_t bin[2048];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[4200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(exp, hex);
}

void test_maturity_hooks_claim() {
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::Invoke)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::SetHook)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::ClaimReward)->maturity);
}

void test_invoke_self() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::InvokeParams p;
  common(p.common);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillInvoke(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kInvokeSelf);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "120063"));
  xahau::secureClear(keys);
}

void test_invoke_dest() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::InvokeParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillInvoke(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kInvokeDest);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  xahau::secureClear(keys);
}

void test_sethook_tiny() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  static const uint8_t code[] = {0x00, 0x61, 0x73, 0x6d};  // wasm magic
  xahau::SetHookParams p;
  common(p.common);
  p.createCode = code;
  p.createCodeLen = sizeof(code);
  memset(p.hookNamespace, 0x11, 32);
  memset(p.hookOn, 0x00, 32);
  p.hookApiVersion = 0;
  p.hookFlags = 1;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillSetHook(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kSetHookTiny);
  // No buildAndSignSetHook (fill-only Tier A encode path).
  xahau::secureClear(keys);
}

void test_claimreward_genesis() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::ClaimRewardParams p;
  common(p.common);
  strncpy(p.issuer, GENESIS, sizeof(p.issuer) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillClaimReward(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kClaimGenesis);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignClaimReward(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignClaimReward(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "120062"));
  xahau::secureClear(keys);
}

void test_claimreward_optout() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::ClaimRewardParams p;
  common(p.common);
  p.optOut = true;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillClaimReward(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kClaimOptOut);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignClaimReward(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignClaimReward(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "2280000001"));
  xahau::secureClear(keys);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_maturity_hooks_claim);
  RUN_TEST(test_invoke_self);
  RUN_TEST(test_invoke_dest);
  RUN_TEST(test_sethook_tiny);
  RUN_TEST(test_claimreward_genesis);
  RUN_TEST(test_claimreward_optout);
  return UNITY_END();
}
