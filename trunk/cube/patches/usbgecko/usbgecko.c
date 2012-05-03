/***************************************************************************
* USB Gecko In-Game Read code for GC/Wii
* emu_kidid 2012
***************************************************************************/

// we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
// This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
// memory map for our variables that sit at the top 0x100 of memory
#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x100)				// Size of our variables block
#define VAR_DISC_1_LBA 		(VAR_AREA-0x100)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA-0xFC)		// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA-0xF8)		// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA-0xF4)		// is the EXI bus speed (16mhz vs 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA-0xF0)		// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA-0xDC)		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA-0xD8)		// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_TMP1  			(VAR_AREA-0xD4)		// space for a variable if required
#define VAR_TMP2  			(VAR_AREA-0xD0)		// space for a variable if required
#define VAR_TMP3  			(VAR_AREA-0xCC)		// space for a variable if required
#define VAR_TMP4  			(VAR_AREA-0xC8)		// space for a variable if required
#define VAR_CB_ADDR			(VAR_AREA-0xC4)		// high level read callback addr
#define VAR_CB_ARG1			(VAR_AREA-0xC0)		// high level read callback r3
#define VAR_CB_ARG2			(VAR_AREA-0xBC)		// high level read callback r4
#define VAR_PROG_MODE		(VAR_AREA-0xB8)		// data/code to overwrite GXRMode obj with for 480p forcing
#define VAR_MUTE_AUDIO		(VAR_AREA-0x20)		// does the user want audio muted during reads?

#define EXI_SPEED32MHZ				5			/*!< EXI device frequency 32MHz */
#define EXI_READ					0			/*!< EXI transfer type read */
#define EXI_WRITE					1			/*!< EXI transfer type write */
#define EXI_READWRITE				2			/*!< EXI transfer type read-write */

#define __lwbrx(base,index)			\
({	register u32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })
#define _SHIFTL(v, s, w)	\
	((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
	((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} usb_data_req;

inline u32 bswap32(u32 val) {
	u32 tmp = val;
	return __lwbrx(&tmp,0);
}

inline void exi_select()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[5] = (exi[5] & 0x405) | (208);
}

inline void exi_deselect()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[5] &= 0x405;
}

void exi_imm(void* data, int len, int mode)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	if (mode != EXI_READ)
		exi[9] = *(unsigned long*)data;
	else
		exi[9] = -1;
	// Tell EXI if this is a read or a write
	exi[8] = ((len - 1) << 4) | (mode << 2) | 1;
	// Wait for it to do its thing
	while (exi[8] & 1);

	if (mode != EXI_WRITE)
	{
		// Read the 4 byte data off the EXI bus
		unsigned long d = exi[9];
		if(len == 4) {
			*(u32*)data = d;
		}
		else {
			int i;	
			for (i = 0; i < len; ++i)
				((unsigned char*)data)[i] = (d >> ((3 - i) * 8)) & 0xFF;
		}
	}

}

// simple wrapper for transfers > 4bytes 
void exi_imm_ex(void* data, int len, int mode)
{
	unsigned char* d = (unsigned char*)data;
	while (len)
	{
		int tc = len;
		if (tc > 4)
			tc = 4;
		exi_imm(d, tc, mode);
		len -= tc;
		d += tc;
	}
}

static __inline__ int __send_command(u16 *cmd)
{
	exi_select();
	exi_imm_ex(cmd,2,EXI_READWRITE);
	exi_deselect();
}

static void __usb_sendbyte(char ch)
{
	u16 val;
	val = (0xB000|_SHIFTL(ch,4,8));
	__send_command(&val);
}

static int __usb_recvbyte(char *ch)
{
	int ret;
	u16 val;

	*ch = 0;
	val = 0xA000;
	__send_command(&val);
	if(!(val&0x0800)) ret = 0;
	*ch = (val&0xff);

	return ret;
}

int __usb_checksend()
{
	int ret = 1;
	u16 val;

	val = 0xC000;
	__send_command(&val);
	if(!(val&0x0400)) ret = 0;

	return ret;
}

int __usb_checkrecv()
{
	int ret = 1;
	u16 val;

	val = 0xD000;
	ret = __send_command(&val);
	if(!(val&0x0400)) ret = 0;

	return ret;
}

void usb_flush()
{
	char tmp;
	while(__usb_recvbyte(&tmp));
}

int usb_recvbuffer_safe(void *buffer,int size)
{
	int ret;
	int left = size;
	char *ptr = (char*)buffer;

	while(left>0) {
		if(__usb_checkrecv()) {
			__usb_recvbyte(ptr);
			ptr++;
			left--;
		}
	}

	return (size - left);
}

int usb_sendbuffer_safe(const void *buffer,int size)
{
	int ret;
	int left = size;
	char *ptr = (char*)buffer;

	while(left>0) {
		if(__usb_checksend()) {
			__usb_sendbyte(*ptr);
			ptr++;
			left--;
		}
	}

	return (size - left);
}

void do_read(void *dst,u32 size, u32 offset) {
	if(!size) return;
	usb_data_req req;
	req.offset = bswap32(offset);
	req.size = bswap32(size);
	usb_sendbuffer_safe(&req, sizeof(usb_data_req));
	usb_recvbuffer_safe(dst, size);
}
