# Lab 7: Networking

## Overview

This lab writes an xv6 device driver for a network interface card (NIC). You will
complete the E1000 driver using qemu's emulated E1000 card and user-mode network
stack.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/net.html

## Exercises

### Your Job (hard)
Complete `e1000_transmit()` and `e1000_recv()` in `kernel/e1000.c` so that the
driver can transmit and receive packets.

- `e1000_transmit()`: place a pointer to the packet data in a TX ring descriptor,
  free the mbuf only after the E1000 has finished transmitting, and update the ring
  position.
- `e1000_recv()`: scan the RX ring, deliver each new packet's mbuf to the network
  stack via `net_rx()`, allocate a fresh mbuf to replace it, and update the ring.

The network stack (`net.c`, `net.h`) implements IP, UDP, and ARP, and provides the
`mbuf` packet data structure. You are done when `make grade` says your solution
passes all the tests.