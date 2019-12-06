//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <math.h>


//
// TODO:Student Information
//
const char *studentName = "SUMIRAN SHUBHI";
const char *studentID   = "A53314039";
const char *email       = "sshubhi@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

typedef struct set {
	uint32_t* block;
	int size;
} set;


set* icache;
set* dcache;
set* l2cache;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

uint32_t get_index_from_addr (uint32_t addr, uint32_t num_sets) {
	if (num_sets == 1)
		return 1;
	else {
		int index_bits = int(log2(num_sets - 1)) + 1;
		int block_offset_bits = int(log2(blocksize - 1)) + 1;
		uint32_t index = addr >> block_offset_bits;
		uint32_t mask = 1;
		
		for (int i = 0; i < index_bits-1; i++) {
			mask = mask << 1;
			mask = mask | 1;
		}
		index = index & mask;
		return index;		
	}
}

uint32_t get_tag_from_addr (uint32_t addr, uint32_t num_sets) {
		int index_bits = int(log2(num_sets - 1)) + 1;
		int block_offset_bits = int(log2(blocksize - 1)) + 1;
		uint32_t tag = addr >> block_offset_bits;
		tag = tag >> index_bits;
		return tag;
	
}

void remove_cache_line (set* cache, uint32_t addr, uint32_t num_sets) {
  	int index = get_index_from_addr(addr, num_sets);
	uint32_t tag   = get_tag_from_addr(addr, num_sets);	
	int b_idx = -1;

	for (int i=0; i<cache[index].size; i++) {
		if (tag == cache[index].block[i]) {
			b_idx = i;
			break;
		}
	}
	
	if (b_idx != -1) {	// If line is present in L1 cache
		for (int i = b_idx; i <  (cache[index].size - 1); i++) {
			cache[index].block[i] = cache[index].block[i+1];
		}
		cache[index].size--;
	}	
}
// Initialize the Cache Hierarchy
//
void
init_cache()
{
	// Initialize cache stats
	icacheRefs        = 0;
	icacheMisses      = 0;
	icachePenalties   = 0;
	dcacheRefs        = 0;
	dcacheMisses      = 0;
	dcachePenalties   = 0;
	l2cacheRefs       = 0;
	l2cacheMisses     = 0;
	l2cachePenalties  = 0;
  
    icache = (set*) malloc(icacheSets*sizeof(set));
	dcache = (set*) malloc(dcacheSets*sizeof(set));
    l2cache = (set*) malloc(l2cacheSets*sizeof(set));

	
	for (int i=0; i < icacheSets; i++) {
		icache[i].block = (uint32_t*) malloc(icacheAssoc*sizeof(uint32_t));
		icache[i].size = 0;
	}
	for (int i=0; i < dcacheSets; i++) {
		dcache[i].block = (uint32_t*) malloc(dcacheAssoc*sizeof(uint32_t));
		dcache[i].size = 0;
	}
	for (int i=0; i < l2cacheSets; i++) {
		l2cache[i].block = (uint32_t*) malloc(l2cacheAssoc*sizeof(uint32_t));
		l2cache[i].size = 0;
	}
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  	int index = get_index_from_addr(addr, icacheSets);
	uint32_t tag   = get_tag_from_addr(addr, icacheSets);	
	int b_idx = -1;
	int size = icache[index].size;

	
	for (int i=0; i<icache[index].size; i++) {
		if (tag == icache[index].block[i]) {
			b_idx = i;
			break;
		}
	}
	
	if (b_idx == -1) {    //Miss. It is a a new line
		//l2cache_access(addr);
		if (icache[index].size == 0) {		// First tag element, goes to the head of the queue
			icache[index].block[0] = tag;
			icache[index].size++;
		}
		else if (icache[index].size < icacheAssoc) { 	// No need to eliminate.
			icache[index].block[size] = tag;
			icache[index].size++;
		}
		else if (icache[index].size == icacheAssoc) {
			for (int i=1; i< icache[index].size; i++) {
				icache[index].block[i-1] = icache[index].block[i];	
			}
			icache[index].block[size-1] = tag;
		}	
	}
	else {		// It is a hit. No size updates. Reshuffling to satisfy LRU policy.
		
		if (icache[index].size == 1) {;}
		
		else if (icache[index].size < icacheAssoc) {
			for (int i = b_idx; i< icache[index].size - 1; i++) {
				icache[index].block[i] = icache[index].block[i+1];	
			}
			icache[index].block[size-1] = tag;
		}
		else if (icache[index].size == icacheAssoc) {
			for (int i = b_idx; i< icache[index].size - 1; i++) {
				icache[index].block[i] = icache[index].block[i+1];	
			}
			icache[index].block[size-1] = tag;
		}
	}
  return memspeed;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
   	int index = get_index_from_addr(addr, dcacheSets);
	uint32_t tag   = get_tag_from_addr(addr, dcacheSets);	
	int b_idx = -1;
	int size = dcache[index].size;
	
	for (int i=0; i<dcache[index].size; i++) {
		if (tag == dcache[index].block[i]) {
			b_idx = i;
			break;
		}
	}
	
	if (b_idx == -1) {    //Miss. It is a a new line
		//l2cache_access(addr);
		if (dcache[index].size == 0) {		// First tag element, goes to the head of the queue
			dcache[index].block[0] = tag;
			dcache[index].size++;
		}
		else if (dcache[index].size < icacheAssoc) { 	// No need to eliminate.
			dcache[index].block[size] = tag;
			dcache[index].size++;
		}
		else if (dcache[index].size == icacheAssoc) {
			for (int i=1; i< dcache[index].size; i++) {
				dcache[index].block[i-1] = dcache[index].block[i];	
			}
			dcache[index].block[size-1] = tag;
		}	
	}
	else {		// It is a hit. No size updates. Reshuffling to satisfy LRU policy.
		
		if (dcache[index].size == 1) {;}
		
		else if (dcache[index].size < icacheAssoc) {
			for (int i = b_idx; i< dcache[index].size - 1; i++) {
				dcache[index].block[i] = dcache[index].block[i+1];	
			}
			dcache[index].block[size-1] = tag;
		}
		else if (dcache[index].size == icacheAssoc) {
			for (int i = b_idx; i< dcache[index].size - 1; i++) {
				dcache[index].block[i] = dcache[index].block[i+1];	
			}
			dcache[index].block[size-1] = tag;
		}
	}
  return memspeed;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  	int index = get_index_from_addr(addr, l2cacheSets);
	uint32_t tag   = get_tag_from_addr(addr, l2cacheSets);	
	int b_idx = -1;
	int size = l2cache[index].size;	
	
	for (int i=0; i<l2cache[index].size; i++) {
		if (tag == l2cache[index].block[i]) {
			b_idx = i;
			break;
		}
	}
	
	if (b_idx == -1) {    //Miss. It is a a new line
		if (l2cache[index].size == 0) {		// First tag element, goes to the head of the queue
			l2cache[index].block[0] = tag;
			l2cache[index].size++;
		}
		else if (l2cache[index].size < icacheAssoc) { 	// No need to eliminate.
			l2cache[index].block[size] = tag;
			l2cache[index].size++;
		}
		else if (l2cache[index].size == icacheAssoc) {
			for (int i=1; i< l2cache[index].size; i++) {
				l2cache[index].block[i-1] = l2cache[index].block[i];	
			}
			l2cache[index].block[size-1] = tag;
		}	
		if (inclusive) {
			remove_cache_line(icache, addr, icacheSets);
			remove_cache_line(dcache, addr, dcacheSets);
		}
	}
	else {		// It is a hit. No size updates. Reshuffling to satisfy LRU policy.
		
		if (l2cache[index].size == 1) {;}
		
		else if (l2cache[index].size < icacheAssoc) {
			for (int i = b_idx; i< l2cache[index].size - 1; i++) {
				l2cache[index].block[i] = l2cache[index].block[i+1];	
			}
			l2cache[index].block[size-1] = tag;
		}
		else if (l2cache[index].size == icacheAssoc) {
			for (int i = b_idx; i< l2cache[index].size - 1; i++) {
				l2cache[index].block[i] = l2cache[index].block[i+1];	
			}
			l2cache[index].block[size-1] = tag;
		}
	}
  return memspeed;
}
