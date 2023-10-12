#include <math.h>

#include "l1.hpp"
#include "lru.hpp"

L1::L1(uint64_t c, uint64_t b, uint64_t s) : c{s}, b{b}, s{s} {
  numSets = pow(2, c - b - s);
  blocksPerSet = pow(2, s);
  // each set holds `blocksPerSet` blocks
  for (uint64_t i = 0; i < numSets; i++) {
    sets.push_back(new Set(blocksPerSet));
  }
  cMinusS = c - s;
  cMinusSPlusB = cMinusS + b;
  // create LRU associated with this L1
  lru = new LRU(numSets, blocksPerSet);
}

uint64_t L1::getTag(uint64_t addr) const {
  return addr >> cMinusS;
}

uint64_t L1::getIdx(uint64_t addr) const {
  if (cMinusS == b)
    return 0;
  uint64_t offset = (64 - cMinusS);
  uint64_t temp = addr << offset;
  return temp >> (offset + b);
}

L1::~L1() {
  for (uint64_t i = 0; i < numSets; i++) {
    delete sets[i];
  }
  delete lru;
}
