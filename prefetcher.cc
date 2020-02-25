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

//#define QUEUE_SIZE 10
#define AIT_SIZE 10
#define GHB_SIZE 10
//
//void ghb_add_entry(Addr* address);

//Addr* queue;
//int q = -1;
//int init = true;
//

struct ghb_entry {
  Addr address;
  ghb_entry* prev_occurence;
};

struct ait_entry {
  Addr address;
  ait_entry* prev;
  ghb_entry* ghb_occurence;
};

//ghb_entry* ait_add_entry(Addr address, ghb_entry* new_entry);
ait_entry* ait_add_entry(Addr address);
ghb_entry* ghb_add_entry(Addr address);

ait_entry** ait = NULL;
int ait_head = -1;

ghb_entry** ghb = NULL;
int ghb_head = -1;

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    
    //initialize_ghb();
    //queue = (Addr* ) malloc(sizeof(Addr) * );
    ait = (ait_entry**) malloc(sizeof(ait_entry*) * AIT_SIZE);
    ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * GHB_SIZE);
}

//ghb_entry* ghb_add_entry(Addr address) {
//  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
//  entry->address = address;
//  if (ghb_head >= 0) entry->prev = ghb[ghb_head];
//  else entry->prev = NULL;
//  ghb_head = (ghb_head + 1) % GHB_SIZE;
//  ghb[ghb_head] = entry;
//  return entry->prev;
//}

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
  //if (!init) {
  //  Addr pf_addr = queue[q];
  //  if (stat.miss && !in_cache(pf_addr)) {
  //      issue_prefetch(pf_addr);
  //  }
  //} else {
  //  init = false;
  //}
  //q = (q + 1) % QUEUE_SIZE;
  //queue[q] = stat.mem_addr;
  //ghb_entry* entry = ghb_add_entry(stat.mem_addr);
  ait_entry* entry = ait_add_entry(stat.mem_addr);
  if (entry != NULL) {
    Addr pf_addr = entry->address;
    if (stat.miss && !in_cache(pf_addr)) {
        issue_prefetch(pf_addr);
    }
  }
  //ghb_entry* prev = ghb_add_entry(stat.mem_addr);
  //if (prev != NULL) {// && stat.miss && !in_cache(prev->address)) {
  //    issue_prefetch(prev->address);
  //}

}

ghb_entry* ghb_add_entry(Addr address) {
  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
  entry->address = address;
  // Call ait_add_entry to get prev ghb occurence
  //entry->prev_occurence = ait_add_entry(address, entry);

  ghb_head = (ghb_head + 1) % GHB_SIZE;
  ghb[ghb_head] = entry;
  // Return new ghb entry
  return entry;
}

//ghb_entry* ait_add_entry(Addr address, ghb_entry* new_entry) {
ait_entry* ait_add_entry(Addr address) {
  // If ait entry exists, update ghb_occurence with new_entry
  ait_entry* entry = NULL;
  //if (ait_head >= 0) {
  //  while (entry == NULL) {
  //    entry = ait[ait_head];
  //    if (entry->address == new_entry->address) {
  //      ghb_entry* old_entry = entry->ghb_occurence;
  //      entry->ghb_occurence = new_entry;
  //      return old_entry;
  //    } else if (entry->prev == NULL) {
  //      // Exhaustive search, no entry exists
  //      break;
  //    }
  //    entry = entry->prev;
  //  }
  //}

  // If ait entry not exists, create one
  entry = (ait_entry*) malloc(sizeof(ait_entry));
  entry->address = address;
  //entry->ghb_occurence = new_entry;
  if (ait_head >= 0) {
    entry->prev = ait[ait_head];
  } else {
    entry->prev = NULL;
  }
  ait_head = (ait_head + 1) % AIT_SIZE;
  ait[ait_head] = entry;
  // Return prev ghb occurence
  //return NULL;
  return entry->prev;
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */

    // Thoughts: Set prefetch bit and use this for something?
    //           No need to set this bit if no usage
}
