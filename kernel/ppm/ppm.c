#include "ppm.h"
#include <stddef.h>
#include <lib/printf.h>

extern char kernel_end;

typedef struct {
	uint32_t type;
	uint32_t size;
} GenericTag;

typedef struct {
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
	uint32_t reserved;
} MemoryMapEntry;

typedef struct {
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	MemoryMapEntry entries[];
} MemoryMap;

void ppm_init(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
	if (multiboot_magic != 0x36d76289) {
		printf("\nOS loaded by a non-Multiboot2-compliant boot loader.");
		return;
	}
	
	GenericTag *tag = (GenericTag *)(multiboot_info_addr + 8);
	while (tag->type != 6) {
		if (tag->type == 0) {
			printf("\nMemory map tag not found in multiboot info struct");
			return;
		}

		uint8_t *next = (uint8_t *)tag + tag->size;
		next = (uint8_t *)(((uintptr_t)next + 7) & ~7);	// 8-byte alligned
		tag = (GenericTag *)(next);
	}

	uint64_t max_addr = 0;
	MemoryMap *mmap = (MemoryMap *)tag;

	// find max address of mmap
	for (size_t i = 0; i < (mmap->size - sizeof(MemoryMap)) / mmap->entry_size; i++) {
		MemoryMapEntry entry = mmap->entries[i];
		if (entry.base_addr + entry.length > max_addr) {
			max_addr = entry.base_addr + entry.length;
		}	
	}

	uint64_t bitmap_size = max_addr / (4096 * 8); // 4 KB physical frames
	uint8_t *bitmap = (uint8_t *)&kernel_end;
	
	// assume all entries are reserved
	for (size_t i = 0; i < bitmap_size; i++) {
		bitmap[i] = 0xFF;
	}

	// mark free entries
	for (size_t i = 0; i < (mmap->size - sizeof(MemoryMap)) / mmap->entry_size; i++) {
		MemoryMapEntry entry = mmap->entries[i];
		if (entry.type == 1) {
			uint64_t num_frames = entry.length / 4096;
			for (uint64_t j = 0; j < num_frames; j++) {
				uint64_t frame = (entry.base_addr / 4096) + j;
				bitmap[frame / 8] &= ~(1 << (frame % 8));
			}
		}	
	}
	
	// mark kernel frames as reserved
	uint64_t kernel_start_frame = 0x200000 / 4096;
	uintptr_t kernel_end_frame = (uintptr_t)&kernel_end / 4096;
	for (uint64_t frame = kernel_start_frame; frame < kernel_end_frame; frame++) {
		bitmap[frame / 8] |= 1 << (frame % 8);
	}

	// mark bitmap's own frames as reserved
	uintptr_t bitmap_end_frame = ((uintptr_t)&kernel_end + bitmap_size) / 4096;
	for (uint64_t frame = kernel_end_frame; frame < bitmap_end_frame; frame++) {
		bitmap[frame / 8] |= 1 << (frame % 8);
	}
}
