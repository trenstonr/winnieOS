# winnieOS

A 64-bit operating system kernel for x86-64, written from scratch in C and
assembly. It's a freestanding kernel: no libc, no runtime, and essentially no
borrowed kernel code, apart from a small amount of VGA driver code adapted from
the OSDev [Bare Bones](https://wiki.osdev.org/Bare_Bones) tutorial. GRUB loads
the binary, and the kernel does everything after that itself: switching the CPU
into 64-bit long mode, building page tables by hand, taking interrupts, and
managing physical and virtual memory. That covers most of the genuinely hard
parts of early kernel bring-up, in a codebase small enough to read in one
sitting.

The kernel tests itself as it boots and prints the results, so the boot screen
is a live demo of what works.

**GRUB boot menu with the winnieOS entry:**

<img width="776" height="519" alt="GRUB boot menu showing the winnieOS entry" src="https://github.com/user-attachments/assets/6f731f71-6d58-4ec6-8791-173bea26f162" />

**Actual `kernel.c` self-test output in QEMU:**

<img width="776" height="519" alt="winnieOS kernel self-test output in QEMU" src="https://github.com/user-attachments/assets/f18d186d-3072-4951-a891-808e4ce39ae3" />

## What's implemented

| Subsystem | Code | Notes |
|-----------|------|-------|
| Boot / long mode | [`boot/boot.s`](boot/boot.s) | CPUID + long mode checks, page tables built by hand, PAE, `EFER.LME`, 64-bit GDT, far jump into 64-bit mode. Boots via Multiboot2 (originally Multiboot, from the Bare Bones tutorial, since migrated) |
| Higher-half kernel | [`boot.s`](boot/boot.s) + [`linker.ld`](linker.ld) | linked at `0xFFFFFFFF80200000`, loaded at `0x200000` |
| Interrupts | [`kernel/idt/`](kernel/idt/) | 256-entry IDT with hand-encoded 16-byte gates; handlers for Divide Error (#DE), Double Fault (#DF), General Protection Fault (#GP), and Page Fault (#PF); the page fault handler prints the faulting address from `CR2` |
| Physical memory | [`kernel/pmm/pmm.c`](kernel/pmm/pmm.c) | bitmap allocator, one bit per 4 KiB frame, built from the Multiboot2 memory map |
| Virtual memory | [`kernel/vmm/vmm.c`](kernel/vmm/vmm.c) | 4-level page-table walker: `map_page`, `unmap_page`, `va2pa`; allocates intermediate tables on demand, flushes the TLB with the `invlpg` instruction on unmap |
| Console | [`drivers/vga.c`](drivers/vga.c), [`lib/printf.c`](lib/printf.c) | VGA text driver with scrolling (parts adapted from the OSDev Bare Bones tutorial), freestanding printf |

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
4. Jump to the high address. This has to be an absolute indirect jump, because
   a normal jump is RIP-relative and would leave execution at the low address
   forever.

The higher-half layout is the same split Linux uses: the kernel stays mapped at
the top of every address space, leaving the entire lower half for user programs
(userspace isn't implemented yet).

## Memory management

x86-64 splits a virtual address into four 9-bit table indices and a 12-bit
page offset:

```
MSB                                                              LSB
 63       48 47      39 38      30 29      21 20      12 11       0
+-----------+----------+----------+----------+----------+----------+
|   sign    |   PML4   |   PDPT   |    PD    |    PT    |  offset  |
+-----------+----------+----------+----------+----------+----------+
```

`map_page` walks those four levels, allocating and zeroing any missing table
from the PMM along the way. `unmap_page` clears the leaf entry and flushes the
stale TLB entry with the `invlpg` instruction. `va2pa` does a read-only walk.
The boot self-test maps a page at 1 GiB (deliberately outside the boot-time
identity map), writes through the virtual address, and reads the same value
back through the physical one.

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
(higher-half addressing), and no SSE/MMX/x87 because the kernel doesn't save
FPU state yet.

## Roadmap

- kernel heap (`kmalloc`)
- timer + keyboard interrupts (PIC/APIC)
- preemptive scheduler
- PCI enumeration

---

Named after my dog Winnie.
Main reference: [osdev.wiki](https://wiki.osdev.org)
