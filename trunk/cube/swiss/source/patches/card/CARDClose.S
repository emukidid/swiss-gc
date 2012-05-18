#include "../asm.h"

#	CARDClose - "Closes a file."
#	r3	FileInfo

#	return slot 0 CARD_RESULT_READY
#	return slot 1 CARD_RESULT_NOCARD

.globl CARDClose
CARDClose:

	li	%r3,	0
	blr

	.globl CARDClose_length
	CARDClose_length:
	.long (CARDClose_length - CARDClose)