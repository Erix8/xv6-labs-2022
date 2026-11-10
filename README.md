# 🔐 My MIT 6.1810 Xv6 Labs - Locks Lab

This lab gains experience in re-designing code to increase parallelism on
multi-core machines. You will change both data structures and locking strategies
— for the xv6 memory allocator (one free list per CPU) and the block cache (a hash
table with one lock per bucket).

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official lock lab page](https://pdos.csail.mit.edu/6.828/2022/labs/lock.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout lock
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- [xv6 book](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf) (official textbook; reading Chapter 6, Section 3.5 and Section 8.1-8.3 is recommended before coding)

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
