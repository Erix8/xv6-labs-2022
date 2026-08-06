# Lab 4: Traps

## Overview

This lab explores how system calls are implemented using traps, and how the kernel
can hand control to a user-space function (a primitive user-level interrupt handler).
It has three exercises: a RISC-V assembly warm-up, a kernel stack backtrace, and a
periodic-alarm mechanism implemented with two new system calls.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/traps.html

## How a trap goes user -> kernel -> user

Before starting to code, read Chapter 4 of the xv6 book and the two files that
implement the whole trap path: [`kernel/trampoline.S`](./xv6_for_Lab4/kernel/trampoline.S)
(assembly for entering/exiting the kernel) and
[`kernel/trap.c`](./xv6_for_Lab4/kernel/trap.c) (C handling of all traps).

### Why a trampoline page exists

The hard part of a trap is the **page-table switch**. When the trap happens, the CPU
is still on the *user* page table, but the kernel needs the *kernel* page table.
The code that performs the switch must therefore be mapped at the **same virtual
address in both page tables**. xv6 puts that one page of code at `TRAMPOLINE`
(`MAXVA - PGSIZE`, the highest page):

- mapped in the kernel page table: `kvmmake()` in [`kernel/vm.c`](./xv6_for_Lab4/kernel/vm.c)
  (`kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R|PTE_X)`)
- mapped in every user page table: `proc_pagetable()` in
  [`kernel/proc.c`](./xv6_for_Lab4/kernel/proc.c), *without* `PTE_U` so user code
  cannot touch it
- [`kernel/kernel.ld`](./xv6_for_Lab4/kernel/kernel.ld) forces the `trampsec`
  section to exactly one page

The per-process `struct trapframe` (see
[`kernel/proc.h`](./xv6_for_Lab4/kernel/proc.h)) sits one page below, at `TRAPFRAME`
(`TRAMPOLINE - PGSIZE`), mapped only in the *user* page table. `uservec` needs a
fixed address it can load to save registers into the current process's trapframe.

### Entering the kernel: `uservec` (trampoline.S) -> `usertrap()` (trap.c)

`usertrapret()` points `stvec` at `TRAMPOLINE + (uservec - trampoline)`, so any trap
while the process is in user mode starts at `uservec`, still on the user page table
and with the user's `sp`:

```
uservec (trampoline.S)
  1. csrw sscratch, a0         # park user a0 so a0 can be a pointer
  2. li a0, TRAPFRAME
  3. sd ra/sp/gp/tp/t0-t6/s0-s11/a1-a7, offsets 40..280(a0)
     csrr t0, sscratch; sd t0, 112(a0)   # save user a0 too
  4. ld sp, 8(a0)              # kernel stack top (trapframe->kernel_sp)
     ld tp, 32(a0)             # hartid (trapframe->kernel_hartid)
     ld t0, 16(a0)             # usertrap() address
     ld t1, 0(a0)              # kernel page table satp
     sfence.vma; csrw satp, t1; sfence.vma
  5. jr t0                     # jump (not call) to usertrap()
```

Register offsets in `uservec` correspond exactly to the field order of
`struct trapframe` in [`kernel/proc.h`](./xv6_for_Lab4/kernel/proc.h)
(`kernel_satp`=0, `kernel_sp`=8, `kernel_trap`=16, `epc`=24, `kernel_hartid`=32,
then `ra`=40 ... `a0`=112 ... `t6`=280).

`usertrap()` ([`kernel/trap.c`](./xv6_for_Lab4/kernel/trap.c)) then:

1. panics if `sstatus.SPP == 1` (trap did not come from user mode);
2. immediately points `stvec` at `kernelvec` — from now on we are in the kernel, so
   further traps must go to `kerneltrap()`;
3. saves the user PC: `p->trapframe->epc = r_sepc()`;
4. dispatches on `scause`:
   - `scause == 8` (**ecall / system call**): `p->trapframe->epc += 4` (skip the
     `ecall` instruction itself), then `intr_on()` (safe now that sepc/scause are
     saved) and `syscall()`;
   - `devintr() != 0` (**device/timer interrupt**): remember `which_dev`;
   - otherwise (**unknown exception**): print scause/sepc/stval and `setkilled(p)`;
5. `if(killed(p)) exit(-1);`, and `if(which_dev == 2) yield();` — timer interrupts
   drive the scheduler;
6. `usertrapret()` (does not return).

### Leaving the kernel: `usertrapret()` (trap.c) -> `userret` (trampoline.S) -> sret

`usertrapret()` re-arms everything the next trap will need and hands off to the
assembly `userret`:

| Set here (trap.c) | Consumed by |
| --- | --- |
| `w_stvec(TRAMPOLINE + (uservec - trampoline))` | next user trap enters at `uservec` |
| `trapframe->kernel_satp = r_satp()` | `uservec` step 4 (kernel page table) |
| `trapframe->kernel_sp = p->kstack + PGSIZE` | `uservec` step 4 (kernel stack) |
| `trapframe->kernel_trap = (uint64)usertrap` | `uservec` step 5 |
| `trapframe->kernel_hartid = r_tp()` | `uservec` step 4 (cpuid) |
| `sstatus`: clear `SPP`, set `SPIE` | `sret` returns to U mode with interrupts on |
| `w_sepc(p->trapframe->epc)` | `sret` restarts the user program at the saved PC |
| `MAKE_SATP(p->pagetable)` as arg to `userret` | `userret` switches back to the user page table |

`userret` (trampoline.S) does the reverse of `uservec`: `csrw satp, a0` back to the
user page table (with `sfence.vma` around it), restores all 31 registers from
`TRAPFRAME` in exactly the reverse order (`a0` last, offset 112), then `sret`.

> **Key idea for the Alarm exercise:** `sret` resumes user code at `sepc`, and
> `usertrapret()` sets `sepc` from `trapframe->epc`. Changing `trapframe->epc` in
> `usertrap()` is therefore how the kernel makes the user program jump to an alarm
> handler; restoring the old value (via `sigreturn`) is how it resumes the
> interrupted code.

### Kernel-mode traps: `kernelvec` (kernelvec.S) -> `kerneltrap()` (trap.c)

While the CPU is in S mode, traps go to `kernelvec`, which pushes all 32 registers
onto the *current kernel stack* (no page-table switch needed) and calls
`kerneltrap()`. The C handler saves `sepc/sstatus/scause` immediately, panics if
`SPP == 0` or interrupts are enabled, and calls `devintr()`. Before returning it
restores `sepc` and `sstatus` because `yield()` may have caused other traps.

`devintr()` returns:
- `2` = timer interrupt. The hardware timer fires in M mode; `timervec`
  ([`kernel/kernelvec.S`](./xv6_for_Lab4/kernel/kernelvec.S)) re-arms `mtimecmp` and
  *forwards* the interrupt to S mode by setting `sip` (software interrupt), which
  is what actually lands here. Only hart 0 increments the global `ticks`
  (`clockintr()`), and the SSIP bit is acknowledged by clearing it.
- `1` = other device interrupt (UART / virtio disk via PLIC: `plic_claim()`,
  handler, `plic_complete()`).
- `0` = not an interrupt at all — `kerneltrap()` panics.

## Exercises

### RISC-V assembly (easy) — DONE

Understand RISC-V assembly by reading the generated
[`user/call.asm`](./xv6_for_Lab4/user/call.asm) for the functions `g`, `f`, and
`main` (produced by `make fs.img` from [`user/call.c`](./xv6_for_Lab4/user/call.c)).
Answers are recorded in
[`answers-traps.txt`](./xv6_for_Lab4/answers-traps.txt).

**How the pieces fit together**

RISC-V passes the first 8 integer/pointer arguments in `a0`-`a7`; `ra` holds the
return address set by `jalr`. `%x`/`%d`/`%s` in `vprintf()`
([`user/printf.c`](./xv6_for_Lab4/user/printf.c)) fetch variadic arguments from the
register save area in order, so a missing argument reads garbage from a register.

**Understanding the answers**

*Which register holds 13 in main's call to `printf`?* 

`a2`. `printf("%d %d\n",
  f(8)+1, 13)` compiles to `li a1,12; li a2,13` — `a0` is the format string.

*Where are the calls to `f` and `g` in main?* 

Nowhere. `g` is inlined into `f`
  (`f`'s body is `addiw a0,a0,3; ret`, no `jal g`), `f` is inlined into `main`, and
  `f(8)+1` is constant-folded to `12` (`li a1,12`).

*Where is `printf`?* 

`0x64a` (user programs are linked at address 0, see
  [`user/user.ld`](./xv6_for_Lab4/user/user.ld)).

*What is `ra` just after `jalr` to `printf`?* 

`0x38` — the address of the next
  instruction (`jalr` sets `ra = PC+4`).

*`printf("H%x Wo%s", 57616, &i)` with `i = 0x00646c72`?* 

Prints
  `H110 World`. `57616 == 0xE110`; little-endian memory holds bytes
  `72 6c 64 00` = `"rld"`. Big-endian would need `i = 0x726c6400`; `57616` stays the
  same because `%x` prints the numeric value regardless of endianness.

*`printf("x=%d y=%d", 3)`?* 

After `y=` an **unspecified/garbage value** is
  printed: the second `%d` reads `a2`, which the caller never set up.

### Backtrace (moderate) — TODO

Implement a `backtrace()` function in `kernel/printf.c` that walks up the stack
using frame pointers and prints the saved return address in each stack frame.
Insert a call to it in `sys_sleep`, and add a call from `panic` as well.

Plan: read the frame pointer with an `r_fp()` helper added to `kernel/riscv.h`,
walk saved return address at `-8(fp)` / saved frame pointer at `-16(fp)`, and stop
using `PGROUNDDOWN(fp)` (one page = one kernel stack). Verify with `bttest` and
`addr2line -e kernel/kernel`.

### Alarm (hard) — TODO

Add `sigalarm(interval, handler)` and `sigreturn()` system calls. After every `n`
ticks of CPU time a process consumes, the kernel should call the user's handler
function. When the handler returns, the application should resume where it left off.
`sigalarm(0, 0)` stops the periodic alarms. Pass `alarmtest` and `usertests -q`.

Plan: wire up the two syscalls (user.h, usys.pl, syscall.h, syscall.c), add
`alarm_interval/alarm_handler/alarm_ticks` fields (plus a saved-trapframe area) to
`struct proc`, count ticks in `usertrap()` when `which_dev == 2`, point
`trapframe->epc` at the handler, and implement `sys_sigreturn()` to restore the
saved registers/`epc` (prevent re-entrant handler calls; remember to restore `a0`).



