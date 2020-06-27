#ifndef AR_H
#define AR_H

#include "common.h"

#define ARAM_DIR_MRAM_TO_ARAM 0
#define ARAM_DIR_ARAM_TO_MRAM 1

static u32 ARGetDMAStatus(void)
{
	return (DSP[2] & 0x200) == 0x200;
}

static void ARStartDMA(u32 type, u32 memAddr, u32 aramAddr, u32 length)
{
	DSP[8] = memAddr;
	DSP[9] = aramAddr;
	DSP[10] = (length & ~(1 << 31)) | ((type & 1) << 31);
}

static void ARClearInterrupt(void)
{
	DSP[2] = (DSP[2] & ~0x88) | 0x20;
}

static u32 ARGetInterruptStatus(void)
{
	return (DSP[2] & 0x20) == 0x20;
}

#define ARStartDMARead(memAddr, aramAddr, length)  ARStartDMA(ARAM_DIR_ARAM_TO_MRAM, memAddr, aramAddr, length)
#define ARStartDMAWrite(memAddr, aramAddr, length) ARStartDMA(ARAM_DIR_MRAM_TO_ARAM, memAddr, aramAddr, length)

#endif /* AR_H */
