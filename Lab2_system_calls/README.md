# Lab 2: System calls

## Overview

This lab adds some new system calls to xv6, which helps you understand how system
calls work and exposes you to some of the internals of the xv6 kernel.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/syscall.html

### How a system call traps into the kernel

Before diving into gdb, it helps to understand the full path a system call takes from
user space to the kernel. In xv6 this is a *two-stage* process driven by RISC-V's
trap machinery:

```
user process (e.g. initcode)          kernel
─────────────────────────────          ──────────────────────────────
  li a0, arg0            ──┐
  li a1, arg1              │  arguments in a0..a5
  li a7, SYS_exec          │  syscall number in a7
  ecall                  ──┘  hardware trap: jump via stvec, switch to S-mode
        │
        ▼
  trampoline.S: uservec        -- still on the *user* page table
    1. save all 32 user registers into p->trapframe
    2. load kernel sp / tp / trap handler from p->trapframe
    3. csrw satp, t1           -- switch to the *kernel* page table
    4. jr t0                   -- jump (not call!) to usertrap()
        │
        ▼
  trap.c: usertrap()           -- now on the kernel stack
    - SPP == 0? must come from user mode
    - save sepc into p->trapframe->epc
    - if scause == 8 (ecall): syscall()
        │
        ▼
  syscall.c: syscall()
    - num = p->trapframe->a7      <-- the a7 you inspect in gdb
    - p->trapframe->a0 = syscalls[num]();
```

Key takeaway for gdb: by the time you hit a breakpoint in `syscall()`, the CPU has
**already switched to the kernel page table**, so the *user* virtual addresses
(`0x0`..) are not mapped and cannot be read with `x/s` directly.

### Reference links

- [xv6 book (Chapter 4 "Traps and system calls" is the relevant reading)](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)
- [RISC-V privileged ISA manual (sstatus bit definitions, §4.1)](https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf)
- [MIT GDB slides ("Using the GNU Debugger")](https://pdos.csail.mit.edu/6.828/2019/lec/gdb_slides.pdf)

## Exercises

### Using gdb (easy)

Become familiar with the xv6 kernel under gdb by breaking inside `syscall()` and
inspecting the stack, registers, and process state. Record your answers in
`answers-syscall.txt`.

**GDB commands used**

| Command | Kind | What you observe |
| --- | --- | --- |
| `b syscall` | breakpoint | Stop every time any process enters the kernel's `syscall()` dispatcher |
| `c` | continue | Run until the next breakpoint (or a trap/interrupt elsewhere) |
| `layout src` | TUI | Split-screen source view showing where the current pc is |
| `backtrace` | stack | The chain of *kernel* frames: `syscall` ← `usertrap` ← `??` |
| `p /x *p` | inspect | `struct proc` of the current process, in hex (see [`kernel/proc.h`](./xv6_for_Lab2/kernel/proc.h)) |
| `p /x p->trapframe->a7` | inspect | The syscall number of the system call being dispatched |
| `p /x $sstatus` | CSR | Current supervisor status register; bit 8 (SPP) tells the previous privilege mode |

**Approach**

1. In the lab directory run `make qemu-gdb` in one terminal; it boots xv6 under QEMU
   with a gdb stub listening on port 25000.
2. In a second terminal, start gdb from the same directory (the generated
   [`.gdbinit`](./xv6_for_Lab2/.gdbinit) attaches to the stub and loads
   `kernel/kernel` automatically). Type:
   ```
   (gdb) b syscall
   (gdb) c
   ```
   The first hit is the very first system call of the machine: `initcode` calling
   `exec("/init", argv)`.
3. `layout src` to see the source, then `backtrace`. The top frame should be the
   familiar `syscall()`; below it `usertrap()`; below that `??` (see "Why is the top
   frame `??`" below).
4. `n` a few times to step past `struct proc *p = myproc();`, then `p /x *p` — you see
   the full `struct proc`: `pid`, `state`, `name`, `kstack`, `trapframe`, ... (the
   fields are defined in [`kernel/proc.h`](./xv6_for_Lab2/kernel/proc.h)).
5. Inspect the syscall number and arguments:
   ```
   (gdb) p /x p->trapframe->a7
   (gdb) p /x p->trapframe->a0
   ```
   `a7` is the dispatch index; `a0..a5` are the arguments. (See "What does a7 mean"
   below.)
6. Inspect the previous privilege mode:
   ```
   (gdb) p /x $sstatus
   ```
   Bit 8 (SPP) of `sstatus` records the privilege mode the CPU was in before the
   trap. (See "How to read $sstatus" below.)

**Understanding the answers**

*Which function called `syscall`?*

`usertrap()` in [`kernel/trap.c`](./xv6_for_Lab2/kernel/trap.c). The chain shown by
`backtrace` is `syscall` ← `usertrap`, because `usertrap()` detects `scause == 8`
(an `ecall`) and calls `syscall()` directly.

*What is `p->trapframe->a7` and what does it represent?*

`a7` is RISC-V register x17. By ABI convention it carries the **system call number**,
while `a0..a5` carry the arguments. `syscall()` uses it as an index into the
`syscalls[]` dispatch table in [`kernel/syscall.c`](./xv6_for_Lab2/kernel/syscall.c):

```c
num = p->trapframe->a7;
p->trapframe->a0 = syscalls[num]();
```

At the first breakpoint hit you will see `a7 = 0x7`, i.e. `SYS_exec` (see
[`kernel/syscall.h`](./xv6_for_Lab2/kernel/syscall.h)). This is `initcode`
([`user/initcode.S`](./xv6_for_Lab2/user/initcode.S)) — the *first* user program xv6
runs — requesting `exec("/init", argv)` to become the real init process
([`user/init.c`](./xv6_for_Lab2/user/init.c) only issues its `exec("sh", argv)` later,
after it is itself running). 

`initcode` is special: it has no C source at all. Its
hand-assembled machine code is embedded into the kernel as the byte array
`initcode[]` in [`kernel/proc.c`](./xv6_for_Lab2/kernel/proc.c), and `userinit()`
copies those bytes into the first user page because the file system is not available
yet at boot. This is why `a0 = 0x24`: offset `0x24` of that page is exactly the
`"/init\0"` string.

*What was the previous mode the CPU was in?*

`0x22` = `0b0010_0010`. Decoding the relevant bits of `sstatus` (defined in
[`kernel/riscv.h`](./xv6_for_Lab2/kernel/riscv.h) and the RISC-V privileged spec):

```
bit 8  SPP   = 0  -> previous mode was User (0=User, 1=Supervisor)
bit 5  SPIE  = 1  -> interrupts were enabled before the trap (in user mode)
bit 1  SIE   = 1  -> interrupts are enabled now (in supervisor mode)
```

SPP = 0 tells us the trap came from **user mode**, consistent with `ecall` from
`initcode`. `usertrap()` even checks this and panics if it is not true:

```c
if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");
```

**Why is the top frame `??` (and how to read user memory)?**

Two things can look surprising when you start:

1. *`backtrace` ends with `0x0505050505050505 in ?? ()`.* This is not a real caller.
   `uservec` reaches `usertrap()` with `jr t0` (**jump**, not `jalr`/call), so no
   return address is pushed on the kernel stack; the "caller" of `usertrap` is the
   hardware trap, not a C function. GDB's backtrace blindly interprets the next
   stack slot as a return address, and that slot holds the junk pattern
   `0x05` (`memset(r, 5, PGSIZE)` in [`kernel/kalloc.c`](./xv6_for_Lab2/kernel/kalloc.c)
   fills freshly allocated stack pages). That value is not a valid code address, so
   gdb prints `??`.

2. *`x/s p->trapframe->a0` fails with "Cannot access memory at address 0x24".*
   Not an error on your side: since `uservec` switched to the **kernel page table**
   before `usertrap()` ran, the kernel page table does not map the user address space
   (`0x0`..). To see the `/init` string anyway, use the kernel's own copy:

   ```
   (gdb) p &initcode
   (gdb) x/s &initcode + 0x24
   ```

**Tracking down a kernel page-fault panic**

*How the panic is provoked*

Change the first line of `syscall()` in
[`kernel/syscall.c`](./xv6_for_Lab2/kernel/syscall.c) to dereference address 0
instead of reading the trapframe, then run `make qemu`:

```c
// num = p->trapframe->a7;
num = *(int *)0;
```

The kernel faults while booting:

```
xv6 kernel is booting
hart 1 starting
hart 2 starting
scause 0x000000000000000d
sepc=0x0000000080001ff4 stval=0x0000000000000000
panic: kerneltrap
```

*Why does the kernel crash (understand, don't just fix)*

1. `*(int *)0` asks the CPU to load a word from **virtual address 0**.
2. Look at the kernel address-space map (xv6 book, figure 3-3): the kernel
   page table maps only `CLINT` (0x2000000) upward and `KERNBASE`
   (0x80000000) upward. Virtual address 0 is **not mapped** at all.
3. The MMU page-table walk for address 0 fails while the CPU is in S-mode,
   raising a **load page fault**. In RISC-V, exception code 13 (`0xd`)
   *is* the load page fault. `stval` records the faulting virtual address
   (`0`, confirming the fault is on address 0) and `sepc` records the
   faulting instruction address (`0x80001ff4`).
4. Because this fault happened in kernel mode, `stvec` points at
   `kernelvec`, so `kerneltrap()` handles it. `devintr()` returns 0 (this
   is neither a device nor a timer interrupt), and `kerneltrap()` panics.

*Why `scause` is 13:* RISC-V supervisor exception codes use the top bit to
distinguish interrupts (bit 63 set, e.g. `0x8000000000000001` = supervisor
timer interrupt); a value without the top bit is a synchronous exception.
`13` with no top bit is the **load page fault** (scause table in the RISC-V
privileged spec, §4.1.3).

*Which assembly instruction faults, and which register is `num`?*

Search for the printed `sepc` value in `kernel/kernel.asm`:

```
80001ff4:  00002683          lw  a3,0(zero)   # 0 <_entry-0x80000000>
```

This is the compiled form of `num = *(int *)0;` — it loads a word from
address 0 (**`zero`** = register x0, always 0) into **`a3`**. So the
register that corresponds to the C variable `num` is **`a3` (x13)**.

*Verify with gdb*

```
(gdb) b *0x0000000080001ff4
(gdb) layout asm
(gdb) c
```

At the faulting instruction, confirm the identity of the process that was
running when the kernel panicked:

```
(gdb) p p->name
(gdb) p p->pid
```

`p->name` is "initcode" and `p->pid` is 1: the panic happened in the very
first system call of the machine (initcode's `exec("/init")`), because the
page-faulting instruction is in `syscall()`, which only runs after a user
`ecall`.

**Solution / answers:** record all the answers above in `answers-syscall.txt`.

### System call tracing (moderate)

Add a `trace` system call that controls tracing. It takes an integer "mask" whose
bits specify which system calls to trace. The kernel prints a line with the process
id, system call name, and return value when a traced system call is about to return.
Tracing applies to the calling process and its children.

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `trace(int mask)` | new system call | Enable tracing for the calling process; bit *n* of `mask` selects syscall number *n* (e.g. `1 << SYS_fork` traces `fork`) |
| `fork()` | system call | Every child inherits the parent's trace mask, so tracing propagates to the whole process tree |
| `exec(path, argv)` | system call | The first traced call you'll observe: `trace` runs its command by `exec`-ing it |
| `printf(fd, ...)` | library (printf.c) | Kernel-side: emits `pid: syscall name -> return value` to the console |
| `argint(0, &n)` | kernel arg parser | In [`kernel/syscall.c`](./xv6_for_Lab2/kernel/syscall.c): fetch the first system-call argument (the mask) from the trapframe |
| `atoi(argv[1])` | library (ulib.c) | Convert the mask string (e.g. `"32"`, `"2147483647"`) to an `int` |

**How the pieces fit together**

Adding a system call touches five places, three in user space and two in the kernel:

```
user space                                  kernel
──────────                                  ──────
user/trace.c  calls trace(mask)      ──►    syscall.h:  #define SYS_trace 22
                                              │
user/user.h   declares int trace(int)        │
user/usys.pl  generates the stub:            ▼
              li a7, SYS_trace  ecall        syscall.c:  syscalls[SYS_trace] = sys_trace
                                              │
                                              ▼
                                             sysproc.c:  sys_trace() {
                                               argint(0, &mask);
                                               myproc()->trace_mask = mask; }
```

1. **Makefile** — add `$U/_trace` to `UPROGS` so `user/trace.c` is compiled into the
   file-system image.
2. **User space stubs**
   - [`user/user.h`](./xv6_for_Lab2/user/user.h): declare `int trace(int);`
   - [`user/usys.pl`](./xv6_for_Lab2/user/usys.pl): add `entry("trace")`; when you run
     `make`, the script generates `user/usys.S` with the assembly stub
     `li a7, SYS_trace; ecall; ret`.
   - [`kernel/syscall.h`](./xv6_for_Lab2/kernel/syscall.h): assign the new syscall
     number `#define SYS_trace 22`.
3. **Kernel dispatch** — register the handler in [`kernel/syscall.c`](./xv6_for_Lab2/kernel/syscall.c):
   - `syscalls[SYS_trace] = sys_trace` in the dispatch table;
   - add `"trace"` to the `syscall_names[]` array used for the printed output;
   - in `syscall()` itself, after `p->trapframe->a0 = syscalls[num]();`, check the mask
     and print the trace line:
     ```c
     if(p->trace_mask & (1 << num)) {
       printf("%d: syscall %s -> %d\n",
              p->pid, syscall_names[num], p->trapframe->a0);
     }
     ```
4. **Kernel state** — add an `int trace_mask` field to `struct proc`
   ([`kernel/proc.h`](./xv6_for_Lab2/kernel/proc.h)) and implement
   `sys_trace()` in [`kernel/sysproc.c`](./xv6_for_Lab2/kernel/sysproc.c): read the
   mask with `argint(0, &mask)` and store it in `myproc()->trace_mask`, returning 0.
5. **Inheritance** — in `fork()` ([`kernel/proc.c`](./xv6_for_Lab2/kernel/proc.c)),
   copy the mask so children are traced too:
   ```c
   np->trace_mask = p->trace_mask;
   ```

**Understanding the output**

- `32` is `1 << SYS_read` (see [`kernel/syscall.h`](./xv6_for_Lab2/kernel/syscall.h)),
  so only `read` is traced.
- `2147483647` has all 31 low bits set, so every system call is traced (`trace ->
  0`, `exec -> 3`, `open -> 3`, `read -> 1023`, ..., `close -> 0`).
- The line is printed **after** the syscall handler returns, so the value shown is the
  return value stored back into `p->trapframe->a0`.
- `trace` itself is traced by its own call (`4: syscall trace -> 0` in the second
  example above) because the mask is already set when `syscall()` dispatches the
  `trace` system call.
- Because each `fork`ed child inherits the mask, descendants of a traced process are
  traced too — this is why the `forkforkfork` example shows traced `fork` calls with
  *different* pids (each descendant inherited the mask from `usertests`).

### Sysinfo (moderate)

Add a `sysinfo` system call that collects information about the running system.
It fills a `struct sysinfo` with the number of bytes of free memory (`freemem`)
and the number of non-`UNUSED` processes (`nproc`). You pass when `sysinfotest`
prints "sysinfotest: OK".