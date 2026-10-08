// test_networks - Xahau network presets + NetworkID rules
#include <unity.h>

#include "xahau/net/networks.h"
#include "xahau/tx/tx_type.h"



using namespace xahau;

void test_presets_wired() {
  TEST_ASSERT_TRUE(allPublicNetworksWired());
  TEST_ASSERT_EQUAL_UINT32(21337u, kNetworkMainnet.protocolNetworkId);
  TEST_ASSERT_EQUAL_UINT32(21338u, kNetworkTestnet.protocolNetworkId);
  TEST_ASSERT_EQUAL_STRING("xahau.network", kNetworkMainnet.wsHost);
  TEST_ASSERT_EQUAL_STRING("xahau-test.net", kNetworkTestnet.wsHost);
}

void test_oracle_live_on_mainnet() {
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::OracleSet, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::OracleDelete, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::Invoke, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::SetHook, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::URITokenMint, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::Remit, NetworkId::Mainnet));
  TEST_ASSERT_TRUE(isTxLiveEnabledOn(TxType::ClaimReward, NetworkId::Mainnet));
}

void test_oracle_type_codes() {
  TEST_ASSERT_EQUAL_UINT16(60, static_cast<uint16_t>(TxType::OracleSet));
  TEST_ASSERT_EQUAL_UINT16(61, static_cast<uint16_t>(TxType::OracleDelete));
  TEST_ASSERT_EQUAL_UINT16(99, static_cast<uint16_t>(TxType::Invoke));
  TEST_ASSERT_EQUAL_UINT16(22, static_cast<uint16_t>(TxType::SetHook));
  TEST_ASSERT_EQUAL_UINT16(45, static_cast<uint16_t>(TxType::URITokenMint));
  const TxTypeInfo* info = txTypeLookupByName("OracleSet");
  TEST_ASSERT_NOT_NULL(info);
  TEST_ASSERT_EQUAL_UINT16(60, info->typeCode);
}

void test_encode_only_not_live() {
  TEST_ASSERT_FALSE(isTxLiveEnabledOn(TxType::VaultCreate, NetworkId::Mainnet));
  TEST_ASSERT_FALSE(isTxLiveEnabledOn(TxType::LoanSet, NetworkId::Mainnet));
  TEST_ASSERT_FALSE(isTxLiveEnabledOn(TxType::Batch, NetworkId::Mainnet));
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_presets_wired);
  RUN_TEST(test_oracle_live_on_mainnet);
  RUN_TEST(test_oracle_type_codes);
  RUN_TEST(test_encode_only_not_live);
  return UNITY_END();
}
