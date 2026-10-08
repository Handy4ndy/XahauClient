// tx_field_bag.h - in-memory field store for canonical STObject serialization
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_types.h"
#include "../core/xahau_error.h"
#include "../tx/tx_type.h"

namespace xahau {

#ifndef XAHAU_TX_FIELD_BAG_MAX_SLOTS
#define XAHAU_TX_FIELD_BAG_MAX_SLOTS 32
#endif

#ifndef XAHAU_TX_FIELD_BAG_ARENA
// Tier A SetHook: CreateCode <= 8 KiB Hooks STArray fits in arena after common fields.
#define XAHAU_TX_FIELD_BAG_ARENA 12288
#endif

enum class FieldValueKind : uint8_t {
  None = 0,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  Hash128,   // 16 bytes
  Hash192,   // 24 bytes (e.g. MPTokenIssuanceID)
  Hash256,   // 32 bytes
  AccountID, // 20 bytes
  Blob,      // VL payload in arena
  /** Pre-encoded STArray/STObject/Issue/XChainBridge/Number body (no VL). */
  Embedded,
  AmountXrp,
  AmountIssued, // value/currency pointers + issuer 20B
  AmountMpt,    // value u64 + issuanceId 24B in arena
  /** Issue (STI type 24): 20-byte currency (XRP) or 40-byte currency+issuer. */
  Issue,
};

struct FieldSlot {
  uint32_t fieldCode;
  FieldValueKind kind;
  union {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    uint64_t xrpDrops;
    struct {
      const uint8_t* data;
      uint16_t len;
    } blob;
    struct {
      const char* value;
      const char* currency;
      const uint8_t* issuer;  // 20 bytes
    } issued;
    struct {
      uint64_t value;
      const uint8_t* issuanceId;  // 24 bytes
    } mpt;
  } v;
};

/**
 * Fixed-capacity bag. Nested STArray/STObject payloads can be stored as
 * pre-encoded Blobs until a structured array API is added.
 */
class TxFieldBag {
 public:
  TxFieldBag();

  void clear();

  Status setUInt8(uint32_t code, uint8_t val);
  Status setUInt16(uint32_t code, uint16_t val);
  Status setUInt32(uint32_t code, uint32_t val);
  Status setUInt64(uint32_t code, uint64_t val);
  Status setHash256(uint32_t code, const uint8_t hash[32]);
  Status setHash192(uint32_t code, const uint8_t hash[24]);
  Status setHash128(uint32_t code, const uint8_t hash[16]);
  Status setAccountID(uint32_t code, const uint8_t id[20]);
  Status setBlob(uint32_t code, const uint8_t* data, uint16_t len);
  Status setEmbedded(uint32_t code, const uint8_t* data, uint16_t len);
  /** XRP issue (20 zero currency) or IOU (currency20 + issuer20). */
  Status setIssueXrp(uint32_t code);
  Status setIssueIou(uint32_t code, const char* currency, const uint8_t issuer[20]);
  /**
   * MPT issue (44 bytes): issuerAccount(20) || 0x00..01(20) || sequence LE(4).
   * @param mptIssuanceId 24-byte id = Sequence(4 BE) || Issuer AccountID(20)
   */
  Status setIssueMpt(uint32_t code, const uint8_t mptIssuanceId[24]);
  Status setAmountXrp(uint32_t code, uint64_t drops);
  Status setAmountIssued(uint32_t code, const char* value, const char* currency,
                         const uint8_t issuer[20]);
  /** MPT amount: integer value + 24-byte MPTokenIssuanceID. */
  Status setAmountMpt(uint32_t code, uint64_t value, const uint8_t issuanceId[24]);
  /** Xahau STNumber (12-byte) from decimal string. */
  Status setNumber(uint32_t code, const char* decimal);

  size_t count() const { return count_; }
  const FieldSlot& slot(size_t i) const { return slots_[i]; }

  /** Write sorted indices (by fieldCode ascending) into outIdx. Returns n. */
  size_t sortedIndices(size_t* outIdx, size_t maxIdx) const;

 private:
  Status upsert(const FieldSlot& s);
  Status storeBytes(const uint8_t* data, uint16_t len, const uint8_t*& outPtr);

  FieldSlot slots_[XAHAU_TX_FIELD_BAG_MAX_SLOTS];
  size_t count_;
  uint8_t arena_[XAHAU_TX_FIELD_BAG_ARENA];
  size_t arenaUsed_;
};

/** Apply common envelope fields + TransactionType + Account + SigningPubKey. */
Status bagSetCommon(TxFieldBag& bag, TxType type, const TxCommon& common,
                    const uint8_t accountId[20], const uint8_t pubKey[33],
                    const uint8_t* signature, uint16_t sigLen, bool includeSig);

/**
 * Shared bag for buildAndSign* helpers.
 * Lives in BSS (not the ESP32 loopTask stack). Not re-entrant - one sign at a time.
 * TxFieldBag is multi-KiB (arena + slots); allocating it on setup()/loop stack
 * triggers "Stack canary watchpoint" on ESP32.
 */
inline TxFieldBag& scratchSignBag() {
  static TxFieldBag bag;
  bag.clear();
  return bag;
}

}  // namespace xahau
