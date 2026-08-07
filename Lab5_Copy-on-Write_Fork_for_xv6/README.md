# Lab 5: Copy-on-Write Fork for xv6

## Overview

This lab explores copy-on-write (COW) fork. Virtual memory provides indirection,
allowing the kernel to intercept memory references and defer work until needed.
The goal is to defer allocating and copying physical memory pages in `fork()` until
the copies are actually needed.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/cow.html

### The problem with the original fork()

The original `fork()` in xv6 copies **all** of the parent process's user-space
memory into the child (see `uvmcopy()` in
[`kernel/vm.c`](./xv6_for_Lab5/kernel/vm.c): it walks every page, `kalloc()`s a new
physical page, and `memmove()`s the contents). This has two serious drawbacks:

- It is **slow**: if the parent is large, copying takes a long time.
- The work is usually **wasted**: `fork()` is commonly followed by `exec()` in the
  child, which discards the copied memory without ever using most of it.

This is why the very first `cowtest` ("simple") fails on an unmodified kernel —
`fork()` needs a full physical copy of a parent that has allocated more than half of
the physical memory, so it runs out of memory.

### The COW idea

Only copy a page when it is **actually written**:

1. On `fork()`, the child's page table maps the **same physical pages** as the parent
   (no copying). All user PTEs that were writable are made read-only (`PTE_W`
   cleared) in **both** parent and child, and marked with a `PTE_COW` flag.
2. When either process writes such a page, the CPU raises a **store page fault**
   (`scause == 15`). The kernel handler allocates a new physical page, copies the
   original contents into it, and reinstalls the PTE with `PTE_W` set.
3. Genuinely read-only pages (e.g. the text segment) stay shared and read-only;
   writing one kills the process, as before.
4. Since a physical page may be referenced by several page tables, each page keeps a
   **reference count**; it is freed only when the last reference disappears.

### Reference links

- [xv6 book (Chapter 3 "Page tables", Chapter 4 "Traps" are the relevant reading)](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)
- [RISC-V privileged ISA manual (PTE flags, Sv39 layout; scause exception codes §4.1.3)](https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf)
- [LWN: "Patching until the COWs come home" (why refcounting is hard in production kernels)](https://lwn.net/Articles/849638/)

## Key mechanisms used

### Marking a COW page: the RSW bits

The RISC-V PTE has two bits (bit 8 and bit 9) **reserved for software** (RSW). The
hardware ignores them during address translation, so the kernel can use them as its
own bookkeeping flags. We use bit 8 as `PTE_COW` (defined in
[`kernel/riscv.h`](./xv6_for_Lab5/kernel/riscv.h)). The existing macro
`PTE_FLAGS(pte)` already masks the low 10 bits, so the flag survives every
`PTE_FLAGS`/`mappages` round trip.

### The store page fault

On RISC-V:

| scause | Meaning |
| --- | --- |
| 8 | `ecall` (system call) |
| 13 | load page fault |
| **15** | **store page fault** |

A COW page is mapped with `PTE_R` still set, so *reads* succeed without a fault in
user mode. *Writes* fault with **scause == 15**, and `stval` holds the faulting
virtual address. This is exactly the hook `usertrap()` in
[`kernel/trap.c`](./xv6_for_Lab5/kernel/trap.c) uses to detect a COW write.

### Why copyout() also needs attention

`copyout()` copies kernel data into user memory (e.g. `read()` into a user buffer, a
`pipe` write to the child's page). It writes through the **physical address** in
kernel mode, which never runs the user page-fault handler. Without special handling
it would happily overwrite a shared COW page, corrupting the other process's data.
So `copyout()` must apply the same COW logic as a page fault.

## Implementation

All changes are in the xv6 kernel for Lab 5:
[`xv6_for_Lab5/kernel/`](./xv6_for_Lab5/kernel).

### 1. Define the COW flag — [`kernel/riscv.h`](./xv6_for_Lab5/kernel/riscv.h)

```c
#define PTE_U (1L << 4) // user can access
#define PTE_COW (1L << 8) // Copy-on-Write: uses an RSW bit
```

**Why bit 8:** it is one of the hardware-ignored RSW bits (the official hint says to
use them), so no extra data structure is needed — the "am I a COW page?" answer
lives inside the PTE itself and automatically dies with the mapping.

### 2. Physical-page reference counting — [`kernel/kalloc.c`](./xv6_for_Lab5/kernel/kalloc.c)

A fixed-size array, one `int` per physical page, protected by `kmem.lock`:

```c
#define PA2IDX(pa) (((uint64)(pa)) >> PGSHIFT)
static int refcnt[PHYSTOP / PGSIZE];
```

- `kalloc()` sets `refcnt = 1` when it hands a page out.
- `krefinc(pa)` (new, declared in [`kernel/defs.h`](./xv6_for_Lab5/kernel/defs.h))
  bumps the count when a child page table starts sharing the page.
- `kfree(pa)` only really frees when the count is 0 or 1:
  - count > 1 → decrement and return (do **not** memset or return to the free list,
    the page is still in use);
  - count ≤ 1 → this is the last reference (or the `kinit()` initialization path),
    so memset junk, link it into the free list, and reset the count to 0.

**Why index by `pa >> 12` and size `PHYSTOP / PGSIZE`:** the official hint suggests
indexing by physical-address/4096 with the array size equal to the highest physical
address put on the free list by `kinit()`. This gives a compile-time constant array
(≈2.2 MB for 128 MB of RAM) and a trivial one-shift index that does not depend on
the linker symbol `end`.

**Why count every shared page (including read-only ones):** a read-only text page is
also referenced by two page tables after a COW fork. Without a count, the second
`kfree()` would return a page the first process still maps — a use-after-free.

### 3. Make fork() share instead of copy — `uvmcopy()` in [`kernel/vm.c`](./xv6_for_Lab5/kernel/vm.c)

```c
if(flags & PTE_W){
  // The page was writable, so after fork it becomes a COW page:
  // clear PTE_W in both parent and child and set PTE_COW.
  *pte = (*pte & ~PTE_W) | PTE_COW;
  flags = (flags & ~PTE_W) | PTE_COW;
}
krefinc((void*)pa);
if(mappages(new, i, PGSIZE, pa, flags) != 0){
  kfree((void*)pa);   // undo the krefinc above
  goto err;           // uvmunmap unwinds the pages already mapped
}
```

Key design points:

- **Clear `PTE_W` in the parent too.** The COW invariant is that a shared page is
  *writable nowhere*. If only the child's PTE were made read-only, the parent could
  keep writing the shared page and silently corrupt the child's view.
- **Only pages that were writable become COW.** Pages that were already read-only
  (text) are simply shared; a write to them still faults and kills the process.
- **`krefinc()` for every shared page**, even read-only ones (see above).
- **Error path keeps counts balanced:** the failing page's extra reference is undone
  with `kfree(pa)` (which only decrements, since the parent still references it),
  and `uvmunmap(new, 0, i / PGSIZE, 1)` unwinds the already-mapped pages, each
  `kfree` dropping one reference.

### 4. Handle the COW page fault and serve copyout() — `cowalloc()` in [`kernel/vm.c`](./xv6_for_Lab5/kernel/vm.c)

A new helper shared by both the trap handler and `copyout()`:

```c
int
cowalloc(pagetable_t pagetable, uint64 va)
{
  // bounds-check va, PGROUNDDOWN it, walk() to the PTE;
  // if not PTE_COW: already writable -> return 0, else return -1
  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);
  if((mem = kalloc()) == 0)
    return -1;                  // out of memory: caller kills the process
  memmove(mem, (char*)pa, PGSIZE);         // copy *before* kfree (junk!)
  *pte = PA2PTE(mem) | (flags & ~PTE_W & ~PTE_COW) | PTE_W;
  sfence_vma();                            // make the old TLB entry stale
  kfree((void*)pa);                        // drop our reference to the old page
  return 0;
}
```

Design points:

- **Copy before freeing.** `kfree()` overwrites a page with junk when the count hits
  zero; the old page must still hold its original contents when we `memmove` them.
- **Keep the other flags.** The new private page keeps `V/R/X/U` from the original,
  clears `PTE_W`/`PTE_COW` and then explicitly sets `PTE_W` — so e.g. an executable
  page stays executable.
- **`sfence_vma()`** flushes the stale TLB entry that still maps the old physical
  page as read-only.
- **Return 0 for a non-COW but already writable page.** This lets `copyout()` call
  `cowalloc()` on *every* destination page cheaply; ordinary pages pass through with
  no work, while a write to a genuine read-only page returns -1.

### 5. Catch the fault in `usertrap()` — [`kernel/trap.c`](./xv6_for_Lab5/kernel/trap.c)

```c
} else if(r_scause() == 15){
  // store page fault: a write to a COW page
  if(cowalloc(p->pagetable, r_stval()) != 0){
    printf("usertrap(): COW fault scause=%p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }
}
```

`stval` carries the faulting virtual address; `cowalloc()` page-aligns it and
validates it. Failure (writing a genuinely read-only page, or no free memory) sets
the kill flag, matching the requirement that out-of-memory COW faults kill the
process.

### 6. Teach copyout() about COW — [`kernel/vm.c`](./xv6_for_Lab5/kernel/vm.c)

```c
while(len > 0){
  va0 = PGROUNDDOWN(dstva);
  if(cowalloc(pagetable, va0) != 0)   // simulate a COW fault
    return -1;
  pa0 = walkaddr(pagetable, va0);
  ...
  memmove((void *)(pa0 + (dstva - va0)), src, n);
```

This is step 4 of the official plan: `copyout()` uses the same scheme as page faults
when it encounters a COW page, because kernel-mode direct writes never trigger a
user-mode page fault.

## Testing

The grading harness checks `cowtest` and `usertests -q` exactly as described in the
lab description. Run it from
[`xv6_for_Lab5`](./xv6_for_Lab5):

```sh
make grade          # run all grading tests
```

Result on this implementation:

```
== Test running cowtest ==
== Test   simple ==   simple: OK
== Test   three ==    three: OK
== Test   file ==     file: OK
== Test usertests ==
== Test   usertests: copyin ==   OK
== Test   usertests: copyout ==  OK
== Test   usertests: all tests == OK
== Test time ==       time: OK
Score: 110/110
```

The `time.txt` file (required by the lab description) is also included.