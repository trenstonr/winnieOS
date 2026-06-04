#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "idt/idt.h"
#include "pmm/pmm.h"

#include <drivers/vga.h>
#include <lib/printf.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__x86_64__)
#error "This kernel needs to be compiled with a x86_64 compiler"
#endif

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) 
{
	terminal_initialize();
	idt_init();
	pmm_init(multiboot_magic, multiboot_info_addr);

	// test IDT with interrupt 0 (divide by 0 error)
	// __asm__("int $0x0");

	// test PMM
	uint64_t a = pmm_alloc_frame(), b = pmm_alloc_frame(), c = pmm_alloc_frame();
	printf("%llx %llx %llx", a, b, c);

	// printf tests
	printf("\n\ns: %s, llx: %llx", "Winnie", 0xdeadbeef12340987);

}
