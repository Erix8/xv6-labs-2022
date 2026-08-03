# Lab 6: Multithreading

## Overview

This lab familiarizes you with multithreading. You will implement switching between
threads in a user-level threads package, use multiple threads to speed up a program,
and implement a barrier.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/thread.html

## Exercises

### Uthread: switching between threads (moderate)
Design and implement the context switch mechanism for a user-level threading
system. Add code to `thread_create()` and `thread_schedule()` in `user/uthread.c`
and `thread_switch` in `user/uthread_switch.S`. `thread_switch` saves/restores the
callee-save registers of the threads.

### Using threads (moderate)
Explore parallel programming with threads and locks using a hash table
(`notxv6/ph.c`). Add locks so that no keys are missing with multiple threads,
then optimize (e.g., a lock per hash bucket) so some `put` operations run in
parallel. Pass `ph_safe` and `ph_fast` tests.

### Barrier (moderate)
Implement a barrier at which all participating threads must wait until all other
threads reach that point. Use `pthread_cond_wait` and `pthread_cond_broadcast` in
`notxv6/barrier.c`. Handle successive rounds correctly. Pass `make grade`'s
`barrier` test.