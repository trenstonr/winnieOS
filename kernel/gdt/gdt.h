#ifndef GDT_H
#define GDT_H

#DEFINE BASE			0x0
#DEFINE LIMIT			0xFFFF
#DEFINE K_MODE_CS_ACCESS	0x9A
#DEFINE K_MODE_DS_ACCESS	0x92
#DEFINE U_MODE_CS_ACCESS	0xFA
#DEFINE U_MODE_DS_ACCESS	0xF2
#DEFINE FLAGS			0xC

// global descriptor table register
volatile uint16_t gdtr;

// segment descriptor
typedef struct {
	unsigned int 	limit : 20;
	uint32_t	base;
	uint8_t		access;
	unsigned int	flags : 4;
} SegmentDescriptor;

// global descriptor table
struct GDT table {
	SegmentDescriptor entries[6];
};


void encode_entry(uint8_t *entry); 
void gdt_init();

uint64_t encoded_table[6];

#endif
