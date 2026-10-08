// Host test: full transaction catalog
#include <unity.h>
#include "XahauClient.h"



void test_catalog_count() {
  TEST_ASSERT_EQUAL_UINT(94, xahauTxTypeCount());
}

void test_payment_lookup() {
  const xahau::TxTypeInfo* p = xahau::txTypeLookup(TxType::Payment);
  TEST_ASSERT_NOT_NULL(p);
  TEST_ASSERT_EQUAL_STRING("Payment", p->name);
  TEST_ASSERT_EQUAL_UINT16(0, p->typeCode);
  TEST_ASSERT_TRUE(p->userSubmittable);
}

void test_lookup_by_name() {
  const xahau::TxTypeInfo* t = xahau::txTypeLookupByName("TrustSet");
  TEST_ASSERT_NOT_NULL(t);
  TEST_ASSERT_EQUAL(TxType::TrustSet, t->type);
  TEST_ASSERT_EQUAL_UINT16(20, t->typeCode);
}

void test_pseudo_not_user() {
  const xahau::TxTypeInfo* t = xahau::txTypeLookup(TxType::EnableAmendment);
  TEST_ASSERT_NOT_NULL(t);
  TEST_ASSERT_FALSE(t->userSubmittable);
}

void test_cron_emitted_not_user_and_decode_stub() {
  const xahau::TxTypeInfo* t = xahau::txTypeLookup(TxType::Cron);
  TEST_ASSERT_NOT_NULL(t);
  TEST_ASSERT_EQUAL_UINT16(92, t->typeCode);
  TEST_ASSERT_FALSE(t->userSubmittable);

  // Empty / wrong-type -> Invalid*
  xahau::CronEmittedView view;
  TEST_ASSERT_EQUAL(xahau::Status::InvalidArgument, xahau::decodeEmittedCron(nullptr, 0, view));
  uint8_t dummy[1] = {0};
  TEST_ASSERT_EQUAL(xahau::Status::InvalidField, xahau::decodeEmittedCron(dummy, 1, view));

  // Captured testnet Cron (hash 8C07EBB4..., Owner=Primary)
  static const char* kCronHex =
      "12005C24000000002600C12FD96840000000000000007300811400000000000000000000000000000000000000008214CD975891590909FB434DB81BF53361841463AF46";
  uint8_t blob[128];
  size_t blobLen = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::hexToBytes(kCronHex, blob, sizeof(blob), blobLen));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeEmittedCron(blob, blobLen, view));
  TEST_ASSERT_EQUAL_UINT32(0, view.sequence);
  TEST_ASSERT_EQUAL_UINT32(12660697, view.ledgerSequence);
  TEST_ASSERT_EQUAL_STRING("rKkhqpbHdfftZddwLFocxfQvCjGcUG3nqU", view.owner);
}

void test_loan_and_vault_catalog_only() {
  const xahau::TxTypeInfo* loan = xahau::txTypeLookup(xahau::TxType::LoanSet);
  TEST_ASSERT_NOT_NULL(loan);
  TEST_ASSERT_EQUAL_STRING("LoanSet", loan->name);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, loan->maturity);
  const xahau::TxTypeInfo* vault = xahau::txTypeLookup(xahau::TxType::VaultCreate);
  TEST_ASSERT_NOT_NULL(vault);
  TEST_ASSERT_EQUAL_STRING("VaultCreate", vault->name);
  TEST_ASSERT_EQUAL(xahau::TxMaturity::Catalog, vault->maturity);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_catalog_count);
  RUN_TEST(test_payment_lookup);
  RUN_TEST(test_lookup_by_name);
  RUN_TEST(test_pseudo_not_user);
  RUN_TEST(test_cron_emitted_not_user_and_decode_stub);
  RUN_TEST(test_loan_and_vault_catalog_only);
  return UNITY_END();
}
