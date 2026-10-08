// xahau_error.h - library status codes
#pragma once

#include <stdint.h>

namespace xahau {

enum class Status : int8_t {
  Ok = 0,
  InvalidArgument = -1,
  BufferTooSmall = -2,
  Unsupported = -3,
  NotImplemented = -4,
  EncodeFailed = -5,
  DecodeFailed = -6,
  CryptoFailed = -7,
  InvalidSeed = -8,
  InvalidAddress = -9,
  MissingField = -10,
  InvalidField = -11,
  TransportFailed = -12,
  Timeout = -13,
  Rejected = -14,
  /**
   * Submit accepted into open-ledger queue (engine_result terQUEUED).
   * Not a hard failure - do NOT re-sign with a new Sequence; resolve by hash.
   */
  Queued = -15,
};

inline const char* statusName(Status s) {
  switch (s) {
    case Status::Ok: return "Ok";
    case Status::InvalidArgument: return "InvalidArgument";
    case Status::BufferTooSmall: return "BufferTooSmall";
    case Status::Unsupported: return "Unsupported";
    case Status::NotImplemented: return "NotImplemented";
    case Status::EncodeFailed: return "EncodeFailed";
    case Status::DecodeFailed: return "DecodeFailed";
    case Status::CryptoFailed: return "CryptoFailed";
    case Status::InvalidSeed: return "InvalidSeed";
    case Status::InvalidAddress: return "InvalidAddress";
    case Status::MissingField: return "MissingField";
    case Status::InvalidField: return "InvalidField";
    case Status::TransportFailed: return "TransportFailed";
    case Status::Timeout: return "Timeout";
    case Status::Rejected: return "Rejected";
    case Status::Queued: return "Queued";
    default: return "Unknown";
  }
}

}  // namespace xahau
