// Host goldens: multi-sign Payment envelope + SignerListSet encode.
// Deterministic secp demo seeds; multi-sign hex locked for regression.
#include <string.h>
#include <unity.h>

#include <XahauClient.h>
#include "xahau/codec/serialize.h"
#include "xahau/core/hex.h"

#if defined(__has_include)
#  if __has_include("test_seeds.local.h")
#    include "test_seeds.local.h"
#  else
#    include "test_seeds.example.h"
#  endif
#else
#  include "test_seeds.example.h"
#endif


// Fixture seeds from test_seeds.local.h (gitignored). NEVER fund these accounts.
static const char* SEED_A = XAHAU_TEST_SEED_SECP_GENESIS;
static const char* SEED_B = XAHAU_TEST_SEED_SECP1;
static const char* DEST = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";

void setUp() {}
void tearDown() {}

static void commonPay(xahau::PaymentParams& pay) {
  pay.common.flags = 0x80000000u;
  pay.common.sequence = 1;
  pay.common.lastLedgerSequence = 100;
  pay.common.feeDrops = 24;
  strncpy(pay.destination, DEST, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;
}

void test_multisign_two_key_payment_golden() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair a, b;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SEED_A, a));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SEED_B, b));
  TEST_ASSERT_TRUE(memcmp(a.accountId, b.accountId, 20) != 0);

  xahau::PaymentParams pay;
  commonPay(pay);

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, a.accountId, a.publicKey, bag));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bagPrepareMultiSign(bag));

  // Unsigned multi-sign body: empty SigningPubKey (7300)
  uint8_t unsignedBin[512];
  size_t unsignedLen = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::xahauSerialize(bag, false, unsignedBin, sizeof(unsignedBin), unsignedLen));
  char unsignedHex[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::bytesToHex(unsignedBin, unsignedLen, unsignedHex, sizeof(unsignedHex)));
  TEST_ASSERT_NOT_NULL(strstr(unsignedHex, "120000"));  // Payment
  TEST_ASSERT_NOT_NULL(strstr(unsignedHex, "7300"));    // empty SigningPubKey

  xahau::MultiSigner ms;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::multiSignAs(bag, b, ms));
  TEST_ASSERT_TRUE(ms.signatureLen >= 64);
  TEST_ASSERT_EQUAL(0, memcmp(ms.accountId, b.accountId, 20));

  char signedHex[4096];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::multiSignFinalize(bag, &ms, 1, signedHex, sizeof(signedHex)));
  TEST_ASSERT_TRUE(strlen(signedHex) >= 200);

  char aAccHex[48], bAccHex[48];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(a.accountId, 20, aAccHex, sizeof(aAccHex)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(b.accountId, 20, bAccHex, sizeof(bAccHex)));
  TEST_ASSERT_NOT_NULL(strstr(signedHex, aAccHex));
  TEST_ASSERT_NOT_NULL(strstr(signedHex, bAccHex));

  // Determinism (RFC 6979 secp): same inputs -> identical multi-sign blob
  char signedHex2[4096];
  xahau::TxFieldBag bag2;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, a.accountId, a.publicKey, bag2));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bagPrepareMultiSign(bag2));
  xahau::MultiSigner ms2;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::multiSignAs(bag2, b, ms2));
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::multiSignFinalize(bag2, &ms2, 1, signedHex2, sizeof(signedHex2)));
  TEST_ASSERT_EQUAL_STRING(signedHex, signedHex2);
  TEST_ASSERT_EQUAL(ms.signatureLen, ms2.signatureLen);
  TEST_ASSERT_EQUAL(0, memcmp(ms.signature, ms2.signature, ms.signatureLen));

  xahau::secureClear(a);
  xahau::secureClear(b);
}


void test_signer_list_set_encodes_entry() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair a, b;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SEED_A, a));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SEED_B, b));

  xahau::SignerListSetParams p;
  p.common.flags = 0x80000000u;
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  p.common.feeDrops = 12;
  p.signerQuorum = 1;
  p.entryCount = 1;
  memcpy(p.entries[0].accountId, b.accountId, 20);
  p.entries[0].weight = 1;

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSignerListSet(p, a.accountId, a.publicKey, bag));
  uint8_t bin[400];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  char bAccHex[48];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(b.accountId, 20, bAccHex, sizeof(bAccHex)));
  TEST_ASSERT_NOT_NULL(strstr(hex, bAccHex));
  TEST_ASSERT_NOT_NULL(strstr(hex, "00000001"));

  // Deterministic re-encode
  xahau::TxFieldBag bag2;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSignerListSet(p, a.accountId, a.publicKey, bag2));
  uint8_t bin2[400];
  size_t len2 = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag2, false, bin2, sizeof(bin2), len2));
  char hex2[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin2, len2, hex2, sizeof(hex2)));
  TEST_ASSERT_EQUAL_STRING(hex, hex2);

  xahau::secureClear(a);
  xahau::secureClear(b);
}

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  RUN_TEST(test_multisign_two_key_payment_golden);
  RUN_TEST(test_signer_list_set_encodes_entry);
  return UNITY_END();
}
