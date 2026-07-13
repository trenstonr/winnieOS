#include <stdint.h>
#include <stddef.h>

#include "selftest.h"
#include "../pmm/pmm.h"
#include "../vmm/vmm.h"

#include <drivers/vga.h>

#define FG(c)		vga_entry_color(c, VGA_COLOR_BLACK)
#define C_TEXT		FG(VGA_COLOR_LIGHT_GREY)
#define C_DIM		FG(VGA_COLOR_DARK_GREY)
#define C_VALUE		FG(VGA_COLOR_LIGHT_BLUE)
#define C_NAME		FG(VGA_COLOR_LIGHT_CYAN)
#define C_GOOD		FG(VGA_COLOR_LIGHT_GREEN)
#define C_BAD		FG(VGA_COLOR_LIGHT_RED)
#define C_HEAD		FG(VGA_COLOR_WHITE)
#define C_BAR		vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY)

static int failures = 0;

static void nl(void) {
	terminal_putchar('\n', C_TEXT);
}

static void heading(const char *s) {
	nl();
	terminal_writestring("self-test: ", C_DIM);
	terminal_writestring(s, C_HEAD);
	nl();
}

static void field(const char *label) {
	terminal_writestring("  ", C_TEXT);
	terminal_writestring(label, C_TEXT);
	for (size_t i = strlen(label); i < 16; i++)
		terminal_putchar(' ', C_TEXT);
}

static void expect(int cond, const char *pass, const char *fail) {
	terminal_writestring("  ", C_TEXT);
	if (cond) {
		terminal_writestring(pass, C_DIM);
	} else {
		terminal_writestring(fail, C_BAD);
		failures++;
	}
}

void banner(void) {
	const char *tag = "an x86-64 kernel from scratch";

	terminal_writestring("winnieos", C_GOOD);
	while (terminal_col < VGA_WIDTH - strlen(tag))
		terminal_putchar(' ', C_TEXT);
	terminal_writestring(tag, C_DIM);

	for (int i = 0; i < VGA_WIDTH; i++)
		terminal_putchar('-', C_DIM);
	nl();
}

void ok(const char *name, const char *desc) {
	terminal_writestring("[ ", C_DIM);
	terminal_writestring("OK", C_GOOD);
	terminal_writestring(" ] ", C_DIM);
	terminal_writestring(name, C_NAME);
	for (size_t i = strlen(name); i < 5; i++)
		terminal_putchar(' ', C_TEXT);
	terminal_writestring(desc, C_TEXT);
	nl();
}

// allocate three frames, free one, and check the allocator reclaims it
void selftest_pmm(void) {
	heading("physical memory");

	uint64_t a = pmm_alloc_frame(), b = pmm_alloc_frame(), c = pmm_alloc_frame();
	field("alloc x3");
	terminal_writehex(a, C_VALUE);
	terminal_writestring("  ", C_TEXT);
	terminal_writehex(b, C_VALUE);
	terminal_writestring("  ", C_TEXT);
	terminal_writehex(c, C_VALUE);
	expect(a && b && c && a != b && b != c, "distinct frames", "BAD FRAMES");
	nl();

	pmm_free_frame(b);
	uint64_t r = pmm_alloc_frame();
	field("free + realloc");
	terminal_writehex(r, C_VALUE);
	expect(r == b, "freed frame reclaimed", "NOT RECLAIMED");
	nl();
}

// map a page at 1 GiB, write through the va,
// read the value back through the pa, then unmap and check the translation is gone
void selftest_vmm(void) {
	heading("virtual memory");

	uint64_t cr3;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
	uint64_t *pml4 = (uint64_t *)BASE(cr3);

	uint64_t va = 0x40000000;
	uint64_t pa = pmm_alloc_frame();

	map_page(pml4, va, pa, 0x3);	// present | writable
	field("map");
	terminal_writehex(va, C_VALUE);
	terminal_writestring(" -> ", C_DIM);
	terminal_writehex(pa, C_VALUE);
	nl();

	*(volatile uint64_t *)va = 0xCAFEBABE;
	field("write via va");
	terminal_writestring("*va = ", C_TEXT);
	terminal_writehex(*(volatile uint64_t *)va, C_VALUE);
	terminal_writestring("   *pa = ", C_TEXT);
	terminal_writehex(*(volatile uint64_t *)pa, C_VALUE);
	expect(*(volatile uint64_t *)pa == 0xCAFEBABE, "same frame", "NO ALIAS");
	nl();

	field("va2pa walk");
	uint64_t walked = va2pa(pml4, va);
	terminal_writehex(walked, C_VALUE);
	expect(walked == pa, "walk agrees with map", "WALK WRONG");
	nl();

	unmap_page(pml4, va);
	field("unmap");
	terminal_writestring("va2pa = ", C_TEXT);
	terminal_writehex(va2pa(pml4, va), C_VALUE);
	expect(va2pa(pml4, va) == (uint64_t)-1, "translation gone", "STILL MAPPED");
	nl();
}

void status_bar(uint64_t total_frames) {
	uint64_t free_frames = pmm_free_count();
	uint64_t free_mib = free_frames * 4096 / (1024 * 1024);
	uint64_t total_mib = total_frames * 4096 / (1024 * 1024);

	nl();
	terminal_writestring(" mem ", C_BAR);
	terminal_writedec(free_mib, C_BAR);
	terminal_writestring(" / ", C_BAR);
	terminal_writedec(total_mib, C_BAR);
	terminal_writestring(" MiB free   ", C_BAR);
	terminal_writedec(free_frames, C_BAR);
	terminal_writestring(" frames   ", C_BAR);
	if (failures == 0) {
		terminal_writestring("all self-tests passed", C_BAR);
	} else {
		terminal_writedec(failures, C_BAR);
		terminal_writestring(" SELF-TESTS FAILED", C_BAR);
	}
	terminal_writestring("   halted", C_BAR);
	while (terminal_col != 0)
		terminal_putchar(' ', C_BAR);
}
