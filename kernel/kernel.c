#include <stdint.h>

#include "idt/idt.h"
#include "pmm/pmm.h"
#include "selftest/selftest.h"

#include <drivers/vga.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__x86_64__)
#error "This kernel needs to be compiled with a x86_64 compiler"
#endif

/*
 * First C code to run. boot.s has already verified long mode support, built
 * the initial page tables, enabled paging, and jumped into the higher half
 * before calling this with the multiboot2 magic and info pointer.
 */
void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr)
{
	terminal_initialize();
	banner();

	ok("cpu", "64-bit long mode, higher-half kernel at 0xffffffff80200000");

	idt_init();
	ok("idt", "256 interrupt gates armed (#DE #DF #GP #PF handled)");

	pmm_init(multiboot_magic, multiboot_info_addr);
	uint64_t total_frames = pmm_free_count();
	ok("pmm", "bitmap frame allocator over the multiboot2 memory map");

	ok("vmm", "4-level paging (PML4 -> PDPT -> PD -> PT)");

	selftest_pmm();
	selftest_vmm();
	status_bar(total_frames);
}
