/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"

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
  ghb_entry* prev_occurence;
  ghb_entry* next_occurence;
  int index;
};

ghb_entry* ghb_add_entry(Addr address);
ghb_entry* ghb_get_prev_occurence(Addr address);

ghb_entry** ghb = NULL;
int ghb_head = -1;
int depth = 0;

void prefetch_init(void) {
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * GHB_SIZE);
    for (unsigned int i = 0; i < GHB_SIZE; i++) {
      ghb[i] = NULL;
    }
}

void prefetch_access(AccessStat stat) {
 // Addr pf_addr = stat.mem_addr + BLOCK_SIZE;
 // ////
 // /////*
 // //// * Issue a prefetch request if a demand miss occured,
 // //// * and the block is not already in cache.
 // //// */
 // if (stat.miss && !in_cache(pf_addr)) {
 //     issue_prefetch(pf_addr);
 // }
  ghb_entry* entry = ghb_add_entry(stat.mem_addr)->prev_occurence;
  while (entry != NULL && depth < GHB_DEPTH) {
    Addr pf_addr = ghb[(entry->index + 1) % GHB_SIZE]->address;
    //if (stat.miss && !in_cache(pf_addr)) {
    issue_prefetch(pf_addr);
    //}
    entry = entry->prev_occurence;
    depth++;
  }
  depth = 0;
}

ghb_entry* ghb_get_prev_occurence(Addr address) {
  if (ghb_head == -1) return NULL;
  size_t index = (ghb_head - 1 + GHB_SIZE) % GHB_SIZE;
  while (index != ghb_head) {
    ghb_entry* entry = ghb[index];
    if (entry == NULL) return NULL;
    else if (entry->address == address) return entry;
    index = (index - 1 + GHB_SIZE) % GHB_SIZE;
  }
  return NULL;
}

ghb_entry* ghb_add_entry(Addr address) {
  // Increment ghb
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  // Handle existing entry
  ghb_entry* old = ghb[ghb_head];
  if (old != NULL) {
    // Reset next_occurence's pointer to this struct
    if (old->next_occurence != NULL) old->next_occurence->prev_occurence = NULL;
    free(old);
    ghb[ghb_head] = NULL;
  }

  ghb_entry* entry = (ghb_entry *) malloc(sizeof(ghb_entry));
  entry->address = address;
  entry->index = ghb_head;
  entry->next_occurence = NULL;
  entry->prev_occurence = ghb_get_prev_occurence(address);
  if (entry->prev_occurence != NULL) entry->prev_occurence->next_occurence = entry;

  // Add entry
  ghb[ghb_head] = entry;

  return entry;
}

void prefetch_complete(Addr addr) {
}
