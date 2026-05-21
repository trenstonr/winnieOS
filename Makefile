CC	:= x86_64-elf-gcc
AS	:= x86_64-elf-as
CFLAGS	:= -std=gnu99 -ffreestanding -O2 -Wall -Wextra -mno-red-zone -mgeneral-regs-only -mno-sse -mno-sse2 -mno-mmx -mno-80387 -I include/
LFLAGS := -ffreestanding -O2 -nostdlib -lgcc -Wl,-z,max-page-size=0x1000

TARGET 	:= myos
ISO	:= myos.iso
ISODIR 	:= isodir

OBJS	:= boot/boot.o \
	   kernel/kernel.o \
	   drivers/vga.o \
	   lib/printf.o

.PHONY: all iso clean run

# Sets default target (running make with no args)
all: $(TARGET)

# Build ISO
iso: $(TARGET)
	mkdir -p $(ISODIR)/boot/grub
	cp $(TARGET) $(ISODIR)/boot/$(TARGET)
	cp grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR)

# Link
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -T linker.ld $^ -o $@

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble
%.o: %.s
	$(AS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(ISODIR) $(ISO)

run: iso
	qemu-system-x86_64 -boot d -cdrom $(ISO) -no-reboot -no-shutdown
