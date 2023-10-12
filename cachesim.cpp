#include <cassert>
#include <tuple>

#include "cachesim.hpp"

bool vipt;
// L1 cache
L1* l1;
// TLB
TLB* tlb;
// HWIVPT
HWIVPT* pt;
// global access to config (used for stat calculation)
sim_config_t* global_config;

void pipt_access(char rw, uint64_t addr, sim_stats_t* stats);
void vipt_access(char rw, uint64_t addr, sim_stats_t* stats);
void tag_compare(char rw,
                 uint64_t idx,
                 uint64_t tag,
                 Set* currentSet,
                 sim_stats_t* stats,
                 bool vipt);
std::tuple<uint64_t, uint64_t> get_vpn_and_page_offset(uint64_t addr);
void flush_l1(sim_stats_t* stats);
void deallocate_resources();

/**
 * The use of virtually indexed physically tagged caches limits
 *      the total number of sets you can have.
 * If the user selected configuration is invalid for VIPT
 *      Update config->s to reflect the minimum value for S (log2 number of
 * ways) If the user selected configuration is valid do not modify it.
 */

void legalize_s(sim_config_t* config) {
  if (config->s < config->c - config->p) {
    config->s = config->c - config->p;
  }
}

/**
 * Subroutine for initializing the cache simulator. You many add and initialize
 * any global or heap variables as needed.
 */

void sim_setup(sim_config_t* config) {
  global_config = config;
  vipt = config->vipt;
  if (vipt) {
    tlb = new TLB(config->t);
    pt = new HWIVPT(config->m);
    l1 = new L1(config->c, config->b, config->s);
  } else {
    l1 = new L1(config->c, config->b, config->s);
  }
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 */
void sim_access(char rw, uint64_t addr, sim_stats_t* stats) {
  if (vipt) {
    vipt_access(rw, addr, stats);
  } else {
    pipt_access(rw, addr, stats);
  }
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating
 * overall statistics such as miss rate or average access time.
 */
void sim_finish(sim_stats_t* stats) {
  deallocate_resources();

  stats->hit_ratio_l1 = (double)stats->hits_l1 / (double)stats->accesses_l1;
  stats->miss_ratio_l1 = 1.0 - stats->hit_ratio_l1;
  double S = global_config->s * 0.2;

  if (vipt) {
    stats->hit_ratio_tlb =
        (double)stats->hits_tlb / (double)stats->accesses_tlb;
    stats->miss_ratio_tlb = 1.0 - stats->hit_ratio_tlb;

    stats->hit_ratio_hw_ivpt =
        (double)stats->hits_hw_ivpt / (double)stats->accesses_hw_ivpt;
    stats->miss_ratio_hw_ivpt = 1.0 - stats->hit_ratio_hw_ivpt;

    double hwivpt_penalty =
        (1.0 + HW_IVPT_ACCESS_TIME_PER_M * global_config->m) *
        DRAM_ACCESS_PENALTY;
    double tag_compare_time = L1_TAG_COMPARE_TIME_CONST + S;
    double hit_time =
        L1_ARRAY_LOOKUP_TIME_CONST + stats->hit_ratio_tlb * tag_compare_time +
        stats->miss_ratio_tlb *
            (hwivpt_penalty + tag_compare_time * stats->hit_ratio_hw_ivpt);

    stats->avg_access_time =
        hit_time + (stats->miss_ratio_l1 * DRAM_ACCESS_PENALTY);
  } else {
    double hit_time_l1 =
        L1_ARRAY_LOOKUP_TIME_CONST + L1_TAG_COMPARE_TIME_CONST + S;
    stats->avg_access_time =
        hit_time_l1 + (stats->miss_ratio_l1 * DRAM_ACCESS_PENALTY);
  }
}

void vipt_access(char rw, uint64_t addr, sim_stats_t* stats) {
  stats->accesses_tlb++;
  auto [vpn, pageOffset] = get_vpn_and_page_offset(addr);
  // array lookup
  uint64_t idx = l1->getIdx(addr);
  auto currentSet = l1->sets[idx];
  // set buffer was populated
  stats->array_lookups_l1++;
  TLBEntry* tlbEntry;
  // tlb access
  for (uint64_t i = 0; i < tlb->nEntries; i++) {
    tlbEntry = tlb->entries[i];
    if (tlbEntry->valid && tlbEntry->vpn == vpn) {
      // valid and tag matched
      stats->hits_tlb++;
      // entry was just accessed, becomes the MRU
      tlb->lru->setMRU(0, i);
      // the i-th entry should be now the MRU
      assert(i == tlb->lru->getMRUNode(0)->indexOfBlockOrEntry);
      auto ppn = tlbEntry->ppn;
      // tlb hit, proceed with l1 tag compare
      stats->tag_compares_l1++;
      stats->accesses_l1++;
      tag_compare(rw, idx, ppn, currentSet, stats, true);
      // return (optimization) as no more work is needed
      return;
    }
  }
  // tlb miss
  stats->misses_tlb++;
  // look for translation in hwivpt
  stats->accesses_hw_ivpt++;
  HWIVPTEntry* pte;
  // hwivpt access
  for (uint64_t i = 0; i < pt->nEntries; i++) {
    pte = pt->entries[i];
    if (pte->valid && pte->vpn == vpn) {
      // valid and tag matched
      stats->hits_hw_ivpt++;
      // entry was just accessed, becomes the MRU
      pt->lru->setMRU(0, i);
      // the i-th entry should be now the MRU
      assert(i == pt->lru->getMRUNode(0)->indexOfBlockOrEntry);
      // replace LRU entry in the TLB, becomes the MRU
      auto mruNode = tlb->lru->convertLRUToMRU(0);
      // this tlb entry should be the MRU
      assert(mruNode->indexOfBlockOrEntry ==
             tlb->lru->getMRUNode(0)->indexOfBlockOrEntry);
      auto mruEntry = tlb->entries[mruNode->indexOfBlockOrEntry];
      // mark it as valid (in case it was a compulsory miss)
      mruEntry->valid = 1;
      // install the translation from the HWIVPT
      mruEntry->vpn = pte->vpn;
      mruEntry->ppn = pte->ppn;
      // proceed to access the cache
      stats->accesses_l1++;
      tag_compare(rw, idx, mruEntry->ppn, currentSet, stats, true);
      // return (optimization) as no more work is needed
      return;
    }
  }
  // hwivpt miss
  stats->misses_hw_ivpt++;
  // flush l1 cache & reset its LRU status
  flush_l1(stats);
  // replace LRU entry in HWIVPT with new VPN (PPN stays as is)
  // LRU entry becomes the MRU
  auto pteIndex = pt->lru->convertLRUToMRU(0)->indexOfBlockOrEntry;
  auto newPte = pt->entries[pteIndex];
  // mark it as valid (in case it was a compulsory miss)
  newPte->valid = 1;
  // install translation
  newPte->vpn = vpn;
  // replace LRU entry in the TLB with new mapping
  auto tlbeIndex = tlb->lru->convertLRUToMRU(0)->indexOfBlockOrEntry;
  auto tlbe = tlb->entries[tlbeIndex];
  // mark it as valid (in case it was a compulsory miss)
  tlbe->valid = 1;
  // install the translation from the HWIVPT
  tlbe->vpn = vpn;
  tlbe->ppn = newPte->ppn;
  // proceed to access the cache
  stats->accesses_l1++;
  tag_compare(rw, idx, newPte->ppn, currentSet, stats, true);
}

std::tuple<uint64_t, uint64_t> get_vpn_and_page_offset(uint64_t addr) {
  uint64_t pageOffset = addr & (0xffffffffffffffff >> (64 - global_config->p));
  uint64_t vpn = addr >> global_config->p;
  return {vpn, pageOffset};
};

void flush_l1(sim_stats_t* stats) {
  Block* block;
  // invalidate all blocks and writeback dirty ones
  for (uint64_t i = 0; i < l1->numSets; i++) {
    for (uint64_t j = 0; j < l1->blocksPerSet; j++) {
      block = l1->sets[i]->blocks[j];
      // mark block as invalid
      block->valid = 0;
      // if block was dirty: clear bit & write back
      if (block->dirty) {
        block->dirty = 0;
        stats->cache_flush_writebacks++;
      }
    }
  }
  // reset LRU status of L1
  delete l1->lru;
  l1->lru = new LRU(l1->numSets, l1->blocksPerSet);
}

void tag_compare(char rw,
                 uint64_t idx,
                 uint64_t tag,
                 Set* currentSet,
                 sim_stats_t* stats,
                 bool vipt) {
  for (uint64_t blockIdx = 0; blockIdx < l1->blocksPerSet; blockIdx++) {
    // get block
    auto block = currentSet->blocks[blockIdx];
    // if valid and tag match, then we have a hit
    if (block->valid && block->tag == tag) {
      stats->hits_l1++;
      if (rw == WRITE) {
        // if its a write hit, as we are following a
        // WBWA policy, just set the dirty bit to true
        block->dirty = true;
        stats->writes++;
      } else {
        stats->reads++;
      }
      // block is now the MRU as it was just accessed
      l1->lru->setMRU(idx, blockIdx);
      // now this block should be the MRU (head of list)
      assert(blockIdx == l1->lru->getMRUNode(idx)->indexOfBlockOrEntry);
      // return because there is nothing more to do for this access
      return;
    }
  }
  // if no block matched the search, we have a miss, so increase counter
  stats->misses_l1++;
  // get LRU block to evict and move it to MRU pos
  auto lruNode = l1->lru->convertLRUToMRU(idx);
  auto lruBlockIdx = lruNode->indexOfBlockOrEntry;
  auto lruBlock = currentSet->blocks[lruBlockIdx];
  if (lruBlock->dirty) {
    // if it's dirty, then write it back to mem
    stats->writebacks_l1++;
    // once the block is written back, it's no longer dirty
    lruBlock->dirty = false;
    if (vipt) {
      // update LRU status of corresponding HWIVTPT entry
      // since it was written back to main memory
      // since `tag == ppn`, we can index the HWIVPT with the tag
      // to find the entry in O(1)
      pt->lru->setMRU(0, lruBlock->tag);
      // now this entry should be the MRU
      assert(lruBlock->tag == pt->lru->getMRUNode(0)->indexOfBlockOrEntry);
    }
  }
  // set it's valid bit, just in case this was a cold-start miss
  lruBlock->valid = true;
  if (rw == WRITE) {
    // if its a write, then the block is immediately dirty (WBWA write-policy)
    lruBlock->dirty = true;
    stats->writes++;
  } else {
    stats->reads++;
  }
  if (vipt) {
    // since we have fetched the block from memory
    // to read/write from/to it, we have accessed the
    // corresponding frame in mem, so we need to update
    // the LRU status of the corresponding HWIVPT entry
    pt->lru->setMRU(0, tag);
    // now this entry should be the MRU
    assert(tag == pt->lru->getMRUNode(0)->indexOfBlockOrEntry);
  }
  // update the tag
  lruBlock->tag = tag;
  // now this block should be the MRU
  assert(lruBlockIdx == l1->lru->getMRUNode(idx)->indexOfBlockOrEntry);
}

void pipt_access(char rw, uint64_t addr, sim_stats_t* stats) {
  // we just accessed the cache, so increase counter
  stats->accesses_l1++;
  // extract tag from address
  uint64_t tag = l1->getTag(addr);
  // extract index from address
  uint64_t idx = l1->getIdx(addr);
  // find the set for this address
  auto currentSet = l1->sets[idx];
  // we just did an array lookup, so increase counter
  stats->array_lookups_l1++;
  // do a `parallel' tag comparison to find a block that matches
  tag_compare(rw, idx, tag, currentSet, stats, false);
}

void deallocate_resources() {
  delete l1;
  if (vipt) {
    delete tlb;
    delete pt;
  }
}

// clang-format off
/*
GDB:
  gdb ./cachesim
  PIPT:
    run -c 10 -b 6 -s 4 < traces/gcc.trace
  VIPT:
    run -v -c 15 -b 7 -s 1 -p 14 -t 7 -m 18 < traces/gcc.trace
VALGRIND:
  PIPT:
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./cachesim -c 10 -b 6 -s 4 < traces/gcc.trace
  VIPT:
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./cachesim -v -c 15 -b 7 -s 1 -p 14 -t 7 -m 18 < traces/gcc.trace
*/
// clang-format on
