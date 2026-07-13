#include <stdint.h>
#include <lib/printf.h>
#include "vmm.h"
#include "../pmm/pmm.h"

#define PML4_IDX(va) 	((va >> 39) & 0x1FF)
#define PDPT_IDX(va) 	((va >> 30) & 0x1FF)
#define PD_IDX(va)	((va >> 21) & 0x1FF)
#define	PT_IDX(va)	((va >> 12) & 0x1FF)

uint64_t alloc_entry() {
	uint64_t frame = pmm_alloc_frame();
	if (frame == 0) printf("\nWARNING: failed to alloc phy frame in alloc_entry()");	// should definitely handle this better	
	for (int i = 0; i < 512; i++) ((uint64_t *)frame)[i] = 0;	// zero garbage							
	return frame | 0x3;	// P|W
}

void map_page(uint64_t *pml4, uint64_t va, uint64_t pa, uint64_t flags) {
	uint64_t *entry;

	// pml4 entry
	entry = &pml4[PML4_IDX(va)];
	if (!(*entry & 0x1)) *entry = alloc_entry();	// if P bit not set

	// pdpt entry
	entry = &((uint64_t *)BASE(*entry))[PDPT_IDX(va)];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// pd entry
	entry = &((uint64_t *)BASE(*entry))[PD_IDX(va)];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// pt entry
	entry = &((uint64_t *)BASE(*entry))[PT_IDX(va)];
	if (!(*entry & 0x1)) *entry = alloc_entry();

	// map page in pt
	*entry = pa | flags | 0x1;
}

void unmap_page(uint64_t *pml4, uint64_t va) {
	uint64_t *entry;

	// pml4 entry
	entry = &pml4[PML4_IDX(va)];
	if (!(*entry & 0x1)) return; 

	// pdpt entry
	entry = &((uint64_t *)BASE(*entry))[PDPT_IDX(va)];
	if (!(*entry & 0x1)) return;

	// pd entry
	entry = &((uint64_t *)BASE(*entry))[PD_IDX(va)];
	if (!(*entry & 0x1)) return; 

	// pt entry
	entry = &((uint64_t *)BASE(*entry))[PT_IDX(va)];
	if (!(*entry & 0x1)) return;

	*entry = 0; 
	__asm__ volatile ("invlpg (%0)" : : "r"(va) : "memory");	// crazy wacky assembly

}

uint64_t va2pa(uint64_t *pml4, uint64_t va) {
	uint64_t *entry;

	// pml4 entry
	entry = &pml4[PML4_IDX(va)];
	if (!(*entry & 0x1)) return -1; 

	// pdpt entry
	entry = &((uint64_t *)BASE(*entry))[PDPT_IDX(va)];
	if (!(*entry & 0x1)) return -1;

	// pd entry
	entry = &((uint64_t *)BASE(*entry))[PD_IDX(va)];
	if (!(*entry & 0x1)) return -1; 

	// pt entry
	entry = &((uint64_t *)BASE(*entry))[PT_IDX(va)];
	if (!(*entry & 0x1)) return -1;

	return BASE(*entry) | (va & 0xFFF);
}
