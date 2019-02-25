#ifndef _COMMON_H_

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef long s32;
typedef volatile u8 vu8;
typedef volatile s8 vs8;
typedef volatile u16 vu16;
typedef volatile s16 vs16;
typedef volatile u32 vu32;
typedef volatile s32 vs32;

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

#define mftb(rval) ({unsigned long u; do { \
	 asm volatile ("mftbu %0" : "=r" (u)); \
	 asm volatile ("mftb %0" : "=r" ((rval)->l)); \
	 asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
	 } while(u != ((rval)->u)); })
	 
typedef struct {
	unsigned long l, u;
} tb_t;

#define TB_CLOCK  40500000

static vu32(*const DI)[10] = (vu32(*)[])0xCC006000;
static vu32(*const EXI)[5] = (vu32(*)[])0xCC006800;

u32 do_read(void *dst, u32 len, u32 offset, u32 sector);
u32 read_frag(void *dst, u32 len, u32 offset);
int is_frag_read(unsigned int offset, unsigned int len);
void device_frag_read(void* dst, u32 len, u32 offset);
void dcache_flush_icache_inv(void* dst, u32 len);
void ADPdecodebuffer(unsigned char *input, short *outl, short * outr, long *histl1, long *histl2, long *histr1, long *histr2);
void StreamStartStream(u32 CurrentStart, u32 CurrentSize);
void StreamEndStream(void);
u32 process_queue(void* dst, u32 len, u32 offset, int readComplete);
unsigned long tb_diff_usec(tb_t* end, tb_t* start);

int usb_sendbuffer_safe(const void *buffer,int size);


#endif
