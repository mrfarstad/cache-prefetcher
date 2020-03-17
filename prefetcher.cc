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

#define GHB_SIZE 512 // uint16_t
#define AIT_SIZE 256 // uint16_t
#define DEPTH 5

struct ghb_entry {
  Addr address;
  bool valid;
  // Previous occurence index
  int16_t prev;
  // Next occurence index
  int16_t next;
};

struct ait_entry {
  int32_t delta;
  bool valid;
  // Index of last GHB entry
  uint16_t entry;
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

int16_t ghb_get_prev_occurence(Addr address) {
  // TODO: Implement hash function
  // Search for previous entry
  if (!ghb[ghb_head].valid) return -1;
  int16_t delta = ghb[ghb_head].address - address;
  int16_t entry;
  for (uint16_t i = 0; i < AIT_SIZE; i++) {
    if (!ait[i].valid) break;
    else if (ait[i].delta == delta) {
      // Handle found entry
      entry = ait[i].entry;
      ait[i].entry = ghb_head;
      return entry;
    }
  }
  // If entry not found, add it to the table
  
  // Increment head pointer
  ait[ait_head].delta = delta;
  ait[ait_head].entry = ghb_head;
  ait[ait_head].valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;
}

void ghb_add_entry(Addr address) {
  // Handle existing entry
  ghb_entry* entry = &ghb[ghb_head];
  if (entry->valid && entry->next != -1) {
    ghb[entry->next].prev = -1;
  }

  // Find previous occurence
  entry->prev = ghb_get_prev_occurence(address);

  // Add new entry
  entry->address = address;
  entry->valid = true;
  entry->next = -1;

  // Handle previous occurrence
  if (entry->prev != -1) {
    ghb[entry->prev].next = ghb_head;
  }

  // Increment head pointer
  ghb_head = (ghb_head + 1) % GHB_SIZE;
}

int misses = 0; 

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
    int16_t candidate = ghb[ghb_head].prev;
    while (candidate != -1 && depth < DEPTH) {
      addr = stat.mem_addr + (ghb[candidate].address - ghb[(candidate + 1) % GHB_SIZE].address);
      if (addr > 0 && addr < MAX_PHYS_MEM_ADDR && !in_cache(addr) && !in_mshr_queue(addr)) {
        issue_prefetch(addr);
        depth++;
      }
      candidate = ghb[candidate].prev;
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}
