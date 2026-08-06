#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
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
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;
  backtrace(); // print the backtrace of the current process

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

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

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;

  argint(0, &interval); // a0 = ticks
  argaddr(1, &handler); // a1 = handler

  struct proc *p = myproc();
  p->alarm_interval = interval;
  p->alarm_handler = handler;
  p->alarm_ticks = 0;
  if (interval == 0)
    p->alarm_active = 0;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();

  // save the a0 value from the backup trapframe
  // syscall() will cover trapframe->a0 with the return value
  // so we need to save it before restoring the trapframe
  uint64 saved_a0 = p->alarm_backup.a0;

  // restore the original trapframe from the backup
  *p->trapframe = p->alarm_backup;

  // allow new alarm to be set again
  p->alarm_active = 0;

  // return the saved a0 value to the user program
  return saved_a0;
}