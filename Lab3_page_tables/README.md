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
Write a `vmprint()` function that takes a `pagetable_t` argument and prints the
page table in the required format. Insert `if(p->pid==1) vmprint(p->pagetable)` in
`exec.c` just before `return argc` to print the first process's page table.

### Detect which pages have been accessed (hard)
Implement the `pgaccess()` system call that reports which pages have been accessed.
It takes the starting virtual address, the number of pages to check, and a buffer
to store the results as a bitmask. RISC-V hardware sets the `PTE_A` access bit in
the PTE on TLB misses; you must inspect and clear this bit.