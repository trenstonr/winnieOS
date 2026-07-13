#include <stdint.h>
#include <lib/printf.h>

void handle_interrupt() {
	printf("\nunhandled_interrupt\n");
	while (1);
}

void handle_divide_error() {
	printf("\nhandle_divide_error\n");
	while (1);
}

void handle_double_fault() {
	printf("\nhandle_double_fault\n");
	while (1);
}

void handle_generic_protection_fault() {
	printf("\nhandle_generic_protection_fault\n");
	while (1);
}

void handle_page_fault() {
	uint64_t cr2;
	__asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

	printf("\nhandle_page_fault @ %llx\n", cr2);
	while (1);
}
