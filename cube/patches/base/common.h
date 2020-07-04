#ifndef COMMON_H
#define COMMON_H

#include "../../reservedarea.h"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;
typedef unsigned long long int u64;
typedef signed long long int s64;
typedef double f64;
typedef volatile u8 vu8;
typedef volatile s8 vs8;
typedef volatile u16 vu16;
typedef volatile s16 vs16;
typedef volatile u32 vu32;
typedef volatile s32 vs32;
typedef volatile f32 vf32;
typedef volatile u64 vu64;
typedef volatile s64 vs64;
typedef volatile f64 vf64;

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define disable_interrupts() ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("rlwinm %0,%0,0,17,15" : "+r" (msr)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
})

#define enable_interrupts() ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("ori %0,%0,0x8000" : "+r" (msr)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
})

#define restore_interrupts(enable) ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("insrwi %0,%1,1,16" : "+r" (msr) : "r" (enable)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
})

#define disable_breakpoint() ({ \
	unsigned long dabr; \
	asm volatile ("mfdabr %0" : "=r" (dabr)); \
	asm volatile ("clrrwi %0,%0,2" : "+r" (dabr)); \
	asm volatile ("mtdabr %0" :: "r" (dabr)); \
})

#define enable_breakpoint() ({ \
	unsigned long dabr; \
	asm volatile ("mfdabr %0" : "=r" (dabr)); \
	asm volatile ("ori %0,%0,1" : "+r" (dabr)); \
	asm volatile ("mtdabr %0" :: "r" (dabr)); \
})

#define mftb(rval) ({unsigned long u; do { \
	 asm volatile ("mftbu %0" : "=r" (u)); \
	 asm volatile ("mftb %0" : "=r" ((rval)->l)); \
	 asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
	 } while(u != ((rval)->u)); })
	 
typedef struct {
	unsigned long l, u;
} tb_t;

#define TB_CLOCK  40500000

static u32(*const DI_EMU)[9] = (u32(*)[])VAR_DI_REGS;

extern volatile u32 PI[13];
extern volatile u16 MI[46];
extern volatile u32 DSP[15];
extern volatile u32 DI[10];
extern volatile u32 EXI[3][5];
extern volatile u32 AI[4];

void do_read_disc(void *dst, u32 len, u32 offset, u32 sector);
u32 do_read(void *dst, u32 len, u32 offset, u32 sector);
void end_read(void);
void read_disc_frag(void *dst, u32 len, u32 offset);
u32 read_frag(void *dst, u32 len, u32 offset);
int is_frag_read(unsigned int offset, unsigned int len);
void device_frag_read(void* dst, u32 len, u32 offset);
void device_reset(void);
int switch_fiber(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 pc, u32 newsp);
void dcache_flush_icache_inv(void* dst, u32 len);
void dcache_store(void* dst, u32 len);
void ADPResetFilter(void);
void ADPDecodeBlock(unsigned char *input, short (*out)[2]);
void ADPdecodebuffer(unsigned char *input, short *outl, short * outr, long *histl1, long *histl2, long *histr1, long *histr2);
unsigned long tb_diff_usec(tb_t* end, tb_t* start);

int usb_sendbuffer_safe(const void *buffer,int size);

#endif
