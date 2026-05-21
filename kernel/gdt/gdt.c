#include "gdt.h"
#include <stdint.h>

#define BASE			0x0
#define LIMIT			0xFFFF
#define K_MODE_CS_ACCESS	0x9A
#define K_MODE_DS_ACCESS	0x92
#define U_MODE_CS_ACCESS	0xFA
#define U_MODE_DS_ACCESS	0xF2
#define FLAGS			0xC

typedef struct {
	unsigned int 	limit : 20;
	uint32_t	base;
	uint8_t		access;
	unsigned int	flags : 4;
} SegmentDescriptor;

typedef struct {
	SegmentDescriptor entries[6];
} GDT;

extern void setGdt(uint16_t limit, uint32_t base);
extern void reloadSegments();

static GDT table = {};
static uint64_t encoded_table[6];

static void encode_entry(uint8_t *target, SegmentDescriptor source) {
	// encode limit
	target[0] = source.limit & 0xFF;
	target[1] = (source.limit >> 8) & 0xFF;
	target[6] = (source.limit >> 16) & 0x0F;

	// encode base
	target[2] = source.base & 0xFF;
	target[3] = (source.base >> 8) & 0xFF;
	target[4] = (source.base >> 16) & 0xFF;
	target[7] = (source.base >> 24) & 0xFF;

	// encode access byte
	target[5] = source.access;

	// encode flags
	target[6] |= (source.flags << 4);
}

void gdt_init() {
	// null descriptor
	table.entries[0].base = 0x0;
	table.entries[0].limit = 0x0;
	table.entries[0].access = 0x0;
	table.entries[0].flags = 0x0;

	// segments
	for (int i = 1; i < 5; i++) {
		table.entries[i].base = BASE;
		table.entries[i].limit = LIMIT;
		table.entries[i].flags = FLAGS;
	}

	table.entries[1].access = K_MODE_CS_ACCESS;
	table.entries[2].access = K_MODE_DS_ACCESS;
	table.entries[3].access = U_MODE_CS_ACCESS;
	table.entries[4].access = U_MODE_DS_ACCESS;

	// task state segment
	// to-do
	
	for (int i = 0; i < 6; i++) {
		encode_entry((uint8_t *)&encoded_table[i], table.entries[i]);
	}

	// do assembly lgdt
	// (sizeof(table) - 1, base addr)
	setGdt(47, (uint32_t)encoded_table);
	reloadSegments();
}
