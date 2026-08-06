#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 va;        // arg0: virtual address
  int npages;       // arg1: number of pages
  uint64 uaddr;     // arg2: user space address to store accessed bits
  uint64 abits = 0;    // temp variable to store accessed bits
  struct proc *p = myproc();

  // Get the arguments from the user stack
  argaddr(0, &va);
  argint(1, &npages);
  argaddr(2, &uaddr);
  
  if (npages <= 0 || npages > 64) {
    return -1; // Invalid number of pages
  }

  for(int i = 0; i < npages; i++){
    pte_t *pte = walk(p->pagetable, va + i*PGSIZE, 0);
    if(pte == 0 || (*pte & PTE_V) == 0)
      continue;                   // the page is not mapped, skip
    if(*pte & PTE_A){
      abits |= (1 << i);          // the page is accessed → set the i-th bit
      *pte &= ~PTE_A;             // clear the A bit for next detection
    }
  }

  if(copyout(p->pagetable, uaddr, (char*)&abits, sizeof(abits)) < 0)
    return -1; // Error in copying out the accessed bits to user space
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
