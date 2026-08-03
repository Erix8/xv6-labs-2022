# Lab 5: Copy-on-Write Fork for xv6

## Overview

This lab explores copy-on-write (COW) fork. Virtual memory provides indirection,
allowing the kernel to intercept memory references and defer work until needed.
The goal is to defer allocating and copying physical memory pages in `fork()` until
the copies are actually needed.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/cow.html

## Exercises

### Implement copy-on-write fork (hard)
Implement COW fork in the xv6 kernel:
- Modify `uvmcopy()` to map the parent's physical pages into the child and clear
  `PTE_W` in both, marking the pages read-only.
- Modify `usertrap()` to handle write page-faults on COW pages: allocate a new page,
  copy the old page into it, and install it with `PTE_W` set.
- Track a reference count per physical page so a page is freed only when the last
  PTE reference is gone.
- Modify `copyout()` to use the same scheme as page faults when it hits a COW page.

You are done if your kernel executes both `cowtest` and `usertests -q` successfully.