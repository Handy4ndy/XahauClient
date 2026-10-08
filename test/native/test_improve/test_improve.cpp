// STNumber, account_objects parsers
#include <unity.h>
#include <string.h>
#include "XahauClient.h"
#include "xahau/core/hex.h"
#include "xahau/codec/serialize.h"
#include "xahau/codec/st_number.h"





void test_stnumber_million() {
  uint8_t out[12];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeStNumber("1000000", out));
  // mantissa 10^18 = 0x0DE0B6B3A7640000, exp -12 = 0xFFFFFFF4
  TEST_ASSERT_EQUAL_HEX8(0x0D, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0xE0, out[1]);
  TEST_ASSERT_EQUAL_HEX8(0xB6, out[2]);
  TEST_ASSERT_EQUAL_HEX8(0xB3, out[3]);
  TEST_ASSERT_EQUAL_HEX8(0xA7, out[4]);
  TEST_ASSERT_EQUAL_HEX8(0x64, out[5]);
  TEST_ASSERT_EQUAL_HEX8(0x00, out[6]);
  TEST_ASSERT_EQUAL_HEX8(0x00, out[7]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, out[8]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, out[9]);
  TEST_ASSERT_EQUAL_HEX8(0xFF, out[10]);
  TEST_ASSERT_EQUAL_HEX8(0xF4, out[11]);
}

void test_stnumber_zero_and_decimal() {
  uint8_t z[12];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeStNumber("0", z));
  // mantissa 0
  for (int i = 0; i < 8; ++i) TEST_ASSERT_EQUAL_HEX8(0, z[i]);
  // INT32_MIN exponent
  TEST_ASSERT_EQUAL_HEX8(0x80, z[8]);

  uint8_t d[12];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeStNumber("1.5", d));
  char dec[48];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeStNumber(d, dec, sizeof(dec)));
  TEST_ASSERT_TRUE(strlen(dec) > 0);
}

void test_rpc_parse_object_ids() {
  const char* json =
      "{\"result\":{\"account_objects\":["
      "{\"LedgerEntryType\":\"Escrow\",\"index\":"
      "\"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\"},"
      "{\"LedgerEntryType\":\"Check\",\"index\":"
      "\"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD\"}"
      "]}}";

  uint8_t ids[4][32];
  uint8_t n = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseObjectIds(json, "\"Escrow\"", ids, 4, n));
  TEST_ASSERT_EQUAL_UINT8(1, n);
  TEST_ASSERT_EQUAL_HEX8(0xCC, ids[0][0]);

  n = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::rpcParseObjectIds(json, "\"Check\"", ids, 4, n));
  TEST_ASSERT_EQUAL_UINT8(1, n);
  TEST_ASSERT_EQUAL_HEX8(0xDD, ids[0][0]);

  n = 0;
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::rpcParseObjectIds(json, nullptr, ids, 4, n));
  TEST_ASSERT_EQUAL_UINT8(2, n);
}



void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_stnumber_million);
  RUN_TEST(test_stnumber_zero_and_decimal);
  RUN_TEST(test_rpc_parse_object_ids);
  return UNITY_END();
}