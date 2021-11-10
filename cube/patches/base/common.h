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

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define assert_interrupt() *(volatile u32 *)0xC4000000

#define disable_interrupts() ({ \
	unsigned long msr, level; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm ("extrwi %0,%1,1,16" : "=r" (level) : "r" (msr)); \
	asm volatile ("rlwinm %0,%0,0,17,15" : "+r" (msr)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
	level; \
})

#define enable_interrupts() ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("ori %0,%0,0x8000" : "+r" (msr)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
})

#define enable_interrupts_idle() ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("ori %0,%0,0x8000" : "+r" (msr)); \
	asm volatile ("oris %0,%0,0x0004" : "+r" (msr)); \
	asm volatile ("sync"); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
	asm volatile ("isync"); \
})

#define restore_interrupts(level) ({ \
	unsigned long msr; \
	asm volatile ("mfmsr %0" : "=r" (msr)); \
	asm volatile ("insrwi %0,%1,1,16" : "+r" (msr) : "r" (level)); \
	asm volatile ("mtmsr %0" :: "r" (msr)); \
})

extern volatile u16 PE[24];
extern volatile u32 PI[13];
extern volatile u16 MI[46];
extern volatile u32 DSP[15];
extern volatile u32 DI[10];
extern volatile u32 EXI[3][5];
extern volatile u32 AI[4];

extern volatile u32 EXIEmuRegs[3][5];

#endif
