// ws_transport.cpp — ESP32 WebSocket transport (persistent + JSON-RPC id match)
//
// IMPORTANT: Arduino IDE compiles this file with library flags only (not sketch
// #defines). XAHAU_FEATURE_NET defaults to 1 on Arduino so this TU is not empty.
#include "ws_transport.h"

#if XAHAU_FEATURE_NET && (defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO))

#include <stdio.h>
#include <string.h>

namespace xahau {

WsXahauTransport* WsXahauTransport::s_active_ = nullptr;

WsXahauTransport::WsXahauTransport()
    : host_(kNetworkTestnet.wsHost),
      port_(kNetworkTestnet.wsPort),
      path_(kNetworkTestnet.wsPath),
      useSsl_(kNetworkTestnet.wsUseSsl),
      networkId_(NetworkId::Testnet),
      sslFingerprint_(nullptr),
      sslCaCert_(nullptr),
      requireSslPin_(false),
      insecureSsl_(true),
      keepAlive_(true),
      respBuf_(nullptr),
      respCap_(0),
      connected_(false),
      responseReady_(false),
      nextId_(1),
      expectedId_(0),
      streamHandler_(nullptr),
      streamUser_(nullptr),
      yieldHandler_(nullptr),
      yieldUser_(nullptr) {}

WsXahauTransport::WsXahauTransport(const NetworkEndpoint& ep)
    : host_(ep.wsHost),
      port_(ep.wsPort),
      path_(ep.wsPath ? ep.wsPath : "/"),
      useSsl_(ep.wsUseSsl),
      networkId_(ep.id),
      sslFingerprint_(nullptr),
      sslCaCert_(nullptr),
      requireSslPin_(false),
      insecureSsl_(true),
      keepAlive_(true),
      respBuf_(nullptr),
      respCap_(0),
      connected_(false),
      responseReady_(false),
      nextId_(1),
      expectedId_(0),
      streamHandler_(nullptr),
      streamUser_(nullptr),
      yieldHandler_(nullptr),
      yieldUser_(nullptr) {}

WsXahauTransport::WsXahauTransport(const char* host, uint16_t port, const char* path)
    : host_(host),
      port_(port),
      path_(path ? path : "/"),
      useSsl_(true),
      networkId_(NetworkId::Custom),
      sslFingerprint_(nullptr),
      sslCaCert_(nullptr),
      requireSslPin_(false),
      insecureSsl_(true),
      keepAlive_(true),
      respBuf_(nullptr),
      respCap_(0),
      connected_(false),
      responseReady_(false),
      nextId_(1),
      expectedId_(0),
      streamHandler_(nullptr),
      streamUser_(nullptr),
      yieldHandler_(nullptr),
      yieldUser_(nullptr) {}

void WsXahauTransport::setEndpoint(const NetworkEndpoint& ep) {
  // Endpoint change requires reconnect.
  if (connected_ &&
      (host_ != ep.wsHost || port_ != ep.wsPort || networkId_ != ep.id)) {
    close();
  }
  host_ = ep.wsHost;
  port_ = ep.wsPort;
  path_ = ep.wsPath ? ep.wsPath : "/";
  useSsl_ = ep.wsUseSsl;
  networkId_ = ep.id;
}

void WsXahauTransport::setHost(const char* host, uint16_t port, const char* path) {
  const char* p = path ? path : "/";
  // Idempotent: do not tear down a live session when host/port/path are unchanged
  // (wallet re-applies config every request / ledger stream ensure).
  if (connected_ && host_ && host && strcmp(host_, host) == 0 && port_ == port &&
      path_ && strcmp(path_, p) == 0) {
    return;
  }
  if (connected_) close();
  host_ = host;
  port_ = port;
  path_ = p;
  networkId_ = NetworkId::Custom;
}

Status WsXahauTransport::beginTlsSession() {
  if (!useSsl_) {
    ws_.begin(host_, port_, path_);
    return Status::Ok;
  }

  const bool hasPin =
      (sslFingerprint_ && sslFingerprint_[0]) || (sslCaCert_ && sslCaCert_[0]);
  if (requireSslPin_ && !hasPin) {
    return Status::InvalidArgument;  // fail closed
  }
  if (!hasPin && !insecureSsl_) {
    return Status::InvalidArgument;
  }

  if (sslCaCert_ && sslCaCert_[0]) {
    ws_.beginSslWithCA(host_, port_, path_, sslCaCert_);
  } else if (sslFingerprint_ && sslFingerprint_[0]) {
    ws_.beginSSL(host_, port_, path_, sslFingerprint_);
  } else {
    ws_.beginSSL(host_, port_, path_);
  }
  return Status::Ok;
}

void WsXahauTransport::onEventStatic(WStype_t type, uint8_t* payload, size_t length) {
  if (s_active_) s_active_->onEvent(type, payload, length);
}

bool WsXahauTransport::parseJsonId(const char* json, uint32_t& outId) {
  outId = 0;
  if (!json) return false;
  // Prefer "id": <number> (our injected ids are numeric).
  const char* p = strstr(json, "\"id\"");
  if (!p) return false;
  p = strchr(p + 4, ':');
  if (!p) return false;
  ++p;
  while (*p == ' ' || *p == '\t' || *p == '"') ++p;
  if (*p < '0' || *p > '9') return false;
  unsigned long v = strtoul(p, nullptr, 10);
  outId = static_cast<uint32_t>(v);
  return true;
}

void WsXahauTransport::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      connected_ = true;
      break;
    case WStype_DISCONNECTED:
      connected_ = false;
      break;
    case WStype_TEXT:
      if (!payload || length == 0) break;
      // Waiting for a correlated RPC response?
            if (expectedId_ != 0 && respBuf_ && respCap_ > 0) {
        size_t n = length;
        if (n + 1 > respCap_) {
          // Truncate oversized RPC into respBuf_ (engine_result/hash are near the
          // start of submit responses). Previously we dropped the frame, which
          // left request() spinning until Status::Timeout with empty hash —
          // the SetHook BIP serial reliability failure mode on ESP32.
          size_t copy = respCap_ - 1;
          memcpy(respBuf_, payload, copy);
          respBuf_[copy] = '\0';
          uint32_t rid = 0;
          if (parseJsonId(respBuf_, rid) && rid == expectedId_) {
            responseReady_ = true;
            break;
          }
          // Uncorrelated oversized — may still be a stream push.
          if (streamHandler_) {
            size_t sn = copy;
            if (sn >= sizeof(streamScratch_)) sn = sizeof(streamScratch_) - 1;
            memcpy(streamScratch_, respBuf_, sn);
            streamScratch_[sn] = '\0';
            streamHandler_(streamScratch_, sn, streamUser_);
          }
          break;
        }
        memcpy(respBuf_, payload, n);
        respBuf_[n] = '\0';
        uint32_t rid = 0;
        if (parseJsonId(respBuf_, rid) && rid == expectedId_) {
          responseReady_ = true;
          break;
        }
        // Unmatched while waiting (ledgerClosed etc.) → stream handler
        if (streamHandler_) {
          size_t sn = n;
          if (sn >= sizeof(streamScratch_)) sn = sizeof(streamScratch_) - 1;
          memcpy(streamScratch_, respBuf_, sn);
          streamScratch_[sn] = '\0';
          streamHandler_(streamScratch_, sn, streamUser_);
        }
        break;
      }
      // Not in request(): deliver stream / unsolicited frames
      if (streamHandler_) {
        size_t sn = length;
        if (sn >= sizeof(streamScratch_)) sn = sizeof(streamScratch_) - 1;
        memcpy(streamScratch_, payload, sn);
        streamScratch_[sn] = '\0';
        streamHandler_(streamScratch_, sn, streamUser_);
      }
      break;
    default:
      break;
  }
}

void WsXahauTransport::poll() { ws_.loop(); }

void WsXahauTransport::yieldHost() {
  if (yieldHandler_) yieldHandler_(yieldUser_);
}

void WsXahauTransport::close() {
  ws_.disconnect();
  connected_ = false;
  expectedId_ = 0;
  if (s_active_ == this) s_active_ = nullptr;
}

Status WsXahauTransport::ensureConnected(uint32_t timeoutMs) {
  if (connected_) return Status::Ok;

  s_active_ = this;
  ws_.onEvent(&WsXahauTransport::onEventStatic);

  Status bst = beginTlsSession();
  if (bst != Status::Ok) {
    if (s_active_ == this) s_active_ = nullptr;
    return bst;
  }
  // Keepalive reconnect interval; 0 = no auto-reconnect storms.
  ws_.setReconnectInterval(keepAlive_ ? 5000 : 0);

  uint32_t start = millis();
  while (!connected_ && (millis() - start) < timeoutMs) {
    ws_.loop();
    yieldHost();
    delay(5);
  }
  if (!connected_) {
    close();
    return Status::Timeout;
  }
  return Status::Ok;
}

Status WsXahauTransport::request(const char* jsonReq, char* resp, size_t respCap,
                                uint32_t timeoutMs) {
  if (!jsonReq || !resp || respCap < 2 || !host_) return Status::InvalidArgument;

  // Inject numeric id for response correlation: {"id":N, ...}
  // Use member BSS buffer wiredReq_ — stack char wired[3200] + TLS blew loopTask canary
  // (ClientTest panics on server_info / account_lines / ledger).
  const uint32_t reqId = nextId_++;
  if (nextId_ == 0) nextId_ = 1;
  if (jsonReq[0] != '{') return Status::InvalidArgument;
  // Skip leading '{' and re-wrap with id.
  int nw = snprintf(wiredReq_, sizeof(wiredReq_), "{\"id\":%lu,%s",
                    static_cast<unsigned long>(reqId), jsonReq + 1);
  if (nw <= 0 || static_cast<size_t>(nw) >= sizeof(wiredReq_)) return Status::BufferTooSmall;

  respBuf_ = resp;
  respCap_ = respCap;
  responseReady_ = false;
  expectedId_ = reqId;
  resp[0] = '\0';

  Status cst = ensureConnected(timeoutMs);
  if (cst != Status::Ok) {
    respBuf_ = nullptr;
    respCap_ = 0;
    expectedId_ = 0;
    return cst;
  }

  if (!ws_.sendTXT(wiredReq_)) {
    // Connection may be half-dead — force reconnect once.
    close();
    cst = ensureConnected(timeoutMs);
    if (cst != Status::Ok) {
      respBuf_ = nullptr;
      respCap_ = 0;
      expectedId_ = 0;
      return cst;
    }
    if (!ws_.sendTXT(wiredReq_)) {
      respBuf_ = nullptr;
      respCap_ = 0;
      expectedId_ = 0;
      if (!keepAlive_) close();
      return Status::TransportFailed;
    }
  }

  uint32_t start = millis();
  while (!responseReady_ && (millis() - start) < timeoutMs) {
    ws_.loop();
    // Let host service portal HTTP so /api/tx/status stays responsive mid-RPC.
    yieldHost();
    delay(5);
  }

  Status st = responseReady_ ? Status::Ok : Status::Timeout;
  if (st == Status::Timeout) {
    // Drop half-open sessions so the next call reconnects cleanly.
    close();
  } else if (!keepAlive_) {
    close();
  }

  respBuf_ = nullptr;
  respCap_ = 0;
  expectedId_ = 0;
  return st;
}

}  // namespace xahau

#endif
