// serialize.h - bag -> canonical STObject bytes
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"
#include "tx_field_bag.h"

namespace xahau {

/**
 * Serialize present bag fields in ascending fieldCode order.
 * When includeSigningFields is false, omits TxnSignature (fc::TxnSignature).
 *
 * @return Status::Ok and sets outLen; on failure outLen is undefined.
 */
Status xahauSerialize(const TxFieldBag& bag, bool includeSigningFields, uint8_t* outBuf,
                     size_t outCap, size_t& outLen);

}  // namespace xahau
