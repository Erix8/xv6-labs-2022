# 🐮 My MIT 6.1810 Xv6 Labs - Copy-on-Write Fork Lab

This lab explores **copy-on-write (COW) fork**. Virtual memory provides indirection,
allowing the kernel to intercept memory references and defer work **until needed**.

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official cow lab page](https://pdos.csail.mit.edu/6.828/2022/labs/cow.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout cow
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- [xv6 book](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf) (Chapter 3 "Page tables", Chapter 4 "Traps" are the relevant reading)
- [RISC-V privileged ISA manual](https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf) (PTE flags, Sv39 layout; scause exception codes §4.1.3)
- [LWN: "Patching until the COWs come home"](https://lwn.net/Articles/849638/) (why refcounting is hard in production kernels)

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
