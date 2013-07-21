#define _LANGUAGE_ASSEMBLY

.section .text
	.globl _start, __main
_start:
	lis		1, 0x816F
	b		boot_dol
