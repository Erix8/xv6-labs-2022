# Lab 6: Multithreading

## Overview

This lab familiarizes you with multithreading. You will implement switching between
threads in a user-level threads package, use multiple threads to speed up a program
with locks, and implement a barrier with condition variables.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/thread.html

### Recommended reading before coding

- [xv6 book, Chapter 7 "Scheduling"](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)

### Key mechanisms used

#### RISC-V caller/callee-save registers

`thread_switch` needs to save/restore **only the callee-save registers**. The C
compiler already guarantees that caller-save registers are saved at the call site,
so a context switch is a plain function call from its point of view:

| Kind | Registers | Saved by |
| --- | --- | --- |
| caller-save | `t0`–`t6`, `a0`–`a7` | the caller (compiler-generated code) |
| callee-save | `ra`, `sp`, `s0`–`s11` | the callee: `thread_switch` itself |

`thread_switch` stores the old thread's 13 callee-save registers and loads the new
thread's, exactly like the kernel's `swtch()` in
[`kernel/swtch.S`](./xv6_for_Lab6/kernel/swtch.S).

#### Per-bucket locking

Two threads writing the same hash bucket race (a lost update), but different buckets
are independent. A **lock per bucket** therefore both guarantees correctness and lets
`put`s in different buckets really run in parallel.

#### Condition variables

`pthread_cond_wait(&cond, &mutex)` atomically **releases the mutex, puts the thread
to sleep on `cond`, and re-acquires the mutex before returning**. `pthread_cond_broadcast(&cond)`
wakes up every thread sleeping on `cond`, but they only run after the broadcaster
releases the mutex. This is the pthread analogue of xv6's `sleep(chan, lock)` /
`wakeup(chan)`.

## Exercises

### 1. Uthread: switching between threads (moderate)

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `thread_switch(old, new)` | assembly routine | save old thread's callee-save registers, load new thread's, return to `new`'s `ra` |
| `thread_create(func)` | user-level library | find a free slot and initialize a new thread's context |
| `thread_schedule()` | user-level library | pick the next runnable thread and switch to it |

**How the pieces fit together**

```
main (thread 0) --thread_schedule()--> thread_switch(t, next)
                                          |  sd ra,sp,s0..s11 -> t->context
                                          |  ld ra,sp,s0..s11 <- next->context
                                          v  ret jumps into func (first run) or
thread_a/b/c: print, thread_yield(), ...      back to the previous switch point
```

**Implementation steps**

1. Add a register-save area at the **beginning** of `struct thread` in
   [`user/uthread.c`](./xv6_for_Lab6/user/uthread.c): `ra`, `sp`, `s0`–`s11`
   (13 × `uint64`). Putting it first makes the `struct thread*` address equal to the
   context base address, so `thread_switch((uint64)t, (uint64)next_thread)` works
   with the `sd/ld` offsets of `kernel/swtch.S` unchanged.
2. In `thread_create()`, initialize the context of the new thread:
   `t->ra = (uint64)func` so the first switch jumps straight into `func`, and
   `t->sp = (uint64)&t->stack[STACK_SIZE]` (RISC-V stacks grow down, so start at the
   top of the thread's private 8 KB stack). Zero `s0`–`s11`.
3. In `thread_schedule()`, replace `/* YOUR CODE HERE */` with
   `thread_switch((uint64)t, (uint64)next_thread);` — switch from the old thread `t`
   to `next_thread`.
4. Implement `thread_switch` in
   [`user/uthread_switch.S`](./xv6_for_Lab6/user/uthread_switch.S), mirroring
   [`kernel/swtch.S`](./xv6_for_Lab6/kernel/swtch.S): 14 `sd`/`ld` pairs for
   `ra, sp, s0..s11` at offsets 0–104, then `ret`.

**Key points**

- Only callee-save registers matter (see the table above).
- `ra` and `sp` cannot be saved on the current stack: switching threads is about
  **changing stacks**, and `sp` itself is part of the context. They live in each
  thread's own context region.
- The context is put at offset 0 of `struct thread` so no offset arithmetic is
  needed between C and assembly.
- `thread_switch` returns into the *new* thread's `ra`: on first run that is `func`,
  afterwards it is wherever that thread last yielded.

**Expected output**

```
$ uthread
thread_a started
thread_b started
thread_c started
thread_c 0
thread_a 0
thread_b 0
...
thread_c 99
thread_a 99
thread_b 99
thread_c: exit after 100
thread_a: exit after 100
thread_b: exit after 100
thread_schedule: no runnable threads
```

Solution: [`user/uthread.c`](./xv6_for_Lab6/user/uthread.c),
[`user/uthread_switch.S`](./xv6_for_Lab6/user/uthread_switch.S)

### 2. Using threads (moderate)

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `pthread_mutex_t` | pthread library | mutual-exclusion lock |
| `pthread_mutex_init()` / `lock()` / `unlock()` | pthread library | initialize / acquire / release a mutex |
| `pthread_create()` / `pthread_join()` | pthread library | spawn and wait for threads |

**Approach**

`notxv6/ph.c` implements a hash table with `NBUCKET = 5` chains. With one thread it
is correct; with two threads it loses keys because `put()` is a non-atomic
read-modify-write:

> Why are there missing keys with 2 threads, but not with 1 thread? Identify a
> sequence of events with 2 threads that can lead to a key being missing.

Answer (submitted in `answers-thread.txt`): suppose T1 inserts key k1 and T2 inserts
key k2 into the same, initially empty bucket `i`:

1. T1 creates `e1` with `e1->next = NULL` (read `table[i] == NULL`).
2. T2 creates `e2` with `e2->next = NULL` (also read `NULL`).
3. T1 writes `table[i] = e1`.
4. T2 writes `table[i] = e2`, overwriting and losing `e1` (and k1).

With one thread the inserts are serialized: the second one reads the already-inserted
node and chains onto it, so no key is lost. With two threads the read and the write
back are not atomic as a whole.

**Implementation steps** ([`notxv6/ph.c`](./xv6_for_Lab6/notxv6/ph.c))

1. Declare one lock per bucket: `pthread_mutex_t locks[NBUCKET];`.
2. In `main()`, initialize them:
   `for (int i = 0; i < NBUCKET; i++) pthread_mutex_init(&locks[i], NULL);`
   (the lab description explicitly reminds you not to forget this).
3. Lock/unlock around the whole body of `put()` using `locks[key % NBUCKET]`.
4. Lock/unlock around the whole body of `get()` the same way.

**Key points**

- The lock index must equal the bucket index (`key % NBUCKET`).
- Different buckets are fully independent, so puts in different buckets run in
  parallel — that is what makes the `ph_fast` speedup (≥ 1.25×) possible.
- `get()` is also locked so a concurrent get/put can never traverse a half-updated
  chain.

**Expected output**

```
$ ./ph 1
100000 puts, 2.445 seconds, 40906 puts/second
0: 0 keys missing
100000 gets, 2.462 seconds, 40618 gets/second
$ ./ph 2
100000 puts, 1.803 seconds, 55468 puts/second
0: 0 keys missing
1: 0 keys missing
200000 gets, 3.238 seconds, 61767 gets/second
```

Solution: [`notxv6/ph.c`](./xv6_for_Lab6/notxv6/ph.c),
[`answers-thread.txt`](./xv6_for_Lab6/answers-thread.txt)

### 3. Barrier (moderate)

**UNIX interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `pthread_cond_wait()` | pthread library | atomically release the mutex and sleep on the condition; re-acquire the mutex before returning |
| `pthread_cond_broadcast()` | pthread library | wake up every thread sleeping on the condition |

**Implementation steps** ([`notxv6/barrier.c`](./xv6_for_Lab6/notxv6/barrier.c))

```c
static void 
barrier()
{
  pthread_mutex_lock(&bstate.barrier_mutex);

  bstate.nthread++;
  if (bstate.nthread == nthread) {
    // All threads reached this round: advance and wake everyone.
    bstate.round++;
    bstate.nthread = 0;
    pthread_cond_broadcast(&bstate.barrier_cond);
  } else {
    // Not all arrived: go to sleep (wait releases the mutex while sleeping).
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  }

  pthread_mutex_unlock(&bstate.barrier_mutex);
}
```

**How the pieces fit together** (`nthread == 2`, round `k`)

```
T0: lock; nthread=1; 1 != 2 -> cond_wait:  release lock, sleep
T1: lock; nthread=2; 2 == 2 -> round=k+1; nthread=0; broadcast; unlock
T0: wait returns (lock re-acquired); unlock; reads round == k+1 -> assert(i == t) OK
```

**Key points**

- `bstate.nthread++` is a read-modify-write and must be protected by the mutex.
- **Reset `bstate.nthread = 0` in the same critical section, before the broadcast.**
  This is the trap the lab description warns about: a woken thread races around the
  loop and enters the next round's `nthread++`, which must not pollute the previous
  round's count.
- The last thread to arrive performs `round++` and the broadcast; the others sleep in
  `pthread_cond_wait` (which atomically releases the mutex, avoiding a
  hold-the-lock-while-sleeping deadlock).
- After being woken, a thread holds the mutex again (wait re-acquires it), so the
  final `unlock` always pairs correctly.

**Expected output**

```
$ ./barrier 1
OK; passed
$ ./barrier 2
OK; passed
$ ./barrier 3
OK; passed
```

Solution: [`notxv6/barrier.c`](./xv6_for_Lab6/notxv6/barrier.c)

## Testing

In the `xv6_for_Lab6` directory:

    make grade          # run all grading tests

The written answer for the `ph` question is in `answers-thread.txt`. The
`time.txt` file (a single integer, the hours spent on the lab) is also
included.

Result on this implementation:

```
== Test uthread ==            uthread: OK (15.3s)
== Test answers-thread.txt == answers-thread.txt: OK
== Test ph_safe ==            ph_safe: OK (6.0s)
== Test ph_fast ==            ph_fast: OK (10.7s)
== Test barrier ==            barrier: OK (11.7s)
== Test time ==               time: OK
Score: 60/60
```

## Optional challenge exercises

From the lab description (not graded):

- The user-level thread package interacts badly with the OS (a blocking system call
  blocks all threads; threads never run in parallel on multiple cores). Fix it with
  scheduler activations or one kernel thread per user-level thread (this needs a
  TLB shootdown for multithreaded processes) — hard.
- Add locks, condition variables, barriers, etc. to the user-level thread package.