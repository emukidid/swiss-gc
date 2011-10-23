/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogc/exi.h>
#include <exi.h>

static void* exi_last_addr;
static int exi_last_len;

void exi_select(int channel, int device, int freq)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	long d;
	d = exi[channel * 5];
	d &= 0x405;
	d |= ((1<<device)<<7) | (freq << 4);
	exi[channel * 5] = d;
}

void exi_deselect(int channel)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[channel * 5] &= 0x405;
}

static void* exi_last_addr;
static int exi_last_len;

/* mode?Read:Write len bytes to/from channel */
/* when read, data will be written back in exi_sync */
inline void exi_imm(int channel, void* data, int len, int mode, int zero)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	if (mode == EXI_WRITE)
		exi[channel * 5 + 4] = *(unsigned long*)data;
	else
		exi[channel * 5 + 4] = -1;

	exi[channel * 5 + 3] = ((len - 1) << 4) | (mode << 2) | 1;
	if (mode == EXI_READ)
	{
		exi_last_addr = data;
		exi_last_len = len;
	}
	else
	{
		exi_last_addr = 0;
		exi_last_len = 0;
	}
}

/* Wait until transfer is done, write back data */
void exi_sync(int channel)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	while (exi[channel * 5 + 3] & 1);

	if (exi_last_addr)
	{
		int i;
		unsigned long d;
		d = exi[channel * 5 + 4];
		for (i = 0; i < exi_last_len; ++i)
			((unsigned char*)exi_last_addr)[i] = (d >> ((3 - i) * 8)) & 0xFF;
	}
}

/* simple wrapper for transfers > 4bytes */
void exi_imm_ex(int channel, void* data, int len, int mode)
{
	unsigned char* d = (unsigned char*)data;
	while (len)
	{
		int tc = len;
		if (tc > 4)
			tc = 4;
		exi_imm(channel, d, tc, mode, 0);
		exi_sync(channel);
		len -= tc;
		d += tc;
	}
}

void exi_read(int channel, void* data, int len)
{
	exi_imm_ex(channel, data, len, EXI_READ);
}

void exi_write(int channel, void* data, int len)
{
	exi_imm_ex(channel, data, len, EXI_WRITE);
}

void ipl_set_config(unsigned char c)
{
	unsigned long val,addr;
	addr=0xc0000000;
	val = c << 24;
	exi_select(0, 1, 3);
	exi_write(0, &addr, 4);
	exi_write(0, &val, 4);
	exi_deselect(0);
}

int exi_bba_exists()
{
	u32 cid = 0;
	EXI_GetID(EXI_CHANNEL_0,EXI_DEVICE_2,&cid);

	if(cid==0x04020200) {
		return 1;
	}
	return 0;
}
