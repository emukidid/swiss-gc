#include "os.h"

void DCInvalidateRange(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		DCBlockInvalidate(addr);
		addr += 32;
	} while (--i);
}

void DCFlushRange(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		DCBlockFlush(addr);
		addr += 32;
	} while (--i);

	asm volatile("sc" ::: "r9", "r10");
}

void DCFlushRangeNoSync(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		DCBlockFlush(addr);
		addr += 32;
	} while (--i);
}

void DCStoreRangeNoSync(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		DCBlockStore(addr);
		addr += 32;
	} while (--i);
}

void DCZeroRange(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		DCBlockZero(addr);
		addr += 32;
	} while (--i);
}

void ICInvalidateRange(void *addr, u32 nBytes)
{
	if (!nBytes) return;
	nBytes = OSRoundUp32B(((u32)(addr) & (32 - 1)) + nBytes);
	int i = nBytes / 32;

	do {
		ICBlockInvalidate(addr);
		addr += 32;
	} while (--i);
}

void OSSetCurrentContext(OSContext *context)
{
	u32 msr; asm("mfmsr %0" : "=r" (msr));

	OSExceptionContext = OSCachedToPhysical(context);
	OSCurrentContext = context;

	if (OSFPUContext == context) {
		context->srr1 |= 0x2000;
		msr |= 0x2;
		asm volatile("mtmsr %0" :: "r" (msr));
	} else {
		context->srr1 &= ~0x2000;
		msr &= ~0x2000;
		msr |= 0x2;
		asm volatile("mtmsr %0" :: "r" (msr));
		asm volatile("isync");
	}
}

void OSClearContext(OSContext *context)
{
	context->mode  = 0;
	context->state = 0;

	if (OSFPUContext == context)
		OSFPUContext = NULL;
}
