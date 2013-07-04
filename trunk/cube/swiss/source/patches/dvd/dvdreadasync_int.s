#include "../asm.h"

/* DVD Struct is as follows (passed in r3):
struct DVDFileInfo
{
0x00	DVDCommandBlock* next;
0x04    DVDCommandBlock* prev;
0x08    u32          command;			-- Make me 1
0x0C    s32          state;				-- Make me 0
0x10    u32          offset;			-- If 0, set to startAddr + length, else, set to offset += offset
0x14    u32          length;			-- Set to read length passed in
0x18    void*        addr;				-- Set to dst passed in
0x1C    u32          currTransferSize;	-- Set to read length passed in
0x20    u32          transferredSize;	-- Set to read length passed in
0x24    DVDDiskID*   id;
0x28    DVDCBCallback callback;
0x2C    void*        userData;

0x30    u32             startAddr;      // disk address of file
0x34    u32             length;         // file size in bytes

0x38    DVDCallback     callback;		-- Set me to the callback function passed in
}
*/	

# Queue read command asynchronously - and return true.
#	We have 47 instructions to play with here only as this is the async function.
#	r3	dvdstruct
#	r4	dst	
#	r5	len
#	r6	off
#	r7	cb

.globl DVDReadAsyncInt
DVDReadAsyncInt:

	mflr    %r0
	stwu    %sp, -0x40(%sp)
	stw     %r0, 0x44(%sp)
	stw		%r7, 16(%sp)
	stw		%r3, 20(%sp)
	
#Update dvdstruct with "waiting to enter queue" status, immediately
	li		%r0,	2
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_WAITING (waiting to go in the queue)
	li		%r0,	1
	stw     %r0,	0x08(%r3)	# command : 1

#update dvdstruct with length and dst passed in
	lwz		%r7, 	16(%sp)		# load cb
	stw     %r5,	0x14(%r3)	# length: passed in length
	stw     %r4,	0x18(%r3)	# dest: passed in dest
	stw		%r6,	0x10(%r3)	# offset = offset (passed in)
	stw     %r5,	0x1C(%r3)	# curTransferSize: length
	li		%r0,	0
	stw     %r0,	0x20(%r3)	# transferredSize: 0 so far
	stw     %r7,	0x38(%r3)	# Set the callback addr in the struct

#Call our method which will add this dvdstruct to our read queue. 
#Once added, it will be read in small pieces every time OSRestoreInterrupts is run.
#Finally, once complete, the struct state is updated to STATE_END (0) and the callback is called (if present).
	lis		%r7,	0x8000
	ori		%r7,	%r7,	0x183C
	mtctr	%r7
	bctrl
	
	li      %r3,	1
	lwz		%r7, 16(%sp)
	lwz     %r0, 0x44(%sp)
	addi    %sp, %sp, 0x40
	mtlr    %r0
	blr
   .globl DVDReadAsyncInt_length
   DVDReadAsyncInt_length:
   .long (DVDReadAsyncInt_length - DVDReadAsyncInt)
   