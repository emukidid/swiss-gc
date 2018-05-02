/* -----------------------------------------------------------
      FrameBufferMagic.c - Framebuffer routines with GX
	      - by emu_kidid & sepp256

      Version 1.0 11/11/2009
        - Initial Code
   ----------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/exi.h>
#include <gctypes.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"
#include "btns.h"
#include "dolparameters.h"
#include "cheats.h"

#define GUI_MSGBOX_ALPHA 200

TPLFile backdropTPL;
GXTexObj backdropTexObj;
TPLFile gcdvdsmallTPL;
GXTexObj gcdvdsmallTexObj;
TPLFile sdsmallTPL;
GXTexObj sdsmallTexObj;
TPLFile hddTPL;
GXTexObj hddTexObj;
TPLFile qoobTPL;
GXTexObj qoobTexObj;
TPLFile wodeimgTPL;
GXTexObj wodeimgTexObj;
TPLFile usbgeckoTPL;
GXTexObj usbgeckoTexObj;
TPLFile memcardTPL;
GXTexObj memcardTexObj;
TPLFile sambaTPL;
GXTexObj sambaTexObj;
TPLFile wiikeyTPL;
GXTexObj wiikeyTexObj;
TPLFile systemTPL;
GXTexObj systemTexObj;
TPLFile btnnohilightTPL;
GXTexObj btnnohilightTexObj;
TPLFile btnhilightTPL;
GXTexObj btnhilightTexObj;
TPLFile btndeviceTPL;
GXTexObj btndeviceTexObj;
TPLFile btnsettingsTPL;
GXTexObj btnsettingsTexObj;
TPLFile btninfoTPL;
GXTexObj btninfoTexObj;
TPLFile btnrefreshTPL;
GXTexObj btnrefreshTexObj;
TPLFile btnexitTPL;
GXTexObj btnexitTexObj;
TPLFile boxinnerTPL;
GXTexObj boxinnerTexObj;
TPLFile boxouterTPL;
GXTexObj boxouterTexObj;
TPLFile ntscjTPL;
GXTexObj ntscjTexObj;
TPLFile ntscuTPL;
GXTexObj ntscuTexObj;
TPLFile palTPL;
GXTexObj palTexObj;
TPLFile checkedTPL;
GXTexObj checkedTexObj;
TPLFile uncheckedTPL;
GXTexObj uncheckedTexObj;

void init_textures() 
{
	TPL_OpenTPLFromMemory(&backdropTPL, (void *)backdrop_tpl, backdrop_tpl_size);
	TPL_GetTexture(&backdropTPL,backdrop,&backdropTexObj);
	TPL_OpenTPLFromMemory(&gcdvdsmallTPL, (void *)gcdvdsmall_tpl, gcdvdsmall_tpl_size);
	TPL_GetTexture(&gcdvdsmallTPL,gcdvdsmall,&gcdvdsmallTexObj);
	TPL_OpenTPLFromMemory(&sdsmallTPL, (void *)sdsmall_tpl, sdsmall_tpl_size);
	TPL_GetTexture(&sdsmallTPL,sdsmall,&sdsmallTexObj);
	TPL_OpenTPLFromMemory(&hddTPL, (void *)hdd_tpl, hdd_tpl_size);
	TPL_GetTexture(&hddTPL,hdd,&hddTexObj);
	TPL_OpenTPLFromMemory(&qoobTPL, (void *)qoob_tpl, qoob_tpl_size);
	TPL_GetTexture(&qoobTPL,qoob,&qoobTexObj);
	TPL_OpenTPLFromMemory(&wodeimgTPL, (void *)wodeimg_tpl, wodeimg_tpl_size);
	TPL_GetTexture(&wodeimgTPL,wodeimg,&wodeimgTexObj);
	TPL_OpenTPLFromMemory(&wiikeyTPL, (void *)wiikeyimg_tpl, wiikeyimg_tpl_size);
	TPL_GetTexture(&wiikeyTPL,wiikeyimg,&wiikeyTexObj);
	TPL_OpenTPLFromMemory(&systemTPL, (void *)systemimg_tpl, systemimg_tpl_size);
	TPL_GetTexture(&systemTPL,systemimg,&systemTexObj);
	TPL_OpenTPLFromMemory(&memcardTPL, (void *)memcardimg_tpl, memcardimg_tpl_size);
	TPL_GetTexture(&memcardTPL,memcardimg,&memcardTexObj);
	TPL_OpenTPLFromMemory(&usbgeckoTPL, (void *)usbgeckoimg_tpl, usbgeckoimg_tpl_size);
	TPL_GetTexture(&usbgeckoTPL,usbgeckoimg,&usbgeckoTexObj);
	TPL_OpenTPLFromMemory(&sambaTPL, (void *)sambaimg_tpl, sambaimg_tpl_size);
	TPL_GetTexture(&sambaTPL,sambaimg,&sambaTexObj);
	TPL_OpenTPLFromMemory(&btnnohilightTPL, (void *)btnnohilight_tpl, btnnohilight_tpl_size);
	TPL_GetTexture(&btnnohilightTPL,btnnohilight,&btnnohilightTexObj);
	TPL_OpenTPLFromMemory(&btnhilightTPL, (void *)btnhilight_tpl, btnhilight_tpl_size);
	TPL_GetTexture(&btnhilightTPL,btnhilight,&btnhilightTexObj);
	TPL_OpenTPLFromMemory(&btndeviceTPL, (void *)btndevice_tpl, btndevice_tpl_size);
	TPL_GetTexture(&btndeviceTPL,btndevice,&btndeviceTexObj);
	TPL_OpenTPLFromMemory(&btnsettingsTPL, (void *)btnsettings_tpl, btnsettings_tpl_size);
	TPL_GetTexture(&btnsettingsTPL,btnsettings,&btnsettingsTexObj);
	TPL_OpenTPLFromMemory(&btninfoTPL, (void *)btninfo_tpl, btninfo_tpl_size);
	TPL_GetTexture(&btninfoTPL,btninfo,&btninfoTexObj);
	TPL_OpenTPLFromMemory(&btnrefreshTPL, (void *)btnrefresh_tpl, btnrefresh_tpl_size);
	TPL_GetTexture(&btnrefreshTPL,btnrefresh,&btnrefreshTexObj);
	TPL_OpenTPLFromMemory(&btnexitTPL, (void *)btnexit_tpl, btnexit_tpl_size);
	TPL_GetTexture(&btnexitTPL,btnexit,&btnexitTexObj);
	TPL_OpenTPLFromMemory(&boxinnerTPL, (void *)boxinner_tpl, boxinner_tpl_size);
	TPL_GetTexture(&boxinnerTPL,boxinner,&boxinnerTexObj);
	GX_InitTexObjWrapMode(&boxinnerTexObj, GX_CLAMP, GX_CLAMP);
	TPL_OpenTPLFromMemory(&boxouterTPL, (void *)boxouter_tpl, boxouter_tpl_size);
	TPL_GetTexture(&boxouterTPL,boxouter,&boxouterTexObj);
	GX_InitTexObjWrapMode(&boxouterTexObj, GX_CLAMP, GX_CLAMP);
	TPL_OpenTPLFromMemory(&ntscjTPL, (void *)ntscj_tpl, ntscj_tpl_size);
	TPL_GetTexture(&ntscjTPL,ntscjimg,&ntscjTexObj);
	TPL_OpenTPLFromMemory(&ntscuTPL, (void *)ntscu_tpl, ntscu_tpl_size);
	TPL_GetTexture(&ntscuTPL,ntscuimg,&ntscuTexObj);
	TPL_OpenTPLFromMemory(&palTPL, (void *)pal_tpl, pal_tpl_size);
	TPL_GetTexture(&palTPL,palimg,&palTexObj);
	TPL_OpenTPLFromMemory(&checkedTPL, (void *)checked_32_tpl, checked_32_tpl_size);
	TPL_GetTexture(&checkedTPL,checked_32,&checkedTexObj);
	TPL_OpenTPLFromMemory(&uncheckedTPL, (void *)unchecked_32_tpl, unchecked_32_tpl_size);
	TPL_GetTexture(&uncheckedTPL,unchecked_32,&uncheckedTexObj);
}

void drawInit()
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
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
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
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//enable textures
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetNumTevStages (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
//	GX_SetAlphaUpdate(GX_ENABLE);
//	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);
}

void drawRect(int x, int y, int width, int height, int depth, GXColor color, float s0, float s1, float t0, float t1)
{
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y,(float) depth );
		GX_Color4u8(color.r, color.g, color.b, color.a);
		GX_TexCoord2f32(s0,t0);
		GX_Position3f32((float) (x+width),(float) y,(float) depth );
		GX_Color4u8(color.r, color.g, color.b, color.a);
		GX_TexCoord2f32(s1,t0);
		GX_Position3f32((float) (x+width),(float) (y+height),(float) depth );
		GX_Color4u8(color.r, color.g, color.b, color.a);
		GX_TexCoord2f32(s1,t1);
		GX_Position3f32((float) x,(float) (y+height),(float) depth );
		GX_Color4u8(color.r, color.g, color.b, color.a);
		GX_TexCoord2f32(s0,t1);
	GX_End();
}

void DrawSimpleBox(int x, int y, int width, int height, int depth, GXColor fillColor, GXColor borderColor) 
{
	//Adjust for blank texture border
	x-=4; y-=4; width+=8; height+=8;
	
	drawInit();
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_InvalidateTexAll();
	GX_LoadTexObj(&boxinnerTexObj, GX_TEXMAP0);

	drawRect(x, y, width/2, height/2, depth, fillColor, 0.0f, ((float)width/32), 0.0f, ((float)height/32));
	drawRect(x+(width/2), y, width/2, height/2, depth, fillColor, ((float)width/32), 0.0f, 0.0f, ((float)height/32));
	drawRect(x, y+(height/2), width/2, height/2, depth, fillColor, 0.0f, ((float)width/32), ((float)height/32), 0.0f);
	drawRect(x+(width/2), y+(height/2), width/2, height/2, depth, fillColor, ((float)width/32), 0.0f, ((float)height/32), 0.0f);

	GX_InvalidateTexAll();
	GX_LoadTexObj(&boxouterTexObj, GX_TEXMAP0);

	drawRect(x, y, width/2, height/2, depth, borderColor, 0.0f, ((float)width/32), 0.0f, ((float)height/32));
	drawRect(x+(width/2), y, width/2, height/2, depth, borderColor, ((float)width/32), 0.0f, 0.0f, ((float)height/32));
	drawRect(x, y+(height/2), width/2, height/2, depth, borderColor, 0.0f, ((float)width/32), ((float)height/32), 0.0f);
	drawRect(x+(width/2), y+(height/2), width/2, height/2, depth, borderColor, ((float)width/32), 0.0f, ((float)height/32), 0.0f);
}

void DrawImage(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered)
{
	drawInit();
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_InvalidateTexAll();

	switch(textureId)
	{
	case TEX_BACKDROP:
		GX_LoadTexObj(&backdropTexObj, GX_TEXMAP0);
		break;
	case TEX_GCDVDSMALL:
		GX_LoadTexObj(&gcdvdsmallTexObj, GX_TEXMAP0);
		break;
	case TEX_SDSMALL:
		GX_LoadTexObj(&sdsmallTexObj, GX_TEXMAP0);
		break;
	case TEX_HDD:
		GX_LoadTexObj(&hddTexObj, GX_TEXMAP0);
		break;
	case TEX_QOOB:
		GX_LoadTexObj(&qoobTexObj, GX_TEXMAP0);
		break;
	case TEX_WODEIMG:
		GX_LoadTexObj(&wodeimgTexObj, GX_TEXMAP0);
		break;
	case TEX_USBGECKO:
		GX_LoadTexObj(&usbgeckoTexObj, GX_TEXMAP0);
		break;
	case TEX_WIIKEY:
		GX_LoadTexObj(&wiikeyTexObj, GX_TEXMAP0);
		break;
	case TEX_SYSTEM:
		GX_LoadTexObj(&systemTexObj, GX_TEXMAP0);
		break;
	case TEX_MEMCARD:
		GX_LoadTexObj(&memcardTexObj, GX_TEXMAP0);
		break;
	case TEX_SAMBA:
		GX_LoadTexObj(&sambaTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNNOHILIGHT:
		GX_LoadTexObj(&btnnohilightTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNHILIGHT:
		GX_LoadTexObj(&btnhilightTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNDEVICE:
		GX_LoadTexObj(&btndeviceTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNSETTINGS:
		GX_LoadTexObj(&btnsettingsTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNINFO:
		GX_LoadTexObj(&btninfoTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNREFRESH:
		GX_LoadTexObj(&btnrefreshTexObj, GX_TEXMAP0);
		break;
	case TEX_BTNEXIT:
		GX_LoadTexObj(&btnexitTexObj, GX_TEXMAP0);
		break;
	case TEX_NTSCJ:
		GX_LoadTexObj(&ntscjTexObj, GX_TEXMAP0);
		break;
	case TEX_NTSCU:
		GX_LoadTexObj(&ntscuTexObj, GX_TEXMAP0);
		break;
	case TEX_PAL:
		GX_LoadTexObj(&palTexObj, GX_TEXMAP0);
		break;
	case TEX_CHECKED:
		GX_LoadTexObj(&checkedTexObj, GX_TEXMAP0);
		break;
	case TEX_UNCHECKED:
		GX_LoadTexObj(&uncheckedTexObj, GX_TEXMAP0);
		break;
	}	

	if(centered)
	{
		x = (int) x - width/2;
	}
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y,(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s1,t1);
		GX_Position3f32((float) (x+width),(float) y,(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s2,t1);
		GX_Position3f32((float) (x+width),(float) (y+height),(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s2,t2);
		GX_Position3f32((float) x,(float) (y+height),(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s1,t2);
	GX_End();
}

void DrawTexObj(GXTexObj *texObj, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered)
{
	drawInit();
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_InvalidateTexAll();
	GX_LoadTexObj(texObj, GX_TEXMAP0);
	if(centered)
		x = (int) x - width/2;
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y,(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s1,t1);
		GX_Position3f32((float) (x+width),(float) y,(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s2,t1);
		GX_Position3f32((float) (x+width),(float) (y+height),(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s2,t2);
		GX_Position3f32((float) x,(float) (y+height),(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s1,t2);
	GX_End();
}

void _DrawBackdrop() 
{
	char ver[32];
	memset(ver,0,16);
	DrawImage(TEX_BACKDROP, 0, 0, 640, 480, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	WriteFontStyled(40,27, "Swiss v0.4", 1.5f, false, defaultColor);
	sprintf(ver, "commit: %s rev: %s", GITREVISION, GITVERSION);
	WriteFontStyled(210,60, ver, 0.55f, false, defaultColor);
}

// Call this when starting a screen
void DrawFrameStart() {
  whichfb ^= 1;
  _DrawBackdrop();
}

// Call this at the end of a screen
void DrawFrameFinish() {
	//Copy EFB->XFB
	GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	GX_Flush();

	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
 	VIDEO_WaitVSync();
}

void DrawProgressBar(int percent, char *message) {
	int x1 = ((640/2) - (PROGRESS_BOX_WIDTH/2));
	int x2 = ((640/2) + (PROGRESS_BOX_WIDTH/2));
	int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
	int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));
	int middleY = (y2+y1)/2;
	float scale = GetTextScaleToFitInWidth(message, x2-x1);
  	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //black
  	GXColor noColor = (GXColor) {0,0,0,0}; //blank
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	GXColor progressBarColor = (GXColor) {255,128,0,GUI_MSGBOX_ALPHA}; //orange
	
	DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, fillColor, borderColor); 
	int multiplier = (PROGRESS_BOX_WIDTH-20)/100;
	int progressBarWidth = multiplier*100;
	DrawSimpleBox( (640/2 - progressBarWidth/2), y1+20,
			(multiplier*100), 20, 0, noColor, borderColor); 
	DrawSimpleBox( (640/2 - progressBarWidth/2), y1+20,
			(multiplier*percent), 20, 0, progressBarColor, noColor); 

	WriteFontStyled(640/2, middleY, message, scale, true, defaultColor);
	sprintf(txtbuffer,"%d%% percent complete",percent);
	WriteFontStyled(640/2, middleY+30, txtbuffer, 1.0f, true, defaultColor);
}

void DrawMessageBox(int type, char *message) 
{
	int x1 = ((640/2) - (PROGRESS_BOX_WIDTH/2));
	int x2 = ((640/2) + (PROGRESS_BOX_WIDTH/2));
	int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
	int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));
	int middleY = y2-y1 < 23 ? y1+3 : (y2+y1)/2-12;
	
  	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, fillColor, borderColor); 

	sprintf(txtbuffer, "%s", message);
	char *tok = strtok(txtbuffer,"\n");
	while(tok != NULL) {
		WriteFontStyled(640/2, middleY, tok, 1.0f, true, defaultColor);
		tok = strtok(NULL,"\n");
		middleY+=24;
	}
}

void DrawRawFont(int x, int y, char *message) {
  WriteFont(x, y, message);
}

void DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode, u32 color) 
{
	int middleY, borderSize;
	color = (color == -1) ? BUTTON_COLOUR_INNER : color; //never used

	borderSize = (mode==B_SELECTED) ? 6 : 4;
	middleY = (((y2-y1)/2)-12)+y1;

	//determine length of the text ourselves if x2 == -1
	x1 = (x2 == -1) ? x1+2:x1;
	x2 = (x2 == -1) ? GetTextSizeInPixels(message)+x1+(borderSize*2)+6 : x2;

	if(middleY+24 > y2) {
		middleY = y1+3;
	}

	float scale = GetTextScaleToFitInWidth(message, (x2-x1)-(borderSize*2));
	GXColor selectColor = (GXColor) {96,107,164,GUI_MSGBOX_ALPHA}; //bluish
	GXColor noColor = (GXColor) {0,0,0,0}; //black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	//Draw Text and backfill (if selected)
	if(mode==B_SELECTED) {
		DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, selectColor, borderColor);
		WriteFontStyled(x1 + borderSize+3, middleY, message, scale, false, defaultColor);
	}
	else {
		DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, noColor, borderColor);
		WriteFontStyled(x1 + borderSize+3, middleY, message, scale, false, defaultColor);
	}
}

void DrawFileBrowserButton(int x1, int y1, int x2, int y2, char *message, file_handle *file, int mode, u32 color) 
{
	char file_name[1024];
	int borderSize = 4;
	
	color = (color == -1) ? BUTTON_COLOUR_INNER : color; //never used
	memset(file_name, 0, 1024);
	strcpy(&file_name[0], message);
	
	// Hide extension when rendering certain files
	if(file->fileAttrib == IS_FILE) {
		if(endsWith(file_name,".gcm") 
			|| endsWith(file_name,".iso")
			|| endsWith(file_name,".dol")
			|| endsWith(file_name,".dol+cli")) {
			file_name[((u32)strrchr(file_name, '.'))-((u32)&file_name)] = '\0';
		}
	}
	float scale = GetTextScaleToFitInWidth(file_name, (x2-x1-96-35)-(borderSize*2));

	GXColor selectColor = (GXColor) {46,57,104,GUI_MSGBOX_ALPHA}; 	//bluish
	GXColor noColor 	= (GXColor) {0,0,0,0}; 						//black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA};	//silver

	//Draw Text and backfill (if selected)
	if(mode==B_SELECTED) {
		DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, selectColor, borderColor);
	}
	else {
		DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, noColor, borderColor);
	}
	// Draw banner if there is one
	if(file->meta && file->meta->banner) {
		DrawTexObj(&file->meta->bannerTexObj, x1+7, y1+4, 96, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}
	if(file->meta && file->meta->regionTexId != -1 && file->meta->regionTexId != 0) {
		DrawImage(file->meta->regionTexId, x2 - 37, y1+borderSize+2, 30,20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}	
	WriteFontStyled(x1 + borderSize+5+96, y1+borderSize, file_name, scale, false, defaultColor);
	
	// Print specific stats
	if(file->fileAttrib==IS_FILE) {
		if(devices[DEVICE_CUR] == &__device_wode) {
			ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
			sprintf(txtbuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
		}
		else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
			sprintf(txtbuffer,"%.2fKB (%ld blocks)", (float)file->size/1024, file->size/8192);
		}
		else if(devices[DEVICE_CUR] == &__device_qoob) {
			sprintf(txtbuffer,"%.2fKB (%ld blocks)", (float)file->size/1024, file->size/0x10000);
		}
		else {
			sprintf(txtbuffer,"%.2f %s",file->size > (1024*1024) ? (float)file->size/(1024*1024):(float)file->size/1024,file->size > (1024*1024) ? "MB":"KB");
		}
		WriteFontStyled(x2 - ((borderSize+3) + (GetTextSizeInPixels(txtbuffer)*0.45)), y1+borderSize+24, txtbuffer, 0.45f, false, defaultColor);
	}
}

void DrawEmptyBox(int x1, int y1, int x2, int y2, int color) 
{
	int borderSize;
	borderSize = (y2-y1) <= 30 ? 3 : 10;
	x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;

	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //Black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, fillColor, borderColor);
}

void DrawTransparentBox(int x1, int y1, int x2, int y2) 
{
	int borderSize;
	borderSize = (y2-y1) <= 30 ? 3 : 10;
	x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;

	GXColor noColor 	= (GXColor) {0,0,0,0};
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, noColor, borderColor);
}

// Buttons
void DrawMenuButtons(int selection) 
{
	// Draw the buttons
	DrawImage(TEX_BTNDEVICE, 40+(0*116), 430, BTNDEVICE_WIDTH,BTNDEVICE_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	DrawImage(TEX_BTNSETTINGS, 40+(1*116), 430, BTNSETTINGS_WIDTH,BTNSETTINGS_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	DrawImage(TEX_BTNINFO, 40+(2*116), 430, BTNINFO_WIDTH,BTNINFO_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	DrawImage(TEX_BTNREFRESH, 40+(3*116), 430, BTNREFRESH_WIDTH,BTNREFRESH_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	DrawImage(TEX_BTNEXIT, 40+(4*116), 430, BTNEXIT_WIDTH,BTNEXIT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	// Highlight selected
	int i;
	for(i=0;i<5;i++)
	{
		if(selection==i)
			DrawImage(TEX_BTNHILIGHT, 40+(i*116), 430, BTNHILIGHT_WIDTH,BTNHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
		else
			DrawImage(TEX_BTNNOHILIGHT, 40+(i*116), 430, BTNNOHILIGHT_WIDTH,BTNNOHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}
}

int DrawYesNoDialog(char *message) {
	int sel = 0;
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	while(1) {
		doBackdrop();
		DrawEmptyBox(75,190, vmode->fbWidth-78, 330, COLOR_BLACK);
		WriteFontStyled(640/2, 215, message, 1.0f, true, defaultColor);
		DrawSelectableButton(100, 280, -1, 310, "Yes", (sel==1) ? B_SELECTED:B_NOSELECT,-1);
		DrawSelectableButton(380, 280, -1, 310, "No", (!sel) ? B_SELECTED:B_NOSELECT,-1);
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)) {
			sel^=1;
		}
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)))
			{ VIDEO_WaitVSync (); }
	}
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	return sel;
} 

void DrawVertScrollBar(int x, int y, int width, int height, float scrollPercent, int scrollHeight) {
	int x1 = x;
	int x2 = x+width;
	int y1 = y;
	int y2 = y+height;
	int scrollStartY = y+3 + (int)(height*scrollPercent);
	scrollHeight = scrollHeight < 10 ? 10:scrollHeight;

	if(scrollStartY + scrollHeight +3 > y2)
		scrollStartY = y2-scrollHeight-3;
	
	GXColor fillColor = (GXColor) {46,57,104,GUI_MSGBOX_ALPHA}; 	//bluish
  	GXColor noColor = (GXColor) {0,0,0,0}; //blank
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, noColor, borderColor);
	
	DrawSimpleBox( x1, scrollStartY,
			width, scrollHeight, 0, fillColor, borderColor); 
}

void drawParameterForArgsSelector(Parameter *param, int x, int y, int selected) {

	char *name = &param->arg.name[0];
	char *selValue = &param->values[param->currentValueIdx].name[0];
	
	int chkWidth = 32, nameWidth = 300, gapWidth = 13, paramWidth = 120;
	// [32px 10px 250px 10px 5px 80px 5px]
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (param->enable || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	if(selected) DrawSimpleBox( x+chkWidth+gapWidth-5, y, nameWidth, 35, 0, deSelectedColor, defaultColor);
	DrawImage(param->enable ? TEX_CHECKED:TEX_UNCHECKED, x, y, 32, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	// Draw the parameter Name
	WriteFontStyled(x+chkWidth+gapWidth+5, y+4, name, GetTextScaleToFitInWidth(name, nameWidth-10), false, fontColor);
	// If enabled, draw arrows indicating where in the param list we are
	if(selected && param->enable && param->num_values > 1) {
		if(param->currentValueIdx != 0) {
			WriteFontStyled(x+(chkWidth+nameWidth+(gapWidth*4)), y+5, "<-", .8f, false, defaultColor);
		}
		if(param->currentValueIdx != param->num_values-1) {
			WriteFontStyled(x+(chkWidth+nameWidth+paramWidth+(gapWidth*6)), y+5, "->", .8f, false, defaultColor);
		}
	}
	// Draw the current value
	WriteFontStyled(x+chkWidth+nameWidth+(gapWidth*6), y+2, selValue, GetTextScaleToFitInWidth(selValue, paramWidth), false, fontColor);
}

void DrawArgsSelector(char *fileName) {

	Parameters* params = getParameters();
	int param_selection = 0;
	int params_per_page = 6;
	
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	while(1) {
		doBackdrop();
		DrawEmptyBox(20,60, vmode->fbWidth-20, 460, COLOR_BLACK);
		sprintf(txtbuffer, "%s Parameters:", fileName);
		WriteFontStyled(25, 62, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, vmode->fbWidth-50), false, defaultColor);

		int j = 0;
		int current_view_start = MIN(MAX(0,param_selection-params_per_page/2),MAX(0,params->num_params-params_per_page));
		int current_view_end = MIN(params->num_params, MAX(param_selection+params_per_page/2,params_per_page));
	
		int scrollBarHeight = 90+(params_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)params->num_params);
		DrawVertScrollBar(vmode->fbWidth-45, 120, 25, scrollBarHeight, (float)((float)param_selection/(float)(params->num_params-1)),scrollBarTabHeight);
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			drawParameterForArgsSelector(&params->parameters[current_view_start], 25, 120+j*35, current_view_start==param_selection);
		}
		// Write about the default if there is any
		DrawTransparentBox( 35, 350, vmode->fbWidth-35, 400);
		WriteFontStyled(33, 345, "Default values will be used by the DOL being loaded if a", 0.8f, false, defaultColor);
		WriteFontStyled(33, 365, "parameter is not enabled. Please check the documentation", 0.8f, false, defaultColor);
		WriteFontStyled(33, 385, "for this DOL if you are unsure of the default values.", 0.8f, false, defaultColor);
		WriteFontStyled(640/2, 440, "(A) Toggle Param - (Start) Load the DOL", 1.0f, true, defaultColor);
		DrawFrameFinish();

		while (!(PAD_ButtonsHeld(0) & (PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_START|PAD_BUTTON_A)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if((btns & (PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT)) && params->parameters[param_selection].enable) {
			int curValIdx = params->parameters[param_selection].currentValueIdx;
			int maxValIdx = params->parameters[param_selection].num_values;
			curValIdx = btns & PAD_BUTTON_LEFT ? 
				((--curValIdx < 0) ? maxValIdx-1 : curValIdx):((curValIdx + 1) % maxValIdx);
			params->parameters[param_selection].currentValueIdx = curValIdx;
		}
		if(btns & (PAD_BUTTON_UP|PAD_BUTTON_DOWN)) {
			param_selection = btns & PAD_BUTTON_UP ? 
				((--param_selection < 0) ? params->num_params-1 : param_selection)
				:((param_selection + 1) % params->num_params);
		}
		if(btns & PAD_BUTTON_A) {
			params->parameters[param_selection].enable ^= 1;
		}
		if(btns & PAD_BUTTON_START) {
			break;
		}
		while (PAD_ButtonsHeld(0) & (PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_START|PAD_BUTTON_A))
			{ VIDEO_WaitVSync (); }
	}
}

void drawCheatForCheatsSelector(CheatEntry *cheat, int x, int y, int selected) {

	char *name = &cheat->name[0];
	
	int chkWidth = 32, nameWidth = 525, gapWidth = 13;
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (cheat->enabled || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	if(selected) DrawSimpleBox( x+chkWidth+gapWidth-5, y, nameWidth, 35, 0, deSelectedColor, defaultColor);
	DrawImage(cheat->enabled ? TEX_CHECKED:TEX_UNCHECKED, x, y, 32, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	// Draw the cheat Name
	WriteFontStyled(x+chkWidth+gapWidth+5, y+4, name, GetTextScaleToFitInWidth(name, nameWidth-10), false, fontColor);
}

void DrawCheatsSelector(char *fileName) {

	CheatEntries* cheats = getCheats();
	int cheat_selection = 0;
	int cheats_per_page = 6;
	
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	while(1) {
		doBackdrop();
		DrawEmptyBox(20,60, vmode->fbWidth-20, 460, COLOR_BLACK);
		sprintf(txtbuffer, "%s Cheats:", fileName);
		WriteFontStyled(25, 62, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, vmode->fbWidth-50), false, defaultColor);

		int j = 0;
		int current_view_start = MIN(MAX(0,cheat_selection-cheats_per_page/2),MAX(0,cheats->num_cheats-cheats_per_page));
		int current_view_end = MIN(cheats->num_cheats, MAX(cheat_selection+cheats_per_page/2,cheats_per_page));
	
		int scrollBarHeight = 90+(cheats_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)cheats->num_cheats);
		DrawVertScrollBar(vmode->fbWidth-45, 120, 25, scrollBarHeight, (float)((float)cheat_selection/(float)(cheats->num_cheats-1)),scrollBarTabHeight);
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			drawCheatForCheatsSelector(&cheats->cheat[current_view_start], 25, 120+j*35, current_view_start==cheat_selection);
		}
		// Write about how many cheats are enabled
		DrawTransparentBox( 35, 350, vmode->fbWidth-35, 410);
		WriteFontStyled(33, 345, "Space taken by cheats:", 0.8f, false, defaultColor);
		GXColor noColor = (GXColor) {0,0,0,0}; //blank
		GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
		GXColor progressBarColor = (GXColor) {255,128,0,GUI_MSGBOX_ALPHA}; //orange
		
		float multiplier = (float)getEnabledCheatsSize() / (float)kenobi_get_maxsize();
		DrawSimpleBox( 33, 370, vmode->fbWidth-66, 20, 0, noColor, borderColor); 
		DrawSimpleBox( 33, 370,	(int)((vmode->fbWidth-66)*multiplier), 20, 0, progressBarColor, noColor);
		sprintf(txtbuffer, "WiiRD Debug %s", swissSettings.wiirdDebug ? "Enabled":"Disabled");
		WriteFontStyled(33, 395, txtbuffer, 0.8f, false, defaultColor);
		WriteFontStyled(640/2, 440, "(A) Toggle Cheat - (X) WiiRD Debug - (B) Return", 0.9f, true, defaultColor);
		DrawFrameFinish();

		while (!(PAD_ButtonsHeld(0) & (PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & (PAD_BUTTON_UP|PAD_BUTTON_DOWN)) {
			cheat_selection = btns & PAD_BUTTON_UP ? 
				((--cheat_selection < 0) ? cheats->num_cheats-1 : cheat_selection)
				:((cheat_selection + 1) % cheats->num_cheats);
		}
		if(btns & PAD_BUTTON_A) {
			cheats->cheat[cheat_selection].enabled ^= 1;
			if(getEnabledCheatsSize() > kenobi_get_maxsize())	// No room
				cheats->cheat[cheat_selection].enabled = 0;
		}
		if(btns & PAD_BUTTON_X) {
			swissSettings.wiirdDebug ^=1;
		}
		if(btns & PAD_BUTTON_B) {
			break;
		}
		while (PAD_ButtonsHeld(0) & (PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X))
			{ VIDEO_WaitVSync (); }
	}
}

