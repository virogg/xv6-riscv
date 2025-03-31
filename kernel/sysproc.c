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

static void
print_pte_flags(uint64 pte)
{
    char flags[8] = "_______";

    if (pte & PTE_R) flags[0] = 'R';
    if (pte & PTE_W) flags[1] = 'W';
    if (pte & PTE_X) flags[2] = 'X';
    if (pte & PTE_U) flags[3] = 'U';
    if (pte & PTE_G) flags[4] = 'G';
    if (pte & PTE_A) flags[5] = 'A';
    if (pte & PTE_D) flags[6] = 'D';

    flags[7] = '\0';
    printf("%s", flags);
}

static char* level_prefix[] = {
        "",
        "..",
        "....",
        "......"
};

static void vmprint_recursive(pagetable_t pagetable, int level) {
    for (int i = 0; i < 512; i++) {
        pte_t pte = pagetable[i];
        if(pte & PTE_V) {
            uint64 child = PTE2PA(pte);

            printf("%s0x%x -> 0x%lx ", level_prefix[level], i, child);
            print_pte_flags(pte);
            printf("\n");

            if((pte & (PTE_R|PTE_W|PTE_X)) == 0) {
                vmprint_recursive((pagetable_t)child, level + 1);
            }
        }
    }
}

uint64 sys_vmprint(void) {
    struct proc *p = myproc();
    printf("PAGETABLE 0x%lx\n", (uint64)p->pagetable);
    vmprint_recursive(p->pagetable, 0);
    return 0;
}

uint64
sys_vmclear(void)
{
    uint64 flags;

    argint(0, (int*)&flags);
    if (flags & ~(PTE_A | PTE_D)) {
        return -1;
    }

    struct proc *p = myproc();
    for (uint64 va = 0; va < MAXVA; va += PGSIZE) {
        pte_t *pte = walk(p->pagetable, va, 0);
        if (pte && (*pte & PTE_V)) {
            *pte &= ~flags;
        }
    }
    return 0;
}
