/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"
#include <map>

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

#define GHB_SIZE 64
#define GHB_DEPTH 1

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

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * GHB_SIZE);
    for (unsigned int i = 0; i < GHB_SIZE; i++) {
      ghb[i] = NULL;
    }
}

void prefetch_access(AccessStat stat){
  //Addr pf_addr = stat.mem_addr + BLOCK_SIZE;
  //////
  ///////*
  ////// * Issue a prefetch request if a demand miss occured,
  ////// * and the block is not already in cache.
  ////// */
  //if (stat.miss && !in_cache(pf_addr)) {
  //    issue_prefetch(pf_addr);
  //}
  //ghb_entry* entry = ghb_add_entry(stat.mem_addr);
  //printf("Hello!\n");
  //for (int i = 0; i < GHB_SIZE; i++) {
  //  //printf("(%d, %lu)", ghb[i], ghb[i]);
  //  if (ghb[i] != NULL) {
  //    //printf("(%d, %lu)", ghb[i]->index, ghb[i]->address);
  //    issue_prefetch(ghb[i]->address);
  //  }
  //}
  //if (entry != NULL) {
  //  Addr pf_addr = entry->address;
  //  if (stat.miss && !in_cache(pf_addr)) {
  //    issue_prefetch(pf_addr);
  //  }
  //}
}

ghb_entry* ghb_get_prev_occurence(Addr address) {
//  if (ghb_head == -1) return NULL;
//  int index = (ghb_head - 1 + GHB_SIZE) % GHB_SIZE;
//  while (index != ghb_head) {
//    ghb_entry* entry = ghb[index];
//    if (entry == NULL) return NULL;
//    else if (entry->address == address) return entry;
//    index = (index - 1 + GHB_SIZE) % GHB_SIZE;
//  }
  return NULL;
}

ghb_entry* ghb_add_entry(Addr address) {
  // Increment ghb
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  //// Handle existing entry
  //ghb_entry* old = ghb[ghb_head];
  //if (old != NULL) {
  //  // Reset next_occurence's pointer to this struct
  //  if (old->next_occurence != NULL) old->next_occurence->prev_occurence = NULL;
  //  free(old);
  //  ghb[ghb_head] = NULL;
  //}

  ghb_entry* entry = (ghb_entry *) malloc(sizeof(ghb_entry));
  entry->address = address;
  entry->index = ghb_head;
  ////entry->next_occurence = NULL;
  ////entry->prev_occurence = ghb_get_prev_occurence(address);
  ////if (entry->prev_occurence != NULL) entry->prev_occurence->next_occurence = entry;

  //// Add entry
  ghb[ghb_head] = entry;

  //return entry;
  return NULL;
}
















//unsigned int decremented_index(unsigned int index) {
//    return (index - 1 + GHB_SIZE) % GHB_SIZE;
//}
//
//ghb_entry* ghb_get_last_occurence(Addr address) {
//  size_t index = decremented_index(ghb_head);
//  while (index != ghb_head) {
//    ghb_entry* entry = ghb[index];
//    if (entry == NULL) return NULL;
//    else if (entry->address == address) return entry;
//    index = decremented_index(index);
//  }
//  return NULL;
//}
//
//ghb_entry* ghb_add_entry(Addr address) {
//  // Increment ghb head
//  ghb_head = (ghb_head + 1) % GHB_SIZE;
//
//  // Remove old entry if exists
//  ghb_entry* head = ghb[ghb_head];
//  if (head != NULL) {
//    if (head->next_occurence != NULL) {
//      head->next_occurence->prev_occurence = NULL;
//    }
//    free(head);
//  }
//  ghb[ghb_head] = NULL;
//
//  // Create new entry
//  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
//  entry->address = address;
//  entry->index = ghb_head;
//  entry->next_occurence = NULL;
//
//  // Find previous occurence of address in ghb
//  if (ghb_head > 0) {
//    ghb_entry* prev_occurence = ghb_get_last_occurence(address);
//    entry->prev_occurence = prev_occurence;
//    if (prev_occurence != NULL) {
//      // Link the relationship both ways to handle memory deallocation properly later
//      prev_occurence->next_occurence = entry;
//    }
//  } else {
//    entry->prev_occurence = NULL;
//  }
//
//  // Add the new entry
//  ghb[ghb_head] = entry;
//
//  // Return new ghb entry
//  return entry;
//}


void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */

    // Thoughts: Set prefetch bit and use this for something?
    //           No need to set this bit if no usage
}
