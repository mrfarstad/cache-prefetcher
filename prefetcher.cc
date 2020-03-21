/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"

/*
 * PREFETCHER
 * */

#define GHB_SIZE 1024
#define AIT_SIZE 1024
#define DEPTH 3
#define WIDTH 3
#define STRIDED 4

struct delta_t {
  Addr delta;
  bool sign;
};

struct deltas_t {
  delta_t d1;
  delta_t d2;
};

struct ghb_entry {
  Addr addr;
  bool valid;
  int16_t prev;
  int16_t next;
};

struct ait_entry {
  deltas_t deltas;
  bool valid;
  int16_t entry;
};

void ghb_add_entry(Addr addr);
int16_t ait_get_prev_ghb_entry(deltas_t d);
int16_t ghb_get_prev_entry(Addr addr);

ghb_entry* ghb = NULL;
int16_t ghb_head = -1;

ait_entry* ait = NULL;
int16_t ait_head = 0;

void prefetch_init(void) {
  ghb = (ghb_entry *) malloc(sizeof(ghb_entry) * GHB_SIZE);
  for (int16_t i = 0; i < GHB_SIZE; i++) {
    ghb[i].valid = false;
    ghb[i].prev = -1;
    ghb[i].next = -1;
  }
  ait = (ait_entry *) malloc(sizeof(ait_entry) * AIT_SIZE);
  for (int16_t i = 0; i < AIT_SIZE; i++) ait[i].valid = false;
}

int16_t ait_get_prev_ghb_entry(deltas_t d) {
  int16_t entry;
  ait_entry* bucket;
  for (int16_t i = 0; i < AIT_SIZE; i++) {
    bucket = &ait[i];
    if (!bucket->valid) break;
    else if (
        bucket->deltas.d1.delta == d.d1.delta &&
        bucket->deltas.d1.sign == d.d1.sign &&
        bucket->deltas.d2.delta == d.d2.delta &&
        bucket->deltas.d2.sign == d.d2.sign
    ) {
      // Handle found entry
      entry = ait[i].entry;
      if (!ghb[bucket->entry].valid) entry = -1;
      ait[i].entry = ghb_head;
      return entry;
    }
  }

  bucket = &ait[ait_head];
  bucket->deltas = d;
  bucket->entry = ghb_head;
  bucket->valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;

  // TODO: Find a better hash function
  // Use std:hash or something
  //int16_t hash = (d.d1.delta ^ d.d2.delta) % AIT_SIZE;
  //int16_t hash = (d.d1.delta + d.d2.delta) % AIT_SIZE;
  //ait_entry* bucket = &ait[hash];
  //deltas_t b_deltas = bucket->deltas;
  //int16_t b_entry = bucket->entry;
  //bool b_valid = bucket->valid;
  //bucket->deltas = d;
  //bucket->entry = ghb_head;
  //bucket->valid = true;
  //// Not ait init && not overwritten ghb entry && equal deltas and signes
  //if (b_valid && ghb[b_entry].valid &&
  //    b_deltas.d1.delta == d.d1.delta && b_deltas.d1.sign == d.d1.sign &&
  //    b_deltas.d2.delta == d.d2.delta && b_deltas.d2.sign == d.d2.sign)
  //  return b_entry;
  //return -1;
}


void ghb_add_entry(Addr addr) {
  int16_t prev_head = (ghb_head - 1 + GHB_SIZE) % GHB_SIZE;
  int16_t tmp_head = ghb_head;
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  ghb_entry* entry = &ghb[ghb_head];
  // Handle existing entry
  if (entry->valid) {
    entry->valid = false;
    if (entry->next != -1)
      ghb[entry->next].prev = -1;
  }

  // Find prev entry
  int16_t prev;
  delta_t d1;
  delta_t d2;
  if (tmp_head != -1 && ghb[prev_head].valid) {
    Addr tmp_addr = ghb[prev_head].addr;
    if (tmp_addr > addr) {
      d1.delta = tmp_addr - addr;
      d1.sign = 0;
    } else {
      d1.delta = addr - tmp_addr;
      d1.sign = 1;
    }

    tmp_addr = ghb[tmp_head].addr;
    if (tmp_addr > addr) {
      d2.delta = tmp_addr - addr;
      d2.sign = 0;
    } else {
      d2.delta = addr - tmp_addr;
      d2.sign = 1;
    }

    prev = ait_get_prev_ghb_entry((struct deltas_t) { d1, d2 });
    if (prev == ghb_head) fprintf(stderr, "error: prev == ghb_head");
  } else {
    prev = -1;
  }

  //// Handle pointer from previous to this
  if (prev != -1)
    ghb[prev].next = ghb_head;

  entry->prev = prev;
  entry->addr = addr;
  entry->valid = true;
  entry->next = -1;
}

void prefetch_access(AccessStat stat) {
  /*
   * Traverse the linked list of previous occurences for the current candidate and fetch
   * prefetch candidates up to the prefetch depth
   * */
  Addr addr = stat.mem_addr;
  bool prefetched = get_prefetch_bit(addr);
  if (stat.miss || prefetched) {
    ghb_add_entry(addr);
    if (prefetched) {
     clear_prefetch_bit(addr);
     return;
    }
    int16_t prev = ghb[ghb_head].prev;
    //Addr tmp_addr = addr + BLOCK_SIZE;
    //if (prev == -1) {
    //  uint8_t strided = 0;
    //  while (strided++ < STRIDED) {
    //    if (!in_cache(tmp_addr) && !in_mshr_queue(tmp_addr))
    //      issue_prefetch(tmp_addr);
    //    tmp_addr += BLOCK_SIZE;
    //  }
    //}
    uint8_t depth = 0;
    uint8_t width = 0;
    Addr prev_addr;
    Addr cand_addr;
    Addr delta;
    bool sign;
    while (prev != -1 && depth < DEPTH && width++ < WIDTH) {
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
      if (sign && addr + delta < MAX_PHYS_MEM_ADDR)
        addr += delta;
      else if (!sign && addr > delta)
        addr -= delta;
      // If delta causes underflow or overflow, do not prefetch
      else continue;
      if (!in_cache(addr) && !in_mshr_queue(addr)) {
        issue_prefetch(addr);
        depth++;
      }
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}

