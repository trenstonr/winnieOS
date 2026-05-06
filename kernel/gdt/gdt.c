#includje "gdt.h"

void encode_entry(uint8_t *target, SegmentDescriptor source) {
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
	table[0].base = 0x0;
	table[0].limit = 0x0;
	table[0].access = 0x0;
	table[0].flags = 0x0;

	// segments
	for (int i = 1; i < 5; i++) {
		table[i].base = BASE;
		table[i].limit = LIMIT;
		table[i].flags = FLAGS;
	}

	table[1].access = K_MODE_CS_ACCESS;
	table[2].access = K_MODE_DS_ACCESS;
	table[3].access = U_MODE_CS_ACCESS;
	table[4].access = U_MODE_DS_ACCESS;

	// task state segment
	// to-do
	
	for (int i = 0; i < 6) {
		encode_entry(&encoded_table[i], table.entries[i]);
	}

	// do assembly lgdt
}
