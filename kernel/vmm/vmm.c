#include <stdint.h>
#include <lib/printf.h>
#include "vmm.h"
#include "../pmm/pmm.h"

#define BASE(x)	((x) & 0x000FFFFFFFFFF000)

uint64_t alloc_entry() {
	uint64_t frame = pmm_alloc_frame();
	if (frame == 0) printf("\nWARNING: failed to alloc phy frame in alloc_entry()");	// should definitely handle this better	
	for (int i = 0; i < 512; i++) ((uint64_t *)frame)[i] = 0;	// zero garbage							
	return frame | 0x3;	// P|W
}

void map_page(uint64_t *pml4, uint64_t va, uint64_t pa, uint64_t flags) {
	uint64_t pml4_i = (va >> 39) & 0x1FF;
	uint64_t pdpt_i = (va >> 30) & 0x1FF;
	uint64_t pd_i	= (va >> 21) & 0x1FF;
	uint64_t pt_i	= (va >> 12) & 0x1FF;

	uint64_t *entry;

	// pml4 entry
	entry = &pml4[pml4_i];
	if (!(*entry & 0x1)) *entry = alloc_entry();	// if P bit not set

	// pdpt entry
	entry = &((uint64_t *)BASE(*entry))[pdpt_i];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// pd entry
	entry = &((uint64_t *)BASE(*entry))[pd_i];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// pt entry
	entry = &((uint64_t *)BASE(*entry))[pt_i];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// map page in pt
	*entry = pa | flags | 0x1;
}
