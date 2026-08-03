# Lab 3: Page tables

## Overview

This lab explores page tables and modifies them to speed up certain system calls
and to detect which pages have been accessed.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/pgtbl.html

## Exercises

### Speed up system calls (easy)
Implement the optimization where each process maps one read-only page at `USYSCALL`
storing a `struct usyscall` with the current PID. This speeds up `getpid()` by
avoiding a kernel crossing. The provided `ugetpid()` on the userspace side will use
this mapping. Pass the `ugetpid` test in `pgtbltest`.

### Print a page table (easy)
Write a `vmprint()` function that takes a `pagetable_t` argument and prints the
page table in the required format. Insert `if(p->pid==1) vmprint(p->pagetable)` in
`exec.c` just before `return argc` to print the first process's page table.

### Detect which pages have been accessed (hard)
Implement the `pgaccess()` system call that reports which pages have been accessed.
It takes the starting virtual address, the number of pages to check, and a buffer
to store the results as a bitmask. RISC-V hardware sets the `PTE_A` access bit in
the PTE on TLB misses; you must inspect and clear this bit.