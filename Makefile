CC	:= i686-elf-gcc
AS	:= i686-elf-as
CFLAGS	:= -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I include/
LFLAGS	:= -ffreestanding -O2 -nostdlib -lgcc

TARGET 	:= myos
ISO	:= myos.iso
ISODIR 	:= isodir

OBJS	:= boot/boot.o kernel/kernel.o drivers/vga.o

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
	qemu-system-i386 -cdrom $(ISO)
