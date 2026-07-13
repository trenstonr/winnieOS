#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "idt/idt.h"
#include "pmm/pmm.h"
#include "vmm/vmm.h"

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
	printf("PMM TEST: %llx %llx %llx", a, b, c);

	// test VMM
	uint64_t cr3;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
	
	uint64_t *pml4 = (uint64_t *)BASE(cr3);
	uint64_t va = 0x40000000;
	uint64_t x = pmm_alloc_frame();
	map_page(pml4, va, x, 0x3);

	uint64_t *mem = (uint64_t *)va;
	*mem = 0xCAFEBABE;

	printf("\n\nVMM TEST: (va)%llx (pa)%llx", *mem, *(uint64_t *)x);

	// printf tests
	printf("\n\nPRINTF TEST: s: %s, llx: %llx", "Winnie", 0xdeadbeef12340987);

}
