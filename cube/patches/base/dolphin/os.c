#include <stddef.h>
#include "os.h"

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
		OSFPUContext  = NULL;
}
