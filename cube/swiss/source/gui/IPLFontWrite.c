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

static u8 fontData[SYS_FONTSIZE_ANSI] ATTRIBUTE_ALIGN (32);
static sys_fontheader *font = (sys_fontheader *)fontData;

GXTexObj fontTexObj;
GXColor defaultColor = (GXColor) {255,255,255,255};
GXColor disabledColor = (GXColor) {175,175,182,255};
GXColor deSelectedColor = (GXColor) {80,80,73,255};

void init_font(void)
{
	SYS_SetFontEncoding(SYS_FONTENC_ANSI);
	if(SYS_InitFont(font)) {
		GX_InitTexObj(&fontTexObj, NULL, font->sheet_width, font->sheet_height, font->sheet_format, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObjLOD(&fontTexObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_TRUE, GX_TRUE, GX_ANISO_4);
	}
}

void drawFontInit(void)
{
	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	// Reset various parameters from gfx plugin
	GX_SetCoPlanar(GX_DISABLE);
	GX_SetClipMode(GX_CLIP_ENABLE);
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadTexMtxImm(GXmodelView2D,GX_TEXMTX0,GX_MTX2x4);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 1);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_ENABLE, 1, 1);

	GX_InvalidateTexAll();
	GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);

	GX_SetNumIndStages (0);
	GX_SetNumTevStages (2);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); // change to (u8) tile later
	GX_SetTevColorIn (GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
	GX_SetTevAlphaOp (GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevDirect (GX_TEVSTAGE0);
	GX_SetTevOrder (GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevColorIn (GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_RASA, GX_CC_ZERO);
	GX_SetTevColorOp (GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevAlphaIn (GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
	GX_SetTevAlphaOp (GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV);
	GX_SetTevDirect (GX_TEVSTAGE1);

	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);
}

void drawString(int x, int y, char *string, float scale, bool centered, GXColor fontColor)
{
	if(string == NULL) {
		return;
	}
	drawFontInit();
	Mtx GXmodelView2D;
	int strWidth = 0;
	int strHeight = font->cell_height;
	if(centered)
	{
		char* string_work = string;
		while(*string_work)
		{
			unsigned char c = *string_work;
			strWidth += SYS_GetFontWidth(c) - 1;
			string_work++;
		}
	}
	guMtxTrans(GXmodelView2D, -strWidth/2, -strHeight/2, 0);
	guMtxScaleApply(GXmodelView2D, GXmodelView2D, scale, scale, 1);
	guMtxTransApply(GXmodelView2D, GXmodelView2D, x, y, 0);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	x = 0; y = 0;

	while (*string)
	{
		unsigned char c = *string;
		if(c == '\n') break;
		void *image;
		int s0, t0, width;
		SYS_GetFontTexture(c, &image, &s0, &t0, &width);
		if (GX_GetTexObjData(&fontTexObj) != (void *)MEM_VIRTUAL_TO_PHYSICAL(image)) {
			GX_InitTexObjData(&fontTexObj, image);
			GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);
		}
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? width : 1;
			int t = (i & 2) ? font->cell_height : 1;
			GX_Position2s16(x + s, y + t);
			GX_Color4u8(fontColor.r, fontColor.g, fontColor.b, fontColor.a);
			GX_TexCoord2s16(s0 + s, t0 + t);
		}
		GX_End();

		x += width - 1;
		string++;
	}
}

void drawStringWithCaret(int x, int y, char *string, float scale, bool centered, GXColor fontColor, int caretPosition, GXColor caretColor)
{
	if(string == NULL) {
		string = "";
	}
	drawFontInit();
	Mtx GXmodelView2D;
	int strWidth = 0;
	int strHeight = font->cell_height;
	if(centered)
	{
		char* string_work = string;
		while(*string_work)
		{
			unsigned char c = *string_work;
			strWidth += SYS_GetFontWidth(c) - 1;
			string_work++;
		}
	}
	guMtxTrans(GXmodelView2D, -strWidth/2, -strHeight/2, 0);
	guMtxScaleApply(GXmodelView2D, GXmodelView2D, scale, scale, 1);
	guMtxTransApply(GXmodelView2D, GXmodelView2D, x, y, 0);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	x = 0; y = 0;

	int pos = 0;
	while (*string || pos <= caretPosition)
	{
		unsigned char c = *string;
		if(pos == caretPosition) { 
			c = '|';
		}
		if(c == '\n') break;
		void *image;
		int s0, t0, width;
		SYS_GetFontTexture(c, &image, &s0, &t0, &width);
		if (GX_GetTexObjData(&fontTexObj) != (void *)MEM_VIRTUAL_TO_PHYSICAL(image)) {
			GX_InitTexObjData(&fontTexObj, image);
			GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);
		}
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? width : 1;
			int t = (i & 2) ? font->cell_height : 1;
			GX_Position2s16(x + s, y + t);
			if(pos == caretPosition)
				GX_Color4u8(caretColor.r, caretColor.g, caretColor.b, caretColor.a);
			else
				GX_Color4u8(fontColor.r, fontColor.g, fontColor.b, fontColor.a);
			GX_TexCoord2s16(s0 + s, t0 + t);
		}
		GX_End();

		x += width - 1;
		if(pos != caretPosition)
			string++;
		pos++;
	}
}

int GetCharsThatFitInWidth(char *string, int max, float scale)
{
	int strWidth = 0;
	int charCount = 0;
	char* string_work = string;
	//print_debug("String [%s] max %i\n", string, max);
	while(*string_work)
	{
		unsigned char c = *string_work;
		strWidth += SYS_GetFontWidth(c) - 1;
		string_work++;
		if(strWidth * scale <= max) {
			charCount++;
		} else {
			return charCount-3;
		}
	}
	return charCount;
}

// maxSize is how far we can draw, abbreviate with "..." if we're going to exceed it.
void drawStringEllipsis(int x, int y, char *string, float scale, bool centered, GXColor fontColor, bool rotateVertical, int maxSize)
{
	if(string == NULL) {
		return;
	}
	drawFontInit();
	Mtx GXmodelView2D;
	if(rotateVertical) {
		guMtxRotDeg(GXmodelView2D, 'z', -90);
	} else {
		guMtxIdentity(GXmodelView2D);
	}
	int strWidth = 0;
	int strHeight = font->cell_height;
	if(centered)
	{
		char* string_work = string;
		while(*string_work)
		{
			unsigned char c = *string_work;
			strWidth += SYS_GetFontWidth(c) - 1;
			string_work++;
		}
	}
	guMtxApplyTrans(GXmodelView2D, GXmodelView2D, -strWidth/2, -strHeight/2, 0);
	guMtxScaleApply(GXmodelView2D, GXmodelView2D, scale, scale, 1);
	guMtxTransApply(GXmodelView2D, GXmodelView2D, x, y, 0);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
	x = 0; y = 0;

	int len = strlen(string);
	int chars_to_draw = GetCharsThatFitInWidth(string, maxSize, scale);
	int dots_to_write = 0;
	while (*string || dots_to_write)
	{
		unsigned char c = *string;
		if(dots_to_write) {
			c = '.';
			dots_to_write --;
			if(!dots_to_write) {
				break;
			}
		}
		if(c == '\n') break;
		void *image;
		int s0, t0, width;
		SYS_GetFontTexture(c, &image, &s0, &t0, &width);
		if (GX_GetTexObjData(&fontTexObj) != (void *)MEM_VIRTUAL_TO_PHYSICAL(image)) {
			GX_InitTexObjData(&fontTexObj, image);
			GX_LoadTexObj(&fontTexObj, GX_TEXMAP0);
		}
		int i;
		GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		for (i=0; i<4; i++) {
			int s = (i & 1) ^ ((i & 2) >> 1) ? width : 1;
			int t = (i & 2) ? font->cell_height : 1;
			GX_Position2s16(x + s, y + t);
			GX_Color4u8(fontColor.r, fontColor.g, fontColor.b, fontColor.a);
			GX_TexCoord2s16(s0 + s, t0 + t);
		}
		GX_End();

		x += width - 1;
		string++;
		len--;
		chars_to_draw--;
		
		// check if we've started (or about to start) our ellipses abbreviation
		if(len > 0 && chars_to_draw == 0) {
			if(dots_to_write == 0) {
				dots_to_write = 4;
			}
		}
	}
}

int GetFontHeight(float scale)
{
	int strHeight = font->cell_height * scale;
	return strHeight;
}

int GetTextSizeInPixels(char *string)
{
	int strWidth = 0;
	char* string_work = string;
	while(*string_work)
	{
		unsigned char c = *string_work;
		strWidth += SYS_GetFontWidth(c) - 1;
		string_work++;
	}
	return strWidth;
}

float GetTextScaleToFitInWidth(char *string, int width) {
	int strWidth = 0;
	char* string_work = string;
	while(*string_work)
	{
		unsigned char c = *string_work;
		if(c == '\n') break;
		strWidth += SYS_GetFontWidth(c) - 1;
		string_work++;
	}
	return width>strWidth ? 1.0f : (float)((float)width/(float)strWidth);
}

float GetTextScaleToFitInWidthWithMax(char *string, int width, float max) {
	float scale = GetTextScaleToFitInWidth(string, width);
	return max < scale ? max:scale;
}
