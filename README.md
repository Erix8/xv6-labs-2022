# 📁 My MIT 6.1810 Xv6 Labs

This repository contains my personal solutions to the labs of
**MIT 6.1810 (6.828): Operating System Engineering**, Fall 2022.
For general information (schedule, lectures, and lab overviews), please see the
[original website](https://pdos.csail.mit.edu/6.828/2022/index.html).

The original labs repository can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
```

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Lab Contents

Each lab has its own branch, `git checkout` to it, run `make qemu` to build xv6
and run all the tests with `make grade`.

| # | Lab | Summary |
| - | --- | ------- |
| 1 | [⚙️ Unix Utilities] | Implement classic Unix utilities in xv6. |
| 2 | [📞 System Calls] | Explore internals of the xv6 kernel and add new system calls. |
| 3 | [📃 Page Tables] | Modify page tables to speed up syscalls and detect accessed pages. |
| 4 | [🕳️ Traps] | Implement an user-level trap handling (alarm). |
| 5 | [🐮 Copy-on-Write] | Implement copy-on-write fork to defer page copying. |
| 6 | [🧵 Multithreading] | User-level threads, speed up program, and a barrier. |
| 7 | [🛜 Networking] | Write an E1000 NIC device driver. |
| 8 | [🔐 Locks] | Reduce lock contention in the memory allocator and buffer cache. |
| 9 | [🗄️ File System] | Add large files and symbolic links. |
| 10 | [🗺️ Mmap] | Implement mmap and munmap for memory-mapped files. |

[⚙️ Unix Utilities]: https://pdos.csail.mit.edu/6.828/2022/labs/util.html
[📞 System Calls]: https://pdos.csail.mit.edu/6.828/2022/labs/syscall.html
[📃 Page Tables]: https://pdos.csail.mit.edu/6.828/2022/labs/pgtbl.html
[🕳️ Traps]: https://pdos.csail.mit.edu/6.828/2022/labs/traps.html
[🐮 Copy-on-Write]: https://pdos.csail.mit.edu/6.828/2022/labs/cow.html
[🧵 Multithreading]: https://pdos.csail.mit.edu/6.828/2022/labs/thread.html
[🛜 Networking]: https://pdos.csail.mit.edu/6.828/2022/labs/net.html
[🔐 Locks]: https://pdos.csail.mit.edu/6.828/2022/labs/lock.html
[🗄️ File System]: https://pdos.csail.mit.edu/6.828/2022/labs/fs.html
[🗺️ Mmap]: https://pdos.csail.mit.edu/6.828/2022/labs/mmap.html

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
