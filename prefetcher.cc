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

#define GHB_SIZE 64
#define AIT_SIZE 64

// Data structures
struct ghb_entry {
  // Address of entry
  Addr key;
  // Previous occurence of this address
  ghb_entry* prev_occurence;
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
void ghb_add_entry(Addr key);
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
}

void prefetch_access(AccessStat stat)
{
    /* pf_addr is now an address within the _next_ cache block */
    Addr pf_addr = stat.mem_addr + BLOCK_SIZE;

    /*
     * Issue a prefetch request if a demand miss occured,
     * and the block is not already in cache.
     */
    if (stat.miss && !in_cache(pf_addr)) {
        issue_prefetch(pf_addr);
    }

    // Add the entry to the GHB
    ghb_add_entry(stat.mem_addr);
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
int ait_head_index = -1;

void initialize_ait() {
  ait = (ait_entry**) malloc(sizeof(ait_entry*) * (AIT_SIZE + 1)); // Extra space for sentinel
}

/*
 * Returns the ghb entry of the last occured of 'key' if previously accessed
 * */
ghb_entry* ait_add_entry(Addr key, ghb_entry* ghb_occurence) {
  // If already exists, don't add entry, just update ghb_index
  ait_entry* entry = NULL;
  
  // If ait is not empty, search for previous occurence
  if (ait_head_index >= 0) {
    while (entry == NULL) {
      ait_entry* candidate = ait[ait_head_index];
      if (candidate->key == key) {
        // Occurence found, save this occurence's ghb_index and return
        ghb_entry* prev_occurence = candidate->ghb_occurence;
        candidate->ghb_occurence = ghb_occurence;
        return prev_occurence;
      } else if (candidate->prev == NULL) {
        // No occurences found, proceed to add entry
        break;
      }
    }
  }

  // Add ait entry
  ait_head_index = (ait_head_index + 1) % AIT_SIZE;
  entry = (ait_entry*) malloc(sizeof(ait_entry));
  *entry = (ait_entry) { .key = key, .ghb_occurence = ghb_occurence };
  if (ait_head_index == 0) {
    entry->prev = NULL;
  } else {
    entry->prev = ait[(ait_head_index - 1 + AIT_SIZE) % AIT_SIZE];
  }
  ait[ait_head_index] = entry;
  return NULL;
}


/* 
 * GLOBAL HISTORY BUFFER (GHB)
 * */

ghb_entry** ghb = NULL;
int ghb_head_index = -1;

void initialize_ghb() {
  ghb = (ghb_entry**) malloc(sizeof(ghb_entry*) * (GHB_SIZE + 1)); // Extra space for sentinel
}

void ghb_add_entry(Addr key) {
  ghb_head_index = (ghb_head_index + 1) % GHB_SIZE;

  ghb_entry* entry = (ghb_entry*) malloc(sizeof(ghb_entry));
  *entry = (ghb_entry) { .key = key };

  if (ghb_head_index >= 0) {
    // Find last occurence in address index table
    entry->prev_occurence = ait_add_entry(key, entry);
  } else {
    entry->prev_occurence = NULL;
  }

  ghb[ghb_head_index] = entry;
}
