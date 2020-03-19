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
#define DEPTH 200

struct ghb_entry {
  Addr address;
  bool valid;
  // Previous occurence index
  int16_t prev;
  // Next occurence index
  int16_t next;
};

struct ait_entry {
  uint64_t index;
  bool sign;
  bool valid;
  // Index of last GHB entry
  int16_t entry;
};

void ghb_add_entry(Addr address);
int16_t ghb_get_prev_occurence(uint64_t delta, bool sign, int16_t entry);

ghb_entry* ghb = NULL;
int16_t ghb_head = -1;

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

int16_t ghb_get_prev_occurence(uint64_t index, bool sign, int16_t head) {
  // TODO: Implement hash function
  // Search for previous entry
  int16_t entry;
  for (uint16_t i = 0; i < AIT_SIZE; i++) {
    if (!ait[i].valid) {
      break;
    } else if (ait[i].index == index && ait[i].sign == sign) {
      // DEBUGGING
      //if (ait[i].entry == head) {
      //  printf("the prev is equal to the current!\n");
      //}
      // Handle found entry
      entry = ait[i].entry;
      if (!ghb[entry].valid)
        break;
      ait[i].entry = head;
      return entry;
    }
  }
  // If entry not found, add it to the table
  
  // Increment head pointer
  ait[ait_head].index = index;
  ait[ait_head].sign = sign;
  ait[ait_head].entry = head;
  ait[ait_head].valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;
}

void ghb_add_entry(Addr address) {
  int16_t tmp_head = (ghb_head + 1) % GHB_SIZE;
  // Handle existing entry
  ghb_entry* entry = &ghb[tmp_head];
  if (entry->valid) {
    entry->valid = false;
    if (entry->next != -1) {
      ghb[entry->next].prev = -1;
    }
  }

  if (ghb_head != -1) { // && ghb[ghb_head].valid) {
    uint64_t delta;
    Addr tmp_addr = ghb[ghb_head].address;
    bool sign;
    if (tmp_addr > address) {
      delta = tmp_addr - address;
      sign = 0;
    } else {
      delta = address - tmp_addr;
      sign = 1;
    }
    entry->prev = ghb_get_prev_occurence(delta, sign, tmp_head);
  }
  else {
    // Handle initial entry before a delta can be calculated
    entry->prev = -1;
  }

  // DEBUGGING
  if (tmp_head == entry->prev) {
    printf("error: bad returned prev!\n");
  }

  // Add new entry
  entry->address = address;
  entry->valid = true;
  entry->next = -1;

  // Handle previous occurrence
  if (entry->prev != -1) {
    ghb[entry->prev].next = tmp_head;
  }

  // Increment head pointer
  ghb_head = tmp_head;
}

void prefetch_access(AccessStat stat) {
  /*
   * Traverse the linked list of previous occurences for the current candidate and fetch
   * prefetch candidates up to the prefetch depth
   * */

  bool prefetched = get_prefetch_bit(stat.mem_addr);
  if (stat.miss || prefetched) {
    int8_t depth = 0;
    bool initial = false;
    if (ghb_head == -1) initial = true; 
    ghb_add_entry(stat.mem_addr);
    if (initial) return;
    if (prefetched) {
     clear_prefetch_bit(stat.mem_addr);
     return;
    }
    // ghb_head incremented after ghb_add_entry, we want to check prev of last added
    int16_t prev = ghb[(ghb_head - 1 + GHB_SIZE) % GHB_SIZE].prev;
    printf("prev: %d, candidates: ", prev);
    while (prev != -1 && depth < DEPTH) {
      int16_t candidate = (prev + 1) % GHB_SIZE;
      printf("%d, ", candidate);
      uint64_t delta;
      Addr prev_addr = ghb[prev].address;
      Addr adj_addr = ghb[candidate].address;
      bool sign;
      if (prev_addr > adj_addr) {
        delta = prev_addr - adj_addr;
        sign = 0;
      } else {
        delta = adj_addr - prev_addr;
        sign = 1;
      }

      Addr addr;
      // If no underflow
      if (!sign && delta < stat.mem_addr) {
         addr = stat.mem_addr - delta;
         if (!in_cache(addr) && !in_mshr_queue(addr)) {
            issue_prefetch(addr);
            depth++;
         }
      }
      // If no overflow
      else if (sign && stat.mem_addr + delta < MAX_PHYS_MEM_ADDR) {
         addr = stat.mem_addr + delta;
         if (!in_cache(addr) && !in_mshr_queue(addr)) {
            issue_prefetch(addr);
            depth++;
         }
      }

      //depth++;
      //if (depth == 199) {
      //  printf("something fishy\n");
      //}
      prev = ghb[prev].prev;
    }
    printf("prev: %d\n", prev);
    if (prev != -1) {
      printf("error: prev should be -1 but was: %d\n", prev);
    }
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}
