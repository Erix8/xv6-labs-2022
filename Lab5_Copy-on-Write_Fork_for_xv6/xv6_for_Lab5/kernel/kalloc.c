// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// Reference count for each physical page.
// Index = physical address >> PGSHIFT (i.e., pa / 4096).
// Array size is PHYSTOP/PGSIZE, a compile-time constant.
#define PA2IDX(pa) (((uint64)(pa)) >> PGSHIFT)
static int refcnt[PHYSTOP / PGSIZE];

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Increment the reference count of the physical page pa.
// Called when a page is newly shared by another page table
// (e.g., by fork() via uvmcopy()).
void
krefinc(void *pa)
{
  acquire(&kmem.lock);
  refcnt[PA2IDX(pa)]++;
  release(&kmem.lock);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);

  if(refcnt[PA2IDX(pa)] > 1){
    // Other page tables still reference this page:
    // only decrement the count, do not memset nor
    // put the page back on the free list.
    refcnt[PA2IDX(pa)]--;
    release(&kmem.lock);
    return;
  }

  // ref == 1 (last reference) or ref == 0 (initialization
  // via freerange()/kinit()): really free the page.
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;
  refcnt[PA2IDX(pa)] = 0;

  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    refcnt[PA2IDX(r)] = 1; // the page just got its first reference
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
