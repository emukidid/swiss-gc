#***************************************************************************
# WKF Patch launch pad
#
# We have 0x1800 bytes to play with at 0x80001800 (code+data)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.globl _start, __main
_start:
	b		adjust_read
	b		swap_disc
	b		fake_lid_interrupt
	nop
	nop
	
	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		r4, r4, 0x20 
1:
	addi 		r4, r4, 0x1f
	srwi 		r4, r4, 5
	mtctr 		r4
2:
	dcbf 		r0, r3
	icbi		r0, r3		#todo kill this off
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr
	
	.globl _readsectorsize
_readsectorsize: .long 2048

	.globl _readsector
_readsector: .space 2048
