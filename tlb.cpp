#include <math.h>

#include "lru.hpp"
#include "tlb.hpp"

TLB::TLB(uint64_t t) {
  nEntries = pow(2, t);
  entries.reserve(nEntries);
  for (uint64_t i = 0; i < nEntries; i++) {
    entries[i] = new TLBEntry();
  }
  // create LRU associated with this TLB
  lru = new LRU(1, nEntries);
}
TLB::~TLB() {
  for (uint64_t i = 0; i < nEntries; i++) {
    delete entries[i];
  }
  delete lru;
}