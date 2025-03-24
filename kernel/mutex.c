#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

int logger = 1;

int
mutexalloc(struct file **f)
{
    struct sleeplock *mu = 0;
    *f = 0;
    if ((*f = filealloc()) == 0) {
        if (logger) printf("ERROR: %d [mutexalloc] filealloc failed\n", myproc()->pid);
        goto bad;
    }
    if (logger) printf("INFO: %d [mutexalloc] filealloc OK (fd=0x%p)\n", myproc()->pid, *f);
    if ((mu = (struct sleeplock*)kalloc()) == 0) {
        if (logger) printf("ERROR: %d [mutexalloc] kalloc failed\n", myproc()->pid);
        goto bad;
    }
    if (logger) printf("INFO: %d [mutexalloc] kalloc OK (mu=0x%p)\n", myproc()->pid, mu);

    (*f)->type = FD_MUTEX;
    (*f)->mutex = mu;

    initsleeplock((*f)->mutex, "mutex");
    (*f)->readable = 0;
    (*f)->writable = 0;

    if (logger) printf("INFO: %d [mutexalloc] mutex created (mu=0x%p)\n", myproc()->pid, mu);
    return 0;
bad:
    if (mu) {
        kfree(mu);
        if (logger) printf("INFO: %d [mutexalloc] kfree mu=0x%p\n", myproc()->pid, mu);
    }
    if (*f) {
        fileclose(*f);
        if (logger) printf("INFO: %d [mutexalloc] fileclosed fd=0x%p\n", myproc()->pid, *f);
    }
    return -1;
}

void
mutexclose(struct file *f)
{
    if (f->mutex) {
        if (f->mutex->locked && f->mutex->pid != myproc()->pid) {
            if (logger) printf("INFO: %d [mutexclose] mutex (mu=0x%p) locked by another process %d\n", myproc()->pid, f->mutex, f->mutex->pid);
            return;
        }
        if (f->mutex->locked) {
            releasesleep(f->mutex);
            if (logger) printf("INFO: %d [mutexclose] mutex unlocked (mu=0x%p)\n", myproc()->pid, f->mutex);
        }

        kfree(f->mutex);
        if (logger) printf("INFO: %d [mutexclose] kfree mu=0x%p\n", myproc()->pid, f->mutex);
    }
    if (logger) printf("WARN: %d [mutexclose] null mutex (f=0x%p)\n", myproc()->pid, f);
}