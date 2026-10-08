// transport.h — pluggable JSON-RPC transport (no sockets in core)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/xahau_error.h"

namespace xahau {

/**
 * Pluggable JSON-RPC transport.
 * - Host tests: MockXahauTransport
 * - ESP32: WsXahauTransport (XAHAU_FEATURE_NET)
 */
class IXahauTransport {
 public:
  virtual ~IXahauTransport() {}

  /**
   * Send one JSON request body; write full JSON response into resp (NUL-terminated).
   * @param timeoutMs  overall wait budget for connect+response
   */
  virtual Status request(const char* jsonReq, char* resp, size_t respCap,
                         uint32_t timeoutMs) = 0;

  /** Optional: keep-alive / drain (default no-op). */
  virtual void poll() {}

  /** Optional: disconnect / free resources. */
  virtual void close() {}
};

}  // namespace xahau
