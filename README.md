# winnieos

A 64-bit operating system kernel for x86-64, written from scratch in C and
assembly. There's no libc and no borrowed kernel code. GRUB loads the binary,
and everything after that (CPU mode switching, page tables, interrupts, memory
management) is done by hand.

The kernel tests itself as it boots and prints the results, so the boot screen
is a live demo of what works. This is the actual QEMU output:

```
winnieos                                           an x86-64 kernel from scratch
--------------------------------------------------------------------------------

[ OK ] cpu  64-bit long mode, higher-half kernel at 0xffffffff80200000
[ OK ] idt  256 interrupt gates armed (#DE #DF #GP #PF handled)
[ OK ] pmm  bitmap frame allocator over the multiboot2 memory map
[ OK ] vmm  4-level paging (PML4 -> PDPT -> PD -> PT)

self-test: physical memory
  alloc x3        0x220e000  0x220f000  0x2210000  distinct frames
  free + realloc  0x220f000  freed frame reclaimed

self-test: virtual memory
  map             0x40000000 -> 0x2211000
  write via va    *va = 0xcafebabe   *pa = 0xcafebabe  same frame
  va2pa walk      0x2211000  walk agrees with map
  unmap           va2pa = 0xffffffffffffffff  translation gone

 mem 93 / 93 MiB free   24011 frames   all self-tests passed   halted
```

## What's implemented

| Subsystem | Code | Notes |
|-----------|------|-------|
| Boot / long mode | [`boot/boot.s`](boot/boot.s) | CPUID + long mode checks, page tables built by hand, PAE, `EFER.LME`, 64-bit GDT, far jump into 64-bit mode |
| Higher-half kernel | [`boot.s`](boot/boot.s) + [`linker.ld`](linker.ld) | linked at `0xFFFFFFFF80200000`, loaded at `0x200000` |
| Interrupts | [`kernel/idt/`](kernel/idt/) | 256-entry IDT with hand-encoded 16-byte gates; #DE, #DF, #GP, #PF handlers; page faults print the address from `CR2` |
| Physical memory | [`kernel/pmm/pmm.c`](kernel/pmm/pmm.c) | bitmap allocator, one bit per 4 KiB frame, built from the Multiboot2 memory map |
| Virtual memory | [`kernel/vmm/vmm.c`](kernel/vmm/vmm.c) | 4-level page-table walker: `map_page`, `unmap_page`, `va2pa`; allocates intermediate tables on demand, `invlpg` on unmap |
| Console | [`drivers/vga.c`](drivers/vga.c), [`lib/printf.c`](lib/printf.c) | VGA text driver with scrolling, freestanding printf |

## The hard part: getting into long mode

GRUB hands off in 32-bit protected mode with paging off, and the kernel wants
to run as 64-bit code at the top of the address space. [`boot/boot.s`](boot/boot.s)
gets there in four steps:

1. Confirm `CPUID` works and long mode is supported.
2. Build page tables that map the first 1 GiB twice: once identity-mapped (so
   the next instruction still exists the moment paging turns on) and once at
   `0xFFFFFFFF80000000`.
3. Enable PAE, load `CR3`, set `EFER.LME`, enable paging, load a 64-bit GDT,
   and far-jump to reload `CS` into 64-bit mode.
4. Jump to the high address. This has to be an absolute indirect jump — a
   normal jump is RIP-relative and would leave execution at the low address
   forever.

The higher-half layout is the same split Linux uses: the kernel stays mapped at
the top of every address space, leaving the entire lower half for user programs.

## Memory management

x86-64 splits a virtual address into four 9-bit table indices and a 12-bit
page offset:

```
 63     48 47    39 38    30 29    21 20    12 11         0
|  sign   |  PML4  |  PDPT  |   PD   |   PT   |   offset   |
```

`map_page` walks those four levels, allocating and zeroing any missing table
from the PMM along the way. `unmap_page` clears the leaf entry and flushes it
from the TLB with `invlpg`. `va2pa` does a read-only walk. The boot self-test
maps a page at 1 GiB — deliberately outside the boot-time identity map — writes
through the virtual address, and reads the same value back through the physical
one.

The PMM places its bitmap right after the kernel image (`kernel_end`, exported
by the linker script), marks everything reserved, frees the regions the
firmware reports usable, then re-reserves the first 2 MiB, the kernel itself,
and the bitmap's own frames.

## Build and run

Needs an `x86_64-elf` cross-compiler, `grub-mkrescue`, and QEMU:

```sh
make        # build the kernel ELF
make iso    # wrap it in a bootable GRUB image
make run    # boot it in QEMU
```

Compiler flags worth noting: `-ffreestanding` (no libc), `-mcmodel=kernel`
(higher-half addressing), `-mno-red-zone` (interrupts would clobber it), and
no SSE/MMX/x87 because the kernel doesn't save FPU state yet.

## Next

- kernel heap (`kmalloc`)
- timer + keyboard interrupts (PIC/APIC)
- preemptive scheduler
- PCI enumeration

---

Named after my dog, Winnie. Written with the [OSDev wiki](https://wiki.osdev.org)
open on the other monitor.
