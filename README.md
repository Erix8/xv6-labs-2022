# xv6 Labs 2022 — Personal Implementation

This repository contains my personal implementations of the labs for the
[**MIT 6.1810 (6.828): Operating System Engineering**, Fall 2022](https://pdos.csail.mit.edu/6.828/2022/index.html). The labs
extend the xv6 operating system, a simple Unix-like teaching OS for RISC-V. 
For general course information (schedule, lectures, and lab overviews), please see the original website.

The original labs repository can be cloned with:

```
git clone git://g.csail.mit.edu/xv6-labs-2022
```

Note: the original labs repository manages each lab on a separate branch, whereas
this repository keeps all of the lab work on the `main` branch. For details on how
the original repository organizes its lab branches, please refer to the official
course information.

## Lab Contents

Below is a brief summary of the ten labs. Each directory contains a `README.md`
with more details about the exercises in that lab.

| # | Lab | Directory | Summary |
|---|-----|-----------|---------|
| 1 | Xv6 and Unix utilities | [Lab1_Xv6 and Unix utilities](Lab1_Xv6%20and%20Unix%20utilities/README.md) | Implement classic Unix utilities (sleep, pingpong, primes, find, xargs) in xv6. |
| 2 | System calls | [Lab2_system calls](Lab2_system%20calls/README.md) | Add new system calls (trace, sysinfo) and practice debugging with gdb. |
| 3 | Page tables | [Lab3_page tables](Lab3_page%20tables/README.md) | Speed up getpid, print page tables, and detect accessed pages. |
| 4 | Traps | [Lab4_traps](Lab4_traps/README.md) | RISC-V assembly, backtrace, and user-level alarm handling. |
| 5 | Copy-on-Write Fork | [Lab5_Copy-on-Write Fork for xv6](Lab5_Copy-on-Write%20Fork%20for%20xv6/README.md) | Implement copy-on-write fork to defer page copying. |
| 6 | Multithreading | [Lab6_Multithreading](Lab6_Multithreading/README.md) | User-level threads, parallel hash table, and a barrier. |
| 7 | Networking | [Lab7_networking](Lab7_networking/README.md) | Write an E1000 NIC device driver. |
| 8 | Locks | [Lab8_locks](Lab8_locks/README.md) | Reduce lock contention in the memory allocator and buffer cache. |
| 9 | File system | [Lab9_file system](Lab9_file%20system/README.md) | Add large files and symbolic links. |
| 10 | mmap | [Lab10_mmap](Lab10_mmap/README.md) | Implement mmap and munmap for memory-mapped files. |