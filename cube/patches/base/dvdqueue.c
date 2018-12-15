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
u32 process_queue(void* dst, u32 len, u32 offset) {
	
	*(vu32*)VAR_INTERRUPT_TIMES += 1;
	if((*(vu32*)VAR_INTERRUPT_TIMES) < 4)
		return 0;
	*(vu32*)VAR_INTERRUPT_TIMES = 0;
	
	if(len) {
		u32 amountAllowedToRead = len;
		u32 dmaAddr = (((*(vu16*)0xCC005030) & 0x1FFF) << 16) | ((*(vu16*)0xCC005032) & 0xFFFF);
		if(dmaAddr) {
			vu16* dmaBuffer = (vu16*)(0x80000000|dmaAddr);
			if(dmaBuffer[1] || dmaBuffer[3]) {
				// Assume 32KHz, ~128 bytes @ 32KHz is going to play for 1000us
				u32 dmaBytesLeft = (((*(vu16*)0xCC00503A) & 0x7FFF)<<5);
				u32 usecToPlay = ((dmaBytesLeft >> SAMPLE_SIZE_32KHZ_1MS_SHIFT) << 10) >> 1;
				if(usecToPlay > *(vu32*)VAR_DEVICE_SPEED) {
					// Thresholds based on SD speed tests on 5 different cards (EXI limit reached before SPI)
					amountAllowedToRead = usecToPlay<<1;
				}
				else {
					amountAllowedToRead = 0x4000;
				}
#ifdef DEBUG_VERBOSE
				*(vu32*)0x80000AF8 += amountAllowedToRead;
				*(vu32*)0x80000AFC = (*(vu32*)0x80000AFC) + 1;
				usb_sendbuffer_safe("Partial: ",9);
				print_int_hex(amountAllowedToRead);
				usb_sendbuffer_safe(" num: ",6);
				print_int_hex(*(vu32*)0x80000AFC);
				usb_sendbuffer_safe(" dma: ",6);
				print_int_hex(dmaAddr);
				usb_sendbuffer_safe("\r\n",2);
#endif
			}
#ifdef DEBUG_VERBOSE
			else {
				*(vu32*)0x80000AF4 = (*(vu32*)0x80000AF4) + 1;
				usb_sendbuffer_safe("Full: ",6);
				print_int_hex(amountAllowedToRead);
				usb_sendbuffer_safe(" num: ",6);
				print_int_hex(*(vu32*)0x80000AF4);
				usb_sendbuffer_safe(" dma: ",6);
				print_int_hex(dmaAddr);			
				usb_sendbuffer_safe("\r\n",2);
			}
#endif
		}
		u32 amountToRead = len > amountAllowedToRead ? amountAllowedToRead:len;
		device_frag_read(dst, amountToRead, offset);
		return amountToRead;
	}
	return 0;
}
