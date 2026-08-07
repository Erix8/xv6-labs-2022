# Lab 8: Locks

## Overview

This lab gains experience in re-designing code to increase parallelism on
multi-core machines. A common symptom of poor parallelism is high lock
contention, so you will change both data structures and locking strategies — for
the xv6 memory allocator (one free list per CPU) and the block cache (a hash
table with one lock per bucket).

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/lock.html

### Recommended reading before coding

As suggested by the lab description:

- xv6 book [Chapter 6 "Locking"](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf) and the corresponding code.
- xv6 book Section 3.5 "Code: Physical memory allocator".
- xv6 book Sections 8.1–8.3 "Overview", "Buffer cache layer", and "Code: Buffer cache".

### Reference links

- [xv6 book (Chapter 6, Section 3.5, Sections 8.1–8.3 are the relevant reading)](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)

## Key mechanisms used

### How lock contention is measured

`make LAB=lock` defines `LAB_LOCK`, which instruments every `spinlock` with two
counters updated by `acquire()` in
[`kernel/spinlock.c`](./xv6_for_Lab8/kernel/spinlock.c):

| Counter | Meaning |
| --- | --- |
| `n` | number of calls to `acquire()` for this lock |
| `nts` | number of times the acquire loop tried but failed to set the lock (`#test-and-set`) — a rough measure of contention |

`kalloctest` and `bcachetest` read these counters through the `statistics` system
call. The grading threshold is on the *increase* of `#test-and-set` over the
test: less than 10 for `kalloctest` test1, less than 500 for `bcachetest` test0.

### Two strategies to cut contention

| Subsystem | Idea |
| --- | --- |
| Memory allocator | per-CPU free list, each with its own lock; steal from another CPU only when a CPU's list is empty |
| Block cache | hash block numbers into buckets, one lock per bucket; a global lock only serializes cache-miss eviction |

## Exercises

### 1. Memory allocator (moderate)

**The problem**

`kalloc()` and `kfree()` in [`kernel/kalloc.c`](./xv6_for_Lab8/kernel/kalloc.c)
use a single free list protected by a single `kmem.lock`. `kalloctest` spawns
processes that grow and shrink their address spaces, generating many
`kalloc`/`kfree` calls; all CPUs end up contending for that one lock.

**Implementation steps**

1. Turn the single `kmem` struct into `kmem[NCPU]`, an array of `{lock, freelist}`
   indexed by CPU id; `NCPU` comes from
   [`kernel/param.h`](./xv6_for_Lab8/kernel/param.h).
2. `kinit()` initializes all NCPU locks, each named `"kmem"` (the lab allows all
   locks to share the same name; `statslock()` collects them by name prefix).
3. `freerange()` is left unchanged: it calls `kfree()`, which now pushes each page
   onto the free list of the CPU currently running `freerange()` (CPU 0 at boot),
   following the hint.
4. `kfree()` wraps `cpuid()` in `push_off()`/`pop_off()` and pushes the page onto
   `kmem[id].freelist`. `cpuid()` is only safe to call and use with interrupts off.
5. `kalloc()` first tries `kmem[id].freelist`; if it is empty, it steals one page
   from another CPU's list. Only one lock is held at a time, so there is no
   deadlock.

**Key points**

- `push_off()` must come before `cpuid()`, and `pop_off()` after the last
  `release()`, because the CPU number is only reliable while interrupts are off.
- All free memory initially sits on CPU 0's list (boot runs on CPU 0); stealing is
  therefore essential, otherwise the other CPUs could never allocate.
- Stealing a single page at a time suffices: it is infrequent, and the contention
  it adds is negligible.

**Expected output**

Before implementation:

```
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #test-and-set 134312 #acquire() 433034
tot= 134312
test1 FAIL
```

After implementation:

```
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #test-and-set 0 #acquire() 44834
lock: kmem: #test-and-set 0 #acquire() 199272
lock: kmem: #test-and-set 0 #acquire() 188964
tot= 0
test1 OK
```

Solution: [`kernel/kalloc.c`](./xv6_for_Lab8/kernel/kalloc.c)

### 2. Buffer cache (hard)

**The problem**

`bcache.lock` in [`kernel/bio.c`](./xv6_for_Lab8/kernel/bio.c) protects the whole
block cache: the linked list of cached buffers, each buffer's `refcnt`, and the
cached blocks' identities (`dev`, `blockno`). When several processes use the file
system intensively, they all queue up on this one lock.

**Implementation steps**

1. Replace the single LRU list with a hash table: `bucket[NBUCKET]` doubly-linked
   lists plus one `bucket_locks[i]` per bucket. `NBUCKET = 13` (a prime) is fixed,
   as the hints suggest, to reduce hashing conflicts.
2. Keep a global `bcache.lock`, but only to serialize the "find a free buffer"
   part of `bget()`; the lab description explicitly allows this part to be a
   bottleneck.
3. `bget()`:
   - On a cache hit, take only the bucket's lock and return the buffer.
   - On a miss, take the global lock, then **re-check the bucket under its lock**
     (another CPU may have cached the block while the bucket lock was dropped) to
     preserve the invariant that at most one copy of each block is cached.
   - Scan `bcache.buf[]` for a buffer with `refcnt == 0`; under its old bucket's
     lock, unlink it, give it the new block's identity (`dev`, `blockno`,
     `valid = 0`, `refcnt = 1`), then insert it into the new bucket under that
     bucket's lock.
4. `brelse()` no longer maintains an LRU list (the hints say to remove the list of
   all buffers and skip LRU), so it only releases the sleep lock and decrements
   `refcnt` under the bucket's lock — no global lock needed.
5. `bpin()`/`bunpin()` similarly update `refcnt` under the bucket's lock.

**Key points**

- The re-check inside the global lock is essential: without it, two CPUs that miss
  on the same block could each evict a buffer and end up caching two copies.
- The tricky case where the new block hashes to the same bucket as the buffer
  being evicted is safe: the old bucket lock is released after unlinking and
  re-acquired for the insertion; deadlock is impossible because evictions are
  serialized by the global lock (at most two locks held: global + one bucket).
- `refcnt` and the buffer's sleep lock play different roles: `refcnt` counts how
  many processes reference the block (including those waiting for it), while the
  sleep lock guarantees that at most one process uses the buffer's data at a time.

**Expected output**

Before implementation:

```
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: bcache: #test-and-set 75159 #acquire() 65044
tot= 75159
test0: FAIL
start test1
test1 OK
```

After implementation:

```
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: bcache: #test-and-set 0 #acquire() 398
lock: bcache.bucket: #test-and-set 0 #acquire() 2147
lock: bcache.bucket: #test-and-set 0 #acquire() 4149
lock: bcache.bucket: #test-and-set 0 #acquire() 2448
lock: bcache.bucket: #test-and-set 0 #acquire() 4338
lock: bcache.bucket: #test-and-set 0 #acquire() 4589
lock: bcache.bucket: #test-and-set 0 #acquire() 6574
lock: bcache.bucket: #test-and-set 0 #acquire() 7102
lock: bcache.bucket: #test-and-set 0 #acquire() 9319
lock: bcache.bucket: #test-and-set 0 #acquire() 6230
lock: bcache.bucket: #test-and-set 0 #acquire() 6230
lock: bcache.bucket: #test-and-set 0 #acquire() 6224
lock: bcache.bucket: #test-and-set 0 #acquire() 4170
lock: bcache.bucket: #test-and-set 0 #acquire() 4142
tot= 0
test0: OK
start test1
test1 OK
```

Solution: [`kernel/bio.c`](./xv6_for_Lab8/kernel/bio.c)

## Testing

In the `xv6_for_Lab8` directory:

    make grade          # run all grading tests

Result on this implementation:

```
== Test running kalloctest ==
== Test   kalloctest: test1 ==   test1 OK
== Test   kalloctest: test2 ==   test2 OK
== Test   kalloctest: test3 ==   test3 OK
== Test   kalloctest: sbrkmuch ==   sbrkmuch OK
== Test running bcachetest ==
== Test   bcachetest: test0 ==   test0: OK
== Test   bcachetest: test1 ==   test1 OK
== Test usertests ==
== Test   usertests: all tests == OK
== Test time ==       time: OK
Score: 80/80
```

The `time.txt` file (a single integer, the number of hours spent on the lab, as
required by the lab description) is included.

## Optional challenge exercises

From the lab description (not graded):

- Maintain the LRU list so that eviction picks the least-recently used buffer
  instead of any unused buffer. (moderate)
- Make lookup in the buffer cache lock-free. Hint: use gcc's `__sync_*`
  functions. (hard)
