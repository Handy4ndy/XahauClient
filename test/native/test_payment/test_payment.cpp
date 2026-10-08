// Host tests: Payment serialize + sign golden vectors
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


static const char* kSecpUnsigned =
    "12000022800000002400000001201B0000006461400000000000000168400000000000000C7321031482CD1D0C8F"
    "93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C"
    "7C83143839ECDB7361A151F916265838A95D7537CB4637";

static const char* kEdUnsigned =
    "12000022000000002400000001201B0000006461400000000000000168400000000000000C7321ED01FA53FA5A7E77"
    "798F882ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A638114D28B177E48D9A8D057E70F7E464B498367281B98"
    "83146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static const char* kEdSigned =
    "12000022000000002400000001201B0000006461400000000000000168400000000000000C7321ED01FA53FA5A7E77"
    "798F882ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A6374409608AEC5C4C0D753935094635C368E31982B78FC"
    "701246567F6FA306A551130FAA7DD564EF8146E9A85687F14EE14BE050CE65F041F4D770C7ECA2D37E386A028114D2"
    "8B177E48D9A8D057E70F7E464B498367281B9883146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static void fillXrpPayment(xahau::PaymentParams& pay, const char* dest, uint64_t drops,
                           uint32_t flags, uint32_t seq, uint32_t lls, uint64_t fee) {
  pay = xahau::PaymentParams();
  strncpy(pay.destination, dest, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = drops;
  pay.common.flags = flags;
  pay.common.sequence = seq;
  pay.common.lastLedgerSequence = lls;
  pay.common.feeDrops = fee;
}

void test_payment_unsigned_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_SECP1, keys));

  xahau::PaymentParams pay;
  fillXrpPayment(pay, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", 1, 0x80000000u, 1, 100, 12);

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));

  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));

  char hex[512];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kSecpUnsigned, hex);

  // RFC6979 deterministic secp (uECC + SHA-256); stable library golden
  static const char* kSecpSigned =
      "12000022800000002400000001201B0000006461400000000000000168400000000000000C7321031482CD1D0C8F"
      "93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE6744730450221009F3B1788560593987B35F32EBFFF"
      "78B5A4B55CB5E8B7D4A5F913B48D6878266D02203412BE667B54C1DD57792316925A76BE095AD47F54154B038D0D34"
      "04C1850BE281146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB"
      "4637";
  char signedHex[1024];
  char signedHex2[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, signedHex, sizeof(signedHex)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, signedHex2, sizeof(signedHex2)));
  TEST_ASSERT_EQUAL_STRING(signedHex, signedHex2);
  TEST_ASSERT_EQUAL_STRING(kSecpSigned, signedHex);

  xahau::secureClear(keys);
}

void test_payment_signed_ed25519() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));

  xahau::PaymentParams pay;
  fillXrpPayment(pay, XAHAU_TEST_ADDR_SECP1, 1, 0, 1, 100, 12);

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));

  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[512];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kEdUnsigned, hex);

  char signedHex[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, signedHex, sizeof(signedHex)));
  TEST_ASSERT_EQUAL_STRING(kEdSigned, signedHex);

  xahau::secureClear(keys);
}

void test_build_and_sign_payment_helper() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));
  xahau::PaymentParams pay;
  fillXrpPayment(pay, XAHAU_TEST_ADDR_SECP1, 1, 0, 1, 100, 12);
  char blob[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::buildAndSignPayment(keys, pay, blob, sizeof(blob)));
  TEST_ASSERT_EQUAL_STRING(kEdSigned, blob);
  xahau::secureClear(keys);
}

/** Stack-local signTx: two sequential signs must match (no shared static corruption). */
void test_sign_tx_reentrant_stable() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));
  xahau::PaymentParams pay;
  fillXrpPayment(pay, XAHAU_TEST_ADDR_SECP1, 1, 0, 1, 100, 12);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));
  char a[1024], b[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::signTx(bag, keys, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_EQUAL_STRING(kEdSigned, a);
  xahau::secureClear(keys);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_payment_unsigned_secp);
  RUN_TEST(test_payment_signed_ed25519);
  RUN_TEST(test_build_and_sign_payment_helper);
  RUN_TEST(test_sign_tx_reentrant_stable);
  return UNITY_END();
}
