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
#define VAR_ASPECT_FLOAT	(VAR_AREA-0x1C)		// Aspect ratio multiply float (8 bytes)

#define __lwbrx(base,index)			\
({	register u32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })
#define _SHIFTL(v, s, w)	\
	((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
	((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} usb_data_req;

extern void pause_audio();

inline u32 bswap32(u32 val) {
	u32 tmp = val;
	return __lwbrx(&tmp,0);
}

#define EXI_CHAN1SR		*(volatile unsigned long*) 0xCC006814 // Channel 1 Status Register
#define EXI_CHAN1CR		*(volatile unsigned long*) 0xCC006820 // Channel 1 Control Register
#define EXI_CHAN1DATA	*(volatile unsigned long*) 0xCC006824 // Channel 1 Immediate Data

#define EXI_TSTART			1


static unsigned int gecko_sendbyte(s32 chn, char data)
{
	unsigned int i = 0;

	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xB0000000 | (data << 20);
	EXI_CHAN1CR = 0x19;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;
	
	if (i&0x04000000)
	{
		return 1;
	}

	return 0;
}


static unsigned int gecko_receivebyte(s32 chn, char* data)
{
	unsigned int i = 0;
	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xA0000000;
	EXI_CHAN1CR = 0x19;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0;

	if (i&0x08000000)
	{
		*data = (i>>16)&0xff;
		return 1;
	} 
	
	return 0;
}


// return 1, it is ok to send data to PC
// return 0, FIFO full
static unsigned int gecko_checktx(s32 chn)
{
	unsigned int i  = 0;

	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xC0000000;
	EXI_CHAN1CR = 0x09;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0x0;

	return (i&0x04000000);
}


// return 1, there is data in the FIFO to recieve
// return 0, FIFO is empty
static unsigned int gecko_checkrx(s32 chn)
{
	unsigned int i = 0;
	EXI_CHAN1SR = 0x000000D0;
	EXI_CHAN1DATA = 0xD0000000;
	EXI_CHAN1CR = 0x09;
	
	while((EXI_CHAN1CR) & EXI_TSTART);
	i = EXI_CHAN1DATA;
	EXI_CHAN1SR = 0x0;

	return (i&0x04000000);
}

int usb_recvbuffer_safe(s32 chn,void *buffer,int size)
{
	char *receivebyte = (char*)buffer;
	unsigned int ret = 0;

	while (size > 0)
	{
		if(gecko_checkrx(chn))
		{
			ret = gecko_receivebyte(chn, receivebyte);
			if(ret == 1)
			{
				receivebyte++;
				size--;
			}
			pause_audio();
		}
	}

	return 0;
}

int usb_sendbuffer_safe(s32 chn,const void *buffer,int size)
{
	char *sendbyte = (char*) buffer;
	unsigned int ret = 0;

	while (size  > 0)
	{
		if(gecko_checktx(chn))
		{
			ret = gecko_sendbyte(chn, *sendbyte);
			if(ret == 1)
			{
				sendbyte++;
				size--;
			}
		}
	}

	return 0;
}


void do_read(void *dst,u32 size, u32 offset) {
	if(!size) return;
	usb_data_req req;
	req.offset = bswap32(offset);
	req.size = bswap32(size);
	usb_sendbuffer_safe(1, &req, sizeof(usb_data_req));
	usb_recvbuffer_safe(1, dst, size);
}
