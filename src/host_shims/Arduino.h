// Minimal Arduino.h stub for native host builds of dependent libs (e.g. Crypto RNG).
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

inline void yield() {}
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

#ifndef PROGMEM
#define PROGMEM
#endif

class HardwareSerial {
 public:
  void begin(unsigned long) {}
  size_t print(const char*) { return 0; }
  size_t println(const char*) { return 0; }
  size_t printf(const char*, ...) { return 0; }
};
static HardwareSerial Serial;
