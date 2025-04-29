#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

#define Reg(reg) ((volatile uint32*)(RTC0 + (reg)))
#define ReadReg(reg) (*(Reg(reg)))

uint64
rtcget(void)
{
    const uint32 low = ReadReg(0x0);
    const uint32 high = ReadReg(0x4);

    return ((uint64)high << 32) | low;
}