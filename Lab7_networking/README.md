# Lab 7: Networking

## Overview

This lab writes an xv6 device driver for a network interface card (NIC): the Intel
E1000, emulated by qemu and connected to an emulated Ethernet LAN. You complete
`e1000_transmit()` and `e1000_recv()` in `kernel/e1000.c` so that the driver can send
and receive packets, working with the IP/UDP/ARP network stack already provided in
`kernel/net.c`.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/net.html

### Recommended reading before coding

- [xv6 book, Chapter 5 "Interrupts and device drivers"](https://pdos.csail.mit.edu/6.828/2022/xv6/book-riscv-rev3.pdf)
- [Intel 8254x GbE Software Developer's Manual](https://pdos.csail.mit.edu/6.1810/2022/readings/8254x_GBe_SDM.pdf)
  (Sections 2, 3.2-3.4, 4.1, 13, and 14 are the most relevant)

### Key mechanisms used

#### DMA and the descriptor rings

The E1000 uses **DMA**: it reads packets to transmit straight from RAM, and writes
received packets straight into RAM. The driver describes those RAM buffers with two
**circular rings of descriptors** (16 entries each), whose format is fixed by the
hardware:

| Descriptor | Field | Meaning |
| --- | --- | --- |
| `struct tx_desc` | `addr` | physical address of the packet data |
| | `length` | packet length |
| | `cmd` | command bits: `E1000_TXD_CMD_EOP` (end of packet), `E1000_TXD_CMD_RS` (report status) |
| | `status` | `E1000_TXD_STAT_DD` (descriptor done) is set by the E1000 when transmission finishes |
| `struct rx_desc` | `addr` | physical address of the buffer the E1000 DMAs a packet into |
| | `length` | length of the received packet |
| | `status` | `E1000_RXD_STAT_DD` is set by the E1000 when a packet has been written |

#### Control registers

`regs` points to the first memory-mapped control register; other registers are
reached by indexing `regs` as an array. The ones the driver needs:

| Register | Role |
| --- | --- |
| `E1000_TDT` | TX descriptor tail: the next TX slot to fill. Writing `TDT = (TDT+1) % TX_RING_SIZE` hands a packet to the E1000 |
| `E1000_RDT` | RX descriptor tail: the last RX descriptor the driver has processed. `RDT+1` is where the next received packet waits |
| `E1000_ICR` | interrupt cause register; writing `0xffffffff` acknowledges the interrupt |

#### The mbuf

`struct mbuf` (in [`kernel/net.h`](./xv6_for_Lab7/kernel/net.h)) is the packet
buffer: `head` points at the packet's content, `len` is its length, and the
`buf[2048]` array is the backing store. `mbufalloc(0)`/`mbuffree()` allocate and
free them.

## Exercises

### Your Job: the E1000 driver (hard)

**Interfaces used**

| Interface | Kind | Description |
| --- | --- | --- |
| `e1000_transmit(struct mbuf *m)` | kernel driver | place the packet in a TX ring descriptor so the E1000 sends it; free the mbuf only after transmission completes |
| `e1000_recv()` | kernel driver | scan the RX ring, deliver each new packet to the network stack via `net_rx()`, and replace the consumed buffer |
| `net_rx(struct mbuf *)` | kernel network stack | dispatch a received Ethernet frame to the IP/ARP/UDP handlers in [`kernel/net.c`](./xv6_for_Lab7/kernel/net.c) |
| `mbufalloc(0)` / `mbuffree(m)` | kernel helper | allocate / free packet buffers |

**How the pieces fit together**

```
  user: nettests
     | sys_socket / sys_send / sys_recv
     v
 net.c: net_tx_udp() ... net_tx_eth()        net_rx() <----+
     |                                            |        |
     v                                            |        |
 e1000_transmit(m)                           e1000_recv()  |  (called from
     |  fill TX ring, TDT++                  (interrupt    |   e1000_intr)
     v                                            handler) |
  TX ring ----DMA----> E1000 ----DMA----> RX ring --------+
     ^  (SLIRP)          |          10.0.2.15
     |                   v
  host 10.0.2.2 <----> qemu user-mode network stack (packets.pcap)
```

**Implementation steps** ([`kernel/e1000.c`](./xv6_for_Lab7/kernel/e1000.c))

1. **`e1000_transmit(struct mbuf *m)`**
   - Read `regs[E1000_TDT]` to learn the TX slot the E1000 expects next.
   - If `tx_ring[idx].status & E1000_TXD_STAT_DD` is not set, the E1000 has not
     finished the previous transmission at this slot: the ring is full -- release the
     lock and return **-1** so the caller frees the mbuf.
   - Otherwise `mbuffree(tx_mbufs[idx])` if a previous mbuf is still stashed there
     (it was already sent, since DD is set).
   - Fill the descriptor: `addr = (uint64)m->head`, `length = m->len`,
     `cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS`, `status = 0`, and stash
     `tx_mbufs[idx] = m`.
   - Advance the ring: `regs[E1000_TDT] = (idx + 1) % TX_RING_SIZE`, release the
     lock, return 0.

2. **`e1000_recv()`**
   - Compute `idx = (regs[E1000_RDT] + 1) % RX_RING_SIZE` -- the slot holding the
     next packet, if any.
   - Loop while `rx_ring[idx].status & E1000_RXD_STAT_DD`: set `m->len` from the
     descriptor, allocate a replacement mbuf (`mbufalloc(0)`), point
     `rx_ring[idx].addr` at the new buffer, clear the status bits, update
     `regs[E1000_RDT] = idx`, advance `idx`, and hand the old mbuf to `net_rx(m)`.
   - The loop (rather than a single packet) is required because bursts can outrun the
     driver: more packets than the ring size can arrive back to back.

**Key points**

- **The e1000 lock is released around `net_rx()`.** `net_rx()` may call
  `e1000_transmit()` (e.g. to send the ARP reply), which re-acquires
  `e1000_lock`; calling it while holding the lock would self-deadlock.
- **A TX mbuf may only be freed when `E1000_TXD_STAT_DD` is set** -- the E1000 reads
  `m->head` via DMA asynchronously, so freeing early would free memory the card may
  still read.
- **On TX failure return -1** (the caller frees the mbuf); on success the driver owns
  the mbuf and frees it on the next lap around the ring.
- **The RX ring must never run out of buffers**: every descriptor must always hold a
  fresh `mbufalloc` buffer, otherwise the E1000 has nowhere to DMA an incoming packet.
- All shared state (the rings, `tx_mbufs`/`rx_mbufs`, `regs`) is protected by the
  `e1000_lock` spinlock, since the kernel may transmit from several processes and
  receive in interrupt context concurrently.

**Expected output** (from the lab description)

In one window run `make server`, in another `make qemu`, then in xv6:

```
$ nettests
nettests running on port 25603
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
```

`make server` prints `a message from xv6!`. The recorded traffic can be inspected
with `tcpdump -XXnr packets.pcap`, which should contain the strings "ARP, Request",
"ARP, Reply", "UDP", "a.message.from.xv6" and "this.is.the.host".

Solution: [`kernel/e1000.c`](./xv6_for_Lab7/kernel/e1000.c)

## Testing

In the `xv6_for_Lab7` directory:

    make grade          # run all grading tests

The `time.txt` file (a single integer, the hours spent on the lab) is also
included.

Result on this implementation:

```
== Test running nettests ==
== Test   nettest: ping ==              nettest: ping: OK
== Test   nettest: single process ==    nettest: single process: OK
== Test   nettest: multi-process ==     nettest: multi-process: OK
== Test   nettest: DNS ==               nettest: DNS: OK
== Test time ==                         time: OK
Score: 100/100
```

## Optional challenge exercises

From the lab description (not graded):

- Queue egress packets in software and only hand a limited number to the NIC at a
  time, relying on TX interrupts to refill the transmit ring (instead of the
  current interrupt-driven ingress / polled egress scheme). This makes it possible
  to prioritize different types of egress traffic. (easy)
- The provided networking code only partially supports ARP: implement a full
  [ARP cache](https://tools.ietf.org/html/rfc826) and wire it into `net_tx_eth()`.
  (moderate)
- The E1000 supports multiple RX and TX rings: configure one ring pair per core and
  modify the networking stack to use them, increasing throughput and reducing lock
  contention. (moderate, but difficult to test/measure)
- `sockrecvudp()` uses a singly-linked list to find the destination socket; replace
  it with a hash table and RCU. (easy, but a serious implementation is hard to
  test/measure)
- Detect [ICMP](https://tools.ietf.org/html/rfc792) notifications of failed
  networking flows and propagate them as errors through the socket system call
  interface.
- Use one or more of the E1000's stateless hardware offloads (checksum calculation,
  RSC, GRO) to increase throughput. (moderate, but hard to test/measure)
- The networking stack in this lab is susceptible to receive livelock; devise and
  implement a solution to fix it. (moderate, but hard to test)
- Implement a UDP server for xv6. (moderate)
- Implement a minimal TCP stack and download a web page. (hard)

