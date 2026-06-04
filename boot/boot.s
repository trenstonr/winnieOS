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
.long GDT


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

	// initalize stack
	mov $stack_top, %esp
	
	// set multiboot info variables
	mov %eax, multiboot_magic
	mov %ebx, multiboot_info_addr

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
	mov $pdpt + 3, %eax // set first 2 bits (+ 3)
	movl %eax, (pml4)
	mov $pd + 3, %eax
	movl %eax, (pdpt)

	fill_page_directory:
	.set ENTRIES_PER_PD, 512
	.set SIZEOF_PD_ENTRY, 8
	.set PAGE_SIZE, 0x200000
	mov $pd, %edi
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
	mov $pml4, %eax
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

	.enableLongMode:
	lgdt .Pointer
	ljmpl $8, $long_mode_start
	.code64
	long_mode_start:
		// reload stack/segment pointers/registers
		mov $stack_top, %rsp
		mov $0x10, %ax
		mov %ax, %ds
		mov %ax, %es
		mov %ax, %ss
		
		// pass multiboot info into entry point
		movl multiboot_magic, %edi
		movl multiboot_info_addr, %esi

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
