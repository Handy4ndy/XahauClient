// Phase 2: IOU payment, offers, tickets, regular key, signer list
// Secp signatures: RFC6979 via uECC (deterministic library goldens).
// Ed25519: full match to ripple-keypairs.
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
static const char* ED = XAHAU_TEST_SEED_ED1;

// Library-native deterministic secp signed blobs (uECC RFC6979 + SHA-256 + low-S BIP66)
static const char* kPaySecpSigned =
    "12000022800000002400000001201B0000006461400000000000000168400000000000000C7321031482CD1D0C8F"
    "93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE6744730450221009F3B1788560593987B35F32EBFFF"
    "78B5A4B55CB5E8B7D4A5F913B48D6878266D02203412BE667B54C1DD57792316925A76BE095AD47F54154B038D0D34"
    "04C1850BE281146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB"
    "4637";
static const char* kOfferCreateSigned =
    "12000722800000002400000001201B0000006464D4C38D7EA4C6800000000000000000000000000055534400000000"
    "003839ECDB7361A151F916265838A95D7537CB46376540000000000F424068400000000000000C7321031482CD1D0C"
    "8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE674473045022100B55250FD08DCA36CFBF39A61D1"
    "188ADC8B40F16270EE0ABCCE7123C8C2E2988502202169EEC269C71CA4DF3FB4F8826CC682AB6962D82C455230BF77"
    "C38FA8B761F781146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";
static const char* kOfferCancelSigned =
    "12000822800000002400000001201900000005201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC"
    "379C68F970A0FBAD827646E41F8346FC515D64CFFEE67446304402203A44F25A7DD14ED82FA49FC3A5E32CE3AF4381"
    "6F639EA9C636C5C9802B6DFC3202203BE77CD97024FD1A581E0A08E9815A5DF47742E2AE45E1C11E25808B33F091CF"
    "81146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";
static const char* kAccountSetSigned =
    "12000322800000002400000001201B0000006420210000000868400000000000000C7321031482CD1D0C8F93A4EFBC"
    "379C68F970A0FBAD827646E41F8346FC515D64CFFEE67446304402200D29D5FDAEA458A04A530A6CC2A8BE19639832"
    "EA54E394A25E3EC10AE9ACCB6A02200CC6E92721832C49B1B9D0081B053C61E53FD88188489E69D76E44B24473F1BE"
    "81146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";
static const char* kTrustSetSigned =
    "12001422800000002400000001201B0000006463D5438D7EA4C6800000000000000000000000000055534400000000"
    "003839ECDB7361A151F916265838A95D7537CB463768400000000000000C7321031482CD1D0C8F93A4EFBC379C68F9"
    "70A0FBAD827646E41F8346FC515D64CFFEE674473045022100BDD7C64E3066B1D6DFB82B70873364D4C020340FEC9A"
    "565A07278876131B5F6602206C6C962645FFD8CCAE7EAFCF5055D7ED6591B3AD6E7EFDCB2CD154E392D226FD81146D"
    "E8B4EADE2E2306E80543259BD5DE11A5E10C7C";

static void commonSecp(xahau::TxCommon& c) {
  c.flags = 0x80000000u;
  c.sequence = 1;
  c.lastLedgerSequence = 100;
  c.feeDrops = 12;
}

static void assertSignTwiceEqual(const char* a, const char* b) {
  TEST_ASSERT_EQUAL_STRING(a, b);
  TEST_ASSERT_TRUE(strlen(a) > 80);
}

void test_payment_signed_secp_deterministic() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::PaymentParams pay;
  commonSecp(pay.common);
  strncpy(pay.destination, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;
  char a[1024], b[1024];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kPaySecpSigned, a);
  xahau::secureClear(keys);
}

void test_payment_iou_tags_memo_ed() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(ED, keys));
  xahau::PaymentParams pay;
  pay.common.flags = 0;
  pay.common.sequence = 1;
  pay.common.lastLedgerSequence = 100;
  pay.common.feeDrops = 12;
  pay.common.sourceTag = 7;
  pay.common.destinationTag = 99;
  strncpy(pay.destination, XAHAU_TEST_ADDR_SECP1, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::Issued;
  strncpy(pay.amount.issued.value, "10", sizeof(pay.amount.issued.value) - 1);
  strncpy(pay.amount.issued.currency, "USD", sizeof(pay.amount.issued.currency) - 1);
  strncpy(pay.amount.issued.issuer, XAHAU_TEST_ADDR_SECP1,
          sizeof(pay.amount.issued.issuer) - 1);
  strncpy(pay.memoData, "hello", sizeof(pay.memoData) - 1);

  static const char* kUnsigned =
      "1200002200000000230000000724000000012E00000063201B0000006461D4C38D7EA4C68000000000000000000000"
      "00000055534400000000006DE8B4EADE2E2306E80543259BD5DE11A5E10C7C68400000000000000C7321ED01FA53FA"
      "5A7E77798F882ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A638114D28B177E48D9A8D057E70F7E464B498367"
      "281B9883146DE8B4EADE2E2306E80543259BD5DE11A5E10C7CF9EA7D0568656C6C6FE1F1";
  static const char* kSigned =
      "1200002200000000230000000724000000012E00000063201B0000006461D4C38D7EA4C68000000000000000000000"
      "00000055534400000000006DE8B4EADE2E2306E80543259BD5DE11A5E10C7C68400000000000000C7321ED01FA53FA"
      "5A7E77798F882ECE20B1ABC00BB358A9E55A202D0D0676BD0CE37A637440FC0B95D05E93AB1F311C31A4282B155CA6"
      "20A6BF0F3F4C2F24933B77569C66CA1A08EA06A79974DB9C9A977F10C06FF2A5DA01FEB1B4B99DFA89930FF0FB6105"
      "8114D28B177E48D9A8D057E70F7E464B498367281B9883146DE8B4EADE2E2306E80543259BD5DE11A5E10C7CF9EA7D"
      "0568656C6C6FE1F1";

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));
  uint8_t bin[400];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kUnsigned, hex);

  char blob[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, blob, sizeof(blob)));
  TEST_ASSERT_EQUAL_STRING(kSigned, blob);
  xahau::secureClear(keys);
}

void test_offer_create_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::OfferCreateParams p;
  commonSecp(p.common);
  p.takerGets.kind = xahau::AmountKind::XrpDrops;
  p.takerGets.drops = 1000000;
  p.takerPays.kind = xahau::AmountKind::Issued;
  strncpy(p.takerPays.issued.value, "10", sizeof(p.takerPays.issued.value) - 1);
  strncpy(p.takerPays.issued.currency, "USD", sizeof(p.takerPays.issued.currency) - 1);
  strncpy(p.takerPays.issued.issuer, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN",
          sizeof(p.takerPays.issued.issuer) - 1);

  static const char* kU =
      "12000722800000002400000001201B0000006464D4C38D7EA4C6800000000000000000000000000055534400000000"
      "003839ECDB7361A151F916265838A95D7537CB46376540000000000F424068400000000000000C7321031482CD1D0C"
      "8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E1"
      "0C7C";

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillOfferCreate(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[300];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[700];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kU, hex);

  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignOfferCreate(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignOfferCreate(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kOfferCreateSigned, a);
  xahau::secureClear(keys);
}

void test_offer_cancel_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::OfferCancelParams p;
  commonSecp(p.common);
  p.offerSequence = 5;
  char a[800], b[800];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignOfferCancel(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignOfferCancel(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kOfferCancelSigned, a);
  xahau::secureClear(keys);
}

void test_ticket_create_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::TicketCreateParams p;
  commonSecp(p.common);
  p.ticketCount = 3;
  static const char* kU =
      "12000A22800000002400000001201B0000006420280000000368400000000000000C7321031482CD1D0C8F93A4EF"
      "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C";
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillTicketCreate(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[200];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[500];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kU, hex);
  char a[800], b[800];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignTicketCreate(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignTicketCreate(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  xahau::secureClear(keys);
}

void test_set_regular_key_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SetRegularKeyParams p;
  commonSecp(p.common);
  strncpy(p.regularKey, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(p.regularKey) - 1);
  static const char* kU =
      "12000522800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93A4EFBC379C68F970"
      "A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C88143839ECDB73"
      "61A151F916265838A95D7537CB4637";
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSetRegularKey(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[220];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[500];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kU, hex);
  char a[900], b[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRegularKey(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRegularKey(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  xahau::secureClear(keys);
}

void test_signer_list_set_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SignerListSetParams p;
  commonSecp(p.common);
  p.signerQuorum = 1;
  p.entryCount = 1;
  p.entries[0].weight = 1;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeClassicAddress("ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN",
                                                                 p.entries[0].accountId));
  static const char* kU =
      "12000C22800000002400000001201B0000006420230000000168400000000000000C7321031482CD1D0C8F93A4EF"
      "BC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7CF4"
      "EB13000181143839ECDB7361A151F916265838A95D7537CB4637E1F1";
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSignerListSet(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[280];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[600];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kU, hex);
  char a[1000], b[1000];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  xahau::secureClear(keys);
}

void test_account_set_signed_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::AccountSetParams p;
  commonSecp(p.common);
  p.hasSetFlag = true;
  p.setFlag = 8;
  char a[900], b[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignAccountSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignAccountSet(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kAccountSetSigned, a);
  xahau::secureClear(keys);
}

void test_trust_set_signed_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::TrustSetParams p;
  commonSecp(p.common);
  strncpy(p.limit.value, "1000", sizeof(p.limit.value) - 1);
  strncpy(p.limit.currency, "USD", sizeof(p.limit.currency) - 1);
  strncpy(p.limit.issuer, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(p.limit.issuer) - 1);
  char a[1000], b[1000];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignTrustSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignTrustSet(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kTrustSetSigned, a);
  xahau::secureClear(keys);
}


// ---------------------------------------------------------------------------
// Coverage-gap goldens (2026-09-26): TicketSequence spend, clear RegularKey,
// SignerListSet multi/quorum>1 / delete. Deterministic secp (same SECP seed).
// ---------------------------------------------------------------------------
static const char* DEST2 = XAHAU_TEST_ADDR_ED1; // ed25519 fixture address

static const char* kPayTicketSeqU =
    "12000022800000002400000000201B0000006420290000000561400000000000000168400000"
    "000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CF"
    "FEE681146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F9162658"
    "38A95D7537CB4637";
static const char* kPayTicketSeqS =
    "12000022800000002400000000201B0000006420290000000561400000000000000168400000"
    "000000000C7321031482CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CF"
    "FEE674473045022100BBD45CA2417DFFF0367DC3C3135903FBA5362D62D3FE34CBE35018FD7D"
    "356620022064BFA787DE2122865A94C578F1766001288562142D03ACB2DA6EF08EF941457281"
    "146DE8B4EADE2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D"
    "7537CB4637";
static const char* kInvokeTicketSeqU =
    "12006322800000002400000000201B0000006420290000000768400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE"
    "2E2306E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637";
static const char* kInvokeTicketSeqS =
    "12006322800000002400000000201B0000006420290000000768400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE674473045022100"
    "FEAA845AA699B027FB33F33D4B5D03824022AD02EA990057393B9C409E443AE702206E753641"
    "18B4EB2B33CF77188274F139AFEE33387CD458CE5A7C054D5944294381146DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7C83143839ECDB7361A151F916265838A95D7537CB4637";
static const char* kSetRegularKeyClearU =
    "12000522800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93"
    "A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543"
    "259BD5DE11A5E10C7C";
static const char* kSetRegularKeyClearS =
    "12000522800000002400000001201B0000006468400000000000000C7321031482CD1D0C8F93"
    "A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE67446304402204F4D21258DFAF8"
    "B814F6061263D584D988EB5AE967BCF1C50BC3323950EDD616022067BE35CC84C22245583738"
    "2F225C6B195EF66C446F2277A028830D1CC4C396A381146DE8B4EADE2E2306E80543259BD5DE"
    "11A5E10C7C";
static const char* kSignerListMultiU =
    "12000C22800000002400000001201B0000006420230000000268400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE"
    "2E2306E80543259BD5DE11A5E10C7CF4EB13000181143839ECDB7361A151F916265838A95D75"
    "37CB4637E1EB1300018114D28B177E48D9A8D057E70F7E464B498367281B98E1F1";
static const char* kSignerListMultiS =
    "12000C22800000002400000001201B0000006420230000000268400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE674473045022100"
    "D742C7C125DA60D0DF540C46F9095E222A7066175987B62CAE09D012BBACD6FD022055FDC849"
    "D9DF7A8177558BC11E88D196269ADBD29AA287FE8CFB44AB3E81FD4881146DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7CF4EB13000181143839ECDB7361A151F916265838A95D7537CB46"
    "37E1EB1300018114D28B177E48D9A8D057E70F7E464B498367281B98E1F1";
static const char* kSignerListDeleteU =
    "12000C22800000002400000001201B0000006420230000000068400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE"
    "2E2306E80543259BD5DE11A5E10C7C";
static const char* kSignerListDeleteS =
    "12000C22800000002400000001201B0000006420230000000068400000000000000C73210314"
    "82CD1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE674473045022100"
    "D624DF386D8B974D71ADCB549BFA698015E06D0EFBF844728F9FADFF9A6798F30220667CD219"
    "3777F5B896CF273E15718BB5563E3E36604B9EC4C6647EE5F752E79581146DE8B4EADE2E2306"
    "E80543259BD5DE11A5E10C7C";

void test_payment_ticket_sequence_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::PaymentParams pay;
  commonSecp(pay.common);
  pay.common.sequence = 0;           // ticket spend: Sequence=0
  pay.common.ticketSequence = 5;
  strncpy(pay.destination, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillPayment(pay, keys.accountId, keys.publicKey, bag));
  uint8_t bin[300];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[700];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kPayTicketSeqU, hex);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignPayment(keys, pay, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kPayTicketSeqS, a);
  xahau::secureClear(keys);
}

void test_invoke_ticket_sequence_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::InvokeParams inv;
  commonSecp(inv.common);
  inv.common.sequence = 0;
  inv.common.ticketSequence = 7;
  strncpy(inv.destination, "ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN", sizeof(inv.destination) - 1);
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillInvoke(inv, keys.accountId, keys.publicKey, bag));
  uint8_t bin[280];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[650];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kInvokeTicketSeqU, hex);
  char a[1200], b[1200];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, inv, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignInvoke(keys, inv, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kInvokeTicketSeqS, a);
  xahau::secureClear(keys);
}

void test_set_regular_key_clear_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SetRegularKeyParams p;
  commonSecp(p.common);
  // Empty regularKey + hasRegularKeyId=false => omit RegularKey (clear)
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSetRegularKey(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[220];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[500];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kSetRegularKeyClearU, hex);
  char a[900], b[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRegularKey(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSetRegularKey(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kSetRegularKeyClearS, a);
  xahau::secureClear(keys);
}

void test_signer_list_multi_quorum2_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SignerListSetParams p;
  commonSecp(p.common);
  p.signerQuorum = 2;
  p.entryCount = 2;
  p.entries[0].weight = 1;
  p.entries[1].weight = 1;
  // AccountID ascending for canonical SignerEntries order
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeClassicAddress("ra3JNTYMhTR9sUkT1By1ucuteEfdPqojhN",
                                                                 p.entries[0].accountId));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeClassicAddress(DEST2, p.entries[1].accountId));
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSignerListSet(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[360];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[800];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kSignerListMultiU, hex);
  char a[1400], b[1400];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kSignerListMultiS, a);
  xahau::secureClear(keys);
}

void test_signer_list_delete_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::SignerListSetParams p;
  commonSecp(p.common);
  p.signerQuorum = 0;
  p.entryCount = 0;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::fillSignerListSet(p, keys.accountId, keys.publicKey, bag));
  uint8_t bin[220];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  char hex[500];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bin, len, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING(kSignerListDeleteU, hex);
  char a[900], b[900];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, a, sizeof(a)));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::buildAndSignSignerListSet(keys, p, b, sizeof(b)));
  assertSignTwiceEqual(a, b);
  TEST_ASSERT_EQUAL_STRING(kSignerListDeleteS, a);
  xahau::secureClear(keys);
}
void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_payment_signed_secp_deterministic);
  RUN_TEST(test_payment_iou_tags_memo_ed);
  RUN_TEST(test_offer_create_secp);
  RUN_TEST(test_offer_cancel_secp);
  RUN_TEST(test_ticket_create_secp);
  RUN_TEST(test_set_regular_key_secp);
  RUN_TEST(test_signer_list_set_secp);
  RUN_TEST(test_account_set_signed_secp);
  RUN_TEST(test_trust_set_signed_secp);
    RUN_TEST(test_payment_ticket_sequence_secp);
  RUN_TEST(test_invoke_ticket_sequence_secp);
  RUN_TEST(test_set_regular_key_clear_secp);
  RUN_TEST(test_signer_list_multi_quorum2_secp);
  RUN_TEST(test_signer_list_delete_secp);
  return UNITY_END();
}
