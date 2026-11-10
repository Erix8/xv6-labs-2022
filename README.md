# 📞 My MIT 6.1810 Xv6 Labs - System Calls Lab

This lab adds some new system calls (trace & sysinfo) to xv6, which helps you
understand how system calls work and exposes you to some of the internals of
the xv6 kernel.

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official syscall lab page](https://pdos.csail.mit.edu/6.828/2022/labs/syscall.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout syscall
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- [xv6 book](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf) (official textbook; reading Chapter 2 plus Sections 4.3 and 4.4 of Chapter 4 is recommended before coding)
- **User-space stubs**: [`user/usys.S`](./user/usys.S), generated from
  [`user/usys.pl`](./user/usys.pl) by `make`; declarations in
  [`user/user.h`](./user/user.h).
- **Kernel-space dispatch**: [`kernel/syscall.c`](./kernel/syscall.c)
  and [`kernel/syscall.h`](./kernel/syscall.h).
- **Process-related code**: [`kernel/proc.h`](./kernel/proc.h) and
  [`kernel/proc.c`](./kernel/proc.c).

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
