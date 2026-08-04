# Lab 2: System calls

## Overview

This lab adds some new system calls to xv6, which helps you understand how system
calls work and exposes you to some of the internals of the xv6 kernel.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/syscall.html

## Exercises

### Using gdb (easy)
Become familiar with gdb by running `make qemu-gdb`, setting a breakpoint at
`syscall`, and inspecting the kernel stack, registers, and process state. Also
track down a kernel page-fault panic. Record your answers in `answers-syscall.txt`.

### System call tracing (moderate)
Add a `trace` system call that controls tracing. It takes an integer "mask" whose
bits specify which system calls to trace. The kernel prints a line with the process
id, system call name, and return value when a traced system call is about to return.
Tracing applies to the calling process and its children.

### Sysinfo (moderate)
Add a `sysinfo` system call that collects information about the running system.
It fills a `struct sysinfo` with the number of bytes of free memory (`freemem`)
and the number of non-`UNUSED` processes (`nproc`). You pass when `sysinfotest`
prints "sysinfotest: OK".