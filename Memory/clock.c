#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int clock;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int evicted = -1;
	while (evicted == -1) {
		if (clock == memsize){
			clock = 0;
		}
		if (coremap[clock].ref_bit == 0) {
			evicted = clock;
		} else {
			coremap[clock].ref_bit = 0;
		}
		clock += 1;
	}
	return evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	int i = 0;
	while (i < memsize) {
		if (coremap[i].in_use) {
			if (coremap[i].pte == p) {
				coremap[i].ref_bit = 1;
			}
		}
		i += 1;
	}
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock = 0;
}
