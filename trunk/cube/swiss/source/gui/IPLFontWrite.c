/* -----------------------------------------------------------
      IPLFontWrite.c - Font blitter for IPL ROM fonts
	      - by emu_kidid
	   
      Version 1.0 11/11/2009
        - Initial Code
   ----------------------------------------------------------- */

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <string.h>
#include "IPLFontWrite.h"

extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);
extern void usleep(int s);

#define FONT_TEX_SIZE_I4 ((512*512)>>1)
#define FONT_SIZE_ANSI (288 + 131072)
//#define STRHEIGHT_OFFSET 6
#define STRHEIGHT_OFFSET 0

typedef struct {
	u16 s[256], t[256], font_size[256], fheight;
} CHAR_INFO;

static unsigned char fontFont[ 0x40000 ] __attribute__((aligned(32)));

u16 frameWidth;
CHAR_INFO fontChars;
GXTexObj fontTexObj;
GXColor defaultColor = (GXColor) {255,255,255,255};
GXColor fontColor = (GXColor) {255,255,255,255};

/****************************************************************************
 * YAY0 Decoding
 ****************************************************************************/
/* Yay0 decompression */
void decodeYay0(unsigned char *s, unsigned char *d)
{
	int i, j, k, p, q, cnt;

	i = *(unsigned long *)(s + 4);	  // size of decoded data
	j = *(unsigned long *)(s + 8);	  // link table
	k = *(unsigned long *)(s + 12);	 // byte chunks and count modifiers

	q = 0;					// current offset in dest buffer
	cnt = 0;				// mask bit counter
	p = 16;					// current offset in mask table

	unsigned long r22 = 0, r5;
	
	do
	{
		// if all bits are done, get next mask
		if(cnt == 0)
		{
			// read word from mask data block
			r22 = *(unsigned long *)(s + p);
			p += 4;
			cnt = 32;   // bit counter
		}
		// if next bit is set, chunk is non-linked
		if(r22 & 0x80000000)
		{
			// get next byte
			*(unsigned char *)(d + q) = *(unsigned char *)(s + k);
			k++, q++;
		}
		// do copy, otherwise
		else
		{
			// read 16-bit from link table
			int r26 = *(unsigned short *)(s + j);
			j += 2;
			// 'offset'
			int r25 = q - (r26 & 0xfff);
			// 'count'
			int r30 = r26 >> 12;
			if(r30 == 0)
			{
				// get 'count' modifier
				r5 = *(unsigned char *)(s + k);
				k++;
				r30 = r5 + 18;
			}
			else r30 += 2;
			// do block copy
			unsigned char *pt = ((unsigned char*)d) + r25;
			int i;
			for(i=0; i<r30; i++)
			{
				*(unsigned char *)(d + q) = *(unsigned char *)(pt - 1);
				q++, pt++;
			}
		}
		// next bit in mask
		r22 <<= 1;
		cnt--;

	} while(q < i);
}

void convertI2toI4(void *dst, void *src, int xres, int yres)
{
	// I4 has 8x8 tiles
	int x, y;
	unsigned char *d = (unsigned char*)dst;
	unsigned char *s = (unsigned char*)src;

	for (y = 0; y < yres; y += 8)
		for (x = 0; x < xres; x += 8)
		{
			int iy, ix;
			for (iy = 0; iy < 8; ++iy, s+=2)
			{
				for (ix = 0; ix < 2; ++ix)
				{
					int v = s[ix];
					*d++ = (((v>>6)&3)<<6) | (((v>>6)&3)<<4) | (((v>>4)&3)<<2) | ((v>>4)&3);
					*d++ = (((v>>2)&3)<<6) | (((v>>2)&3)<<4) | (((v)&3)<<2) | ((v)&3);
				}
			}
		}
}

void init_font(void)
{
	void* fontArea = memalign(32,FONT_SIZE_ANSI);
	memset(fontArea,0,FONT_SIZE_ANSI);
	void* packed_data = (void*)(((u32)fontArea+119072)&~31);
	void* unpacked_data = (void*)((u32)fontArea+288);
	__SYS_ReadROM(packed_data,0x3000,0x1FCF00);
	decodeYay0(packed_data,unpacked_data);

	sys_fontheader *fontData = (sys_fontheader*)unpacked_data;

	convertI2toI4((void*)fontFont, (void*)((u32)unpacked_data+fontData->sheet_image), fontData->sheet_width, fontData->sheet_height);
	DCFlushRange(fontFont, FONT_TEX_SIZE_I4);

	int i;
	for (i=0; i<256; ++i)
	{
		int c = i;

		if ((c < fontData->first_char) || (c > fontData->last_char)) c = fontData->inval_char;
		else c -= fontData->first_char;

		fontChars.font_size[i] = ((unsigned char*)fontData)[fontData->width_table + c];

		int r = c / fontData->sheet_column;
		c %= fontData->sheet_column;

		fontChars.s[i] = c * fontData->cell_width;
		fontChars.t[i] = r * fontData->cell_height;
	}
	
	fontChars.fheight = fontData->cell_height;

	free(fontArea);
}

void drawFontInit(GXColor fontColor)
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	// Reset various parameters from gfx plugin
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_ENABLE);
//	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
//	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
/*	if(screenMode && menuActive)
		guOrtho(GXprojection2D, 0, 479, -104, 743, 0, 700);
	else if(screenMode == SCREENMODE_16x9_PILLARBOX)
		guOrtho(GXprojection2D, 0, 479, -104, 743, 0, 700);
	else
		guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 700);*/
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 700);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);
//	GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);

	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_TRUE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_U16, 7);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
//	GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_InvalidateTexAll();
	GX_InitTexObj(&fontTexObj, &fontFont[0], 512, 512, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);

	GX_SetTevColor(GX_TEVREG1,fontColor);
//	GX_SetTevKColor(GX_KCOLOR0, fontColor);
//	GX_SetTevKColorSel(GX_TEVSTAGE0,GX_TEV_KCSEL_K0);
//	GX_SetTevKAlphaSel(GX_TEVSTAGE0,GX_TEV_KCSEL_K0_A);

	GX_SetNumTevStages (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
//	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_KONST, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A1, GX_CA_TEXA, GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
//	GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_RED);
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
//	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP1);

	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
//	GX_SetAlphaUpdate(GX_ENABLE);
//	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);
}

void drawString(int x, int y, char *string, float scale, bool centered)
{
	if(centered)
	{
		int strWidth = 0;
		int strHeight = (fontChars.fheight+STRHEIGHT_OFFSET) * scale;
		char* string_work = string;
//		while(*string_work && (x < back_framewidth))
		while(*string_work)
		{
			unsigned char c = *string_work;
			strWidth += (int) fontChars.font_size[c] * scale;
			string_work++;
		}
//		x0 = (int) MAX(0, (back_framewidth - x)/2);
//		x = (int) (frameWidth - x)/2;
		x = (int) x - strWidth/2;
		y = (int) y - strHeight/2;
//		write_font(x0,y,string,scale);
	}

//	int ox = x;
	
//	while (*string && (x < (ox + back_framewidth)))
	while (*string)
	{
		//blit_char(axfb,whichfb,x, y, *string, norm_blit ? blit_lookup_norm : blit_lookup);
		unsigned char c = *string;
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? fontChars.font_size[c] : 1;
			int t = (i & 2) ? fontChars.fheight : 1;
			float s0 = ((float) (fontChars.s[c] + s))/512;
			float t0 = ((float) (fontChars.t[c] + t))/512;
			s = (int) s * scale;
			t = (int) t * scale;
			GX_Position3s16(x + s, y + t, 0);
			GX_Color4u8(fontColor.r, fontColor.g, fontColor.b, fontColor.a);
//			GX_Color4u8(fontState->colour.r, fontState->colour.g, fontState->colour.b, fontState->colour.a);
//			GX_TexCoord2f32(((float) (fontChars.s[c] + s))/512, ((float) (fontChars.t[c] + t))/512);
//			GX_TexCoord2u16(fontChars.s[c] + s, fontChars.t[c] + t);
			GX_TexCoord2f32(s0, t0);
		}
		GX_End();

		x += (int) fontChars.font_size[c] * scale;
		string++;
	}

}

void WriteFontStyled(int x, int y, char *string, float size, bool centered, GXColor color)
{
	fontColor = color;
	drawFontInit(fontColor);
	drawString(x, y, string, size, centered);
}

void WriteFont(int x, int y, char *string)
{
	WriteFontStyled(x, y, string, 1.0f, false, (GXColor) {255, 255, 255, 255});
	fontColor = (GXColor) {255, 255, 255, 255};
	drawFontInit(fontColor);
	drawString(x, y, string, 1.0f, false);
}

int GetTextSizeInPixels(char *string)
{
	int strWidth = 0;
	float scale = 1.0f;
	char* string_work = string;
	while(*string_work)
	{
		unsigned char c = *string_work;
		strWidth += (int) fontChars.font_size[c] * scale;
		string_work++;
	}
	return strWidth;
}
