// ws_transport.h - ESP32 WebSocket JSON-RPC transport (optional)
#pragma once

#include "../platform/xahau_platform.h"

#if XAHAU_FEATURE_NET

#include <stddef.h>
#include <stdint.h>

#include "../rpc/transport.h"
#include "networks.h"

// Arduino / ESP32 only
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)

#include <WebSocketsClient.h>

namespace xahau {

/**
 * Callback for unsolicited WS TEXT frames (subscribe streams).
 * json is NUL-terminated; valid only during the call (scratch buffer).
 * Must not call request() re-entrantly from the handler.
 */
using WsStreamHandler = void (*)(const char* json, size_t len, void* user);

/**
 * Optional yield during blocking connect/request waits.
 * Use to service HTTP (portal) so UI can poll status while RPC is in flight.
 * Must not call request() re-entrantly from the handler.
 */
using WsYieldHandler = void (*)(void* user);

/**
 * JSON-RPC over WebSocketsClient (wss).
 *
 * Connection policy:
 * - Reuses one TLS/WS session across request() calls.
 * - Injects a monotonic numeric "id" into each request and matches the response
 *   "id" so interleaved/server chatter is ignored.
 * - Unmatched TEXT frames are delivered to the stream handler (subscribe pushes).
 * - close() disconnects; next request() reconnects.
 *
 * TLS (see docs/SECURITY.md + applyDefaultTlsPolicy):
 * - Default: SSL on; optional fingerprint / CA PEM.
 * - requireSslPin_ true without pin/CA -> fail closed.
 *
 * Depends on: WebSockets library, WiFi already connected.
 */
class WsXahauTransport : public IXahauTransport {
 public:
  /** Default: public Xahau Testnet (xahau-test.net:443). */
  WsXahauTransport();

  explicit WsXahauTransport(const NetworkEndpoint& ep);

  WsXahauTransport(const char* host, uint16_t port, const char* path = "/");

  void setEndpoint(const NetworkEndpoint& ep);
  void setHost(const char* host, uint16_t port, const char* path = "/");
  void setUseSsl(bool ssl) { useSsl_ = ssl; }

  /**
   * Receive ledger/transaction/etc. stream pushes (non-matching RPC replies).
   * Pass nullptr to clear. Keep handlers short (no nested request()).
   */
  void setStreamHandler(WsStreamHandler handler, void* user = nullptr) {
    streamHandler_ = handler;
    streamUser_ = user;
  }
  WsStreamHandler streamHandler() const { return streamHandler_; }

  /** Yield hook for host apps (HTTP pump) during long RPC waits. */
  void setYieldHandler(WsYieldHandler handler, void* user = nullptr) {
    yieldHandler_ = handler;
    yieldUser_ = user;
  }

  /** Connect (or ensure connected) without sending a request. */
  Status connect(uint32_t timeoutMs = 20000) { return ensureConnected(timeoutMs); }

  /**
   * Optional TLS server cert SHA1 fingerprint (WebSockets format).
   * Pass nullptr to clear.
   */
  void setSslFingerprint(const char* sha1Fingerprint) { sslFingerprint_ = sha1Fingerprint; }

  /**
   * Optional PEM CA certificate for beginSslWithCA. Pass nullptr to clear.
   * When set, preferred over fingerprint-only beginSSL.
   */
  void setSslCaCert(const char* caPem) { sslCaCert_ = caPem; }

  /**
   * When true: refuse SSL connect if neither fingerprint nor CA is configured.
   * Fail closed for production Mainnet sketches (applyDefaultTlsPolicy sets this).
   */
  void setRequireSslPin(bool require) { requireSslPin_ = require; }

  /**
   * When true: allow SSL without pin (public Testnet convenience).
   * Ignored when requireSslPin_ is true.
   */
  void setInsecureSsl(bool allow) { insecureSsl_ = allow; }

  /** When false, each request() disconnects after the response (legacy). Default true. */
  void setKeepAlive(bool keep) { keepAlive_ = keep; }
  bool keepAlive() const { return keepAlive_; }

  const char* host() const { return host_; }
  uint16_t port() const { return port_; }
  NetworkId networkId() const { return networkId_; }
  bool useSsl() const { return useSsl_; }
  const char* sslFingerprint() const { return sslFingerprint_; }
  bool requireSslPin() const { return requireSslPin_; }
  bool isConnected() const { return connected_; }

  Status request(const char* jsonReq, char* resp, size_t respCap, uint32_t timeoutMs) override;
  void poll() override;
  void close() override;

 private:
  Status ensureConnected(uint32_t timeoutMs);
  Status beginTlsSession();
  static void onEventStatic(WStype_t type, uint8_t* payload, size_t length);
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  static bool parseJsonId(const char* json, uint32_t& outId);

  WebSocketsClient ws_;
  const char* host_;
  uint16_t port_;
  const char* path_;
  bool useSsl_;
  NetworkId networkId_;

  const char* sslFingerprint_;
  const char* sslCaCert_;
  bool requireSslPin_;
  bool insecureSsl_;
  bool keepAlive_;

  char* respBuf_;
  size_t respCap_;
  volatile bool connected_;
  volatile bool responseReady_;
  uint32_t nextId_;
  uint32_t expectedId_;
  // BSS request wire buffer - never put multi-KB JSON on loopTask stack (stack canary panics).
  // Tier A SetHook: submit JSON with ~8 KiB CreateCode hex blob (~17 KiB).
  char wiredReq_[17408];
  // Scratch for stream pushes (NUL-terminated for handlers).
  char streamScratch_[4096];
  WsStreamHandler streamHandler_;
  void* streamUser_;
  WsYieldHandler yieldHandler_;
  void* yieldUser_;
  void yieldHost();
  static WsXahauTransport* s_active_;
};

}  // namespace xahau

#endif  // ARDUINO
#endif  // XAHAU_FEATURE_NET
