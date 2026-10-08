// Host test: binary writers + field bag serialize
#include <unity.h>
#include <string.h>
#include "XahauClient.h"



void test_field_header_compact() {
  uint8_t buf[8];
  size_t pos = 0;
  // TransactionType = type 1, nth 2 -> 0x12
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::writeFieldHeader(buf, pos, sizeof(buf), 1, 2));
  TEST_ASSERT_EQUAL_UINT(1, pos);
  TEST_ASSERT_EQUAL_HEX8(0x12, buf[0]);
}

void test_xrp_amount_encoding() {
  uint8_t buf[8];
  size_t pos = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::writeXrpAmount(buf, pos, sizeof(buf), 1000000ULL));
  TEST_ASSERT_EQUAL_UINT(8, pos);
  // 0x40000000000F4240
  TEST_ASSERT_EQUAL_HEX8(0x40, buf[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, buf[1]);
  TEST_ASSERT_EQUAL_HEX8(0x0F, buf[5]);
  TEST_ASSERT_EQUAL_HEX8(0x42, buf[6]);
  TEST_ASSERT_EQUAL_HEX8(0x40, buf[7]);
}

void test_vl_short() {
  uint8_t buf[4];
  size_t pos = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::writeVLLength(buf, pos, sizeof(buf), 20));
  TEST_ASSERT_EQUAL_UINT(1, pos);
  TEST_ASSERT_EQUAL_HEX8(20, buf[0]);
}

void test_bag_sort_order_fee_before_sendmax() {
  // Critical: Fee (6:8) must serialize before SendMax (6:9)
  xahau::TxFieldBag bag;
  bag.setUInt16(xahau::fc::TransactionType, 0);
  bag.setAmountXrp(xahau::fc::SendMax, 50);
  bag.setAmountXrp(xahau::fc::Fee, 12);
  bag.setAmountXrp(xahau::fc::Amount, 100);

  size_t idx[16];
  size_t n = bag.sortedIndices(idx, 16);
  TEST_ASSERT_EQUAL_UINT(4, n);

  // Expect Amount(6:1), Fee(6:8), SendMax(6:9) among amount fields after TransactionType
  TEST_ASSERT_EQUAL_UINT32(xahau::fc::TransactionType, bag.slot(idx[0]).fieldCode);
  TEST_ASSERT_EQUAL_UINT32(xahau::fc::Amount, bag.slot(idx[1]).fieldCode);
  TEST_ASSERT_EQUAL_UINT32(xahau::fc::Fee, bag.slot(idx[2]).fieldCode);
  TEST_ASSERT_EQUAL_UINT32(xahau::fc::SendMax, bag.slot(idx[3]).fieldCode);
}

void test_hex_roundtrip() {
  const uint8_t in[] = {0xDE, 0xAD, 0xBE, 0xEF};
  char hex[16];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(in, 4, hex, sizeof(hex)));
  TEST_ASSERT_EQUAL_STRING("DEADBEEF", hex);
  uint8_t out[4];
  size_t outLen = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::hexToBytes(hex, out, sizeof(out), outLen));
  TEST_ASSERT_EQUAL_UINT(4, outLen);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(in, out, 4);
}

void test_rpc_account_info_build() {
  char buf[160];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcBuildAccountInfo("rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh", buf, sizeof(buf)));
  TEST_ASSERT_NOT_NULL(strstr(buf, "account_info"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh"));
}

void test_minimal_payment_serialize_xrp() {
  // No Base58: use raw account IDs. Proves bag + canonical order end-to-end.
  uint8_t account[20];
  uint8_t dest[20];
  uint8_t pub[33];
  memset(account, 0x11, 20);
  memset(dest, 0x22, 20);
  memset(pub, 0xED, 33);

  xahau::PaymentParams pay;
  pay.hasDestinationId = true;
  memcpy(pay.destinationId, dest, 20);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1000000;
  pay.common.feeDrops = 12;
  pay.common.sequence = 1;
  pay.common.lastLedgerSequence = 100;

  xahau::TxFieldBag bag;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::fillPayment(pay, account, pub, bag));

  uint8_t bin[256];
  size_t len = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::xahauSerialize(bag, false, bin, sizeof(bin), len));
  TEST_ASSERT_TRUE(len > 40);
  // First field header TransactionType 0x12, then type value 0x0000
  TEST_ASSERT_EQUAL_HEX8(0x12, bin[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bin[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, bin[2]);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_field_header_compact);
  RUN_TEST(test_xrp_amount_encoding);
  RUN_TEST(test_vl_short);
  RUN_TEST(test_bag_sort_order_fee_before_sendmax);
  RUN_TEST(test_hex_roundtrip);
  RUN_TEST(test_rpc_account_info_build);
  RUN_TEST(test_minimal_payment_serialize_xrp);
  return UNITY_END();
}
