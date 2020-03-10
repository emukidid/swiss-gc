#***************************************************************************
# Device independent Read code launch pad
#
# We have 0x2400 bytes to play with at 0x80000C00 (code+data)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.org 0x80000C00 - 0x80000C00
	.globl system_call_exception
system_call_exception:
	mfhid0	r9
	ori		r10, r9, 0x8
	mthid0	r10
	isync
	sync
	mthid0	r9
	rfi

	.globl exception_handler
exception_handler:
	stswi	r0, r4, 12
	stmw	r6, 24 (r4)
	mfgqr	r0, 1
	stw		r0, 424 (r4)
	mfgqr	r0, 2
	stw		r0, 428 (r4)
	mfgqr	r0, 3
	stw		r0, 432 (r4)
	mfgqr	r0, 4
	stw		r0, 436 (r4)
	mfgqr	r0, 5
	stw		r0, 440 (r4)
	mfgqr	r0, 6
	stw		r0, 444 (r4)
	mfgqr	r0, 7
	stw		r0, 448 (r4)
	mfdsisr	r5
	mfdar	r6
	stwu	sp, -8 (sp)
	bl		service_exception

	.globl load_context, load_context_end
load_context:
	bl		realmode
	lwz		r3, 192 (r0)
	lswi	r0, r3, 12
	lmw		r5, 20 (r3)
	lwz		r4, 424 (r3)
	mtgqr	1, r4
	lwz		r4, 428 (r3)
	mtgqr	2, r4
	lwz		r4, 432 (r3)
	mtgqr	3, r4
	lwz		r4, 436 (r3)
	mtgqr	4, r4
	lwz		r4, 440 (r3)
	mtgqr	5, r4
	lwz		r4, 444 (r3)
	mtgqr	6, r4
	lwz		r4, 448 (r3)
	mtgqr	7, r4
	lwz		r4, 128 (r3)
	mtcr	r4
	lwz		r4, 132 (r3)
	mtlr	r4
	lwz		r4, 136 (r3)
	mtctr	r4
	lwz		r4, 140 (r3)
	mtxer	r4
	lwz		r4, 408 (r3)
	mtsrr0	r4
	lwz		r4, 412 (r3)
	mtsrr1	r4
	lhz		r4, 418 (r3)
	rlwinm	r4, r4, 0, 31, 29
	sth		r4, 418 (r3)
	lwz		r4, 16 (r3)
	lwz		r3, 12 (r3)
load_context_end:
	nop
	rfi

	.org 0x80000D00 - 0x80000C00
	.globl trace_exception
trace_exception:
	rfi

	.globl _start
_start:
	b		exi_probe
	b		exi_trylock
	b		set_di_handler
	b		idle_thread
	b		check_pad
	b		igr_exit

	.globl jump_table, jump_table_end
jump_table:
	.globl OSUnhandledException
	.globl OSMaskInterrupts, OSUnmaskInterrupts
	.globl OSResetSystem
	.globl EXILock, EXIUnlock
OSUnhandledException:
	.long 0
OSMaskInterrupts:
	.long 0
OSUnmaskInterrupts:
	.long 0
OSResetSystem:
	.long 0
EXILock:
	.long 0
EXIUnlock:
	.long 0
	.long 0
jump_table_end:
	.long 0

	.globl realmode
realmode:
	mflr	r3
	clrlwi	r3, r3, 2
	mtsrr0	r3
	mfmsr	r3
	rlwinm	r3, r3, 0, 28, 25
	mtsrr1	r3
	rfi

	.globl switch_fiber
switch_fiber:
	mflr	r0
	mr		r9, sp
	stwu	r9, -8 (r8)
	mr		sp, r8
	stw		r0, 4 (r9)
	mtlr	r7
	blrl
	lwz		r5, 0 (sp)
	lwz		r0, 4 (r5)
	mtlr	r0
	mr		sp, r5
	blr

#include "../base/memcpy.S"

	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	cmplwi	r4, 0
	blelr
	clrlwi	r5, r3, 27
	add		r4, r4, r5
	addi	r4, r4, 31
	srwi	r4, r4, 5
	mtctr	r4
1:	dcbf	r0, r3
	icbi	r0, r3
	addi	r3, r3, 32
	bdnz	1b
	sc
	blr

	.globl dcache_store
dcache_store:
	cmplwi	r4, 0
	blelr
	clrlwi	r5, r3, 27
	add		r4, r4, r5
	addi	r4, r4, 31
	srwi	r4, r4, 5
	mtctr	r4
1:	dcbst	r0, r3
	addi	r3, r3, 32
	bdnz	1b
	blr

	.globl wait_pecopydone
wait_pecopydone:
	lis		r3, 0xCC00
	li		r4, 0
1:	stw		r4, 0x404E (r3)
	mftb	r5
2:	mftb	r6
	sub		r6, r6, r5
	cmplwi	r6, 10
	ble		2b
	lwz		r4, 0x404E (r3)
	cmplwi	r4, 0
	bne		1b
	blr

	.org 0x80000F00 - 0x80000C00 - 0x80
	.globl performance_monitor_interrupt
performance_monitor_interrupt:
	mtsprg	0, r4
	lwz		r4, 192 (r0)
	stw		r3, 12 (r4)
	mfsprg	r3, 0
	stw		r3, 16 (r4)
	stw		r5, 20 (r4)
	lhz		r3, 418 (r4)
	ori		r3, r3, 0x2
	sth		r3, 418 (r4)
	mfcr	r3
	stw		r3, 128 (r4)
	mflr	r3
	stw		r3, 132 (r4)
	mfctr	r3
	stw		r3, 136 (r4)
	mfxer	r3
	stw		r3, 140 (r4)
	mfsrr0	r3
	stw		r3, 408 (r4)
	mfsrr1	r3
	stw		r3, 412 (r4)
	mr		r5, r3
	nop
	mfmsr	r3
	ori		r3, r3, 0x30
	mtsrr1	r3
	li		r3, 11
	lwz		r4, 212 (r0)
	lis		r5, exception_handler@ha
	addi	r5, r5, exception_handler@l
	mtsrr0	r5
	rfi
	b		performance_monitor_interrupt
