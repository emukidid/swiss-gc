#***************************************************************************
# Device independent Read code launch pad
#
# We have 0x2400 bytes to play with at 0x80000C00 (code+data)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.globl _start, __main
_start:
	mfspr	r9, 1008
	ori		r10, r9, 0x0008
	mtspr	1008, r10
	isync
	sync
	mtspr	1008, r9
	rfi
	nop

	b		exi_handler
	b		exi_lock
	b		perform_read
	b		tickle_read
	b		tickle_read_hook
	b		tickle_read_idle
	b		exit_to_pref			# checks for a button combo and exits if true

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
