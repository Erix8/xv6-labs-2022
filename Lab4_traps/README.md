# Lab 4: Traps

## Overview

This lab explores how system calls are implemented using traps. You will first do a
warm-up exercise with stacks, and then implement an example of user-level trap
handling.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/traps.html

## Exercises

### RISC-V assembly (easy)
Understand a bit of RISC-V assembly by reading the generated code in `user/call.asm`
for the functions `g`, `f`, and `main`. Answer a set of questions about registers,
function calls, endianness, and printf behavior. Record your answers in
`answers-traps.txt`.

### Backtrace (moderate)
Implement a `backtrace()` function in `kernel/printf.c` that walks up the stack
using frame pointers and prints the saved return address in each stack frame.
Insert a call to it in `sys_sleep`, and add a call from `panic` as well.

### Alarm (hard)
Add `sigalarm(interval, handler)` and `sigreturn()` system calls. After every `n`
ticks of CPU time a process consumes, the kernel should call the user's handler
function. When the handler returns, the application should resume where it left off.
`sigalarm(0, 0)` stops the periodic alarms. Pass `alarmtest` and `usertests -q`.