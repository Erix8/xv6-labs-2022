# Lab 3: Page tables

## Overview

This lab explores page tables and modifies them to speed up certain system calls
and to detect which pages have been accessed.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/pgtbl.html

## Exercises

### Speed up system calls (easy)

Some operating systems (e.g. Linux) speed up certain system calls by sharing a
**read-only page** of data between userspace and the kernel. This eliminates the
kernel crossing (mode switch) normally needed to perform these system calls. In
this exercise we apply the same trick to `getpid()`: each process maps one
read-only page at `USYSCALL` storing a `struct usyscall { int pid; }`, so a user
program can read its own PID directly from memory — no trap, no system call.

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `USYSCALL` | kernel macro ([`kernel/memlayout.h`](./xv6_for_Lab3/kernel/memlayout.h)) | User virtual address where the shared read-only page is mapped (`TRAPFRAME - PGSIZE`) |
| `struct usyscall` | kernel struct ([`kernel/memlayout.h`](./xv6_for_Lab3/kernel/memlayout.h)) | `{ int pid; }` — stored at the start of the USYSCALL page |
| `kalloc()` | kernel allocator ([`kernel/kalloc.c`](./xv6_for_Lab3/kernel/kalloc.c)) | Allocate the physical page that backs USYSCALL |
| `mappages()` | kernel VM helper ([`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)) | Insert the USYSCALL mapping into the user page table |
| `proc_pagetable()` | kernel proc ([`kernel/proc.c`](./xv6_for_Lab3/kernel/proc.c)) | Builds the per-process user page table (trampoline + trapframe + usyscall) |
| `ugetpid()` | library ([`user/ulib.c`](./xv6_for_Lab3/user/ulib.c)) | Reads `((struct usyscall *)USYSCALL)->pid` directly, no trap |
| `pgtbltest` | user test ([`user/pgtbltest.c`](./xv6_for_Lab3/user/pgtbltest.c)) | Forks 64 children; each checks `getpid() == ugetpid()` |

**How the pieces fit together**

```
user space                                kernel
──────────                                ──────
user/pgtbltest.c                          memlayout.h
  fork() / ugetpid()                ──►    #define USYSCALL (TRAPFRAME - PGSIZE)
                                            struct usyscall { int pid; };
user/ulib.c: ugetpid()                      │
  struct usyscall *u =                      ▼
    (struct usyscall *)USYSCALL;          proc.c: allocproc()
  return u->pid;                          1. p->usyscall = kalloc();
  (zero trap / syscall)                   2. p->usyscall->pid = p->pid;
                                                │
                                                ▼
                                            proc.c: proc_pagetable(p)
                                            mappages(pagetable, USYSCALL, PGSIZE,
                                                     (uint64)p->usyscall, PTE_R | PTE_U); 
```

**Implementation steps**

1. **[`kernel/proc.h`](./xv6_for_Lab3/kernel/proc.h)** — add a field to `struct proc`.
2. **[`kernel/proc.c`](./xv6_for_Lab3/kernel/proc.c) — `allocproc()`**: allocate and initialize the page, **before**
   `proc_pagetable()` (which needs the physical address).
3. **[`kernel/proc.c`](./xv6_for_Lab3/kernel/proc.c) — `proc_pagetable()`**: map it read-only for the user, after
   the TRAPFRAME mapping.
4. **[`kernel/proc.c`](./xv6_for_Lab3/kernel/proc.c) — `proc_freepagetable()`**: unmap only (do *not* free the
   physical page — it belongs to `freeproc()`).
5. **[`kernel/proc.c`](./xv6_for_Lab3/kernel/proc.c) — `freeproc()`**: free the physical page.

**Key points**

- Permissions are `PTE_R | PTE_U`, *without* `PTE_W`: the user can read but not
  write the page.
- The PID page is allocated in `allocproc()` **before** `proc_pagetable()`, because
  the mapping needs `p->usyscall`'s physical address.
- `exec()` builds a fresh page table via `proc_pagetable()`, so the new page table
  automatically gets the USYSCALL mapping pointing at the same `p->usyscall` page;
  `proc_freepagetable()` only removes the mapping (`do_free=0`), so the physical
  page survives an `exec()`.
- `fork()` gives each child its own usyscall page (its own `kalloc()` and its own
  PID set in `allocproc()`), so no parent-to-child copying is needed.
- All cleanup funnels through `freeproc()` (called by `wait()` and by every failed
  `allocproc()` path), so there is a single place that frees the page.

**Why is this faster?**

A traditional `getpid()` does: user → `ecall` → trap vector → `syscall()` →
kernel → back to user (two mode switches). `ugetpid()` is just a load from a
user-mapped read-only page, so it costs **zero traps**. This is the same idea Linux
uses in its vDSO for `gettimeofday()` / `clock_gettime()`.

**Question**

> Which other xv6 system call(s) could be made faster using this shared page?
> Explain how.

Idea: any system call that returns small, slowly-changing read-only kernel state
can be moved into the shared page, e.g.:

- `uptime()` — publish the current ticks counter on the shared page; the kernel
  updates it as the timer ticks. User reads it directly instead of trapping.
- `getpid()` (already done) — the canonical example.
- `sbrk(0)` — publish the current heap size `p->sz` on the page.
- `getppid()` — publish the parent PID, if that semantics is required.

Trade-off: the value is a snapshot that may be slightly stale between kernel
updates, so it only works for data that may be stale, or that the kernel updates in
lockstep with its real state.

**Verification**

```bash
make qemu        # inside qemu:
pgtbltest        # ugetpid_test: OK (pgaccess_test: OK once part 3 is done)
make grade       # official grading script
```

### Print a page table (easy)

To help you visualize RISC-V page tables (and future debugging), write a
`vmprint()` function that recursively walks a page table and prints every valid
PTE with its depth, index, raw PTE bits and physical address. It is invoked once
for the first process right after `init` finishes `exec`-ing, so you can see the
address-space layout of the very first user process.

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `vmprint(pagetable_t)` | new kernel function ([`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)) | Print the page table rooted at `pagetable` in the required format |
| `vmprint_rec(pagetable_t, int)` | static helper ([`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)) | Recursive depth-first walk; `depth` controls the `" .."` indentation |
| `PTE_V` | kernel macro ([`kernel/riscv.h`](./xv6_for_Lab3/kernel/riscv.h)) | Valid bit — skip PTEs without it |
| `PTE_R\|PTE_W\|PTE_X` | kernel macros ([`kernel/riscv.h`](./xv6_for_Lab3/kernel/riscv.h)) | Leaf permission bits — a PTE with none of them points to a lower-level page-table page |
| `PTE2PA(pte)` | kernel macro ([`kernel/riscv.h`](./xv6_for_Lab3/kernel/riscv.h)) | Extract the physical address from a PTE |
| `%p` | printf format ([`kernel/printf.c`](./xv6_for_Lab3/kernel/printf.c)) | Print a full 64-bit value in hex (`0x...`) |
| `exec()` | kernel ([`kernel/exec.c`](./xv6_for_Lab3/kernel/exec.c)) | Call site: `if(p->pid==1) vmprint(p->pagetable);` right before `return argc` |

**How the pieces fit together**

```
boot: initcode --exec("/init")--> init process (pid=1)
                                    │
                                    ▼
                        exec.c: exec() (p->pid == 1)
                          ┌───────────────────────────────────────┐
                          │  commit new image:                     │
                          │  oldpagetable = p->pagetable;          │
                          │  p->pagetable = pagetable;   ← new page│
                          │  proc_freepagetable(oldpagetable,...); │
                          │  vmprint(p->pagetable);    ← print it  │
                          └───────────────────────────────────────┘
                                    │
                                    ▼
                        vm.c: vmprint()  "page table 0x..."
                          └─ vmprint_rec(pagetable, depth=1)
                               for each valid PTE:
                                 print " .."*depth, index, pte, pa
                                 if PTE has no R/W/X → recurse one level down
```

**Expected output** (physical addresses may differ; entry counts and virtual
addresses must match)

```
page table 0x0000000087f6b000
 ..0: pte 0x0000000021fd9c01 pa 0x0000000087f67000
 .. ..0: pte 0x0000000021fd9801 pa 0x0000000087f66000
 .. .. ..0: pte 0x0000000021fda01b pa 0x0000000087f68000
 .. .. ..1: pte 0x0000000021fd9417 pa 0x0000000087f65000
 .. .. ..2: pte 0x0000000021fd9007 pa 0x0000000087f64000
 .. .. ..3: pte 0x0000000021fd8c17 pa 0x0000000087f63000
 ..255: pte 0x0000000021fda801 pa 0x0000000087f6a000
 .. ..511: pte 0x0000000021fda401 pa 0x0000000087f69000
 .. .. ..509: pte 0x0000000021fdcc13 pa 0x0000000087f73000
 .. .. ..510: pte 0x0000000021fdd007 pa 0x0000000087f74000
 .. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
init: starting sh
```

Decoding the output (Sv39 three-level page tables):

- Line `..0:` at depth 1 is the **page-table page PTE** for virtual-address
  indices 0..511 at level 2. It has no `R/W/X` bits, so it points to another
  page-table page.
- Under it, `.. ..0:` is the level-1 page-table page for index 0.
- Under that, `.. .. ..0/1/2/3:` are **leaf** PTEs mapping the program text,
  data and stack pages near address 0.
- `..255:` (depth 1) is the level-2 index 255 — the high user addresses where
  the user stack guard, stack, trapframe and trampoline live, and its
  descendants `.. .. ..509/510/511:` are the corresponding leaves.

**Implementation steps**

1. **[`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)** — implement
   `vmprint(pagetable_t)` plus a static recursive helper
   `vmprint_rec(pagetable, depth)`, placed near `freewalk()` (the recursion
   structure mirrors it). The helper iterates over all 512 PTEs, skips invalid
   ones (`PTE_V`), prints the indentation/`pte`/`pa` line, and recurses one level
   deeper whenever the PTE has no `R/W/X` bits (i.e. it points to a lower-level
   page-table page).
2. **[`kernel/defs.h`](./xv6_for_Lab3/kernel/defs.h)** — declare the prototype
   `void vmprint(pagetable_t);` in the `// vm.c` section so `exec.c` can call it.
3. **[`kernel/exec.c`](./xv6_for_Lab3/kernel/exec.c)** — after committing the new
   user image (just before `return argc`), insert `if(p->pid==1) vmprint(p->pagetable);`
   to print the first process's page table.

**Key points**

- Only print PTEs with `PTE_V` set — otherwise you would dump 512 garbage lines.
- Page-table pages deeper in the tree are **still printed** (e.g. `..0:`, `..255:`),
  not just leaf PTEs.
- A PTE whose `R/W/X` bits are all zero points to a lower-level page table;
  recurse with `PTE2PA(pte)` as the next `pagetable_t`.
- Indentation: `depth` 1 → `" .."`, 2 → `" .. .."`, 3 → `" .. .. .."` — the same
  leading spaces as the example.
- Index printed is the PTE's slot `i` (0-511) in the current page-table page, not
  a virtual address.
- Both the raw PTE and the physical address are printed with `%p` so they show as
  full 64-bit hex values.

**Question**

> Explain the output of `vmprint` in terms of Fig 3-4 from the text. What does
> page 0 contain? What is in page 2? When running in user mode, could the process
> read/write the memory mapped by page 1? What does the third to last page contain?

Idea: page 0 is the mapped user program (text/data), page 2 is the user stack;
page 1 is the guard page (its PTE has no `PTE_U`, so the process can neither read
nor write it — any access faults); the third-to-last page is the trapframe.
Correlate the leaf PTE indices and their permission bits with Fig 3-4.

**Verification**

```bash
make qemu        # boot output should include "page table 0x..." before init starts
make grade       # pte printout test must pass
```

### Detect which pages have been accessed (hard)

Some garbage collectors (a form of automatic memory management) benefit from
knowing which pages have been accessed (read or write). RISC-V's hardware page
walker automatically sets the `PTE_A` (Accessed) bit in a PTE whenever it resolves
a TLB miss. This exercise adds a `pgaccess()` system call that inspects these bits
and reports them to userspace as a bitmask.

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `pgaccess(va, npages, mask)` | new system call | Report which pages in `[va, va+npages*PGSIZE)` were accessed; page *i* → bit *i* (LSB = first page) |
| `PTE_A` | new kernel macro ([`kernel/riscv.h`](./xv6_for_Lab3/kernel/riscv.h)) | `1L << 6` — the Accessed bit; set by hardware on TLB misses, cleared by software |
| `argaddr(0/2, ...)` | kernel arg parser ([`kernel/syscall.c`](./xv6_for_Lab3/kernel/syscall.c)) | Fetch the start address and the user bitmask pointer |
| `argint(1, &npages)` | kernel arg parser ([`kernel/syscall.c`](./xv6_for_Lab3/kernel/syscall.c)) | Fetch the number of pages to check |
| `walk(pagetable, va, 0)` | kernel VM helper ([`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)) | Find the leaf PTE for a user virtual address without allocating |
| `copyout(...)` | kernel VM helper ([`kernel/vm.c`](./xv6_for_Lab3/kernel/vm.c)) | Copy the kernel-side bitmask back to the user buffer |
| `pgtbltest` | user test ([`user/pgtbltest.c`](./xv6_for_Lab3/user/pgtbltest.c)) | `pgaccess_test`: accesses pages 1, 2, 30 then checks the returned bitmask |

**How the pieces fit together**

```
user space                                kernel
──────────                                ──────
user/pgtbltest.c                          syscall.h:  SYS_pgaccess 30 (already wired)
  buf = malloc(32*PGSIZE);                syscall.c:  syscalls[SYS_pgaccess] = sys_pgaccess
  pgaccess(buf, 32, &abits)               user/usys.pl: entry("pgaccess") (already generated)
        │                                 user/user.h: int pgaccess(void*,int,void*) (already)
        ▼                                        │
                                     sysproc.c: sys_pgaccess()
                                       argaddr(0, &va);  argint(1, &npages);
                                       argaddr(2, &uaddr);
                                       for each page i:
                                         pte = walk(p->pagetable, va + i*PGSIZE, 0)
                                         if pte valid && PTE_A set:
                                           abits |= (1 << i);
                                           *pte &= ~PTE_A;      // clear for next call
                                       copyout(p->pagetable, uaddr, &abits, ...)
```

**Implementation steps**

1. **[`kernel/riscv.h`](./xv6_for_Lab3/kernel/riscv.h)** — define the access bit near
   the other PTE flags (RISC-V privileged spec: bit 6):
   ```c
   #define PTE_A (1L << 6) // accessed
   ```
2. **[`kernel/sysproc.c`](./xv6_for_Lab3/kernel/sysproc.c) — `sys_pgaccess()`** —
   replace the provided stub:
   - parse the three arguments (`argaddr` ×2, `argint` ×1);
   - reject `npages <= 0` or a page count above a sane cap (e.g. 64);
   - loop `i` in `[0, npages)`: `va + i*PGSIZE` → `walk(..., 0)`;
   - skip unmapped pages (`pte == 0 || !(PTE_V)`); if `PTE_A` is set, set bit `i`
     in a kernel-side temp and **clear `PTE_A`**;
   - `copyout` the temp bitmask to the user buffer; return `0`, or `-1` on error.

**Key points**

- `PTE_A` is the RISC-V **Accessed** bit (`1 << 6`); hardware sets it on a TLB-miss
  page-table walk, software clears it.
- Use `walk(..., alloc=0)` so unmapped addresses just return `0` — never allocate a
  page table page here.
- Build the bitmask in a **kernel temporary** and `copyout` it once at the end
  (safer than writing into user memory repeatedly).
- **Always clear `PTE_A`** after reading it; otherwise the bit stays set forever
  and you cannot tell what was accessed since the last `pgaccess()` call.
- The first page (LSB) corresponds to `va`, page *i* to `va + i*PGSIZE`.
- Note that `argint`/`argaddr` return `void` in this xv6 — legality is checked by
  `walk` and `copyout`, not by the argument fetch.

**Verification**

```bash
make qemu        # inside qemu:
pgtbltest        # pgaccess_test: OK — all tests succeeded
make grade       # official grading script
```
