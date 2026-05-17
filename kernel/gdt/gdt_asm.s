.section .data
gdtr:
    .word 0    # For limit storage
    .long 0    # For base storage

.section .text

.global setGdt
setGdt:
    mov   4(%esp), %ax
    mov   %ax, gdtr
    mov   8(%esp), %eax
    mov   %eax, gdtr + 2
    lgdt  gdtr
    ret

.global reloadSegments
reloadSegments:
    # Reload CS register containing code selector:
    ljmp  $0x08, $.reload_CS   # far jump to reload CS
.reload_CS:
    # Reload data segment registers:
    mov   $0x10, %ax
    mov   %ax, %ds
    mov   %ax, %es
    mov   %ax, %fs
    mov   %ax, %gs
    mov   %ax, %ss
    ret
