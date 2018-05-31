/***************************************************************************
* In Game Reset code for Swiss
* emu_kidid 2016
*
***************************************************************************/

#include "../../reservedarea.h"
#include "common.h"

#ifdef WKF
extern void wkfReinit();
#endif

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

void *memcpy(void *dest, const void *src, u32 size)
{
	char *d = dest;
	const char *s = src;

	while (size--)
		*d++ = *s++;

	return dest;
}

void load_dol() {
	u32 base = 0x81300000;
	// Read the binary file mapped to 0x60000000
	device_frag_read((void*)base, *(vu32*)VAR_IGR_DOL_SIZE, 0x60000000);
	
	int i = 0;
	DOLHEADER *hdr = (DOLHEADER *) base;
	for (i = 0; i < MAXTEXTSECTION; i++) {
		if (hdr->textAddress[i] && hdr->textLength[i]) {
			memcpy((void*)hdr->textAddress[i], (void*)(base+hdr->textOffset[i]), hdr->textLength[i]);
			dcache_flush_icache_inv((void*)hdr->textAddress[i], hdr->textLength[i]);
		}
	}
	for (i = 0; i < MAXDATASECTION; i++) {
		if (hdr->dataAddress[i] && hdr->dataLength[i]) {
			memcpy((void*)hdr->dataAddress[i], (void*)(base+hdr->dataOffset[i]), hdr->dataLength[i]);
			dcache_flush_icache_inv((void*)hdr->dataAddress[i], hdr->dataLength[i]);
		}
	}
	
	for(i = 0; i < hdr->bssLength; i+=4)
		*(vu32*)(hdr->bssAddress+i) = 0;
	void (*entrypoint)() = (void(*)())hdr->entryPoint;
	entrypoint();
}

// exits to user preferred method
void exit_to_pref(void) {
	u8 igr_exit_type = *(vu8*)VAR_IGR_EXIT_TYPE;
	
#ifdef WKF
	wkfReinit();
#endif
	
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
	else if (igr_exit_type == IGR_BOOTBIN && *(vu32*)VAR_IGR_DOL_SIZE) {
		// B = 02000000
		// R = 00200000
		// Z = 00100000
		// D = 00040000
		/* disable interrupts */
		asm volatile("mfmsr 3 ; rlwinm 3,3,0,17,15 ; mtmsr 3");
		volatile u16* const _memReg = (u16*)0xCC004000;
		volatile u16* const _dspReg = (u16*)0xCC005000;
		/* stop audio dma */
		_dspReg[27] = (_dspReg[27]&~0x8000);
		/* disable dcache and icache */
		//asm volatile("sync ; isync ; mfspr 3,1008 ; rlwinm 3,3,0,18,15 ; mtspr 1008,3");
		/* disable memory protection */
		_memReg[15] = 0xF;
		_memReg[16] = 0;
		_memReg[8] = 0xFF;
		load_dol();
	}
	else if (igr_exit_type == IGR_USBGKOFLASH) {
		
	}
}
