#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int least_used;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int i = 0;
	int evicted = -1;
	int min = 99999999;

	while (i < memsize) {
		if (coremap[i].in_use) {
			if (coremap[i].least_counter < min) {
				min = coremap[i].least_counter;
				evicted = i;
			}
		}
		i += 1;
	}


	return evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int i = 0;
	while (i < memsize) {
		if (coremap[i].in_use) {
			if (coremap[i].pte == p) {
				coremap[i].least_counter = least_used;
			}
		}
		i += 1;
	}
	least_used += 1;
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	least_used = 0;
}
