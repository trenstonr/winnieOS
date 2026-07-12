.section .multiboot, "a"
.align 8
multiboot_start:
    .long 0xE85250D6           # magic
    .long 0                    # architecture (i386)
    .long multiboot_end - multiboot_start   # length
    .long -(0xE85250D6 + 0 + (multiboot_end - multiboot_start))  # checksum

    # request tag
    .align 8
    .word 1	# type
    .word 0	# flags
    .long 12	# size
    .long 6	# arr of tags to request (currently only mmap)

    # end tag (terminator)
    .align 8
    .word 0	# type
    .word 0	# flags
    .long 8	# size

multiboot_end:


# The whole image is now LINKED HIGH (VMA = 0xFFFFFFFF80200000) but LOADED LOW
# (LMA = 0x200000). So every symbol's value is a high virtual address. The 32-bit
# pre-paging code below runs with paging OFF, addressing physical memory, in 32-bit
# registers. A high 64-bit symbol can't be used there. Subtracting KERNEL_VMA turns
# any high symbol back into its physical address, which fits in 32 bits.
.set KERNEL_VMA, 0xFFFFFFFF80000000


.section .bss

// stack
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

// page tables
.align 4096
pml4:
.skip 4096
pdpt:
.skip 4096
pd:
.skip 4096


.section .rodata

GDT:
.Null:
.quad 0x0000000000000000
.Code:
.quad 0x00209A0000000000
.quad 0x0000920000000000
.align 4
.word 0
.Pointer:
.word . - GDT - 1
.long GDT - KERNEL_VMA          # base must be the PHYSICAL address of GDT: lgdt runs
                                # in 32-bit mode, so this .long has to hold a 32-bit value


.section .data

.global multiboot_magic
multiboot_magic:
	.long 0

.global multiboot_info_addr
multiboot_info_addr:
	.long 0


.section .text

.global _start
.type _start, @function
.code32
_start:
	cli

	// initalize stack (physical address: still in 32-bit mode, paging off)
	mov $(stack_top - KERNEL_VMA), %esp

	// set multiboot info variables (store to their physical addresses)
	mov %eax, (multiboot_magic - KERNEL_VMA)
	mov %ebx, (multiboot_info_addr - KERNEL_VMA)

	// check if CPUID is supported
	check_cpuid:
	pushfl
	pop %eax
	mov %eax, %ecx
	xor $(1 << 21), %eax
	push %eax
	popfl
	pushfl
	pop %eax
	xor %ecx, %eax
	and $(1 << 21), %eax
	jz no_cpuid

	// check if long mode is supported
	check_long_mode:
	mov $0x80000000, %eax
	cpuid
	cmp $0x80000001, %eax
	jb no_long_mode
	mov $0x80000001, %eax
	cpuid
	test $(1 << 29), %edx
	jz no_long_mode

	link_page_table:
	// pdpt+3 (present|writable) goes into BOTH pml4[0] (low identity map)
	// and pml4[511] (high half). 511*8 = byte offset of entry 511.
	mov $(pdpt - KERNEL_VMA + 3), %eax
	movl %eax, (pml4 - KERNEL_VMA)              // pml4[0]   -> pdpt  (low)
	movl %eax, (pml4 - KERNEL_VMA + 511*8)      // pml4[511] -> pdpt  (high)

	// pd+3 goes into BOTH pdpt[0] (low) and pdpt[510] (high).
	// 0xFFFFFFFF80000000 decodes to pml4 index 511, pdpt index 510, pd index 0.
	mov $(pd - KERNEL_VMA + 3), %eax
	movl %eax, (pdpt - KERNEL_VMA)              // pdpt[0]   -> pd    (low)
	movl %eax, (pdpt - KERNEL_VMA + 510*8)      // pdpt[510] -> pd    (high)

	fill_page_directory:
	.set ENTRIES_PER_PD, 512
	.set SIZEOF_PD_ENTRY, 8
	.set PAGE_SIZE, 0x200000
	mov $(pd - KERNEL_VMA), %edi
	mov $0x83, %ebx // present, writable, 2 MiB page
	mov $ENTRIES_PER_PD, %ecx

	.setEntry:
	mov %ebx, (%edi)
	movl $0, 4(%edi)
	add $PAGE_SIZE, %ebx
	add $SIZEOF_PD_ENTRY, %edi
	dec %ecx
	jnz .setEntry

	.enablePAE:
	mov %cr4, %eax
	or $(1 << 5), %eax
	mov %eax, %cr4
	mov $(pml4 - KERNEL_VMA), %eax   // CR3 takes the PHYSICAL address of the PML4
	mov %eax, %cr3

	.setLMBit:
	mov $(0xC0000080), %ecx
	rdmsr
	or $(1 << 8), %eax
	wrmsr

	.enablePaging:
	.set CR0_PG_ENABLE, (1 << 31)
	mov %cr0, %eax
	or $(CR0_PG_ENABLE), %eax
	mov %eax, %cr0
	// paging is now ON. Both the low identity map and the high map are live.

	.enableLongMode:
	lgdt (.Pointer - KERNEL_VMA)             // GDTR loaded from physical addr of .Pointer
	// Far jump reloads CS and enters 64-bit mode. A 32-bit far jump encodes a
	// 32-bit offset, so we MUST target the LOW physical address of the entry.
	ljmpl $8, $(long_mode_entry - KERNEL_VMA)

	.code64
	long_mode_entry:
		// Still executing at the LOW address here (RIP is low). A near jmp is
		// RIP-relative and would keep us low forever. To reach the high half we
		// load the full 64-bit high address and do an ABSOLUTE indirect jump.
		movabs $higher_half_start, %rax
		jmp *%rax

	higher_half_start:
		// RIP is now HIGH. From here every kernel symbol is reachable RIP-relative,
		// so no more KERNEL_VMA subtraction is needed.

		// reload stack to the high virtual stack, and the segment registers
		mov $stack_top, %rsp
		mov $0x10, %ax
		mov %ax, %ds
		mov %ax, %es
		mov %ax, %ss

		// pass multiboot info into entry point (SysV: rdi, rsi)
		mov multiboot_magic, %edi
		mov multiboot_info_addr, %esi

		// call entry point
		call kernel_main

1:	hlt // halt infinitely
	jmp 1b

	// halt if CPUID is not supported
	no_cpuid:
2:	hlt
	jmp 2b

	// halt if long mode is not supported
	no_long_mode:
3:	hlt
	jmp 3b


/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
