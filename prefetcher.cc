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

#define GHB_SIZE 512
#define AIT_SIZE 512
#define DEPTH 3


void ghb_add_entry(Addr addr);
int16_t ait_get_prev_ghb_entry(Addr addr, bool sign);
int16_t ghb_get_prev_entry(Addr addr);

struct ghb_entry {
  Addr addr;
  bool valid;
  int16_t prev;
  int16_t next;
};

struct ait_entry {
  Addr delta;
  bool sign;
  bool valid;
  int16_t entry;
};

ghb_entry* ghb = NULL;
int16_t ghb_head = -1;

ait_entry* ait = NULL;
int16_t ait_head = 0;

void prefetch_init(void) {
  ghb = (ghb_entry *) malloc(sizeof(ghb_entry) * GHB_SIZE);
  for (int16_t i = 0; i < GHB_SIZE; i++) {
    ghb[i].valid = false;
  }
  ait = (ait_entry *) malloc(sizeof(ait_entry) * AIT_SIZE);
  for (int16_t i = 0; i < AIT_SIZE; i++) {
    ait[i].valid = false;
  }
}

int16_t ghb_get_prev_entry(Addr addr) {
  int16_t prev = -1;
  int16_t index = (ghb_head + 1) % GHB_SIZE;
  while (index != ghb_head) {
    if (!ghb[index].valid) return -1;
    else if (ghb[index].addr == addr) return index;
    index = (index + 1) % GHB_SIZE;
  }
  return prev;
}

int16_t ait_get_prev_ghb_entry(Addr delta, bool sign) {
  // TODO: Implement hash function
  // Search for previous entry
  //int16_t entry;
  //for (int16_t i = 0; i < AIT_SIZE; i++) {
  //  if (!ait[i].valid) break;
  //  else if (ait[i].addr == addr) {
  //    // Handle found entry
  //    entry = ait[i].entry;
  //    if (!ghb[entry].valid) entry = -1;
  //    ait[i].entry = ghb_head;
  //    return entry;
  //  }
  //}

  ////// If entry not found in ait, search in ghb. TODO: Remove?
  ////int16_t prev = ghb_get_prev_entry(addr);

  //// Increment head pointer
  //ait[ait_head].addr = addr;
  //ait[ait_head].entry = ghb_head;
  //ait[ait_head].valid = true;
  //ait_head = (ait_head + 1) % AIT_SIZE;
  //return -1;
  //return prev;

  // TODO: Implement later
  int16_t hash = (delta + MAX_PHYS_MEM_ADDR) % AIT_SIZE;
  ait_entry* bucket = &ait[hash];
  int16_t b_delta = bucket->delta;
  int16_t entry = bucket->entry;
  bool valid = bucket->valid;
  bool b_sign = bucket->sign;
  bucket->delta = delta;
  bucket->sign = sign;
  bucket->entry = ghb_head;
  bucket->valid = true;
  if (valid && b_delta == delta && b_sign == sign) return entry;
  return -1;
}


void ghb_add_entry(Addr addr) {
  int16_t tmp_head = ghb_head;
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  ghb_entry* entry = &ghb[ghb_head];
  // Handle existing entry
  if (entry->valid) {
    entry->valid = false;
    if (entry->next != -1) {
      ghb[entry->next].prev = -1;
    }
  }

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
    if (prev == ghb_head) fprintf(stderr, "error: prev == ghb_head");
  } else {
    prev = -1;
  }

  // Handle pointer from previous to this
  if (prev != -1) {
    ghb[prev].next = ghb_head;
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
  bool prefetched = get_prefetch_bit(addr);
  if (stat.miss || prefetched) {
    //ghb_add_entry(addr);
    //for (int16_t i = 0; i < GHB_SIZE; i++) {
    //  if (i == ghb_head) continue;
    //  else if (ghb[i].addr == addr) {
    //    printf("found!\n");
    //  }
    //}
    // TODO: Add entry to GHB
    ghb_add_entry(addr);
    if (prefetched) {
     clear_prefetch_bit(addr);
     return;
    }
    // TODO: Handle prefetch candidates
    int16_t prev = ghb[ghb_head].prev;
    //printf("prev: %d\n", prev);
    int16_t depth = 0;
    Addr prev_addr;
    Addr cand_addr;
    Addr delta;
    bool sign;
    while (prev != -1 && depth < DEPTH) {
      prev_addr = ghb[prev].addr;
      cand_addr = ghb[(prev + 1) % GHB_SIZE].addr;
      if (prev_addr > cand_addr) {
        delta = prev_addr - cand_addr;
        sign = 0;
      } else {
        delta = cand_addr - prev_addr;
        sign = 1;
      }
      prev = ghb[prev].prev;
      depth++;
      if (sign && addr + delta < MAX_PHYS_MEM_ADDR) addr += delta;
      else if (!sign && addr > delta) addr -= delta;
      else continue;
      if (!in_cache(addr) && !in_mshr_queue(addr)) {
        issue_prefetch(addr);
      }
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}
