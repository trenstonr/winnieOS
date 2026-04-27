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
			else if (*str == 'd') {
				// skip for now
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
