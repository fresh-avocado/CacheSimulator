#ifndef L1_HPP
#define L1_HPP

#include <stdint.h>
#include <vector>

class LRU;

class Block {
 public:
  // all L1 entries are initially invalid
  bool valid = false;
  // and not dirty
  bool dirty = false;
  uint64_t tag;
  Block() = default;
};

// a set contains an array `blocksPerSet` blocks
class Set {
 private:
  uint32_t blocksPerSet;

 public:
  std::vector<Block*> blocks;
  Set(uint32_t blocksPerSet) : blocksPerSet{blocksPerSet} {
    for (uint32_t i = 0; i < blocksPerSet; i++) {
      blocks.push_back(new Block());
    }
  }
  ~Set() {
    for (uint32_t i = 0; i < blocksPerSet; i++) {
      delete blocks[i];
    }
  }
};

class L1 {
 public:
  std::vector<Set*> sets;
  uint32_t numSets, blocksPerSet;
  LRU* lru;
  uint64_t cMinusS, cMinusSPlusB, c, b, s;
  L1() = default;
  L1(uint64_t c, uint64_t b, uint64_t s);
  uint64_t getTag(uint64_t addr) const;
  uint64_t getIdx(uint64_t addr) const;
  ~L1();
};

#endif