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

#define GHB_SIZE 8192
#define AIT_SIZE 4096


void ghb_add_entry(Addr addr);
int16_t ghb_get_prev_entry(Addr addr);

struct ghb_entry {
  Addr addr;
  bool valid;
  int16_t prev;
  int16_t next;
};

struct ait_entry {
  Addr addr;
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
  // TODO: Implement hash function
  // Search for previous entry
  int16_t entry;
  for (int16_t i = 0; i < AIT_SIZE; i++) {
    if (!ait[i].valid) break;
    else if (ait[i].addr == addr) {
      // Handle found entry
      entry = ait[i].entry;
      if (!ghb[entry].valid) entry = -1;
      ait[i].entry = ghb_head;
      return entry;
    }
  }
  // If entry not found, add it to the table

  // Increment head pointer
  ait[ait_head].addr = addr;
  ait[ait_head].entry = ghb_head;
  ait[ait_head].valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;
}


void ghb_add_entry(Addr addr) {
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
  int16_t prev = ghb_get_prev_entry(addr);

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
    while (prev != -1) {
      Addr addr = ghb[(prev + 1) % GHB_SIZE].addr;
      //if (!in_cache(addr) && !in_mshr_queue(addr)) {
      issue_prefetch(addr);
      //}
      prev = ghb[prev].prev;
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}
