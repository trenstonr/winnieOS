#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/vga.h>
#include <lib/printf.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#include "gdt/gdt.h" 

void kernel_main(void) 
{
	// initalize global descriptor table
	gdt_init();
	
	terminal_initialize();

	printf("test1: %s, test2: %s,\ntest3: %s !", "abc", "ABC", "Abc");
}
