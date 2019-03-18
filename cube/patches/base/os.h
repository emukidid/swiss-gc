#ifndef OS_H
#define OS_H

#include "common.h"

typedef struct {
	u32 gpr[32];
	u32 cr, lr, ctr, xer;
	f64 fpr[32];
	u64 fpscr;
	u32 srr0, srr1;
	u16 mode, state;
	u32 gqr[8];
	f64 psf[32];
} OSContext;

typedef enum {
	OS_EXCEPTION_SYSTEM_RESET = 0,
	OS_EXCEPTION_MACHINE_CHECK,
	OS_EXCEPTION_DSI,
	OS_EXCEPTION_ISI,
	OS_EXCEPTION_EXTERNAL_INTERRUPT,
	OS_EXCEPTION_ALIGNMENT,
	OS_EXCEPTION_PROGRAM,
	OS_EXCEPTION_FLOATING_POINT,
	OS_EXCEPTION_DECREMENTER,
	OS_EXCEPTION_SYSTEM_CALL,
	OS_EXCEPTION_TRACE,
	OS_EXCEPTION_PERFORMANCE_MONITOR,
	OS_EXCEPTION_BREAKPOINT,
	OS_EXCEPTION_SYSTEM_INTERRUPT,
	OS_EXCEPTION_THERMAL_INTERRUPT,
	OS_EXCEPTION_MAX
} OSException;

typedef void (*OSExceptionHandler)(OSException exception, OSContext *context, ...);

static OSExceptionHandler *const OSExceptionTable = (OSExceptionHandler *)0x80003000;

#endif /* OS_H */
