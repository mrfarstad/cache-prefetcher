/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"
#include <map>
#include <algorithm>

/*
 * Thoughts:
 *
 * GHB strided:
 *  - Instead of constant stride, use GBH to store more complex access patterns
 *
 *  Markov prefetching:
 *  - Index table with absolute addresses, store pointer to previous next accessses.
 *    Assign probability to each entry. Can be implemented with GBH.
 *
 * Distance prefething:
 *  - Instead of indexing absolute addresses, create a table that is indexed by
 *    deltas and which stores the deltas of the next accesses. Each next delta
 *    can be assigned a probability.
 * */

/*
 * PREFETCHER
 * */

#define GHB_SIZE 4096
#define AIT_SIZE 2048
// This size cannot be dynamic, because hardware=)
#define MARKOV_SIZE 20

struct ghb_entry {
  Addr address;
  bool valid;
  // Previous occurence index
  int64_t prev;
  // Next occurence index
  int64_t next;
};

struct ait_entry {
  Addr address;
  bool valid;
  // Index of last GHB entry
  int64_t entry;
};

struct markov_entry {
  Addr address;
  bool valid;
  int64_t count;
};

void ghb_add_entry(Addr address);
int64_t ghb_get_prev_occurence(Addr address);

ghb_entry* ghb = NULL;
int64_t ghb_head = -1;

ait_entry* ait = NULL;
int64_t ait_head = -1;

markov_entry* markov = NULL;
uint64_t markov_head = 0;

void prefetch_init(void) {
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    ghb = (ghb_entry*) malloc(sizeof(ghb_entry) * GHB_SIZE);
    ait = (ait_entry*) malloc(sizeof(ait_entry) * AIT_SIZE);
    for (int64_t i = 0; i < GHB_SIZE; i++) {
      ghb[i].valid = false;
    }
    for (int64_t i = 0; i < AIT_SIZE; i++) {
      ait[i].valid = false;
    }
    markov = (markov_entry *) malloc(sizeof(markov_entry) * MARKOV_SIZE);
}

int64_t ghb_get_prev_occurence(Addr address) {
  // Search for previous entry
  int64_t entry;
  for (int64_t i = 0; i < AIT_SIZE; i++) {
    if (!ait[i].valid) break;
    else if (ait[i].address == address) {
      // Handle found entry
      entry = ait[i].entry;
      ait[i].entry = ghb_head;
      return entry;
    }
  }
  // If entry not found, add it to the table
  
  // Increment head pointer
  ait_head = (ait_head + 1) % AIT_SIZE;
  ait[ait_head].address = address;
  ait[ait_head].entry = ghb_head;
  ait[ait_head].valid = true;
  return -1;
  //int64_t index = (ghb_head - 1 + GHB_SIZE) % GHB_SIZE;
  //while (index != ghb_head) {
  //  if (!ghb[index].valid) return -1;
  //  else if (ghb[index].address == address) return index;
  //  index = (index - 1 + GHB_SIZE) % GHB_SIZE;
  //}
  //return -1;
}

void ghb_add_entry(Addr address) {
  // Increment head pointer
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  // Handle existing entry
  ghb_entry* entry = &ghb[ghb_head];
  if (entry->valid && entry->next != -1) {
    ghb[entry->next].prev = -1;
  }

  // Add new entry
  entry->address = address;
  entry->valid = true;
  entry->next = -1;

  // Find previous occurence
  entry->prev = ghb_get_prev_occurence(address);

  // Handle previous occurrence
  if (entry->prev != -1) {
    ghb[entry->prev].next = ghb_head;
  }
}

int count = 0;


void prefetch_access(AccessStat stat) {
  ghb_add_entry(stat.mem_addr);
  int64_t prev = ghb[ghb_head].prev;

  Addr candidate = 0;
  if (prev == -1 ) {
    candidate = stat.mem_addr + BLOCK_SIZE;
    if (!in_cache(candidate) && !in_mshr_queue(candidate)) issue_prefetch(candidate);
    return;
  }

  int64_t next;

  // TODO: Remome std::map, because we don't know how to handle size issues
  //std::map<Addr, int64_t> m;
  //while (prev != -1) {
  //  next = (prev + 1) % GHB_SIZE;
  //  m[ghb[next].address]++;
  //  prev = ghb[prev].prev;
  //}

  ////Addr candidate = 0;
  //int64_t current_max = 0;
  //std::map<Addr, int64_t>::iterator it;
  //for(it = m.begin(); it != m.end(); ++it ) {
  //    if (it->second > current_max) {
  //        candidate = it->first;
  //        current_max = it->second;
  //    }
  //  printf("%lu (%ld) ", it->first, it->second);
  //}
  //printf("=> %lu\n", candidate);

  // Initialize the data structures
  for (int64_t i = 0; i < MARKOV_SIZE; i++) {
    markov[i].valid = false;
  }
  // Loop throught the GHB linked list of previous occurences
  while (prev != -1) {
    next = (prev + 1) % GHB_SIZE;
    // Increment markov entry if exists
    bool exists = false;
    for (uint64_t i = 0; i < MARKOV_SIZE; i++) {
      if (markov[i].valid && markov[i].address == ghb[next].address) {
        markov[i].count++;
        exists = true;
        break;
      } 
    }
  //  // If markov entry not exists, create entry
    if (!exists && markov_head < MARKOV_SIZE) {
      markov[markov_head].address = ghb[next].address;
      markov[markov_head].count = 1;
      markov[markov_head++].valid = true;
    }
    prev = ghb[prev].prev;
  }

  uint64_t count = 0;
  for (uint64_t i = 0; i < MARKOV_SIZE; i++) {
    if (!markov[i].valid) break;
    else if (markov[i].count > count) {
      candidate = markov[i].address;
      count = markov[i].count;
    }
  }
  markov_head = 0;

  for (uint64_t i = 0; i < MARKOV_SIZE; i++) {
    if (!markov[i].valid) break;
    printf("%lu (%ld) ", markov[i].address, markov[i].count);
  }
  printf("=> %lu\n", candidate);
  
  if (!in_cache(candidate) && !in_mshr_queue(candidate)) {
    issue_prefetch(candidate);
  }
}

void prefetch_complete(Addr addr) {
}
