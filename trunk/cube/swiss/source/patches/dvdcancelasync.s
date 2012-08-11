#include "asm.h"

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

# cancel command asynchronously - and return true.
# BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback);
#	r3	block
#	r4	cb	

.globl DVDCancelAsync
DVDCancelAsync:

	stwu    %sp, -0x40(%sp)
	mflr    %r0
	stw     %r0, 8(%sp)
	stw		%r7, 16(%sp)
	
#Disable external interrupts
	mfmsr	%r7
	stw		%r7, 0x20(%sp)		# store old MSR
	rlwinm	%r7,%r7,0,17,15
	mtmsr	%r7
	
#Update block with "cancelled" status, if it hasn't been completed
	lwz     %r0,	0x0C(%r3)	# get state
	cmpwi	%r0,	0			# if this read is already completed, just call the callback and return true
	beq		skip_cancel
	li		%r0,	10
	stw     %r0,	0x0C(%r3)	# state: DVD_STATE_CANCELED
skip_cancel:
# Call the callback if there is one
	cmpwi	%r4,	0
	beq		skip_callback
	li		%r3,	0
	mtctr	%r4					# typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);
	bctrl

skip_callback:
#Enable external interrupts if we disabled them earlier
	lwz		%r7, 0x20(%sp)		# load old MSR
	rlwinm	%r7, %r7, 17, 30,31
	cmpwi	%r7,	0
	beq		skip_setting_msr
	mfmsr	%r7
	ori		%r7,%r7,0x8000
	mtmsr	%r7

skip_setting_msr:		
	li      %r3,	1
	lwz		%r7, 16(%sp)
	lwz     %r0, 8(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x40
	blr
   .globl DVDCancelAsync_length
   DVDCancelAsync_length:
   .long (DVDCancelAsync_length - DVDCancelAsync)
   