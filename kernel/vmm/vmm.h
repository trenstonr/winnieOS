#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define BASE(x)	((x) & 0x000FFFFFFFFFF000)

void map_page(uint64_t *pml4, uint64_t va, uint64_t pa, uint64_t flags);
void unmap_page(uint64_t *pml4, uint64_t va);

uint64_t va2pa(uint64_t *pml4, uint64_t va);

#endif

