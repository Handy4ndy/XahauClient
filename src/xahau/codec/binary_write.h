// binary_write.h — low-level Xahau binary field writers
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

Status writeByte(uint8_t* buf, size_t& pos, size_t max, uint8_t v);
Status writeBytes(uint8_t* buf, size_t& pos, size_t max, const uint8_t* data, size_t len);
Status writeUInt16BE(uint8_t* buf, size_t& pos, size_t max, uint16_t v);
Status writeUInt32BE(uint8_t* buf, size_t& pos, size_t max, uint32_t v);
Status writeUInt64BE(uint8_t* buf, size_t& pos, size_t max, uint64_t v);

/** Xahau field header from STI type + field nth. */
Status writeFieldHeader(uint8_t* buf, size_t& pos, size_t max, uint16_t type, uint16_t nth);

/** Xahau variable-length length prefix. */
Status writeVLLength(uint8_t* buf, size_t& pos, size_t max, size_t len);

/** Native XRP amount (8 bytes). */
Status writeXrpAmount(uint8_t* buf, size_t& pos, size_t max, uint64_t drops);

/**
 * Issued amount (48 bytes): 8B number + 20B currency + 20B issuer.
 * currency: 3-char ASCII or 40-char hex (160-bit).
 */
Status writeIssuedAmount(uint8_t* buf, size_t& pos, size_t max, const char* valueStr,
                         const char* currency, const uint8_t issuer[20]);

/**
 * MPT amount (33 bytes, no VL): 0x60 | value u64 BE | issuanceId 24B.
 * @see https://Xahau.org/docs/references/protocol/binary-format#amount-fields
 */
Status writeMptAmount(uint8_t* buf, size_t& pos, size_t max, uint64_t value,
                      const uint8_t issuanceId[24]);

/** Encode fieldCode (type<<16|nth) header. */
Status writeFieldHeaderCode(uint8_t* buf, size_t& pos, size_t max, uint32_t code);

}  // namespace xahau
