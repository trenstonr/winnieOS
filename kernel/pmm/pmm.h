#ifndef PPM_H
#define PPM_H

#include <stdint.h>

#define KERNEL_VMA 0xFFFFFFFF80000000UL

void pmm_init(uint32_t multiboot_magic, uint32_t multiboot_info_addr);

uint64_t pmm_alloc_frame(void);
void pmm_free_frame(uint64_t addr);

uint64_t pmm_free_count(void);	// number of free 4 KiB frames

#endif
