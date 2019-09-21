/***************************************************************************
* In Game Reset code for Swiss
* emu_kidid 2016
*
***************************************************************************/

#include "../../reservedarea.h"
#include "common.h"

void memset32(u32 dst,u32 fill,u32 len)
{
	u32 i;
	for(i = 0; i < len; i+=4)
		*((vu32*)(dst+i)) = fill;
}

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

#ifndef DEBUG
void *memcpy(void *dest, const void *src, u32 size);

static void load_dol() {
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
	memset32(hdr->bssAddress, 0, hdr->bssLength);
	
	void (*entrypoint)() = (void(*)())hdr->entryPoint;
	entrypoint();
}

#define PAD_BUTTON_LEFT		0x0001
#define PAD_BUTTON_RIGHT	0x0002
#define PAD_BUTTON_DOWN		0x0004
#define PAD_BUTTON_UP		0x0008
#define PAD_BUTTON_Z		0x0010
#define PAD_BUTTON_R		0x0020
#define PAD_BUTTON_L		0x0040
#define PAD_USE_ORIGIN		0x0080
#define PAD_BUTTON_A		0x0100
#define PAD_BUTTON_B		0x0200
#define PAD_BUTTON_X		0x0400
#define PAD_BUTTON_Y		0x0800
#define PAD_BUTTON_START	0x1000
#define PAD_GET_ORIGIN		0x2000

#define PAD_COMBO_EXIT	(PAD_BUTTON_B | PAD_BUTTON_R | PAD_BUTTON_Z | PAD_BUTTON_DOWN)

typedef struct {
	u16 button;
	s8 stickX;
	s8 stickY;
	s8 substickX;
	s8 substickY;
	u8 triggerL;
	u8 triggerR;
	u8 analogA;
	u8 analogB;
	s8 err;
} PADStatus;

// exits to user preferred method
void igr_exit(void) {
	disable_interrupts();
	end_read();
	
	u8 igr_exit_type = *(vu8*)VAR_IGR_EXIT_TYPE;
	
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
#else
void igr_exit(void) {
	disable_interrupts();
	end_read();
}
#endif

void check_pad(s32 chan, PADStatus *status) {
	status->button &= ~PAD_USE_ORIGIN;
	
	if((status->button & PAD_COMBO_EXIT) == PAD_COMBO_EXIT) {
		igr_exit();
	}
}
