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
#include "pseudodev.h"

struct spinlock random_lock, nullstat_lock;
static uint32 base_seed = 1337;
static uint32 current_seed = 1337;
uint64 cnt = 0;

static uint8
nextbyte(void)
{
    current_seed = current_seed * 1664525U + 1013904223U; // LCG
    return (current_seed >> 24) & 0xFF;
}

int
pseudodevwrite(int user_src, uint64 src, int n, short minor)
{
    switch(minor) {
        case DEV_NULL:
            return n;

        case DEV_ZERO:
            return -1;

        case DEV_URANDOM:
            if (n != sizeof(current_seed))
                return -1;

            uint32 new_seed;
            if (either_copyin((char*)&new_seed, user_src, src, n) == -1) {
                return 0;
            }
            acquire(&random_lock);
            base_seed = new_seed;
            current_seed = base_seed;
            release(&random_lock);
            return n;

        case DEV_NULLSTAT: {
            acquire(&nullstat_lock);
            cnt += n;
            release(&nullstat_lock);
            return n;
        }

        default:
            return -1;
    }
}

int
pseudodevread(int user_dst, uint64 dst, int n, short minor)
{
    char *kbuf;
    int ret = -1;

    if((kbuf = kalloc()) == 0)
        return -1;

    switch(minor) {
        case DEV_NULL:
            ret = 0;
            break;

        case DEV_ZERO:
            memset(kbuf, 0, n);
            ret = n;
            break;

        case DEV_URANDOM:
            acquire(&random_lock);
            for (int i = 0; i < n; i++) {
                kbuf[i] = nextbyte();
            }
            release(&random_lock);
            ret = n;
            break;

        case DEV_NULLSTAT:
            if (n != sizeof(uint64)) {
                ret = -1;
                break;
            }
            acquire(&nullstat_lock);
            *(uint64*)kbuf = cnt;
            release(&nullstat_lock);
            ret = sizeof(cnt);
            break;

        default:
            ret = -1;
    }

    // copy the input byte to the user-space buffer.
    if(ret > 0) {
        if(either_copyout(user_dst, dst, kbuf, ret) < 0)
            ret = -1;
    }

    kfree(kbuf);
    return ret;
}

void 
pseudodevinit(void) {
    initlock(&random_lock, "random");
    initlock(&nullstat_lock, "nullstat");

    base_seed = 1337;
    current_seed = base_seed;
    cnt = 0;

    devsw[PSEUDO].read = pseudodevread;
    devsw[PSEUDO].write = pseudodevwrite;
}