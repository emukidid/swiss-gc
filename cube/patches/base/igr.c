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

#define DOLHDRLENGTH 256
#define MAXTEXTSECTION 7
#define MAXDATASECTION 11
typedef struct {
    unsigned int textOffset[MAXTEXTSECTION];
    unsigned int dataOffset[MAXDATASECTION];

    unsigned int textAddress[MAXTEXTSECTION];
    unsigned int dataAddress[MAXDATASECTION];

    unsigned int textLength[MAXTEXTSECTION];
    unsigned int dataLength[MAXDATASECTION];

    unsigned int bssAddress;
    unsigned int bssLength;

    unsigned int entryPoint;
    unsigned int unused[MAXTEXTSECTION];
} DOLHEADER;
/*
void load_dol() {

	int i = 0;
	DOLHEADER *hdr = (DOLHEADER *) data;
	// Inspect text sections to see if what we found lies in here
	for (i = 0; i < MAXTEXTSECTION; i++) {
		if (hdr->textAddress[i] && hdr->textLength[i]) {
			// Does what we found lie in this section?
			if((offsetFoundAt >= hdr->textOffset[i]) && offsetFoundAt <= (hdr->textOffset[i] + hdr->textLength[i])) {
				// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
				return offsetFoundAt+hdr->textAddress[i]-hdr->textOffset[i];
			}
		}
	}

	// Inspect data sections (shouldn't really need to unless someone was sneaky..)
	for (i = 0; i < MAXDATASECTION; i++) {
		if (hdr->dataAddress[i] && hdr->dataLength[i]) {
			// Does what we found lie in this section?
			if((offsetFoundAt >= hdr->dataOffset[i]) && offsetFoundAt <= (hdr->dataOffset[i] + hdr->dataLength[i])) {
				// Yes it does, return the load address + offsetFoundAt as that's where it'll end up!
				return offsetFoundAt+hdr->dataAddress[i]-hdr->dataOffset[i];
			}
		}
	}
	
	// Read the binary file mapped to 0x60000000 (4MB max)
	device_frag_read((void*)0x80003100, 4*1024*1024, 0x60000000);
	void (*entrypoint)() = (void(*)())0x80003100;
	entrypoint();
}
*/
// exits to user preferred method
void exit_to_pref(void) {
	u8 igr_exit_type = *(u8*)VAR_IGR_EXIT_TYPE;
	
	if(igr_exit_type == IGR_HARDRESET) {
		// int i = 0;
		// for (i = 0; i < 10; i++) {
			// // renable qoob here
			// volatile u32* exi = (volatile u32*)0xCC006800;
			// exi[0] = (exi[0] & 0x405)|0xB0;	// exi_select(0, 1, 3);
			// exi[4] = 0xC0000000;
			// exi[3] = 0x35;	
			// while (exi[3] & 1);	// exi_write(0, &addr, 4);
			// exi[4] = 0;
			// exi[3] = 0x35;	
			// while (exi[3] & 1);	// exi_write(0, &val, 4);
			// exi[0] &= 0x405;	// exi_deselect(0);
		// }
		*((volatile u32*)0xCC003024) = 0;
	}
	else if (igr_exit_type == IGR_BOOTBIN) {
		// B = 02000000
		// R = 00200000
		// Z = 00100000
		// D = 00040000
		volatile u16* const _memReg = (u16*)0xCC004000;
		volatile u16* const _dspReg = (u16*)0xCC005000;
		/* stop audio dma */
		_dspReg[27] = (_dspReg[27]&~0x8000);
		/* disable dcache and icache */
		asm volatile("sync ; isync ; mfspr 3,1008 ; rlwinm 3,3,0,18,15 ; mtspr 1008,3");
		/* disable memory protection */
		_memReg[15] = 0xF;
		_memReg[16] = 0;
		_memReg[8] = 0xFF;
		//load_dol();
	}
	else if (igr_exit_type == IGR_USBGKOFLASH) {
		
	}
}
