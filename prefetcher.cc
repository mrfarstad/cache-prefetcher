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

#define GHB_SIZE 1024 // uint16_t
#define AIT_SIZE 512 // uint16_t
#define DEPTH 3

struct ghb_entry {
  Addr address;
  bool valid;
  // Previous occurence index
  int16_t prev;
  // Next occurence index
  int16_t next;
};

struct ait_entry {
  int64_t index;
  bool valid;
  // Index of last GHB entry
  int16_t entry;
};

void ghb_add_entry(Addr address);
int16_t ghb_get_prev_occurence(Addr address);

ghb_entry* ghb = NULL;
uint16_t ghb_head = 0;

ait_entry* ait = NULL;
uint16_t ait_head = 0;

void prefetch_init(void) {
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */
    // TODO: Find a proper way to free this memory after the simulation is complete
    ghb = (ghb_entry*) malloc(sizeof(ghb_entry) * GHB_SIZE);
    ait = (ait_entry*) malloc(sizeof(ait_entry) * AIT_SIZE);
    for (uint16_t i = 0; i < GHB_SIZE; i++) {
      ghb[i].valid = false;
    }
    for (uint16_t i = 0; i < AIT_SIZE; i++) {
      ait[i].valid = false;
    }
}

int16_t ghb_get_prev_occurence(int64_t index) {
  // TODO: Implement hash function
  // Search for previous entry
  int16_t entry;
  for (uint16_t i = 0; i < AIT_SIZE; i++) {
    if (!ait[i].valid) break; 
    else if (!ghb[ait[i].entry].valid) continue;
    else if (ait[i].index == index) {
      //if (ait[i].entry == ghb_head) {
      //  printf("the prev is equal to the current!\n");
      //}
      // Handle found entry
      entry = ait[i].entry;
      ait[i].entry = ghb_head;
      return entry;
    }
  }
  // If entry not found, add it to the table
  
  // Increment head pointer
  ait[ait_head].index = index;
  ait[ait_head].entry = ghb_head;
  ait[ait_head].valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;
}

void ghb_add_entry(Addr address) {
  // Increment head pointer
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  // Handle existing entry
  ghb_entry* entry = &ghb[ghb_head];
  if (entry->valid) {
    entry->valid = false;
    if (entry->next != -1) {
      ghb[entry->next].prev = -1;
    }
  }

  // Find previous occurence
  int64_t delta = ghb[ghb_head].address - address;
  if (!delta) entry->prev = -1;
  else entry->prev = ghb_get_prev_occurence(delta);

  //if (ghb_head == entry->prev) {
  //  printf("bad returned prev!\n");
  //}

  // Add new entry
  entry->address = address;
  entry->valid = true;
  entry->next = -1;

  // Handle previous occurrence
  if (entry->prev != -1) {
    ghb[entry->prev].next = ghb_head;
  }
}

void prefetch_access(AccessStat stat) {
  /*
   * Traverse the linked list of previous occurences for the current candidate and fetch
   * prefetch candidates up to the prefetch depth
   * */

  bool prefetched = get_prefetch_bit(stat.mem_addr);
  if (stat.miss || prefetched) {
    Addr addr;
    int8_t depth = 0;
    ghb_add_entry(stat.mem_addr);
    if (prefetched) {
     clear_prefetch_bit(stat.mem_addr);
     return;
    }
    int16_t prev = ghb[ghb_head].prev;
    while (prev != -1 && depth < DEPTH) {
      int16_t candidate = (prev + 1) % GHB_SIZE;
      addr = stat.mem_addr + (ghb[prev].address - ghb[candidate].address);
      if (!in_cache(addr) && !in_mshr_queue(addr)) {
        issue_prefetch(addr);
        depth++;
      }
      prev = ghb[prev].prev;
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}
