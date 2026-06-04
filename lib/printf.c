#include <stdint.h>
#include <stddef.h>
#include <lib/printf.h>
#include <drivers/vga.h>

#define MAXARGS	16

void printf(const char *args, ...) {
	const char *str = args;
	args++;

	va_list ap;
	va_start(ap, args);

	int curr_arg = 0;

	while (*str != '\0') {
		while (*str != '\0' && *str == '%') {
			str++;

			if (*str == 's') {
				const char *arg = va_arg(ap, const char *);
				while (arg && *arg != '\0') {
					terminal_putchar(*arg, VGA_COLOR_LIGHT_MAGENTA);
					arg++;
				}
			}
			else if (str[0] == 'l' && str[1] == 'l' && str[2] == 'x') {
				// 64-bit hex  
				uint64_t arg = va_arg(ap, uint64_t);
				for (size_t i = 1; i <= 16; i++) {
					uint64_t nibble = (arg >> (64 - (i * 4))) & 0xF;
					char digit;
					if (nibble <= 9) digit = '0' + nibble;
					else digit = 'a' + (nibble - 10);
					terminal_putchar(digit, VGA_COLOR_BLUE);
				}
				str += 2;
			}
			
			str++;
			curr_arg++;
		}

		if (*str == '\0') break;

		terminal_putchar(*str, VGA_COLOR_WHITE);
		str++;
	}

	va_end(ap);
}
