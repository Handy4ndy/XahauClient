// Host golden vectors: Remit + URIToken encode shapes (unsigned STObject hex)
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

static const char* kRemitXah1 =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637F05CE05B614000000000000001"
    "E1F1";

static const char* kRemitTwoAmounts =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637F05CE05B61400000000000000A"
    "E1E05B614000000000000014E1F1";

static const char* kRemitUriTokenIds =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB463700136320ABABABABABABABABAB"
    "ABABABABABABABABABABABABABABABABABABABABABABAB";

static const char* kRemitMintUri =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637E05C751768747470733A2F2F65"
    "78616D706C652E636F6D2F752F31E1";

static const char* kRemitXahUsd =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637F05CE05B614000000000002710"
    "E1E05B61D491C37937E0800000000000000000000000000055534400000000006DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7CE1F1";

static const char* kRemitXahUsdUri =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637F05CE05B614000000000002710"
    "E1E05B61D491C37937E0800000000000000000000000000055534400000000006DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7CE1F100136320CDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD"
    "CDCDCDCDCDCDCDCDCDCD";

static const char* kRemitTwoIou =
    "12005F22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637F05CE05B61D4838D7EA4C68000"
    "00000000000000000000000055534400000000006DE8B4EADE2E2306E80543259BD5DE11A5E10C7C"
    "E1E05B61D4871AFD498D000000000000000000000000000045555200000000006DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7CE1F1";
static const char* kUriMint =
    "12002D22800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EF"
    "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE6751968747470733A2F2F6578616D706C65"
    "2E636F6D2F6E66742F3181146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static const char* kUriMintDigest =
    "12002D22800000002400000001201B000000645015AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAAAAAAAAA68400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970"
    "A0FBAD827646E41F8346FC515D64CFFEE6750A697066733A2F2F63696481146DE8B4EADE2E2306E8"
    "0543259BD5DE11A5E10C7C";

static const char* kUriBurn =
    "12002E22800000002400000001201B00000064502411111111111111111111111111111111111111"
    "1111111111111111111111111168400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970"
    "A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static const char* kUriBuy =
    "12002F22800000002400000001201B00000064502422222222222222222222222222222222222222"
    "2222222222222222222222222261400000000000006468400000000000000C7321031482CD1D0C8F"
    "93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E8054325"
    "9BD5DE11A5E10C7C";

static const char* kUriCreateSell =
    "12003022800000002400000001201B00000064502433333333333333333333333333333333333333"
    "3333333333333333333333333361400000000000003268400000000000000C7321031482CD1D0C8F"
    "93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E8054325"
    "9BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637";

static const char* kUriCancelSell =
    "12003122800000002400000001201B00000064502444444444444444444444444444444444444444"
    "4444444444444444444444444468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970"
    "A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static void common(xahau::TxCommon& c) {
  c.flags = 0x80000000u;
  c.sequence = 1;
  c.lastLedgerSequence = 100;
  c.feeDrops = 12;
}

static void fill32(uint8_t* p, uint8_t b) { memset(p, b, 32); }

static void assertU(const xahau::TxFieldBag& bag, const char* exp) {
  uint8_t bin[512];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(exp, hex);
}

void test_remit_single_xah_amount() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitXah1);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignRemit(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignRemit(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "12005F"));
  xahau::secureClear(keys);
}

void test_remit_two_amounts() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.amountCount = 2;
  p.amounts[0].kind = xahau::AmountKind::XrpDrops;
  p.amounts[0].drops = 10;
  p.amounts[1].kind = xahau::AmountKind::XrpDrops;
  p.amounts[1].drops = 20;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitTwoAmounts);
  xahau::secureClear(keys);
}

void test_remit_uritoken_ids() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.uriTokenIdCount = 1;
  fill32(p.uriTokenIds[0], 0xAB);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitUriTokenIds);
  xahau::secureClear(keys);
}

void test_remit_mint_uritoken() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  strncpy(p.mintUri, "https://example.com/u/1", sizeof(p.mintUri) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitMintUri);
  xahau::secureClear(keys);
}

void test_uritoken_mint_plain() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenMintParams p;
  common(p.common);
  strncpy(p.uri, "https://example.com/nft/1", sizeof(p.uri) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillURITokenMint(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriMint);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignURITokenMint(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignURITokenMint(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_NOT_NULL(strstr(a, "12002D"));
  xahau::secureClear(keys);
}

void test_uritoken_mint_with_digest() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenMintParams p;
  common(p.common);
  strncpy(p.uri, "ipfs://cid", sizeof(p.uri) - 1);
  p.hasDigest = true;
  memset(p.digestHex, 0, sizeof(p.digestHex));
  memcpy(p.digestHex, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 64);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillURITokenMint(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriMintDigest);
  xahau::secureClear(keys);
}

void test_uritoken_burn() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenBurnParams p;
  common(p.common);
  fill32(p.uriTokenId, 0x11);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillURITokenBurn(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriBurn);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignURITokenBurn(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignURITokenBurn(keys, p, b, sizeof(b)));
  TEST_ASSERT_EQUAL_STRING(a, b);
  xahau::secureClear(keys);
}

void test_uritoken_buy() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenBuyParams p;
  common(p.common);
  fill32(p.uriTokenId, 0x22);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 100;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillURITokenBuy(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriBuy);
  xahau::secureClear(keys);
}

void test_uritoken_create_sell_offer() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenCreateSellOfferParams p;
  common(p.common);
  fill32(p.uriTokenId, 0x33);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 50;
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillURITokenCreateSellOffer(
                                           p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriCreateSell);
  xahau::secureClear(keys);
}

void test_uritoken_cancel_sell_offer() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::URITokenCancelSellOfferParams p;
  common(p.common);
  fill32(p.uriTokenId, 0x44);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillURITokenCancelSellOffer(
                                           p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kUriCancelSell);
  xahau::secureClear(keys);
}


void test_remit_xah_usd_iou() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.amountCount = 2;
  p.amounts[0].kind = xahau::AmountKind::XrpDrops;
  p.amounts[0].drops = 10000;
  p.amounts[1].kind = xahau::AmountKind::Issued;
  strncpy(p.amounts[1].issued.value, "5", sizeof(p.amounts[1].issued.value) - 1);
  strncpy(p.amounts[1].issued.currency, "USD", sizeof(p.amounts[1].issued.currency) - 1);
  strncpy(p.amounts[1].issued.issuer, XAHAU_TEST_ADDR_SECP1,
          sizeof(p.amounts[1].issued.issuer) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitXahUsd);
  xahau::secureClear(keys);
}

void test_remit_xah_usd_uritoken_id() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.amountCount = 2;
  p.amounts[0].kind = xahau::AmountKind::XrpDrops;
  p.amounts[0].drops = 10000;
  p.amounts[1].kind = xahau::AmountKind::Issued;
  strncpy(p.amounts[1].issued.value, "5", sizeof(p.amounts[1].issued.value) - 1);
  strncpy(p.amounts[1].issued.currency, "USD", sizeof(p.amounts[1].issued.currency) - 1);
  strncpy(p.amounts[1].issued.issuer, XAHAU_TEST_ADDR_SECP1,
          sizeof(p.amounts[1].issued.issuer) - 1);
  p.uriTokenIdCount = 1;
  fill32(p.uriTokenIds[0], 0xCD);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitXahUsdUri);
  xahau::secureClear(keys);
}

void test_remit_two_iou_amounts() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::RemitParams p;
  common(p.common);
  strncpy(p.destination, DEST, sizeof(p.destination) - 1);
  p.amountCount = 2;
  p.amounts[0].kind = xahau::AmountKind::Issued;
  strncpy(p.amounts[0].issued.value, "1", sizeof(p.amounts[0].issued.value) - 1);
  strncpy(p.amounts[0].issued.currency, "USD", sizeof(p.amounts[0].issued.currency) - 1);
  strncpy(p.amounts[0].issued.issuer, XAHAU_TEST_ADDR_SECP1,
          sizeof(p.amounts[0].issued.issuer) - 1);
  p.amounts[1].kind = xahau::AmountKind::Issued;
  strncpy(p.amounts[1].issued.value, "2", sizeof(p.amounts[1].issued.value) - 1);
  strncpy(p.amounts[1].issued.currency, "EUR", sizeof(p.amounts[1].issued.currency) - 1);
  strncpy(p.amounts[1].issued.issuer, XAHAU_TEST_ADDR_SECP1,
          sizeof(p.amounts[1].issued.issuer) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillRemit(p, keys.accountId, keys.publicKey, bag));
  assertU(bag, kRemitTwoIou);
  xahau::secureClear(keys);
}
void test_maturity_remit_uritoken_golden() {
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::Remit)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::URITokenMint)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::URITokenBurn)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden, xahau::txTypeLookup(TxType::URITokenBuy)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden,
                    xahau::txTypeLookup(TxType::URITokenCreateSellOffer)->maturity);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Golden,
                    xahau::txTypeLookup(TxType::URITokenCancelSellOffer)->maturity);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_maturity_remit_uritoken_golden);
  RUN_TEST(test_remit_single_xah_amount);
  RUN_TEST(test_remit_two_amounts);
  RUN_TEST(test_remit_uritoken_ids);
  RUN_TEST(test_remit_mint_uritoken);
  RUN_TEST(test_remit_xah_usd_iou);
  RUN_TEST(test_remit_xah_usd_uritoken_id);
  RUN_TEST(test_remit_two_iou_amounts);
  RUN_TEST(test_uritoken_mint_plain);
  RUN_TEST(test_uritoken_mint_with_digest);
  RUN_TEST(test_uritoken_burn);
  RUN_TEST(test_uritoken_buy);
  RUN_TEST(test_uritoken_create_sell_offer);
  RUN_TEST(test_uritoken_cancel_sell_offer);
  return UNITY_END();
}