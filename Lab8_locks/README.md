# Lab 8: Locks

## Overview

This lab gains experience in re-designing code to increase parallelism. A common
symptom of poor parallelism on multi-core machines is high lock contention, so you
will change both data structures and locking strategies for the xv6 memory
allocator and block cache.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/lock.html

## Exercises

### Memory allocator (moderate)
The root cause of lock contention in `kalloctest` is that `kalloc()` has a single
free list protected by a single lock. Redesign the memory allocator to maintain a
free list per CPU, each with its own lock, and implement stealing when one CPU's
free list is empty. Give all locks names starting with "kmem". Run `kalloctest` and
make sure `usertests -q` passes.

### Buffer cache (hard)
Reduce contention on `bcache.lock` in `kernel/bio.c`. Modify `bget` and `brelse`
so that concurrent lookups and releases for different cached blocks don't all
contend on `bcache.lock`. Use a hash table with a lock per hash bucket, maintaining
the invariant that at most one copy of each block is cached. Give all locks names
starting with "bcache". Pass `bcachetest` and `usertests -q`.