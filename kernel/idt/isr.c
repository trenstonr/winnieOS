#include <lib/printf.h>

void handle_interrupt() {
	printf("unhandled_interrupt\n");
	while (1);
}

void handle_divide_error() {
	printf("handle_divide_error\n");
	while (1);
}

void handle_double_fault() {
	printf("handle_double_fault\n");
	while (1);
}

void handle_generic_protection_fault() {
	printf("handle_generic_protection_fault\n");
	while (1);
}

void handle_page_fault() {
	printf("handle_page_fault\n");
	while (1);
}
