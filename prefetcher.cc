/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include "stdlib.h"
#include "stdio.h"
//#include <map>
//#include <algorithm>
#include <queue>

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
using namespace std;

#define GHB_SIZE 4096 // uint16_t
#define AIT_SIZE 2048 // uint16_t
#define MARKOV_SIZE 10 // uint8_t
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
  Addr address;
  bool valid;
  // Index of last GHB entry
  int16_t entry;
};

struct markov_entry {
  Addr address;
  bool valid;
  uint8_t count;
};

void ghb_add_entry(Addr address);
int16_t ghb_get_prev_occurence(Addr address);

ghb_entry* ghb = NULL;
int16_t ghb_head = -1;

ait_entry* ait = NULL;
uint16_t ait_head = 0;

markov_entry* markov = NULL;
uint8_t markov_head = 0;

class markovComparator {
  public:
    int operator() (const markov_entry* e1, const markov_entry* e2) {
      return e1->count > e2->count;
    }
};

priority_queue <markov_entry*, vector<markov_entry*>, markovComparator> pq;

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
    markov = (markov_entry*) malloc(sizeof(markov_entry) * MARKOV_SIZE);
}

int16_t ghb_get_prev_occurence(Addr address) {
  // Search for previous entry
  int16_t entry;
  for (uint16_t i = 0; i < AIT_SIZE; i++) {
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
  ait[ait_head].address = address;
  ait[ait_head].entry = ghb_head;
  ait[ait_head].valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
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
  int16_t prev = ghb[ghb_head].prev;

  Addr candidate = 0;
  if (prev == -1 ) {
    candidate = stat.mem_addr + BLOCK_SIZE;
    if (!in_cache(candidate) && !in_mshr_queue(candidate)) issue_prefetch(candidate);
    return;
  }

  int16_t next;

  // Initialize the data structures
  for (uint8_t i = 0; i < MARKOV_SIZE; i++) {
    markov[i].valid = false;
  }
  // Loop throught the GHB linked list of previous occurences
  while (prev != -1) {
    next = (prev + 1) % GHB_SIZE;
    // Increment markov entry if exists
    bool exists = false;
    for (uint8_t i = 0; i < MARKOV_SIZE; i++) {
      if (markov[i].valid && markov[i].address == ghb[next].address) {
        markov[i].count++;
        exists = true;
        break;
      } 
    }
    // If markov entry not exists, create entry
    if (!exists && markov_head < MARKOV_SIZE) {
      markov[markov_head].address = ghb[next].address;
      markov[markov_head].count = 1;
      markov[markov_head++].valid = true;
    }
    prev = ghb[prev].prev;
  }
  markov_head = 0;

  /*
   * This is for prefetch degree = 1
   * */
  //uint8_t i;
  //uint8_t count = 0;
  //for (i = 0; i < MARKOV_SIZE; i++) {
  //  if (!markov[i].valid) break;
  //  else if (markov[i].count > count) {
  //    candidate = markov[i].address;
  //    count = markov[i].count;
  //  }
  //}
  //if (!in_cache(candidate) && !in_mshr_queue(candidate)) {
  //  issue_prefetch(candidate);
  //}

  // Heap of best candidates
  for (uint8_t i = 0; i < MARKOV_SIZE; i++) {
    if (!markov[i].valid) break;
    if (pq.size() < DEPTH) {
      pq.push(&markov[i]);
    } else if (pq.top()->count < markov[i].count) {
      pq.pop();
      pq.push(&markov[i]);
    }
  }

  // Fetch best candidates
  for (uint8_t i = 0; i < DEPTH; i++) {
    if (!markov[i].valid) break;
    Addr candidate = pq.top()->address;
    if (!in_cache(candidate)) {// && !in_mshr_queue(candidate)) {
      issue_prefetch(candidate);
    }
    pq.pop();
  }

}

void prefetch_complete(Addr addr) {
}
