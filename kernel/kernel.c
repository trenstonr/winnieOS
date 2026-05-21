#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <drivers/vga.h>

#include <lib/printf.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

#if !defined(__x86_64__)
#error "This kernel needs to be compiled with a x86_64 compiler"
#endif

void kernel_main(void) 
{
	terminal_initialize();

	printf("test1: %s, test2: %s,\ntest3: %s !", "abc", "ABC", "Abc");
}
