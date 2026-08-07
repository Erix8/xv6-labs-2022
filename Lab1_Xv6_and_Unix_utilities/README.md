# Lab 1: Xv6 and Unix utilities

## Overview

This lab familiarizes you with xv6 and its system calls. It is the only lab that is
purely about *user space*: you implement five classic Unix utilities
(sleep / pingpong / primes / find / xargs) as ordinary xv6 user programs, which is
also a warm-up for reading xv6 source code and using the build/test infrastructure.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/util.html

### Boot xv6

The lab repository is set up so that `git clone` checks out the `util` branch. Build
and run xv6 with:

```sh
make qemu
```

You should see the xv6 kernel boot and then a shell prompt `$`. Try `ls` to list the
initial file system, and `Ctrl-p` to make the kernel print the process table (you
will see one line for `init` and one for `sh`). To quit qemu type `Ctrl-a x`.

**Making sense of the initial file system:** the files listed by `ls` are the
programs that `mkfs` baked into `fs.img`; most of them are the standard xv6 user
programs you can run from the shell.

### User-program basics in xv6

- Every user program starts at `main(int argc, char *argv[])`; `argc` / `argv` are set
  up by the kernel during `exec`.
- System calls are declared as ordinary functions in [`user/user.h`](./xv6_for_Lab1/user/user.h),
  dispatched to the kernel through the assembly stubs in [`user/usys.S`](./xv6_for_Lab1/user/usys.S),
  and implemented in [`kernel/sysproc.c`](./xv6_for_Lab1/kernel/sysproc.c) (e.g., `sys_sleep`).
- The xv6 user library is intentionally small: apart from system calls, only
  [`user/ulib.c`](./xv6_for_Lab1/user/ulib.c) (strings, `atoi`, ...),
  [`user/printf.c`](./xv6_for_Lab1/user/printf.c) (formatted I/O), and
  [`user/umalloc.c`](./xv6_for_Lab1/user/umalloc.c) (malloc/free) are available.
- New programs must be added to the `UPROGS` list in the
  [`Makefile`](./xv6_for_Lab1/Makefile), otherwise they won't be compiled into the
  file-system image.

### Reference links

- [xv6 book (official textbook; reading Chapter 1 is recommended before coding)](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)
- [xv6-riscv source code on GitHub](https://github.com/mit-pdos/xv6-riscv)
- [Doug McIlroy's original prime-sieve page (source of the primes exercise)](http://swtch.com/~rsc/thread/)

## Exercises

### 1. sleep (easy)

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `sleep(int)` | system call | Pause the calling process for the given number of ticks (a tick is the time between two timer-chip interrupts) |
| `atoi(const char *)` | library (ulib.c) | Convert a string to an integer |
| `exit(int)` | system call | Terminate the current process with a status |
| `fprintf(fd, ...)` | library (printf.c) | Formatted output to a given fd (fd 2 = stderr) |
| `argc` / `argv` | C main arguments | Command-line arguments set up by the kernel |

**Approach**

1. Check `argc < 2`: if the argument is missing, print `Usage: sleep ticks` to stderr
   and call `exit(1)`.
2. Convert the string argument with `atoi(argv[1])`.
3. Call `sleep(ticks)`; the kernel puts the process to sleep and wakes it up after the
   timer fires.
4. End with an explicit `exit(0)` — unlike Linux, xv6 has no fallback "return from
   main" path that terminates the process for you.

**Expected output**

```
$ sleep 10
(nothing happens for a little while)
$
```

Solution: [`user/sleep.c`](./xv6_for_Lab1/user/sleep.c)

### 2. pingpong (easy)

**UNIX interfaces used**

| Interface | Description |
| --- | --- |
| `pipe(int p[2])` | Create a pipe; `p[0]` is the read end, `p[1]` the write end |
| `fork()` | Duplicate the calling process; returns 0 in the child, the child's pid in the parent |
| `read(fd, buf, n)` / `write(fd, buf, n)` | Blocking byte-stream I/O; on a pipe, `read` blocks until data is written and `write` blocks until the reader drains |
| `close(fd)` | Close a file descriptor |
| `getpid()` | Return the pid of the calling process |
| `wait(int *)` | Parent waits for a child to exit (and reaps it) |

**Approach**

1. Create two pipes: `p1` (parent → child) and `p2` (child → parent), one per direction.
2. After `fork()`, each process immediately closes the descriptors it doesn't need, so
   each side keeps only "the end it reads + the end it writes":
   - child closes `p1[1]`, `p2[0]`; parent closes `p1[0]`, `p2[1]`.
3. Parent writes one byte (`write(p1[1], "x", 1)`); the child reads it from `p1[0]`,
   prints `<child_pid>: received ping`, and writes the same byte back on `p2[1]`.
4. Parent reads the byte from `p2[0]` and prints `<parent_pid>: received pong`.
5. Clean up: close all pipe ends, and the parent calls `wait(0)` to reap the child.

**Why a pair of pipes (not one)?**

A pipe is a unidirectional FIFO byte stream. With a single pipe, both directions would
share the same byte stream. For this specific "one byte, one round trip" scenario a
single pipe would *happen* to work, because the child drains the pipe (reads the
parent's byte) before writing its reply back. But that is not a correct general design:
once messages grow larger or more round trips occur, the two directions' data gets
mixed into one stream and both sides blocking on `write` (full pipe) can deadlock.
That is why the lab explicitly requires *a pair of pipes, one for each direction*.

**Expected output**

```
$ pingpong
4: received ping
3: received pong
$
```

Solution: [`user/pingpong.c`](./xv6_for_Lab1/user/pingpong.c)

### 3. primes (moderate/hard)

**UNIX interfaces used**

| Interface | Description |
| --- | --- |
| `pipe(int p[2])` | Create a pipe between two neighboring stages |
| `fork()` | Spawn one process per prime in the pipeline |
| `read(fd, ...)` | Returns 0 once all write ends of the pipe are closed → EOF signal |
| `write(fd, ...)` | Pass filtered numbers to the next stage |
| `close(fd)` | Critical: processes must close unused ends to avoid holding the pipe open forever and exhausting xv6's limited fds/processes |
| `wait(int *)` | The head process waits for the whole pipeline (children, grandchildren, ...) to finish |
| `exit(int)` | Terminate a stage when its input is empty |

**Approach (pipeline of processes)**

The first process feeds integers 2–35 into a pipe. Each process that receives a number
`p` on its left pipe is the "holder" of prime `p`; it prints `prime p`, creates a right
pipe, forks a child, and forwards every subsequent number that is **not** divisible by
`p` to that child:

```
2,3,4,...,35 ──► [prime 2] ──► odd numbers ──► [prime 3] ──► ... ──► [prime 31]
```

1. `main` creates pipe `p[2]`, forks a writer that writes 2..35 and closes the write end.
2. The parent calls `sieve(p)`: close the write end, read the first number — if `read`
   returns ≤ 0, the pipe is drained and the process exits.
3. First number `p` is a prime: print it, create `right_pipe`, fork:
   - child: close `right_pipe[1]`, recurse into `sieve(right_pipe)`;
   - parent: close `right_pipe[0]`, keep reading from the left pipe, forwarding numbers
     with `num % p != 0` to the right pipe, then close everything and `wait`.
4. Key points: only create a stage when it's actually needed; close all unneeded fds
   immediately, otherwise `read` will never see EOF and xv6 runs out of resources.
   Write raw 4-byte `int`s to the pipes rather than formatted ASCII.

![image](https://swtch.com/~rsc/thread/sieve.gif)

**Expected output**

```
$ primes
prime 2
prime 3
prime 5
prime 7
prime 11
prime 13
prime 17
prime 19
prime 23
prime 29
prime 31
$
```

Solution: [`user/primes.c`](./xv6_for_Lab1/user/primes.c)

### 4. find (moderate)

**UNIX interfaces used**

| Interface | Description |
| --- | --- |
| `open(path, 0)` | Open a file or directory; directories are just files whose contents are `struct dirent` entries |
| `fstat(fd, &st)` | Get the `struct stat` metadata: `st.type` is `T_DIR`, `T_FILE`, or `T_DEVICE` |
| `read(fd, &de, sizeof(de))` | Read one directory entry (`struct dirent { ushort inum; char name[DIRSIZ]; }` in [`kernel/fs.h`](./xv6_for_Lab1/kernel/fs.h)) |
| `strcmp(a, b)` | Compare C strings (note: `==` doesn't work on strings!) |
| `strcpy` / `memmove` / `strlen` | Build child paths like `buf/<name>` |
| `strchr` (or loop) | Find the last `/` to extract the basename of a path |

**Approach (recursive directory walk)**

1. Validate `argc == 3` (`find path filename`).
2. `find(path, name)`:
   - `open(path, 0)` then `fstat`; on failure print an error and return.
   - Switch on `st.type`:
     - `T_FILE` / `T_DEVICE`: extract the basename after the last `/`; if
       `strcmp(basename, name) == 0`, print the full path.
     - `T_DIR`: iterate with `read(fd, &de, sizeof(de))`; skip entries with
       `de.inum == 0`, and skip `"."` / `".."` to avoid infinite recursion; build the
       child path `parent + "/" + de.name` and recurse.
   - Close the fd when done.
3. Reference: [`user/ls.c`](./xv6_for_Lab1/user/ls.c) shows the canonical idiom for walking a directory.

**Expected output** (file system contains `b`, `a/b`, and `a/aa/b`)

```
$ echo > b
$ mkdir a
$ echo > a/b
$ mkdir a/aa
$ echo > a/aa/b
$ find . b
./b
./a/b
./a/aa/b
$
```

Solution: [`user/find.c`](./xv6_for_Lab1/user/find.c)

### 5. xargs (moderate)

**UNIX interfaces used**

| Interface | Description |
| --- | --- |
| `fork()` | Create a child per input line |
| `exec(path, argv)` | Replace the child's image with the command; `argv` must be NULL-terminated |
| `wait(int *)` | Parent waits for the child running the command to finish before reading the next line |
| `read(0, &ch, 1)` | Read stdin one character at a time until `'\n'` |
| `MAXARG` | Maximum number of argv entries, defined in [`kernel/param.h`](./xv6_for_Lab1/kernel/param.h) |

**Approach**

1. Copy the base command from `argv[1..argc-1]` into `new_argv` (`cmd_argc = argc - 1`).
2. Read stdin character by character (`read(0, &ch, 1)`), appending each character to a
   buffer. When a `'\n'` is found, null-terminate the line and run the command.
   Handle the last line without a trailing newline too, and skip empty lines.
3. `run_cmd`: tokenize the line on spaces/tabs, pointing `new_argv[cmd_argc + i]` at each
   token (replacing separators with `'\0'`), and NULL-terminate `new_argv`.
4. `fork()`; in the child call `exec(new_argv[0], new_argv)` (print an error and
   `exit(1)` if it fails); in the parent call `wait(0)`.
5. Note: the real Unix `xargs -n 1` batches one argument per command — exactly what this
   lab asks for, so no batching optimization is needed.

**Expected output** (run the provided shell script `xargstest.sh`)

```
$ sh < xargstest.sh
$ $ $ $ $ $ hello
hello
hello
$ $
```

(The many `$` are printed because the xv6 shell does not realize it is reading commands
from a file rather than from the console.)

Solution: [`user/xargs.c`](./xv6_for_Lab1/user/xargs.c)

## Testing

In the `xv6_for_Lab1` directory:

```sh
make grade          # run all grading tests
make GRADEFLAGS=sleep grade      # grade only sleep
./grade-lab-util pingpong        # or invoke the grading script directly
```

Before handing in, remember to create `time.txt` containing a single integer — the
number of hours spent on the lab — and to `git add` / `git commit` it, as described in
the lab description.

## Optional challenge exercises

From the lab description (not graded):

- Write an `uptime` program that prints the uptime in ticks using the `uptime` system
  call. (easy)
- Support regular expressions in name matching for `find`; `grep.c` has some primitive
  support. (easy)
- Improve the shell `user/sh.c`: suppress the `$` when processing commands from a file
  (moderate), add `wait` support (easy), command lists separated by `;` (moderate),
  sub-shells `(` `)` (moderate), tab completion (easy), command history (moderate), etc.