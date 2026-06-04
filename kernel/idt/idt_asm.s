.section .data

idtr:
idtr_limit:
	.word 0
idtr_base:
	.quad 0

.section .text

.global setIdt
setIdt:
	movw %di, idtr_limit
	movq %rsi, idtr_base
	lidt idtr
	ret


/* interrupt handlers */

.macro save_registers
    pushq %rax
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
.endm

.macro restore_registers
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rax
.endm

.global unhandled_interrupt
unhandled_interrupt:
	pushq $0
	save_registers
	call handle_interrupt
	restore_registers
	addq $8, %rsp		/* discard error code so rsp points at saved RIP */
	iretq

.global isr0
isr0:
	pushq $0
	save_registers
	call handle_divide_error
	restore_registers
	addq $8, %rsp		
	iretq

.global isr8
isr8:
	save_registers
	call handle_double_fault
	restore_registers
	addq $8, %rsp	
	iretq

.global isr13
isr13:
	save_registers
	call handle_generic_protection_fault
	restore_registers
	addq $8, %rsp
	iretq

.global isr14
isr14:
	save_registers
	call handle_page_fault
	restore_registers
	addq $8, %rsp	
	iretq
	
	
