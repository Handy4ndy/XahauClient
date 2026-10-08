#pragma once
#include <stddef.h>
#include <stdint.h>
class RNGClass {
public:
  static void begin(const char* = nullptr) {}
  static void rand(uint8_t* d, size_t n) { for(size_t i=0;i<n;i++) d[i]=(uint8_t)(i*17+3); }
};
static RNGClass RNG;
