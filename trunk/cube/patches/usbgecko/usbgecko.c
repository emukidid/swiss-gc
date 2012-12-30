/***************************************************************************
* USB Gecko In-Game Read code for GC/Wii
* emu_kidid 2012
***************************************************************************/

#include "../../reservedarea.h"

#define __lwbrx(base,index)			\
({	register u32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

typedef unsigned int u32;
typedef int s32;
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


void do_read(void *dst,u32 size, u32 offset, u32 unused) {
	if(!size) return;
	usb_data_req req;
	req.offset = bswap32(offset);
	req.size = bswap32(size);
	usb_sendbuffer_safe(1, &req, sizeof(usb_data_req));
	usb_recvbuffer_safe(1, dst, size);
}
