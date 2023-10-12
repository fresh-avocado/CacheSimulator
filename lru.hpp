#ifndef LRU_HPP
#define LRU_HPP

#include <stdint.h>
#include <vector>

#include "linkedlist.hpp"

class LRU {
 public:
  // for each set, we have the 'repushing stack' which tracks the LRU status
  std::vector<LinkedList*> sets;

  // a LRU can be generalized to keep track of
  // `numSets` sets with `blocksPerSet` blocks each
  LRU(uint32_t numSets, uint32_t blocksPerSet) {
    for (uint32_t i = 0; i < numSets; i++) {
      sets.push_back(new LinkedList());
      for (uint32_t j = 0; j < blocksPerSet; j++) {
        // each LRU node represents the LRU status
        // of the j-th block/entry of the i-th set
        sets[i]->pushBack(j);
      }
    }
  }
  // returns the LRU of the specified set
  Node* getLRUNode(uint64_t idx) const {
    // get the set and return the last element (that is, the LRU)
    return sets[idx]->backNode();
  }
  // returns the MRU of the specified set
  Node* getMRUNode(uint64_t idx) const {
    // get the set and return the first element (that is, the MRU)
    return sets[idx]->frontNode();
  }
  // given a set (idx) and a pointer to the node
  // that maintains the LRU status of a specific
  // block/entry inside that set, it moves that
  // node to the MRU position in O(1) time
  void makeMRU(uint64_t idx, Node* nodeOfEntryOrBlock) {
    sets[idx]->moveToFront(nodeOfEntryOrBlock);
  }
  // moves the LRU to the MRU position in O(1) time
  // useful for evicting blocks/entries
  Node* convertLRUToMRU(uint64_t idx) {
    sets[idx]->moveToFront(sets[idx]->backNode());
    return sets[idx]->frontNode();
  }
  // given a set (idx) and the index of the
  // block/entry inside that set, it 'marks' that
  // block/entry as the MRU in O(n) time where
  // n is 2^S (blocks per set)
  void setMRU(uint64_t idx, uint64_t indexOfBlockOrEntry) {
    // get set of the block
    auto setOfIndexOrBlock = sets[idx];
    // find the LRUNode corresponding to the block
    auto nodeOfEntryOrBlock = setOfIndexOrBlock->head->next;
    while (nodeOfEntryOrBlock->indexOfBlockOrEntry != indexOfBlockOrEntry) {
      nodeOfEntryOrBlock = nodeOfEntryOrBlock->next;
    }
    // add that node to the front of the list (thus, it is now the MRU)
    setOfIndexOrBlock->moveToFront(nodeOfEntryOrBlock);
  }
  ~LRU() {
    for (uint32_t i = 0; i < sets.size(); i++) {
      delete sets[i];
    }
  }
};

#endif