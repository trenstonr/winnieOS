#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define BASE			0x0
#define LIMIT			0xFFFF
#define K_MODE_CS_ACCESS	0x9A
#define K_MODE_DS_ACCESS	0x92
#define U_MODE_CS_ACCESS	0xFA
#define U_MODE_DS_ACCESS	0xF2
#define FLAGS			0xC

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
typedef struct {
	SegmentDescriptor entries[6];
} GDT;

GDT table = {};
uint64_t encoded_table[6];

void encode_entry(uint8_t *target, SegmentDescriptor source);

void gdt_init();

#endif
