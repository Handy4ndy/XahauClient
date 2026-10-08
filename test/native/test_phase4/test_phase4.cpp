// Phase4 host tests — OracleSet + protocol field table (EncodeOnly families cut 2026-09-28)
#include <unity.h>
#include <string.h>
#include "XahauClient.h"
#include "xahau/core/hex.h"
#include "xahau/codec/serialize.h"
#include "xahau/generated/protocol_fields.h"

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

void test_protocol_field_table() {
  TEST_ASSERT_TRUE(xahau::protocol::kFieldCount >= 300);
  const xahau::protocol::FieldDef* a = xahau::protocol::findField("Asset");
  TEST_ASSERT_NOT_NULL(a);
  TEST_ASSERT_EQUAL_UINT16(24, a->typeCode);
  TEST_ASSERT_EQUAL_UINT16(3, a->nth);
  TEST_ASSERT_NOT_NULL(xahau::protocol::findField("XChainBridge"));
  TEST_ASSERT_NOT_NULL(xahau::protocol::findField("PriceDataSeries"));
}

void test_oracle_set_encode() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::deriveKeysFromSeed(SECP, keys));
  xahau::OracleSetParams p;
  common(p.common);
  p.oracleDocumentID = 1;
  p.lastUpdateTime = 700000000;
  strncpy(p.baseAsset, "XRP", sizeof(p.baseAsset) - 1);
  strncpy(p.quoteAsset, "USD", sizeof(p.quoteAsset) - 1);
  p.hasPrice = true;
  p.assetPrice = 1000000;
  p.scale = 6;
  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillOracleSet(p, keys.accountId, keys.publicKey, bag));
  assertU(bag,
          "12003C228000000024000000012F62266A80201B0000006420330000000168400000000000000C7321031482CD"
          "1D0C8F93A4EFBC379C68F970A0FBAD827646E41F8346FC515D64CFFEE681146DE8B4EADE2E2306E80543259BD5"
          "DE11A5E10C7CF018E020301700000000000F4240041006011A000000000000000000000000000000000000000"
          "0021A0000000000000000000000005553440000000000E1F1");
  xahau::secureClear(keys);
}

void test_cut_families_catalog_only() {
  // EncodeOnly stubs removed from src; catalog codes remain as Catalog maturity.
  const xahau::TxTypeInfo* did = xahau::txTypeLookup(xahau::TxType::DIDSet);
  TEST_ASSERT_NOT_NULL(did);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, did->maturity);
  const xahau::TxTypeInfo* amm = xahau::txTypeLookup(xahau::TxType::AMMCreate);
  TEST_ASSERT_NOT_NULL(amm);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, amm->maturity);
  const xahau::TxTypeInfo* loan = xahau::txTypeLookup(xahau::TxType::LoanSet);
  TEST_ASSERT_NOT_NULL(loan);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, loan->maturity);
  const xahau::TxTypeInfo* batch = xahau::txTypeLookup(xahau::TxType::Batch);
  TEST_ASSERT_NOT_NULL(batch);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, batch->maturity);
  const xahau::TxTypeInfo* imp = xahau::txTypeLookup(xahau::TxType::Import);
  TEST_ASSERT_NOT_NULL(imp);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Encode, imp->maturity);
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_protocol_field_table);
  RUN_TEST(test_oracle_set_encode);
  RUN_TEST(test_cut_families_catalog_only);
  return UNITY_END();
}
