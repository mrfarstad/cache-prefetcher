/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"
#include <queue>

/*
 * PREFETCHER
 * */

#define GHB_SIZE 32
#define AIT_SIZE 32
#define DEGREE 3
#define WIDTH 3
#define DEPTH 2


void ghb_add_entry(Addr addr);
int16_t ait_get_prev_ghb_entry(Addr addr, bool sign);
int16_t ghb_get_prev_entry(Addr addr);

struct ghb_entry {
  Addr addr;
  bool valid;
  int16_t prev;
};

struct ait_entry {
  Addr delta;
  bool sign;
  bool valid;
  int16_t entry;
};

ghb_entry ghb[GHB_SIZE];
int16_t ghb_head = -1;

ait_entry ait[AIT_SIZE];

void prefetch_init(void) {
  for (uint16_t i = 0; i < GHB_SIZE; i++) {
    ghb[i].valid = false;
    ghb[i].prev = -1;
  }
  for (uint16_t i = 0; i < AIT_SIZE; i++) ait[i].valid = false;
}

int16_t ait_get_prev_ghb_entry(Addr delta, bool sign) {
  uint16_t hash = delta % AIT_SIZE;
  ait_entry* bucket = &ait[hash];
  uint16_t b_delta = bucket->delta;
  int16_t b_entry = bucket->entry;
  bool b_valid = bucket->valid;
  bool b_sign = bucket->sign;
  bucket->delta = delta;
  bucket->sign = sign;
  bucket->entry = ghb_head;
  bucket->valid = true;
  // Not ait init && not overwritten ghb entry && equal deltas and signes
  if (b_valid && ghb[b_entry].valid && b_delta == delta && b_sign == sign)
    return b_entry;
  return -1;
}


void ghb_add_entry(Addr addr) {
  int16_t tmp_head = ghb_head;
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  ghb_entry* entry = &ghb[ghb_head];
  entry->valid = false;

  // Find prev entry
  int16_t prev;
  if (tmp_head != -1) {
    Addr delta;
    Addr tmp_addr = ghb[tmp_head].addr;
    bool sign;
    if (tmp_addr > addr) {
      delta = tmp_addr - addr;
      sign = 0;
    } else {
      delta = addr - tmp_addr;
      sign = 1;
    }
    prev = ait_get_prev_ghb_entry(delta, sign);
  } else {
    prev = -1;
  }

  entry->prev = prev;
  entry->addr = addr;
  entry->valid = true;
}

void prefetch_access(AccessStat stat) {
  /*
   * Traverse the linked list of previous occurences for the current candidate and fetch
   * prefetch candidates up to the prefetch depth
   * */
  Addr addr = stat.mem_addr;
  if (stat.miss) {
    ghb_add_entry(addr);
    int16_t prev = ghb[ghb_head].prev;
    if (prev == -1) {
      Addr tmp_addr = addr;
      uint8_t strided = 0;
      while (strided++ < DEGREE) {
        tmp_addr += BLOCK_SIZE;
        if (!in_cache(tmp_addr))
          issue_prefetch(tmp_addr);
      }
      return;
    }
    uint8_t width = 0;
    uint8_t depth = 0;
    Addr prev_addr;
    Addr cand_addr;
    Addr delta;
    bool sign;
    int16_t cand;
    while (prev != -1 && width++ < WIDTH) {
      prev_addr = ghb[prev].addr;
      cand = (prev + 1) % GHB_SIZE;
      while (cand != ghb_head && depth++ < DEPTH) {
        cand_addr = ghb[cand].addr;
        cand = (cand + 1) % GHB_SIZE;
        prev = ghb[prev].prev;
        if (prev_addr > cand_addr) {
          delta = prev_addr - cand_addr;
          sign = 0;
        } else {
          delta = cand_addr - prev_addr;
          sign = 1;
        }
        if (sign && addr + delta < MAX_PHYS_MEM_ADDR)
          addr += delta;
        else if (!sign && addr > delta)
          addr -= delta;
        // If delta causes underflow or overflow, do not prefetch
        else continue;
        if (!in_cache(addr)) {
          issue_prefetch(addr);
        }
      }
    }
  }
}


void prefetch_complete(Addr addr) {
}


