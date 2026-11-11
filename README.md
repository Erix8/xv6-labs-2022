# 🗺️ My MIT 6.1810 Xv6 Labs - Mmap Lab

In this lab you'll add `mmap` and `munmap` to xv6, focusing on memory-mapped files.
They can be used to share memory among processes, to map files into process
address spaces, and as part of user-level page fault schemes such as the garbage-collection algorithms.

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official mmap lab page](https://pdos.csail.mit.edu/6.828/2022/labs/mmap.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout mmap
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- Mmap Linux Manual Page (run `man mmap` to read the doc, this lab requires only a subset of its features relevant to memory-mapping a file).

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
