// xahau_platform.h - host vs Arduino portability
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
  #ifndef XAHAU_PLATFORM_ARDUINO
    #define XAHAU_PLATFORM_ARDUINO 1
  #endif
  #include <Arduino.h>
#else
  #ifndef XAHAU_PLATFORM_HOST
    #define XAHAU_PLATFORM_HOST 1
  #endif
  // Host: no Arduino.h. Provide minimal stand-ins used by optional debug only.
  #ifndef PROGMEM
    #define PROGMEM
  #endif
  #ifndef pgm_read_byte
    #define pgm_read_byte(a) (*(const uint8_t*)(a))
  #endif
  #ifndef pgm_read_word
    #define pgm_read_word(a) (*(const uint16_t*)(a))
  #endif
  #ifndef pgm_read_dword
    #define pgm_read_dword(a) (*(const uint32_t*)(a))
  #endif
#endif

// Feature defaults
#ifndef XAHAU_FEATURE_ED25519
  #define XAHAU_FEATURE_ED25519 1
#endif
#ifndef XAHAU_FEATURE_SECP256K1
  #define XAHAU_FEATURE_SECP256K1 1
#endif
// Arduino IDE compiles library .cpp files WITHOUT sketch #defines. If NET defaults
// to 0, ws_transport.cpp is empty and sketches get "undefined reference to
// WsXahauTransport". Default ON for Arduino; host/native tests set it to 0.
#ifndef XAHAU_FEATURE_NET
  #if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
    #define XAHAU_FEATURE_NET 1
  #else
    #define XAHAU_FEATURE_NET 0
  #endif
#endif
#ifndef XAHAU_FEATURE_DEBUG
  #define XAHAU_FEATURE_DEBUG 0
#endif

#if XAHAU_FEATURE_DEBUG
  #if XAHAU_PLATFORM_ARDUINO
    #define XAHAU_LOGF(...) Serial.printf(__VA_ARGS__)
  #else
    #include <stdio.h>
    #define XAHAU_LOGF(...) printf(__VA_ARGS__)
  #endif
#else
  #define XAHAU_LOGF(...) ((void)0)
#endif

namespace xahau {

inline void secureZero(void* p, size_t n) {
  if (!p || n == 0) return;
  volatile uint8_t* v = static_cast<volatile uint8_t*>(p);
  while (n--) {
    *v++ = 0;
  }
}

}  // namespace xahau
