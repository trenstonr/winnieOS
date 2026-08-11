# winnieOS

A 64-bit operating system kernel for x86-64, written from scratch in C and
assembly.

GRUB loads the binary and everything after is manually done: switching
into 64-bit long mode, the page tables, the interrupt handlers, and the memory
allocators. The kernel is freestanding, with no libc and no runtime. 

## What happens on boot

The kernel runs a self-test suite every time it boots and prints the results
through its own VGA text driver, so the boot screen is a live readout of which
subsystems work. This is the actual QEMU output:

<img width="774" height="512" alt="winnieOS kernel self-test output in QEMU" src="https://github.com/user-attachments/assets/21abf2b2-79ef-4197-92ef-74e34530b26d" />

Reading it top to bottom:

- The `[ OK ]` lines are printed by [`kernel_main`](kernel/kernel.c) as each
  subsystem initializes: long mode, the interrupt descriptor table, the
  physical allocator, paging.
- `self-test: physical memory` allocates three frames, checks they're
  distinct, frees one, and checks the allocator hands the same frame back.
- `self-test: virtual memory` maps a fresh page, writes through the virtual
  address, and reads the value back through the physical one. If those don't
  match, paging is broken. Then it unmaps and confirms the translation is
  gone.
- The status bar reports free memory (as tracked by the physical allocator)
  and the overall pass/fail result.

Both tests live in [`kernel/selftest/selftest.c`](kernel/selftest/selftest.c).
The physical memory test:

```c
uint64_t a = pmm_alloc_frame(), b = pmm_alloc_frame(), c = pmm_alloc_frame();
expect(a && b && c && a != b && b != c, "distinct frames", "BAD FRAMES");
// three allocations should return three different non-null frames

pmm_free_frame(b);
uint64_t r = pmm_alloc_frame();
expect(r == b, "freed frame reclaimed", "NOT RECLAIMED");
// freeing b and allocating again should reclaim that same frame
```

And the core of the virtual memory test:

```c
map_page(pml4, va, pa, 0x3);              /// P|W
// map va to pa, so the two addresses point at the same physical frame

*(volatile uint64_t *)va = 0xCAFEBABE;    // write to the virtual address
expect(*(volatile uint64_t *)pa == 0xCAFEBABE, "same frame", "NO ALIAS");
// reading back through the physical address should see the same value

unmap_page(pml4, va);
expect(va2pa(pml4, va) == (uint64_t)-1, "translation gone", "STILL MAPPED");
// after unmapping, the translation should no longer exist
```

The test page lives at 1 GiB, outside the boot-time identity map, so it can
only work if `map_page` actually built the intermediate page tables.

## What's implemented

| Subsystem | Code | Notes |
|-----------|------|-------|
| Boot / long mode | [`boot/boot.s`](boot/boot.s) | Multiboot2 entry, CPUID and long mode checks, page tables built by hand, far jump into 64-bit mode |
| Higher-half kernel | [`boot/boot.s`](boot/boot.s) + [`linker.ld`](linker.ld) | linked at `0xFFFFFFFF80200000`, loaded at physical `0x200000` |
| Interrupts | [`kernel/idt/`](kernel/idt/) | 256-entry interrupt descriptor table, gates encoded byte by byte; handlers for Divide Error, Double Fault, General Protection Fault, and Page Fault |
| Physical memory | [`kernel/pmm/pmm.c`](kernel/pmm/pmm.c) | bitmap allocator, one bit per 4 KiB frame, built from the Multiboot2 memory map |
| Virtual memory | [`kernel/vmm/vmm.c`](kernel/vmm/vmm.c) | walks the 4-level page tables: `map_page`, `unmap_page`, `va2pa` |
| Console | [`drivers/vga.c`](drivers/vga.c), [`lib/printf.c`](lib/printf.c) | VGA text driver with scrolling (partly from Bare Bones), freestanding printf |

## From GRUB to long mode

GRUB hands off in 32-bit protected mode with paging disabled. winnieOS is a
higher-half kernel: it runs as 64-bit code mapped at the top of the virtual
address space, at `0xFFFFFFFF80200000`. [`boot/boot.s`](boot/boot.s) bridges
that gap in four steps:

1. Confirm the `CPUID` instruction works and the CPU supports long mode.
2. Build page tables that map the first 1 GiB twice: identity-mapped, so the
   next instruction still exists the moment paging turns on, and again at
   `0xFFFFFFFF80000000`.
3. Enable PAE (Physical Address Extension), load the page tables into `CR3`,
   set the long mode enable bit in the `EFER` register, enable paging, load a
   64-bit GDT (Global Descriptor Table), and far-jump to reload `CS` into
   64-bit mode.
4. Jump to the high mapping. A normal jump is relative to the instruction
   pointer and would keep executing at the low address, so this one is an
   absolute jump through a register:

```asm
.code64                                # assembler directive: everything below is 64-bit code
long_mode_entry:
    movabs $higher_half_start, %rax    # load the full 64-bit high address into a register
    jmp *%rax                          # absolute jump to it

higher_half_start:
    mov $stack_top, %rsp               # execution is now in the higher half;
                                       # switch to the high virtual stack
```

This is the same higher-half split Linux uses: the kernel stays mapped at the
top of every address space, and the lower half is left for user programs
(userspace isn't implemented yet, see the roadmap).

## Interrupts

When an exception fires, the CPU uses its vector number to index the interrupt
descriptor table and jumps through the matching gate to a handler. winnieOS
installs a full 256-entry table: vectors 0, 8, 13, and 14 (Divide Error,
Double Fault, General Protection Fault, and Page Fault) get dedicated
handlers, and every other vector routes to a catch-all, so an unexpected
interrupt is still caught instead of crashing the machine. The page fault
handler reads the faulting address out of the
`CR2` register and prints it.

Each gate packs a 64-bit handler address (split across three fields), a code
segment selector, and type and privilege bits into the 16-byte format the
hardware expects. Descriptors are defined as C structs and encoded byte by
byte in [`kernel/idt/idt.c`](kernel/idt/idt.c):

```c
static void encode_entry(uint8_t *target, Gate src) {
    target[0] = src.offset & 0xFF;
    target[1] = (src.offset >> 8);
    target[2] = src.seg_selector;
    /* ... type, DPL, present bit, upper offset bytes ... */
}
```

## Memory management

x86-64 translates virtual addresses through four levels of page tables: the
Page Map Level 4 (PML4), the Page Directory Pointer Table (PDPT), the Page
Directory (PD), and the Page Table (PT). A virtual address encodes a 9-bit
index into each level, plus a 12-bit offset into the final page:

```
MSB                                                              LSB
 63       48 47      39 38      30 29      21 20      12 11       0
+-----------+----------+----------+----------+----------+----------+
|   sign    |   PML4   |   PDPT   |    PD    |    PT    |  offset  |
+-----------+----------+----------+----------+----------+----------+
```

`map_page` walks all four levels, allocating and zeroing any missing table
along the way. Each level follows the same pattern, from
[`kernel/vmm/vmm.c`](kernel/vmm/vmm.c):

```c
entry = &pml4[PML4_IDX(va)];
if (!(*entry & 0x1)) *entry = alloc_entry();   /* allocate missing table */

entry = &((uint64_t *)BASE(*entry))[PDPT_IDX(va)];
/* ... same for PD and PT, then write pa | flags into the leaf */
```

`unmap_page` clears the leaf entry and flushes the stale TLB (Translation
Lookaside Buffer) entry with the `invlpg` instruction. `va2pa` does a
read-only walk and returns the physical address, which is what the self-test
uses to verify mappings independently.

The physical allocator ([`kernel/pmm/pmm.c`](kernel/pmm/pmm.c)) parses the
Multiboot2 memory map, sizes a bitmap with one bit per 4 KiB frame, and places
it right after the kernel image at `kernel_end`, a symbol exported by the
linker script. It starts with all memory reserved, frees the regions the
firmware reports usable, then re-reserves the first 2 MiB, the kernel itself,
and the bitmap's own frames.

## Build and run

Needs an `x86_64-elf` cross-compiler, `grub-mkrescue`, and QEMU.

The OSDev wiki has a
[guide for building the cross-compiler](https://wiki.osdev.org/GCC_Cross-Compiler);
follow it with `x86_64-elf` as the target.

```sh
make        # build the kernel ELF
make iso    # wrap it in a bootable GRUB image
make run    # boot it in QEMU
```

Compiler flags worth noting: `-ffreestanding`, which stops GCC from assuming a
libc or a hosted environment, and `-mcmodel=kernel`, which tells GCC the code
lives in the top 2 GiB of the address space, so kernel symbols can be reached
with sign-extended 32-bit offsets instead of full 64-bit addresses.

## Resources

The [OSDev wiki](https://wiki.osdev.org) was the main reference for this
project. If you want to learn more, here are some of the references I used:

- Long mode: [Setting Up Long Mode](https://wiki.osdev.org/Setting_Up_Long_Mode)
- Higher-half layout: [Higher Half Kernel](https://wiki.osdev.org/Higher_Half_Kernel)
- Interrupts: [Interrupt Descriptor Table](https://wiki.osdev.org/Interrupt_Descriptor_Table)
- Paging: [Paging](https://wiki.osdev.org/Paging)
- Physical memory: [Page Frame Allocation](https://wiki.osdev.org/Page_Frame_Allocation)
- x86-64 architecture reference: [Intel SDM, Volume 3](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## Credit

Parts of this kernel started from [OSDev wiki](https://osdev.wiki/wiki/Expanded_Main_Page) tutorials:

- The VGA text driver boilerplate is adapted from
  [Bare Bones](https://wiki.osdev.org/Bare_Bones).
- Notable amounts of assembly for components throughout this project are adapted from
  [Setting Up Long Mode](https://wiki.osdev.org/Setting_Up_Long_Mode),
  [Higher Half x86 Bare Bones](https://wiki.osdev.org/Higher_Half_x86_Bare_Bones), and various other pages/tutorials from the wiki.

---

_winnieOS is named after my dog winnie_
