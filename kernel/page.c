#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define FLAG_D (1 << 0)
#define FLAG_A (1 << 1)

char*
get_pte_flags(pte_t pte)
{
    static char attr_str[8];
    const pte_t attr_mask[] = {PTE_R, PTE_W, PTE_X, PTE_U, PTE_G, PTE_A, PTE_D};
    const char attr_chars[] = {'R','W','X','U','G','A','D'};

    for (int i = 0; i < 7; i++) {
        attr_str[i] = (pte & attr_mask[i]) ? attr_chars[i] : '_';
    }
    attr_str[7] = '\0';
    return attr_str;
}

char*
fmt_idx(uint64 idx) {
    static char buf[6];
    const char* hex = "0123456789abcdef";
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = hex[(idx >> 8) & 0xF];
    buf[3] = hex[(idx >> 4) & 0xF];
    buf[4] = hex[idx & 0xF];
    buf[5] = '\0';
    return buf;
}

void
vmprint_recursive(pagetable_t pagetable, int level, uint64 va_base, uint64 va_start, uint64 va_end, int flags)
{
    for (int i = 0; i < 512; i++) {
        pte_t *pte = &pagetable[i];
        if(!(*pte & PTE_V)) continue;

        uint64 child_va = va_base | (i << PXSHIFT(level));
        uint64 page_size = PAGESZ(level);

        if (va_start && (child_va + page_size <= va_start || child_va >= va_end)) continue;

        if (level == 0 && flags && !(*pte & flags)) continue;

        for (int s = 0; s < (2 - level) * 4; s++) printf(".");
        printf("%s -> %p %s\n", fmt_idx(i), (void *) PTE2PA(*pte), get_pte_flags(*pte));

        if ((*pte & (PTE_R | PTE_W | PTE_X)) == 0) {
            vmprint_recursive((pagetable_t)PTE2PA(*pte), level - 1, child_va, va_start, va_end, flags);
        }
    }
}

uint64
sys_vmprint(void)
{
    struct proc *p = myproc();
    uint64 buf;
    uint64 len;
    int flags;
    int pte_flags = 0;

    argaddr(0, &buf);
    argaddr(1, &len);
    argint(2, &flags);

    if (!buf || !len) {
        buf = 0;
        len = MAXVA;
    }

    if (buf + len > MAXVA) return -1;

    if (flags & FLAG_D) pte_flags |= PTE_D;
    if (flags & FLAG_A) pte_flags |= PTE_A;
    if (flags & ~(FLAG_D | FLAG_A)) return -1;


    printf("PAGETABLE 0x%p\n", p->pagetable);
    vmprint_recursive(p->pagetable, 2, 0, buf, buf + len, pte_flags);

    return 0;
}

void
vmclear_recursive(pagetable_t pagetable, int level, uint64 va_base, uint64 va_start, uint64 va_end, int flags)
{
    for (int i = 0; i < 512; i++) {
        pte_t *pte = &pagetable[i];
        if(!(*pte & PTE_V)) continue;

        uint64 child_va = va_base | (i << PXSHIFT(level));
        uint64 page_size = PAGESZ(level);

        if (va_start && (child_va + page_size <= va_start || child_va >= va_end)) continue;

        if (level == 0) *pte &= ~flags;
        else if (level > 0) vmclear_recursive((pagetable_t)PTE2PA(*pte), level - 1, child_va, va_start, va_end, flags);
    }
}


uint64
sys_vmclear(void)
{
    struct proc *p = myproc();
    uint64 buf;
    uint64 len;
    int flags;
    int pte_flags = 0;

    argaddr(0, &buf);
    argaddr(1, &len);
    argint(2, &flags);

    if (!buf || !len) {
        buf = 0;
        len = MAXVA;
    }

    if (buf + len > MAXVA) return -1;

    if (flags & FLAG_D) pte_flags |= PTE_D;
    if (flags & FLAG_A) pte_flags |= PTE_A;
    if (flags & ~(FLAG_D | FLAG_A)) return -1;

    vmclear_recursive(p->pagetable, 2, 0, buf, buf + len, pte_flags);

    return 0;
}
