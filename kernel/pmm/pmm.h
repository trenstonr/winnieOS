#ifndef PPM_H
#define PPM_H

#include <stdint.h>

void pmm_init(uint32_t multiboot_magic, uint32_t multiboot_info_addr);

uint64_t pmm_alloc_frame(void);
void pmm_free_frame(uint64_t addr);

#endif
