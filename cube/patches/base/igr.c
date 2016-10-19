/***************************************************************************
* In Game Reset code for Swiss
* emu_kidid 2016
*
***************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

// exits to user preferred method
void exit_to_pref(void) {
	u8 igr_exit_type = *(u8*)VAR_IGR_EXIT_TYPE;
	
	if(igr_exit_type == IGR_HARDRESET) {
		*((volatile u32*)0xCC003024) = 0;
	}
	else if (igr_exit_type == IGR_BOOTBIN) {
		// B = 02000000
		// R = 00200000
		// Z = 00100000
		// D = 00040000
		volatile u16* const _memReg = (u16*)0xCC004000;
		volatile u16* const _dspReg = (u16*)0xCC005000;
		/* disable interrupts */
		asm volatile("mfmsr 3 ; rlwinm 3,3,0,17,15 ; mtmsr 3");
		/* stop audio dma */
		_dspReg[27] = (_dspReg[27]&~0x8000);
		/* disable dcache and icache */
		asm volatile("sync ; isync ; mfspr 3,1008 ; rlwinm 3,3,0,18,15 ; mtspr 1008,3");
		/* disable memory protection */
		_memReg[15] = 0xF;
		_memReg[16] = 0;
		_memReg[8] = 0xFF;
		// Read the binary file mapped to 0x60000000 (4MB max)
		device_frag_read((void*)0x80003100, 4*1024*1024, 0x60000000);
		void (*entrypoint)() = (void(*)())0x80003100;
		entrypoint();
	}
	else if (igr_exit_type == IGR_USBGKOFLASH) {
		
	}
}
