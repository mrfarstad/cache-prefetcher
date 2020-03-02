/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"

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

#define GHB_SIZE 512
#define GHB_DEPTH 2

struct ghb_entry {
  Addr address;
  bool valid;
  // Current index
  int index;
  // Previous occurence index
  int prev;
  // Next occurence index
  int next;
};

void ghb_add_entry(Addr address);
int ghb_get_prev_occurence(Addr address);

ghb_entry* ghb = NULL;
int ghb_head = -1;
int depth = 0;

void prefetch_init(void) {
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    ghb = (ghb_entry*) malloc(sizeof(ghb_entry) * GHB_SIZE);
    for (int i = 0; i < GHB_SIZE; i++) {
      ghb[i].valid = false;
      ghb[i].index = i;
    }
}

int ghb_get_prev_occurence(Addr address) {
  int index = (ghb_head - 1 + GHB_SIZE) % GHB_SIZE;
  while (index != ghb_head) {
    if (!ghb[index].valid) return -1;
    else if (ghb[index].address == address) return index;
    index = (index - 1 + GHB_SIZE) % GHB_SIZE;
  }
  return -1;
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

void prefetch_access(AccessStat stat) {
  ghb_add_entry(stat.mem_addr);
  int prev = ghb[ghb_head].prev;
  int next;
  while (prev != -1 && depth < GHB_DEPTH) {
    next = (prev + 1) % GHB_SIZE;
    if (!in_cache(ghb[next].address)) issue_prefetch(ghb[next].address);
    prev = ghb[prev].prev;
    depth++;
  }
  depth = 0;
}

void prefetch_complete(Addr addr) {
}
