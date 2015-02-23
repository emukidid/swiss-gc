#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
# This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.section .text
	.globl _start, __main
_start:
	b		handle_disc_swap			# Re-authenticates a DVD-R disc
	b		dvd_read_patched_section	# Either reads from SD, or allows the DVD read to occur.
	
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
	icbi		r0, r3
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr
