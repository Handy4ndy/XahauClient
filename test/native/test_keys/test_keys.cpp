// Host tests: dual-curve seed derivation + seed encode/generate from entropy
#include <unity.h>
#include <string.h>
#include "XahauClient.h"
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


// Public demo fixtures + fixed-entropy vectors (match ClientTest / ripple-keypairs).
static const uint8_t* kDemoEdEntropy = XAHAU_TEST_ENTROPY_ED1;
static const uint8_t* kDemoSecpEntropy = XAHAU_TEST_ENTROPY_SECP1;
static const uint8_t kFixedEntropy[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

static void hexEq(const char* expectedHex, const uint8_t* bytes, size_t len) {
  char got[128];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::bytesToHex(bytes, len, got, sizeof(got)));
  TEST_ASSERT_EQUAL_STRING_LEN(expectedHex, got, strlen(expectedHex));
}

void test_derive_secp256k1() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_SECP1, keys));
  TEST_ASSERT_EQUAL(xahau::Curve::Secp256k1, keys.curve);
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_ADDR_SECP1, keys.address);
  hexEq(XAHAU_TEST_PUB_SECP1, keys.publicKey, 33);
  hexEq(XAHAU_TEST_PRIV_SECP1, keys.privateKey, 32);
  xahau::secureClear(keys);
}

void test_derive_ed25519() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::deriveKeysFromSeed(XAHAU_TEST_SEED_ED1, keys));
  TEST_ASSERT_EQUAL(xahau::Curve::Ed25519, keys.curve);
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_ADDR_ED1, keys.address);
  hexEq(XAHAU_TEST_PUB_ED1, keys.publicKey, 33);
  hexEq(XAHAU_TEST_PRIV_ED1, keys.privateKey, 32);
  xahau::secureClear(keys);
}

void test_address_roundtrip() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  uint8_t id[20];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::decodeClassicAddress(XAHAU_TEST_ADDR_SECP1, id));
  char addr[36];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeClassicAddress(id, addr));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_ADDR_SECP1, addr);
}

void test_encode_demo_seeds_from_entropy() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  char seed[xahau::kFamilySeedBufSize];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::encodeEd25519Seed(kDemoEdEntropy, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_ED1, seed);

  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::encodeSecp256k1Seed(kDemoSecpEntropy, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_SECP1, seed);
}

void test_seed_from_entropy_fixed_vectors() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  // Goldens from ClientTest: ripple-keypairs generateSeed / Wallet.fromEntropy
  char seed[xahau::kFamilySeedBufSize];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::seedFromEntropy(kFixedEntropy, xahau::Curve::Ed25519, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_ED2, seed);

  TEST_ASSERT_EQUAL(
      xahau::Status::Ok,
      xahau::seedFromEntropy(kFixedEntropy, xahau::Curve::Secp256k1, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_SECP2, seed);
}

void test_generate_keypair_from_entropy_ed25519() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  char seed[xahau::kFamilySeedBufSize];
  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::generateKeyPairFromEntropy(kFixedEntropy, xahau::Curve::Ed25519, seed,
                                                     sizeof(seed), keys));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_ED2, seed);
  TEST_ASSERT_EQUAL(xahau::Curve::Ed25519, keys.curve);
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_ADDR_ED2, keys.address);

  // Round-trip: decode seed -> same entropy
  uint8_t got[16];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeEd25519Seed(seed, got));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(kFixedEntropy, got, 16);
  xahau::secureClear(keys);
}

void test_generate_keypair_from_entropy_secp() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  char seed[xahau::kFamilySeedBufSize];
  xahau::KeyPair keys;
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::generateKeyPairFromEntropy(kFixedEntropy, xahau::Curve::Secp256k1, seed,
                                                     sizeof(seed), keys));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_SECP2, seed);
  TEST_ASSERT_EQUAL(xahau::Curve::Secp256k1, keys.curve);
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_ADDR_SECP2, keys.address);

  uint8_t got[16];
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::decodeSecp256k1Seed(seed, got));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(kFixedEntropy, got, 16);
  xahau::secureClear(keys);
}

void test_seed_roundtrip_decode_encode() {
  if (!XAHAU_HAVE_TEST_SEEDS) {
    TEST_IGNORE_MESSAGE("test_seeds.local.h missing - copy test_seeds.example.h and fill fixture seeds");
  }

  uint8_t entropy[16];
  char seed[xahau::kFamilySeedBufSize];
  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::decodeEd25519Seed(XAHAU_TEST_SEED_ED1, entropy));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeEd25519Seed(entropy, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_ED1, seed);

  TEST_ASSERT_EQUAL(xahau::Status::Ok,
                    xahau::decodeSecp256k1Seed(XAHAU_TEST_SEED_SECP1, entropy));
  TEST_ASSERT_EQUAL(xahau::Status::Ok, xahau::encodeSecp256k1Seed(entropy, seed, sizeof(seed)));
  TEST_ASSERT_EQUAL_STRING(XAHAU_TEST_SEED_SECP1, seed);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_derive_secp256k1);
  RUN_TEST(test_derive_ed25519);
  RUN_TEST(test_address_roundtrip);
  RUN_TEST(test_encode_demo_seeds_from_entropy);
  RUN_TEST(test_seed_from_entropy_fixed_vectors);
  RUN_TEST(test_generate_keypair_from_entropy_ed25519);
  RUN_TEST(test_generate_keypair_from_entropy_secp);
  RUN_TEST(test_seed_roundtrip_decode_encode);
  return UNITY_END();
}
