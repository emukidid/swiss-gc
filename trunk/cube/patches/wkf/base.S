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
