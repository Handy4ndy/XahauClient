// mock_transport.h - scripted IXahauTransport for host tests
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "transport.h"

namespace xahau {

#ifndef XAHAU_MOCK_MAX_SCRIPTS
#define XAHAU_MOCK_MAX_SCRIPTS 16
#endif

#ifndef XAHAU_MOCK_RESP_MAX
#define XAHAU_MOCK_RESP_MAX 2048
#endif

/**
 * Matches requests by substring (e.g. "account_info") and returns a canned JSON body.
 * Call order is FIFO among scripts that match; unmatched -> TransportFailed.
 */
class MockXahauTransport : public IXahauTransport {
 public:
  MockXahauTransport() : count_(0), cursor_(0), lastReqLen_(0) { lastReq_[0] = '\0'; }

  void reset() {
    count_ = 0;
    cursor_ = 0;
    lastReq_[0] = '\0';
    lastReqLen_ = 0;
  }

  /** Add a scripted response. matchSubstr found in request selects this entry (in order). */
  bool push(const char* matchSubstr, const char* responseJson) {
    if (count_ >= XAHAU_MOCK_MAX_SCRIPTS || !matchSubstr || !responseJson) return false;
    size_t ml = strlen(matchSubstr);
    size_t rl = strlen(responseJson);
    if (ml >= sizeof(scripts_[0].match) || rl >= XAHAU_MOCK_RESP_MAX) return false;
    strcpy(scripts_[count_].match, matchSubstr);
    strcpy(scripts_[count_].resp, responseJson);
    scripts_[count_].used = false;
    scripts_[count_].isTimeout = false;
    ++count_;
    return true;
  }

  /**
   * Script a Timeout for the next matching request (e.g. first "submit" times out,
   * second "submit" succeeds). Used to test same-blob retry paths.
   */
  bool pushTimeout(const char* matchSubstr) {
    if (count_ >= XAHAU_MOCK_MAX_SCRIPTS || !matchSubstr) return false;
    size_t ml = strlen(matchSubstr);
    if (ml >= sizeof(scripts_[0].match)) return false;
    strcpy(scripts_[count_].match, matchSubstr);
    scripts_[count_].resp[0] = '\0';
    scripts_[count_].used = false;
    scripts_[count_].isTimeout = true;
    ++count_;
    return true;
  }

  const char* lastRequest() const { return lastReq_; }
  size_t scriptCount() const { return count_; }

  Status request(const char* jsonReq, char* resp, size_t respCap, uint32_t /*timeoutMs*/) override {
    if (!jsonReq || !resp || respCap == 0) return Status::InvalidArgument;
    size_t n = strlen(jsonReq);
    if (n >= sizeof(lastReq_)) n = sizeof(lastReq_) - 1;
    memcpy(lastReq_, jsonReq, n);
    lastReq_[n] = '\0';
    lastReqLen_ = n;

    for (size_t i = 0; i < count_; ++i) {
      if (scripts_[i].used) continue;
      if (strstr(jsonReq, scripts_[i].match)) {
        scripts_[i].used = true;
        if (scripts_[i].isTimeout) {
          resp[0] = '\0';
          return Status::Timeout;
        }
        size_t rl = strlen(scripts_[i].resp);
        if (rl + 1 > respCap) return Status::BufferTooSmall;
        memcpy(resp, scripts_[i].resp, rl + 1);
        return Status::Ok;
      }
    }
    return Status::TransportFailed;
  }

 private:
  struct Script {
    char match[64];
    char resp[XAHAU_MOCK_RESP_MAX];
    bool used;
    bool isTimeout;
  };
  Script scripts_[XAHAU_MOCK_MAX_SCRIPTS];
  size_t count_;
  size_t cursor_;
  char lastReq_[2200];
  size_t lastReqLen_;
};

}  // namespace xahau
