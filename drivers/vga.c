#include <drivers/vga.h>

size_t terminal_row;
size_t terminal_col;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_col = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

uint16_t terminal_getentry(size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	return terminal_buffer[index];
}

void scroll() {
	for (int row = 1; row < VGA_HEIGHT; row++) {
		for (int col = 0; col < VGA_WIDTH; col++) {
			uint16_t copy = terminal_getentry(col, row);	// full cell: char + color
			terminal_buffer[(row - 1) * VGA_WIDTH + col] = copy;
		}
	}

	for (int col = 0; col < VGA_WIDTH; col++)
		terminal_putentryat(' ', terminal_color, col, VGA_HEIGHT - 1);

	terminal_row = VGA_HEIGHT - 1;
}

void newline() {
	terminal_col = 0;
	++terminal_row;

	if (terminal_row == VGA_HEIGHT)
		scroll();
}

void terminal_putchar(char c, enum vga_color color) 
{
	if (c == '\n') {
		newline();
		return;
	}

	terminal_putentryat(c, color, terminal_col, terminal_row);

	terminal_col++;
	if (terminal_col == VGA_WIDTH)
		newline();
}

void terminal_write(const char* data, size_t size, enum vga_color color) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i], color);
}

void terminal_writestring(const char* data, enum vga_color color) 
{
	terminal_write(data, strlen(data), color);
}
