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

#define GHB_SIZE 1024
#define AIT_SIZE 512
#define DEPTH 5
#define WIDTH 100
#define STRIDED 4

struct delta_t {
  Addr delta;
  bool sign;
};

struct ghb_entry {
  Addr addr;
  bool valid;
  int16_t prev;
  int16_t next;
};

struct ait_entry {
  Addr pc;
  bool valid;
  int16_t entry;
};

//void ghb_add_entry(Addr addr);
void ghb_add_entry(Addr addr, Addr pc);
//int16_t ait_get_prev_ghb_entry(Addr addr, bool sign);
int16_t ait_get_prev_ghb_entry(Addr pc);
int16_t ghb_get_prev_entry(Addr addr);
delta_t calculate_delta(Addr x, Addr y);

ghb_entry* ghb = NULL;
int16_t ghb_head = -1;

ait_entry* ait = NULL;
int16_t ait_head = 0;

void prefetch_init(void) {
  ghb = (ghb_entry *) malloc(sizeof(ghb_entry) * GHB_SIZE);
  for (int16_t i = 0; i < GHB_SIZE; i++) {
    ghb[i].valid = false;
    ghb[i].prev = -1;
    ghb[i].next = -1;
  }
  ait = (ait_entry *) malloc(sizeof(ait_entry) * AIT_SIZE);
  for (int16_t i = 0; i < AIT_SIZE; i++)
    ait[i].valid = false;
}

int16_t ait_get_prev_ghb_entry(Addr pc) {//, bool sign) {
  int16_t entry;
  ait_entry* bucket;
  for (int16_t i = 0; i < AIT_SIZE; i++) {
    bucket = &ait[i];
    if (!bucket->valid) break;
    else if (bucket->pc == pc) {
      // Handle found entry
      entry = ait[i].entry;
      if (!ghb[bucket->entry].valid) entry = -1;
      ait[i].entry = ghb_head;
      return entry;
    }
  }

  bucket = &ait[ait_head];
  bucket->pc = pc;
  bucket->entry = ghb_head;
  bucket->valid = true;
  ait_head = (ait_head + 1) % AIT_SIZE;
  return -1;
  //int16_t hash = pc % AIT_SIZE;
  //ait_entry* bucket = &ait[hash];
  //int16_t b_pc = bucket->pc;
  //int16_t b_entry = bucket->entry;
  //bool b_valid = bucket->valid;
  //bucket->pc = pc;
  //bucket->entry = ghb_head;
  //bucket->valid = true;
  //// Not ait init && not overwritten ghb entry && equal deltas and signes
  //if (b_valid && ghb[b_entry].valid && b_pc == pc)// && b_sign == sign)
  //  return b_entry;
  //return -1;
}


void ghb_add_entry(Addr addr, Addr pc) {
  ghb_head = (ghb_head + 1) % GHB_SIZE;

  ghb_entry* entry = &ghb[ghb_head];
  // Handle existing entry
  if (entry->valid) {
    entry->valid = false;
    if (entry->next != -1)
      ghb[entry->next].prev = -1;
  }

  // Find prev entry
  int16_t prev;
  prev = ait_get_prev_ghb_entry(pc);

  // Handle pointer from previous to this
  if (prev != -1)
    ghb[prev].next = ghb_head;

  entry->prev = prev;
  entry->addr = addr;
  entry->valid = true;
  entry->next = -1;
}

delta_t calculate_delta(Addr x, Addr y) {
    delta_t d;
    if (x > y) {
      d.delta = x - y;
      d.sign = 0;
    } else {
      d.delta = y - x;
      d.sign = 1;
    }
    return d;
}


void prefetch_access(AccessStat stat) {
  /*
   * Traverse the linked list of previous occurences for the current candidate and fetch
   * prefetch candidates up to the prefetch depth
   * */
  Addr addr = stat.mem_addr;
  bool prefetched = get_prefetch_bit(addr);
  if (stat.miss || prefetched) {
    ghb_add_entry(addr, stat.pc);
    if (prefetched) {
     clear_prefetch_bit(addr);
     return;
    }

    //printf("Hello world!\n");

    // Find two-delta sequence
    Addr t_addr = addr;

    int16_t p = ghb[ghb_head].prev;
    if (p == -1) return;
    Addr t_p_addr = ghb[p].addr;
    delta_t d_p = calculate_delta(t_p_addr, t_addr);

    //printf("I am alive!\n");

    int16_t pp = ghb[p].prev;
    if (pp == -1) return;
    Addr t_pp_addr = ghb[pp].addr; 
    delta_t d_pp = calculate_delta(t_pp_addr, t_p_addr);
    
    //printf("LF delta sequence (%lu(%d), %lu(%d))\n", d_p.delta, d_p.sign, d_pp.delta, d_pp.sign);
    // Search for same two-delta sequence
    uint16_t width = 0;
    uint16_t depth = 0;
    int16_t t = p;
    int16_t t_p = pp;
    int16_t t_pp = ghb[pp].prev;
    delta_t t_d_p;
    delta_t t_d_pp;
    while (t_pp != -1 && depth < DEPTH && width++ < WIDTH) {
      t_addr = ghb[t].addr;
      t_p_addr = ghb[t_p].addr;
      t_pp_addr = ghb[t_pp].addr;
      t_d_p = calculate_delta(t_p_addr, t_addr);
      t_d_pp = calculate_delta(t_pp_addr, t_p_addr);
      if (
          t_d_p.delta == d_p.delta &&
          t_d_p.sign == d_p.sign &&
          t_d_pp.delta == d_pp.delta &&
          t_d_pp.sign == d_pp.sign
       ) {
        // Found!
        /* TODO:
         * While loop over next occurrences and fetch until prefetch depth
         * */
        //printf("Found on loop iteration %d! (%lu(%d), %lu(%d)):", t_d_p.delta, t_d_p.sign, t_d_pp.delta, t_d_pp.sign);
        int16_t c = t;
        int16_t n = ghb[c].next;
        Addr f_addr;
        Addr c_addr;
        Addr n_addr;
        delta_t n_d;
        //printf(" candidates: ");
        while (n != ghb_head && n != -1 && depth < DEPTH) {
          f_addr = addr;
          c_addr = ghb[c].addr;
          n_addr = ghb[n].addr;
          n_d = calculate_delta(c_addr, n_addr);
          c = n;
          n = ghb[c].next;
          if (!n_d.sign && f_addr > n_d.delta)
            f_addr -= n_d.delta;
          else if (n_d.sign && f_addr + n_d.delta < MAX_PHYS_MEM_ADDR)
            f_addr += n_d.delta;
          else
            continue;
          if (!in_cache(f_addr) && !in_mshr_queue(f_addr)) {
            issue_prefetch(addr);
          }
          depth++;
        }
      if (n == ghb_head || n == -1)
        break;
      }
      t = t_p;
      t_p = t_pp;
      t_pp = ghb[t_pp].prev;
      //printf("%d, ", t_pp);
    }
    //printf("\n");
  }
}


void prefetch_complete(Addr addr) {
  set_prefetch_bit(addr);
}


