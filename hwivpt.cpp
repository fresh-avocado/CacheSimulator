#include "hwivpt.hpp"
#include "lru.hpp"

HWIVPT::HWIVPT(uint64_t m) {
  nEntries = pow(2, m);
  entries.reserve(nEntries);
  // the i-th entry in the HWIVPT contains the
  // PPN of the i-th frame
  for (uint64_t i = 0; i < nEntries; i++) {
    entries[i] = new HWIVPTEntry((uint32_t)i);
  }
  // create LRU associated with this HWIVPT
  lru = new LRU(1, nEntries);
}

HWIVPT::~HWIVPT() {
  delete lru;
  for (uint64_t i = 0; i < nEntries; i++) {
    delete entries[i];
  }
}