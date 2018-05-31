/***************************************************************************
* Second level (crappy!) DVD Queue code for GC/Wii
* emu_kidid 2012
*
***************************************************************************/

#include "../../reservedarea.h"
#include "common.h"

#define SAMPLE_SIZE_32KHZ_1MS 128
#define SAMPLE_SIZE_32KHZ_1MS_SHIFT 7

// Returns how much was read.
u32 process_queue(void* dst, u32 len, u32 offset, int readComplete) {
	
	*(vu32*)VAR_INTERRUPT_TIMES += 1;
	if((*(vu32*)VAR_INTERRUPT_TIMES) < 4)
		return 0;
	*(vu32*)VAR_INTERRUPT_TIMES = 0;
	
	if(len) {
		// read a bit
		int amountToRead = 0;
		if(readComplete) {
			amountToRead = len;
		}
		else {
			// Assume 32KHz, ~128 bytes @ 32KHz is going to play for 1000us
			int dmaBytesLeft = (((*(vu16*)0xCC00503A) & 0x7FFF)<<5);

			int usecToPlay = ((dmaBytesLeft >> SAMPLE_SIZE_32KHZ_1MS_SHIFT) << 10) >> 1;
			u32 amountAllowedToRead = 8192;
			// Is there enough audio playing at least to make up for 1024 bytes to be read?
			if(usecToPlay > *(vu32*)VAR_DEVICE_SPEED) {
				// Thresholds based on SD speed tests on 5 different cards (EXI limit reached before SPI)
				amountAllowedToRead = usecToPlay<<1;
			}
			amountToRead = len > amountAllowedToRead ? amountAllowedToRead:len;
		}
		device_frag_read(dst, amountToRead, offset);
		return amountToRead;
	}
	return 0;
}
