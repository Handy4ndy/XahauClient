// Host tests: AccountSet + TrustSet golden unsigned (+ Ed25519 signed AccountSet)
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


static const char* kAsSecpUnsigned =
    "12000322800000002400000001201B0000006420210000000868400000000000000C7321031482CD1D0C8F93A4EFBC"
    "379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static const char* kAsEdUnsigned =
    "12000322000000002400000001201B0000006420210000000868400000000000000C7321ED01FA53FA5A7E77798F88"
    "2ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A638114D28B177E48D9A8D057E70F7E464B498367281B98";

static const char* kAsEdSigned =
    "12000322000000002400000001201B0000006420210000000868400000000000000C7321ED01FA53FA5A7E77798F88"
    "2ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A63744076B858925BF15ADF1B09C0DAC5B43A25B303488DA912A8"
    "B382355E213EF41902F30A23D1A40894C82C8FA80AAFE66FD373B9D71AACBD22AFD1E52D39E639650C8114D28B177E"
    "48D9A8D057E70F7E464B498367281B98";

static const char* kTrustSecpUnsigned =
    "12001422800000002400000001201B0000006463D5438D7EA4C6800000000000000000000000000055534400000000"
    "003839ECDB7361A151F916265838A95D7537CB463768400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

void test_account_set_unsigned_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_SECP1, keys));

  xahau::AccountSetParams p;
  p.common.flags = 0x80000000u;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  p.common.feeDrops = 12;
  p.hasSetFlag = true;
  p.setFlag = 8;

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillAccountSet(p, keys.accountId, keys.publicKey, bag));

  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[512];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kAsSecpUnsigned, hex);

  char signedHex[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, signedHex, sizeof(signedHex)));
  TEST_ASSERT_TRUE(strlen(signedHex) > strlen(kAsSecpUnsigned));

  xahau::secureClear(keys);
}

void test_account_set_signed_ed25519() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));

  xahau::AccountSetParams p;
  p.common.flags = 0;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  p.common.feeDrops = 12;
  p.hasSetFlag = true;
  p.setFlag = 8;

  char blob[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignAccountSet(keys, p, blob, sizeof(blob)));
  TEST_ASSERT_EQUAL_STRING(kAsEdSigned, blob);

  // Also check unsigned path
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillAccountSet(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[512];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kAsEdUnsigned, hex);

  xahau::secureClear(keys);
}

void test_trust_set_unsigned_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_SECP1, keys));

  xahau::TrustSetParams p;
  p.common.flags = 0x80000000u;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  p.common.feeDrops = 12;
  strncpy(p.limit.value, "1000", sizeof(p.limit.value) - 1);
  strncpy(p.limit.currency, "USD", sizeof(p.limit.currency) - 1);
  strncpy(p.limit.issuer, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(p.limit.issuer) - 1);

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillTrustSet(p, keys.accountId, keys.publicKey, bag));

  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[512];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kTrustSecpUnsigned, hex);

  char signedHex[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, signedHex, sizeof(signedHex)));
  TEST_ASSERT_TRUE(strlen(signedHex) > 100);

  xahau::secureClear(keys);
}

void test_maturity_flags() {
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::Payment)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::AccountSet)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::TrustSet)->maturity);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_account_set_unsigned_secp);
  RUN_TEST(test_account_set_signed_ed25519);
  RUN_TEST(test_trust_set_unsigned_secp);
  RUN_TEST(test_maturity_flags);
  return UNITY_END();
}
