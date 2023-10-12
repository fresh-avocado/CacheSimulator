#ifndef TLB_HPP
#define TLB_HPP

#include <stdint.h>
#include <vector>

class LRU;

class TLBEntry {
 public:
  // all TLB entries are initially invalid
  bool valid = false;
  uint64_t vpn = 0;
  uint32_t ppn = 0;
  TLBEntry() = default;
};

class TLB {
 public:
  std::vector<TLBEntry*> entries;
  LRU* lru;
  uint64_t nEntries;
  TLB(uint64_t t);
  ~TLB();
};

#endif