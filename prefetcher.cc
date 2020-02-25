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
#define AIT_SIZE 512
#define GHB_SIZE 8
#define GHB_DEPTH 1
//
//void ghb_add_entry(Addr* address);

//Addr* queue;
//int q = -1;
//int init = true;
//

struct ghb_entry {
  Addr address;
  ghb_entry* prev_occurence;
  ghb_entry* next_occurence;
  int index;
};

struct ait_entry {
  Addr address;
  ghb_entry* ghb_occurence;
  int index;
};

//ghb_entry* ait_add_entry(Addr address, ghb_entry* new_entry);
//ait_entry* ait_add_entry(Addr address);
ghb_entry* ghb_add_entry(Addr address);
ghb_entry* ghb_get_last_occurence();

//ait_entry** ait = NULL;
//int ait_head = -1;

ghb_entry** ghb = NULL;
int ghb_head = -1;

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    
    //initialize_ghb();
    //queue = (Addr* ) malloc(sizeof(Addr) * );
    //ait = (ait_entry**) malloc(sizeof(ait_entry*) * AIT_SIZE);
    //for (unsigned int i = 0; i < AIT_SIZE; i++) {
    //  ait[i] = NULL;
    //}
    ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * GHB_SIZE);
    for (unsigned int i = 0; i < GHB_SIZE; i++) {
      ghb[i] = NULL;
    }
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

int depth = 0;

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
  ghb_entry* entry = ghb_add_entry(stat.mem_addr);
  //ghb_entry* prev_occurence = ghb_add_entry(stat.mem_addr)->prev_occurence;

  for (int i = 0; i < GHB_SIZE; i++) {
    if (ghb[i] != NULL) {
      printf("all good: %lu\n", ghb[i]->address);
      issue_prefetch(ghb[i]->address);
    }
      
  }
    //issue_prefetch(ghb[prev_occurence->index]->address);
  /*
   * Debugging stategy
   *
   * Go through all previous occurences
   * If any of them have an index that is not in the GHB -> Error!
   * */
      //if (ghb[i] != NULL) issue_prefetch(ghb[i]->address);
 // while (prev_occurence != NULL && depth < GHB_DEPTH) {
 //   // Get address of next element after previous occurence
 //   //Addr pf_addr = ghb[(prev_occurence->index + 1) % GHB_SIZE]->address;
 //   //bool test = false;
 //   //for (int i = 0; i < GHB_SIZE; i++) {
 //   //  if (ghb[(prev_occurence->index + 1) % GHB_SIZE] == ghb[i]) test = true;
 //   //} 
 //   //if (!test) fprintf(stderr,"SHOULD NOT HAPPEN!\n");
 //   //printf("prev_occurence: (%d,%lu)\n", prev_occurence->index, ghb[prev_occurence->index]->address);
 //   //if (ghb[prev_occurence->index] != NULL) issue_prefetch(ghb[prev_occurence->index]->address);
 //   //for (int i = 0; i < GHB_SIZE; i++) {
 //   //  if (ghb[i] != NULL) {
 //   //    printf("all good: %lu\n", ghb[i]->address);
 //   //  }
 //   //}
 //   //if (stat.miss && !in_cache(pf_addr)) {
 //   //issue_prefetch(pf_addr);
 //   //}
 //   //bool test = true;
 //   //for (int i = 0; i < GHB_SIZE; i++) {
 //   //  if (ghb[prev_occurence->index] == ghb[i]) test = false;
 //   //  //if (ghb[i] != NULL) {
 //   //  //  if (ghb[prev_occurence->index]->address == ghb[i]->address) test = true;
 //   //  //}
 //   //} 
 //   ////if (prev_occurence->index < 0 || prev_occurence->index > GHB_SIZE - 1) test = true;
 //   //if (test) fprintf(stderr,"SHOULD NOT HAPPEN!\n");
 //   //printf("all good: %lu\n", ghb[prev_occurence->index]->address);
 //   ///*
 //   // * Kan problemet være at du prøver å fetche minne som har blitt freet av applikasjonen?
 //   // * */
 //   for (int i = 0; i < GHB_SIZE; i++) {
 //     if (ghb[i] != NULL) issue_prefetch(ghb[i]->address);
 //   }
 //   //issue_prefetch(ghb[prev_occurence->index]->address);
 //   prev_occurence = prev_occurence->prev_occurence;
 //   depth++;
 // }
 // depth = 0;
  //ghb_entry* prev = ghb_add_entry(stat.mem_addr);
  //if (prev != NULL) {// && stat.miss && !in_cache(prev->address)) {
  //    issue_prefetch(prev->address);
  //}

}

unsigned int decremented_index(unsigned int index) {
    return (index - 1 + GHB_SIZE) % GHB_SIZE;
}

ghb_entry* ghb_get_last_occurence(Addr address) {
  size_t index = decremented_index(ghb_head);
  while (index != ghb_head) {
    ghb_entry* entry = ghb[index];
    if (entry == NULL) return NULL;
    else if (entry->address == address) return entry;
    index = decremented_index(index);
  }
  return NULL;
}

ghb_entry* ghb_add_entry(Addr address) {
  // Increment ghb head
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  // Remove old entry if exists
  ghb_entry* head = ghb[ghb_head];
  if (head != NULL) {
    if (head->next_occurence != NULL) {
      head->next_occurence->prev_occurence = NULL;
    }
    free(head);
  }
  ghb[ghb_head] = NULL;

  // Create new entry
  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
  entry->address = address;
  entry->index = ghb_head;
  entry->next_occurence = NULL;

  // Find previous occurence of address in ghb
  if (ghb_head > 0) {
    ghb_entry* prev_occurence = ghb_get_last_occurence(address);
    entry->prev_occurence = prev_occurence;
    if (prev_occurence != NULL) {
      // Link the relationship both ways to handle memory deallocation properly later
      prev_occurence->next_occurence = entry;
    }
  } else {
    entry->prev_occurence = NULL;
  }

  // Add the new entry
  ghb[ghb_head] = entry;

  // Return new ghb entry
  return entry;
}

//ghb_entry* ait_add_entry(Addr address, ghb_entry* new_entry) {
//ait_entry* ait_add_entry(Addr address) {
//  // If ait entry exists, update ghb_occurence with new_entry
//  ait_entry* entry = NULL;
//  //if (ait_head >= 0) {
//  //  while (entry == NULL) {
//  //    entry = ait[ait_head];
//  //    if (entry->address == new_entry->address) {
//  //      ghb_entry* old_entry = entry->ghb_occurence;
//  //      entry->ghb_occurence = new_entry;
//  //      return old_entry;
//  //    } else if (entry->prev == NULL) {
//  //      // Exhaustive search, no entry exists
//  //      break;
//  //    }
//  //    entry = entry->prev;
//  //  }
//  //}
//
//  // If ait entry not exists, create one
//  entry = (ait_entry*) malloc(sizeof(ait_entry));
//  entry->address = address;
//  //entry->ghb_occurence = new_entry;
//  if (ait_head >= 0) {
//    entry->prev = ait[ait_head];
//  } else {
//    entry->prev = NULL;
//  }
//  ait_head = (ait_head + 1) % AIT_SIZE;
//  ait[ait_head] = entry;
//  // Return prev ghb occurence
//  //return NULL;
//  return entry->prev;
//}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */

    // Thoughts: Set prefetch bit and use this for something?
    //           No need to set this bit if no usage
}
