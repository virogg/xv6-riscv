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
  if(n < 0)
    n = 0;
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
sys_ps_listinfo(void)
{
    struct procinfo *plist;
    int lim;
    struct proc *p;
    int cnt = 0;

    if (argaddr(0, &plist) < 0 || argint(1, &lim) < 0) {
        return -1;
    }

    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state != UNUSED) {
            cnt++;
        }
        release(&p->lock);
    }

    if (plist == 0) {
        return cnt;
    }

    if (cnt > lim) {
        return -2;
    }

    int res = 0;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p-> state == UNUSED) {
            release(&p->lock);
            continue;
        }

        struct procinfo info;
        info.pid = p->pid;
        info.state = p->state;
        safestrcpy(info.name, p->name, sizeof(info.name));

        acquire(&wait_lock);
        info.ppid = p->parent ? p->parent->pid : -1; //or 1?
        release(&wait_lock);

        release(&p->lock);


        uint64 t = plist + (uint64)res * sizeof(info);
        if (copyout(myproc()->pagetable, t, (char*)&info, sizeof(info)) < 0) {
            return -3;
        }
        res++;
    }
    return res;
}