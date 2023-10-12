#ifndef HWIVPT_HPP
#define HWIVPT_HPP

#include <stdint.h>
#include <vector>

#include "cachesim.hpp"
#include "lru.hpp"

class LRU;

class HWIVPTEntry {
 public:
  // all HWIVPT entries are initially invalid
  bool valid = false;
  uint64_t vpn = 0;
  uint32_t ppn = 0;
  HWIVPTEntry(uint32_t ppn) : ppn{ppn} {}
};

class HWIVPT {
 public:
  std::vector<HWIVPTEntry*> entries;
  uint64_t nEntries;
  LRU* lru;
  HWIVPT(uint64_t m);
  ~HWIVPT();
};

#endif