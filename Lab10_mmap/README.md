# Lab 10: mmap

## Overview

This lab adds `mmap` and `munmap` to xv6, focusing on memory-mapped files. These
system calls allow programs to map files into their address spaces, which can be
used for sharing memory among processes and as part of user-level page fault
schemes.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/mmap.html

## Exercises

### mmap and munmap (hard)
Implement enough `mmap` and `munmap` functionality to make `mmaptest` work:

- `mmap`: find an unused region in the process's address space, add a VMA (virtual
  memory area) recording the address, length, permissions, and file. Fill in the
  page table lazily on page faults in response to page faults in `usertrap`, using
  `readi` to load file pages.
- `munmap`: find the VMA for the address range, unmap the specified pages with
  `uvmunmap`, and write back modified `MAP_SHARED` pages to the file.
- Modify `exit` to unmap mapped regions, and `fork` so the child inherits the
  parent's mapped regions.

Pass `mmaptest` and `usertests -q`.