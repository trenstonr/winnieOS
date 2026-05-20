/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

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
pt:
.skip 4096

.section .text

.global _start
.type _start, @function
_start:
	// initalize stack
	mov $stack_top, %esp

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

	check_long_mode:
	mov $0x80000000, %eax
	cpuid
	cmp $0x80000001, %eax
	jb no_long_mode
	mov $0x80000001, %eax
	cpuid
	test $(1 << 29), %edx
	jz no_long_mode

	// entry point
	call kernel_main

	no_cpuid:
2:	hlt
	jmp 2b

	no_long_mode:
3:	hlt
	jmp 3b


	// put cpu in infinite loop (kernel should never return)
1:	hlt
	jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
