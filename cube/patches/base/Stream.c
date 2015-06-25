/*
Stream.c for Nintendont (Kernel)

Copyright (C) 2014 FIX94
Swiss specific changes by emu_kidid

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed short s16;

#define ONE_BLOCK_SIZE 32
#define SAMPLES_PER_BLOCK  28

void device_frag_read(void *dst, u32 len, u32 offset);
void WritePCM48to32(u32 *buf_loc);
void WritePCM48(u32 *buf_loc);

#define AI_CR 0xCC006C00
#define AI_32K (1<<6)

void StreamPrepare()
{
	*(u32*)VAR_AS_HIST_0 = 0;
	*(u32*)VAR_AS_HIST_1 = 0;
	*(u32*)VAR_AS_HIST_2 = 0;
	*(u32*)VAR_AS_HIST_3 = 0;
	*(u8*)VAR_AS_SAMPLECNT = 0;
}

static inline void write16INC(u32 buf, u32 *loc, s16 val)
{
	*(s16*)(buf+(*loc)) = val;
	*loc += 2;
}
static inline void SaveSamples(const u32 j, s16 *bufl, s16 *bufr)
{
	*(s16*)VAR_AS_TMP_LSAMPLE = bufl[j];
	*(s16*)VAR_AS_TMP_RSAMPLE = bufr[j];
}
static inline void CombineSamples(const u32 j, s16 *bufl, s16 *bufr)
{	/* average samples to keep quality about the same */
	bufl[j] = (bufl[j] + *(s16*)VAR_AS_TMP_LSAMPLE)>>1;
	bufr[j] = (bufr[j] + *(s16*)VAR_AS_TMP_RSAMPLE)>>1;
}
void StreamUpdate()
{
	u32 buf_loc = 0;
	u32 i;
	u32 r32k = ((*(u32*)(AI_CR)) & AI_32K);
	for(i = 0; i < (r32k ? CHUNK_48to32 : CHUNK_48); i += ONE_BLOCK_SIZE)
	{
		ADPdecodebuffer((u8*)DECODE_WORK_AREA+i,(s16*)VAR_AS_OUTL,(s16*)VAR_AS_OUTR,(u32*)VAR_AS_HIST_0,(u32*)VAR_AS_HIST_1,(u32*)VAR_AS_HIST_2,(u32*)VAR_AS_HIST_3);
		if(r32k) WritePCM48to32(&buf_loc); else WritePCM48(&buf_loc);
	}
	*(u8*)VAR_STREAM_CURBUF = *(u8*)VAR_STREAM_CURBUF ^ 1;
}

void WritePCM48to32(u32 *buf_loc)
{
	u32 CurBuf = (*(u8*)VAR_STREAM_CURBUF == 0) ? DECODED_BUFFER_0 : DECODED_BUFFER_1;
	unsigned int j;
	for(j = 0; j < SAMPLES_PER_BLOCK; j++)
	{
		*(u8*)VAR_AS_SAMPLECNT = *(u8*)VAR_AS_SAMPLECNT+1;
		if(*(u8*)VAR_AS_SAMPLECNT == 2)
		{
			SaveSamples(j, (s16*)VAR_AS_OUTL, (s16*)VAR_AS_OUTR);
			continue;
		}
		if(*(u8*)VAR_AS_SAMPLECNT == 3)
		{
			CombineSamples(j, (s16*)VAR_AS_OUTL, (s16*)VAR_AS_OUTR);
			*(u8*)VAR_AS_SAMPLECNT = 0;
		}
		write16INC(CurBuf, buf_loc, *(((s16*)VAR_AS_OUTL)+j));
		write16INC(CurBuf, buf_loc, *(((s16*)VAR_AS_OUTR)+j));
	}
}

void WritePCM48(u32 *buf_loc)
{
	u32 CurBuf = (*(u8*)VAR_STREAM_CURBUF == 0) ? DECODED_BUFFER_0 : DECODED_BUFFER_1;
	unsigned int j;
	for(j = 0; j < SAMPLES_PER_BLOCK; j++)
	{
		write16INC(CurBuf, buf_loc, *(((s16*)VAR_AS_OUTL)+j));
		write16INC(CurBuf, buf_loc, *(((s16*)VAR_AS_OUTR)+j));
	}
}

u32 StreamGetChunkSize()
{
	return ((*(u32*)(AI_CR)) & AI_32K) ? CHUNK_48to32 : CHUNK_48;
}

void StreamUpdateRegisters()
{
	if(*(u32*)VAR_AS_ENABLED && *(u8*)VAR_STREAM_UPDATE)		//decoder update, it WILL update
	{
		if(*(u8*)VAR_STREAM_ENDING)
		{
			*(u32*)VAR_STREAM_CUR = 0;
			*(u8*)VAR_STREAM_ENDING = 0;
		}
		else if(*(u32*)VAR_STREAM_CUR > 0)
		{
			device_frag_read((u8*)DECODE_WORK_AREA, StreamGetChunkSize(), *(u32*)VAR_STREAM_CUR);
			*(u32*)VAR_STREAM_CUR += StreamGetChunkSize();
			if(*(u32*)VAR_STREAM_CUR >= *(u32*)VAR_STREAM_END)
			{
				if(*(u8*)VAR_STREAM_LOOPING == 1)
				{
					*(u32*)VAR_STREAM_CUR = *(u32*)VAR_STREAM_START;
					StreamPrepare();
				}
				else
					*(u8*)VAR_STREAM_ENDING = 1;
			}
			StreamUpdate();
		}
		*(u8*)VAR_STREAM_UPDATE = 0; //clear status
	}
}

void StreamStartStream(u32 CurrentStart, u32 CurrentSize)
{
	if(CurrentStart > 0 && CurrentSize > 0)
	{
		*(u32*)VAR_STREAM_SIZE = CurrentSize;
		*(u32*)VAR_STREAM_START = CurrentStart;
		u32 StreamCurrent = CurrentStart;
		*(u32*)VAR_STREAM_END = CurrentStart + CurrentSize;
		StreamPrepare();
		device_frag_read((u8*)DECODE_WORK_AREA, StreamGetChunkSize(), StreamCurrent);
		StreamCurrent += StreamGetChunkSize();
		*(u8*)VAR_STREAM_CURBUF = 0; //reset adp buffer
		StreamUpdate();
		/* Directly read in the second buffer */
		device_frag_read((u8*)DECODE_WORK_AREA, StreamGetChunkSize(), StreamCurrent);
		StreamCurrent += StreamGetChunkSize();
		StreamUpdate();
		/* Send stream signal to PPC */
		*(u32*)VAR_STREAM_BUFLOC = 0; //reset adp read pos
		*(u8*)VAR_STREAM_UPDATE = 0; //clear status
		//dbgprintf("Streaming %08x %08x\n", StreamStart, StreamSize);
		*(u8*)VAR_STREAM_LOOPING = 1;
		*(u8*)VAR_STREAM_ENDING = 0;
		*(u32*)VAR_STREAM_CUR = StreamCurrent; //actually start
	}
	else //both offset and length=0 disables loop
		*(u8*)VAR_STREAM_LOOPING = 0;
}

void StreamEndStream()
{
	*(u32*)VAR_STREAM_CUR = 0;
}
