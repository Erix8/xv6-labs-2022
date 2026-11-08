# 🛜 My MIT 6.1810 Xv6 Labs - Networking Lab

This lab writes an xv6 device driver for a **network interface card (NIC)**:
the Intel E1000, emulated by QEMU and connected to an emulated Ethernet LAN.
Complete `e1000_transmit()` and `e1000_recv()` in [`kernel/e1000.c`](kernel/e1000.c)
so that the driver can send and receive packets, working with the IP/UDP/ARP
network stack already provided in [`kernel/net.c`](kernel/net.c).

This branch contains my solutions to the lab. For details, hints, and grading
criteria, please refer to the [official net lab page](https://pdos.csail.mit.edu/6.828/2022/labs/net.html). The original lab branch can be cloned with:

``` bash
git clone git://g.csail.mit.edu/xv6-labs-2022
git fetch && git checkout net
```

## ❗️ Minor Changes to the Lab

- **E1000 Initialization**: QEMU 6.2 defers receiving for one second after an RCTL write,
which affects the ping test often. A interval is add to **my** `e1000_init()`, so
that interval could pass before user programs can send a request whose
reply would otherwise be delayed past the network backend's timeout.
- **Grade Lab DNS Test**: The original lab handout uses `8.8.8.8` Google's name server
for the DNS test, which often fails in my docker container network, thus I change it
to Cloudflare's `1.1.1.1` for more reliable DNS service.

## 🛠️ Building & Running Xv6

On a Debian or Ubuntu machine, install the following dependencies:

``` bash
sudo apt-get update && sudo apt-get upgrade
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

Once they are installed, and in your shell search path, you can run "make qemu".

## 📖 Recommended Reading before Coding

- [Xv6 book, Chapter 5 "Interrupts and device drivers"](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)
- [Intel 8254x GbE Software Developer's Manual](https://pdos.csail.mit.edu/6.1810/2022/readings/8254x_GBe_SDM.pdf)
  (Sections 2, 3.2-3.4, 4.1, 13, and 14 are the most relevant)

## 🙌 ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)).  See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to on-line resources for v6. The code in the files that constitute
xv6 is Copyright 2006-2022 Frans Kaashoek, Robert Morris, and Russ Cox.
