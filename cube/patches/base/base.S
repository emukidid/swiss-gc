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
	mfcr	r4
	mfdsisr	r5
	mfdar	r6
	rlwinm.	r5, r5, 0, 9, 9
	rlwinm	r5, r6, 16, 16, 31
	rlwinm	r6, r6, 16, 16, 23
	cmplwi	cr7, r6, 0x0800
	cmplwi	cr6, r5, 0x0C00
	bne		1f
	beq		cr7, 1f
	beq		cr6, 1f
	b		unhandled_exception
1:	mtcr	r4
	mfsrr0	r5
	mfsrr1	r6
	mfmsr	r4
	ori		r4, r4, 0x30
	mtsrr1	r4
	lis		r4, dsi_exception_handler@ha
	addi	r4, r4, dsi_exception_handler@l
	mtsrr0	r4
	rfi

dsi_exception_handler:
	mfcr	r4
	stw		r4, -152 + 128 (sp)
	mflr	r4
	stw		r4, -152 + 132 (sp)
	mfctr	r4
	stw		r4, -152 + 136 (sp)
	mfxer	r4
	stw		r4, -152 + 140 (sp)
	mfsprg	r4, 0
	stw		r5, -152 + 144 (sp)
	mfsprg	r5, 1
	stw		r6, -152 + 148 (sp)
	mfsprg	r6, 2
	stm		r0, -152 (sp)
	stwu	sp, -152 - 8 (sp)
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
	lwz		r4, 144 (r3)
	mtsrr0	r4
	lwz		r4, 148 (r3)
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
	b		set_irq_handler
	b		mask_irq
	b		unmask_irq
	b		idle_thread
	b		check_pad
	b		fini

unhandled_exception:
	mtcr	r4
	mfsprg	r6, 2
	mfsprg	r5, 1
	mfsprg	r4, 0
	ba		0x00000304

#include "memcpy.S"
