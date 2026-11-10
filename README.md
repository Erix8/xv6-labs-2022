# ⚙️ My MIT 6.1810 Xv6 Labs - Xv6 and Unix utilities Lab

This lab familiarizes you with xv6 and its system calls. It is the only lab that is
purely about **user space**: implement five classic Unix utilities
([sleep](user/sleep.c) / [pingpong](user/pingpong.c) / [primes](user/primes.c) / [find](user/find.c) / [xargs](user/xargs.c)) as ordinary xv6 user programs.

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official util lab page](https://pdos.csail.mit.edu/6.828/2022/labs/util.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout util
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- [xv6 book](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf) (official textbook; reading Chapter 1 is recommended before coding)
- [xv6-riscv source code](https://github.com/mit-pdos/xv6-riscv)  on GitHub
- [Doug McIlroy's original prime-sieve page](http://swtch.com/~rsc/thread/)  (source of the primes exercise)

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
