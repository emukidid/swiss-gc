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

	.globl dsi_exception_vector
dsi_exception_vector:
	mtsprg	0, r4
	mtsprg	1, r5
	mtsprg	2, r6
	mfsrr1	r6
	mfcr	r4
	extrwi.	r5, r6, 1, 29
	rlwinm	r6, r6, 0, 30, 28
	bne		1f
	mtcr	r4
	lis		r4, dsi_exception_handler@ha
	addi	r4, r4, dsi_exception_handler@l
	mfsrr0	r5
	mtsrr0	r4
	mfmsr	r4
	ori		r4, r4, 0x34
	mtsrr1	r4
	rfi
1:	mtcr	r4
	mtsrr1	r6
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	ba		0x00000304

dsi_exception_handler:
	mfcr	r4
	stw		r4, -712 + 128 (sp)
	mflr	r4
	stw		r4, -712 + 132 (sp)
	mfctr	r4
	stw		r4, -712 + 136 (sp)
	mfxer	r4
	stw		r4, -712 + 140 (sp)
	mfsprg	r4, 0
	stw		r5, -712 + 408 (sp)
	mfsprg	r5, 1
	stw		r6, -712 + 412 (sp)
	mfsprg	r6, 2
	stm		r0, -712 (sp)
	stwu	sp, -712 - 8 (sp)
	addi	r3, sp, 8
	bl		service_exception
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
	lm		r0, 0 (r3) // :)
	rfi

	.org 0x80000D00 - 0x80000C00
	.globl trace_exception
trace_exception:
	rfi

	.globl _start
_start:
	b		init
#ifdef CARD_EMULATOR
	trap
	trap
#else
	b		exi_probe
	b		exi_try_lock
#endif
	b		set_irq_handler
	b		mask_irq
	b		unmask_irq
	b		idle_thread
	b		check_pad
	b		fini

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

#include "memcpy.S"

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
