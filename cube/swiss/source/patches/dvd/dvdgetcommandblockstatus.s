#include "../asm.h"

# Return commandBlock->status
#	s32 DVDGetCommandBlockStatus(DVDCommandBlock* commandBlock);
#	r3	dvdstruct

.globl DVDGetCommandBlockStatus
DVDGetCommandBlockStatus:
	lwz     %r3,	0x0C(%r3)	# just return state
	blr

.globl DVDGetCommandBlockStatus_length
   DVDGetCommandBlockStatus_length:
.long (DVDGetCommandBlockStatus_length - DVDGetCommandBlockStatus)
