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

static struct {
    struct spinlock random_lock;
    struct spinlock nullstat_lock;

    uint32 rng_seed;
    uint64 nullstat_cnt;
} pdev;

#define INPUT_BUF_SIZE 128
static char zero_buf[INPUT_BUF_SIZE];

static uint8
nextbyte(void)
{
    pdev.rng_seed = pdev.rng_seed * 1664525U + 1013904223U; // LCG
    return (pdev.rng_seed >> 24) & 0xFF;
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
            if (n != sizeof(uint32))
                return -1;

            uint32 new_seed;
            if (either_copyin((char*)&new_seed, user_src, src, n) == -1) {
                return 0;
            }
            acquire(&pdev.random_lock);
            pdev.rng_seed= new_seed;
            release(&pdev.random_lock);
            return n;

        case DEV_NULLSTAT: {
            acquire(&pdev.nullstat_lock);
            pdev.nullstat_cnt += n;
            release(&pdev.nullstat_lock);
            return n;
        }

        default:
            return -1;
    }
}

#define MIN(a,b)  ((a) < (b) ? (a) : (b))

int
pseudodevread(int user_dst, uint64 dst, int n, short minor)
{
    char buf[64];
    int ret = 0;

    switch(minor) {
        case DEV_NULL:
            return 0;

        case DEV_ZERO:
            while (n > 0) {
                int chunk = MIN(n, PGSIZE);
                if (either_copyout(user_dst, dst, zero_buf, chunk) < 0) {
                    return -1;
                }
                dst += chunk;
                n -= chunk;
                ret += chunk;
            }
            return ret;

        case DEV_URANDOM:
            while (n > 0) {
                int chunk = MIN(n, (int)sizeof(buf));

                acquire(&pdev.random_lock);
                for (int i = 0; i < chunk; i++) {
                    buf[i] = nextbyte();
                }
                release(&pdev.random_lock);

                if (either_copyout(user_dst, dst, buf, chunk) < 0) {
                    return -1;
                }
                dst += chunk;
                n -= chunk;
                ret += chunk;
            }
            return ret;

        case DEV_NULLSTAT:
            if (n != sizeof(uint64)) {
                return -1;
            }

            uint64 total;
            acquire(&pdev.nullstat_lock);
            total = pdev.nullstat_cnt;
            release(&pdev.nullstat_lock);

            if (either_copyout(user_dst, dst, &total, sizeof(total)) < 0) {
                return -1;
            }
            return sizeof(total);

        default:
            return -1;
    }
}

void 
pseudodevinit(void) {
    initlock(&pdev.random_lock, "random");
    initlock(&pdev.nullstat_lock, "nullstat");

    pdev.rng_seed = 1337;
    pdev.nullstat_cnt = 0;

    devsw[PSEUDO].read = pseudodevread;
    devsw[PSEUDO].write = pseudodevwrite;
}