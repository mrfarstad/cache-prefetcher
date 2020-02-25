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

#define GHB_SIZE 10
#define AIT_SIZE 10
#define GHB_DEPTH 1
//#define PREFETCH_PROBABILITY_THRESHOLD 0.3

// Data structures
struct ghb_entry {
  // Address of entry
  Addr key;
  // Previous occurence of this address
  ghb_entry* prev_occurence;
  // Next ghb element
  ghb_entry* next;
};

struct ait_entry {
  // Address of entry
  Addr key;
  // Last occured GHB entry in GHB
  ghb_entry* ghb_occurence;
  // Prev ait_entry
  ait_entry* prev;
};

// Function prototypes
void initialize_ghb();
ghb_entry* ghb_add_entry(Addr key);
void initialize_ait();
ghb_entry* ait_add_entry(Addr key, ghb_entry* ghb_occurence);


/*
 * PREFETCHER
 * */

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
    
    initialize_ghb();
    initialize_ait();
}

void prefetch_access(AccessStat stat)
{
    /////* pf_addr is now an address within the _next_ cache block */
    //Addr pf_addr = stat.mem_addr + BLOCK_SIZE;
    ////
    /////*
    //// * Issue a prefetch request if a demand miss occured,
    //// * and the block is not already in cache.
    //// */
    //if (stat.miss && !in_cache(pf_addr)) {
    //    issue_prefetch(pf_addr);
    //}
    //// Add the entry to the GHB
    ghb_entry* entry = ghb_add_entry(stat.mem_addr);
    //// Note: unordered_map requieres C++11, which seems to ble not available
    ////std::map<Addr, float> occurence_probabilities;
    unsigned int depth = 0;
    // Handle probabilities of next accesses after previously access of same address
    //if (stat.miss) {
    while (entry != NULL && entry->prev_occurence != NULL && depth < GHB_DEPTH) {
      entry = entry->prev_occurence;
      // TODO: REMOVE THIS
      if (entry->next != NULL) {
        issue_prefetch(entry->next->key);
    //    Addr pf_addr = entry->next->key;
    //    //if (!in_cache(pf_addr) && !in_mshr_queue(pf_addr)) issue_prefetch(pf_addr);
    //    issue_prefetch(pf_addr);
    //    //occurence_probabilities[entry->next->key] += 1 / GHB_DEPTH;
      }
    //  depth++;
    //}
    }

    ///*
    // * Issue a prefetch request if a demand miss occured,
    // * and the block is not already in cache.
    // */
    //if (stat.miss && !in_cache(entry->key)) {
    //  // Fetch every element in map above given threshold
    //  std::map<Addr, float>::iterator prob;
    //  for (prob = occurence_probabilities.begin(); prob != occurence_probabilities.end(); prob++) {
    //    if (prob->second > PREFETCH_PROBABILITY_THRESHOLD) {
    //      issue_prefetch(prob->first);
    //    }
    //  }
    //}
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */

    // Thoughts: Set prefetch bit and use this for something?
    //           No need to set this bit if no usage
}

/*
 * ADDRESS INDEX TABLE
 * - This table contains each previously accessed address and a pointer
 *   to the last access in the GHB
 * - This makes the search for last occurences shorter than searching throught the GHB
 * */

ait_entry** ait = NULL;
std::map<Addr, float> ait_table;
int ait_head_index = -1;
unsigned int ait_allocated = 0;

void initialize_ait() {
  ait = (ait_entry**) malloc(sizeof(ait_entry*) * (AIT_SIZE + 1)); // Extra space for sentinel
}

/*
 * Returns the ghb entry of the last occured of 'key' if previously accessed
 * */
ghb_entry* ait_add_entry(Addr key, ghb_entry* ghb_occurence) {
  // If already exists, don't add entry, just update ghb_occurence
  ait_entry* entry = NULL;

  //// TODO: If entry exists, find pointer to ghb entry
  //if(ait_table.count(key) > 0) {
  //  entry = ait_table[key];
  //}
  //// TODO: If not exists, create a new entry in ait_table
  //else {
  //  entry = (ait_entry*) malloc(sizeof(ait_entry));
  //  entry->key = key;
  //  entry->ghb_occurence = ghb_occurence;
  //  //if (ait_head_index == -1) {
  //  //  entry->prev = NULL;
  //  //} else {
  //  //  entry->prev = ait[ait_head_index];
  //  //}
  //  //ait_head_index = (ait_head_index + 1) % AIT_SIZE;
  //  ////if (ait_allocated >= AIT_SIZE) free(ait[ait_head_index]);
  //  ////else ait_allocated++;
  //  //ait[ait_head_index] = entry;
  //  ait_table[key] = entry;
  //}


  ////
  ////// If ait is not empty, search for previous occurence
  if (ait_head_index >= 0) {
    ait_entry* candidate = ait[ait_head_index];
    while (entry == NULL) {
      if (candidate->key == key) {
  //      // Occurence found, save pointer to occurence and return
        ghb_entry* prev_occurence = candidate->ghb_occurence;
        candidate->ghb_occurence = ghb_occurence;
        return prev_occurence;
      } else if (candidate->prev == NULL) {
  //      // No occurences found, proceed to add entry
        break;
      }
      entry = candidate->prev;
    }
  }

  //// Add ait entry
  entry = (ait_entry*) malloc(sizeof(ait_entry));
  entry->key = key;
  entry->ghb_occurence = ghb_occurence;

  if (ait_head_index == -1) {
    entry->prev = NULL;
  } else {
    entry->prev = ait[ait_head_index];
  }
  ait_head_index = (ait_head_index + 1) % AIT_SIZE;
  if (ait_allocated >= AIT_SIZE) free(ait[ait_head_index]);
  else ait_allocated++;
  ait[ait_head_index] = entry;
  return NULL;
}


/* 
 * GLOBAL HISTORY BUFFER (GHB)
 * */

ghb_entry** ghb = NULL;
int ghb_head_index = -1;
unsigned int ghb_allocated = 0;

void initialize_ghb() {
  ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * (GHB_SIZE + 1)); // Extra space for sentinel
}

ghb_entry* ghb_add_entry(Addr key) {

  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
  entry->key = key;
  entry->next = NULL;

  if (ghb_head_index >= 0) {
    // Find last occurence in GHB from address index table
    entry->prev_occurence = ait_add_entry(key, entry);

    // Make previous entry->next point to this entry
    ghb[ghb_head_index]->next = entry;
  } else {
    // No previous occurences if GHB is empty
    entry->prev_occurence = NULL;
  }

  ghb_head_index = (ghb_head_index + 1) % GHB_SIZE;
  if (ghb_allocated >= GHB_SIZE) free(ghb[ghb_head_index]);
  else ghb_allocated++;
  ghb[ghb_head_index] = entry;
  return entry;
}
