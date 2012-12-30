/*
*
*   Swiss - The Gamecube IPL replacement
*
*	frag.c
*		- Wrap normal read requests around a fragmented file table
*/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern void dcache_flush_icache_inv(void*, u32);
extern void do_read(void*, u32, u32);

void perform_read(void *dst, u32 len, u32 offset)
{
	do_read(dst, len, offset);
	dcache_flush_icache_inv(dst, len);
}