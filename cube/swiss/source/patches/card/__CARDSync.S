#include "../asm.h"

#
#	r3	*Async result

.globl __CARDSync
__CARDSync:

	mr		%r3,	%r4
	blr
	
   .globl __CARDSync_length
   __CARDSync_length:
   .long (__CARDSync_length - __CARDSync)