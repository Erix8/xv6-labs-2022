# Lab 1: Xv6 and Unix utilities

## Overview

This lab introduces xv6 and its system calls. After booting xv6, you will implement
several classic Unix utilities as user programs to become familiar with system calls
and the xv6 user environment.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/util.html

## Exercises

### sleep (easy)
Implement the UNIX program `sleep` for xv6. It should pause for a user-specified
number of ticks (the time between two timer-chip interrupts). Solution goes in
`user/sleep.c`.

### pingpong (easy)
Write a program that uses UNIX system calls to "ping-pong" a byte between two
processes over a pair of pipes, one for each direction. The parent sends a byte to
the child, the child prints `<pid>: received ping` and writes the byte back,
then the parent prints `<pid>: received pong`. Solution goes in `user/pingpong.c`.

### primes (moderate/hard)
Write a concurrent version of the prime sieve using pipes. The first process feeds
the numbers 2 through 35 into the pipeline, and for each prime you create a process
that reads from its left neighbor and writes to its right neighbor. Solution goes in
`user/primes.c`.

### find (moderate)
Write a simple version of the UNIX `find` program that finds all the files in a
directory tree with a specific name. Use recursion to descend into sub-directories.
Solution goes in `user/find.c`.

### xargs (moderate)
Write a simple version of the UNIX `xargs` program. Its arguments describe a command
to run; it reads lines from standard input and runs the command for each line,
appending the line to the command's arguments. Solution goes in `user/xargs.c`.