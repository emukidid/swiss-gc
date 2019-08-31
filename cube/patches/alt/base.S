#***************************************************************************
# Device independent Read code launch pad
#
# We have 0x2400 bytes to play with at 0x80000C00 (code+data)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.balign 0x100
	.globl system_call_vector
system_call_vector:
	mfhid0	r9
	ori		r10, r9, 0x8
	mthid0	r10
	isync
	sync
	mthid0	r9
	rfi

	.globl dsi_exception_handler
dsi_exception_handler:
	stw		r0, 0 (r4)
	stw		sp, 4 (r4)
	stw		r2, 8 (r4)
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
	bl		tickle_read_trap

	.globl load_context
load_context:
	lwz		r0, 0 (r3)
	lwz		sp, 4 (r3)
	lwz		r2, 8 (r3)
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
	rfi

	.balign 0x100
	.globl trace_exception_vector
trace_exception_vector:
	mtsprg	0, r0
	mfcr	r0
	mtsprg	1, r0
	mfdabr	r0
	cmplwi	r0, 0
	beq		1f
	ori		r0, r0, 1
	mtdabr	r0
1:	mfsprg	r0, 1
	mtcr	r0
	mfsrr1	r0
	rlwinm	r0, r0, 0, 23, 20
	mtsrr1	r0
	mfsprg	r0, 0
	rfi

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

	.globl _start
_start:
	b		exi_handler
	b		exi_lock
	b		set_breakpoint
	b		unset_breakpoint
	b		perform_read
	b		tickle_read
	b		tickle_read_idle
	b		exit_to_pref			# checks for a button combo and exits if true
