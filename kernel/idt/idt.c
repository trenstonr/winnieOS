#include "idt.h"
#include <stdint.h>

extern void setIdt(uint16_t limit, uint64_t base);

extern void unhandled_interrupt();

extern void isr0();	// divide error
extern void isr8();	// double fault
extern void isr13();	// general protection
extern void isr14();	// paging

typedef struct {
	uint64_t offset;
	uint16_t seg_selector;
	unsigned int ist	: 3;
	unsigned int type	: 4;
	unsigned int dpl	: 2;
	unsigned int present	: 1;
} Gate;

typedef struct {
	Gate entries[256];
} IDT;

static IDT table = {};
static uint8_t encoded_table[4096]; // 16 bytes/gate * 256 gates

static void encode_entry(uint8_t *target, Gate src) {
	target[0] = src.offset & 0xFF;
	target[1] = (src.offset >> 8);
	target[2] = src.seg_selector;
	target[3] = (src.seg_selector >> 8);
	target[4] = src.ist & 0x7;
	target[5] = (src.type) | (src.dpl << 5) | (src.present << 7);
	target[6] = (src.offset >> 16) & 0xFF;
	target[7] = (src.offset >> 24) & 0xFF;
	target[8] = (src.offset >> 32) & 0xFF;
	target[9] = (src.offset >> 40) & 0xFF;
	target[10] = (src.offset >> 48) & 0xFF;
	target[11] = (src.offset >> 56) & 0xFF;
}

void idt_init() {
	for (int i = 0; i < 256; i++) {
		table.entries[i].offset		= (uint64_t)unhandled_interrupt;
		table.entries[i].seg_selector 	= 0x08; // code segment
		table.entries[i].ist 		= 0x0;
		table.entries[i].type 		= 0xE;
		table.entries[i].dpl 		= 0x0;
		table.entries[i].present 	= 0x1;
	}

	table.entries[0].offset		= (uint64_t)isr0;
	table.entries[8].offset		= (uint64_t)isr8;
	table.entries[13].offset	= (uint64_t)isr13;
	table.entries[14].offset	= (uint64_t)isr14;

	for (int i = 0; i < 256; i++) {
		encode_entry((uint8_t *)&encoded_table[i * 16], table.entries[i]);
	}

	setIdt(4095, (uint64_t)encoded_table);
}
