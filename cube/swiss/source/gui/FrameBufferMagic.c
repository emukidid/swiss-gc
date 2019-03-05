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
#include <math.h>
#include <ogc/exi.h>
#include <gctypes.h>
#include <ogc/lwp_watchdog.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "filemeta.h"
#include "swiss.h"
#include "main.h"
#include "util.h"
#include "ata.h"
#include "btns.h"
#include "dolparameters.h"
#include "cheats.h"

#define GUI_MSGBOX_ALPHA 220

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

static char fbTextBuffer[64];

// Video threading vars
#define VIDEO_STACK_SIZE (1024*64)
#define VIDEO_PRIORITY 100
static char  video_thread_stack[VIDEO_STACK_SIZE];
static lwp_t video_thread;
static mutex_t _videomutex;
static int threadAlive = 0;

enum VideoEventType
{
	EV_TEXOBJ = 0,
	EV_MSGBOX,
	EV_IMAGE,
	EV_PROGRESS,
	EV_SELECTABLEBUTTON,
	EV_EMPTYBOX,
	EV_TRANSPARENTBOX,
	EV_FILEBROWSERBUTTON,
	EV_VERTSCROLLBAR,
	EV_STYLEDLABEL,
	EV_CONTAINER,
	EV_MENUBUTTONS,
	EV_TOOLTIP
};

char * typeStrings[] = {"TexObj", "MsgBox", "Image", "Progress", "SelectableButton", "EmptyBox", "TransparentBox",
						"FileBrowserButton", "VertScrollbar", "StyledLabel", "Container", "MenuButtons", "Tooltip"};

typedef struct drawTexObjEvent {
	GXTexObj *texObj;
	int x;
	int y;
	int width;
	int height;
	int depth;
	float s1;
	float s2;
	float t1;
	float t2;
} drawTexObjEvent_t;

typedef struct drawVertScrollbarEvent {
	int x;
	int y;
	int width;
	int height;
	float scrollPercent;
	int scrollHeight;
} drawVertScrollbarEvent_t;

typedef struct drawImageEvent {
	int textureId;
	int x;
	int y;
	int width;
	int height;
	int depth;
	float s1;
	float s2;
	float t1;
	float t2;
} drawImageEvent_t;

typedef struct drawStyledLabelEvent {
	int x;
	int y;
	char *string;
	float size;
	bool centered;
	GXColor color;
	int fading;
	int fadingDirection;
} drawStyledLabelEvent_t;

typedef struct drawSelectableButtonEvent {
	int x1;
	int y1;
	int x2;
	int y2;
	int mode;
	char *msg;
} drawSelectableButtonEvent_t;

typedef struct drawBoxEvent {
	int x1;
	int y1;
	int x2;
	int y2;
} drawBoxEvent_t;

typedef struct drawFileBrowserButtonEvent {
	int x1;
	int y1;
	int x2;
	int y2;
	char *displayName;
	file_handle *file;
	int mode;
} drawFileBrowserButtonEvent_t;

typedef struct drawMenuButtonsEvent {
	int selection;
} drawMenuButtonsEvent_t;

typedef struct drawTooltipEvent {
	char *tooltip;
} drawTooltipEvent_t;

typedef struct drawMsgBoxEvent {
	int type;
} drawMsgBoxEvent_t;

typedef struct drawProgressEvent {
	bool indeterminate;
	int percent;
	int speed;	// in bytes
	int timestart;
	int timeremain;
} drawProgressEvent_t;

typedef struct uiDrawObjQueue {
	struct uiDrawObj *event;
	struct uiDrawObjQueue *next;
} uiDrawObjQueue_t;

static uiDrawObjQueue_t *videoEventQueue = NULL;
static uiDrawObj_t *buttonPanel = NULL;

// Add root level uiDrawObj_t
static uiDrawObj_t* addVideoEvent(uiDrawObj_t *event) {
	// First entry, make it root
	if(videoEventQueue == NULL) {
		videoEventQueue = calloc(1, sizeof(uiDrawObjQueue_t));
		videoEventQueue->event = event;
		//print_gecko("Added first event %08X (type %s)\r\n", (u32)videoEventQueue, typeStrings[event->type]);
		return event;
	}
	
	uiDrawObjQueue_t *current = videoEventQueue;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = calloc(1, sizeof(uiDrawObjQueue_t));
	current->next->event = event;
	event->disposed = false;
	//print_gecko("Added a new event %08X (type %s)\r\n", (u32)event, typeStrings[event->type]);
	return event;
}

static void clearNestedEvent(uiDrawObj_t *event) {
	if(event && !event->disposed) {
		print_gecko("Event was not disposed!!\r\n");
		print_gecko("Event %08X (type %s)\r\n", (u32)event, typeStrings[event->type]);
	}
	
	if(event->child && event->child != event) {
		clearNestedEvent(event->child);
	}
	//print_gecko("Dispose nested event %08X\r\n", (u32)event);
	if(event && event->data) {
		// Free any attached data
		if(event->type == EV_STYLEDLABEL) {
			if(((drawStyledLabelEvent_t*)event->data)->string) {
				//printf("Clear Nested EV_STYLEDLABEL\r\n");
				free(((drawStyledLabelEvent_t*)event->data)->string);
			}
		}
		else if(event->type == EV_FILEBROWSERBUTTON) {
			if(((drawFileBrowserButtonEvent_t*)event->data)->displayName) {
				//printf("Clear Nested EV_FILEBROWSERBUTTON\r\n");
				free(((drawFileBrowserButtonEvent_t*)event->data)->displayName);
			}
			if(((drawFileBrowserButtonEvent_t*)event->data)->file) {
				if(((drawFileBrowserButtonEvent_t*)event->data)->file->meta) {
					if(((drawFileBrowserButtonEvent_t*)event->data)->file->meta->banner) {
						free(((drawFileBrowserButtonEvent_t*)event->data)->file->meta->banner);
					}
					free(((drawFileBrowserButtonEvent_t*)event->data)->file->meta);
				}
				free(((drawFileBrowserButtonEvent_t*)event->data)->file);
			}
		}
		else if(event->type == EV_SELECTABLEBUTTON) {
			if(((drawSelectableButtonEvent_t*)event->data)->msg) {
				//printf("Clear Nested EV_SELECTABLEBUTTON\r\n");
				free(((drawSelectableButtonEvent_t*)event->data)->msg);
			}
		}
		else if(event->type == EV_TOOLTIP) {
			if(((drawTooltipEvent_t*)event->data)->tooltip) {
				//printf("Clear Nested EV_TOOLTIP\r\n");
				free(((drawTooltipEvent_t*)event->data)->tooltip);
			}
		}
		//printf("Clear Nested event->data\r\n");
		free(event->data);
	}
	if(event) {
		//printf("Clear event\r\n");
		memset(event, 0, sizeof(uiDrawObj_t));
		free(event);
	}
}

static void disposeEvent(uiDrawObj_t *event) {
	if(videoEventQueue == NULL) {
		return;
	}

	// See if this is in our root event queue
	uiDrawObjQueue_t *current = videoEventQueue->next;
	uiDrawObjQueue_t *previous = videoEventQueue;
	// First node is what we're after.
	while (current != NULL) {
		if(current->event == event) {
			//print_gecko("Disposing event %08X\r\n", (u32)current);
			clearNestedEvent(current->event);
			if(previous != NULL) {
				previous->next = current->next;
			}
		}
		else {
			previous = current;
		}
		current = current->next;
    }
}


static void init_textures() 
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

static void drawInit()
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
	guOrtho(GXprojection2D, 0, 479, 0, 639, 0, 700);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC);

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

static void _drawRect(int x, int y, int width, int height, int depth, GXColor color, float s0, float s1, float t0, float t1)
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

static void _DrawSimpleBox(int x, int y, int width, int height, int depth, GXColor fillColor, GXColor borderColor) 
{
	//Adjust for blank texture border
	x-=4; y-=4; width+=8; height+=8;
	
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_InvalidateTexAll();
	GX_LoadTexObj(&boxinnerTexObj, GX_TEXMAP0);

	_drawRect(x, y, width/2, height/2, depth, fillColor, 0.0f, ((float)width/32), 0.0f, ((float)height/32));
	_drawRect(x+(width/2), y, width/2, height/2, depth, fillColor, ((float)width/32), 0.0f, 0.0f, ((float)height/32));
	_drawRect(x, y+(height/2), width/2, height/2, depth, fillColor, 0.0f, ((float)width/32), ((float)height/32), 0.0f);
	_drawRect(x+(width/2), y+(height/2), width/2, height/2, depth, fillColor, ((float)width/32), 0.0f, ((float)height/32), 0.0f);

	GX_InvalidateTexAll();
	GX_LoadTexObj(&boxouterTexObj, GX_TEXMAP0);

	_drawRect(x, y, width/2, height/2, depth, borderColor, 0.0f, ((float)width/32), 0.0f, ((float)height/32));
	_drawRect(x+(width/2), y, width/2, height/2, depth, borderColor, ((float)width/32), 0.0f, 0.0f, ((float)height/32));
	_drawRect(x, y+(height/2), width/2, height/2, depth, borderColor, 0.0f, ((float)width/32), ((float)height/32), 0.0f);
	_drawRect(x+(width/2), y+(height/2), width/2, height/2, depth, borderColor, ((float)width/32), 0.0f, ((float)height/32), 0.0f);
}

// Internal
static void _DrawImageNow(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered) {
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
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) x,(float) y,(float) depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(s1,t1);
		GX_Position3f32((float) (x+width), (float)y, (float)depth );
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

// Internal
static void _DrawImage(uiDrawObj_t *evt) {
	drawImageEvent_t *data = (drawImageEvent_t*)evt->data;
	_DrawImageNow(data->textureId, data->x, data->y, data->width, data->height, data->depth, data->s1, data->s2, data->t1, data->t2, 0);
}

// External
uiDrawObj_t* DrawImage(int textureId, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered)
{
	drawImageEvent_t *eventData = calloc(1, sizeof(drawImageEvent_t));
	eventData->textureId = textureId;
	eventData->x = centered ? ((int) x - width/2) : x;
	eventData->y = y;
	eventData->width = width;
	eventData->height = height;
	eventData->depth = depth;
	eventData->s1 = s1;
	eventData->s2 = s2;
	eventData->t1 = t1;
	eventData->t2 = t2;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_IMAGE;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawTexObj(uiDrawObj_t *evt)
{
	drawTexObjEvent_t *data = (drawTexObjEvent_t*)evt->data;
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_InvalidateTexAll();
	GX_LoadTexObj(data->texObj, GX_TEXMAP0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32((float) data->x,(float) data->y,(float) data->depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(data->s1,data->t1);
		GX_Position3f32((float) (data->x+data->width),(float) data->y,(float) data->depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(data->s2,data->t1);
		GX_Position3f32((float) (data->x+data->width),(float) (data->y+data->height),(float) data->depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(data->s2,data->t2);
		GX_Position3f32((float) data->x,(float) (data->y+data->height),(float) data->depth );
		GX_Color4u8(255, 255, 255, 255);
		GX_TexCoord2f32(data->s1,data->t2);
	GX_End();
}

// External
uiDrawObj_t* DrawTexObj(GXTexObj *texObj, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered)
{
	drawTexObjEvent_t *eventData = calloc(1, sizeof(drawTexObjEvent_t));
	eventData->texObj = texObj;
	eventData->x = centered ? ((int) x - width/2) : x;
	eventData->y = y;
	eventData->width = width;
	eventData->height = height;
	eventData->depth = depth;
	eventData->s1 = s1;
	eventData->s2 = s2;
	eventData->t1 = t1;
	eventData->t2 = t2;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_TEXOBJ;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawProgressBar(uiDrawObj_t *evt) {
	
	drawProgressEvent_t *data = (drawProgressEvent_t*)evt->data;
	int x1 = ((640/2) - (PROGRESS_BOX_WIDTH/2));
	int x2 = ((640/2) + (PROGRESS_BOX_WIDTH/2));
	int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
	int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));

  	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //black
  	GXColor noColor = (GXColor) {0,0,0,0}; //blank
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	GXColor progressBarColor = (GXColor) {255,128,0,GUI_MSGBOX_ALPHA}; //orange
	GXColor progressBarIndColor = (GXColor) {0xb3,0xd9,0xff,GUI_MSGBOX_ALPHA}; //orange
	
	_DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, fillColor, borderColor);
	
	int middleY = (y2+y1)/2;
	if(data->indeterminate) {
		data->percent += (data->percent + 2 == 400 ? -398 : 2);
		int multiplier = (PROGRESS_BOX_WIDTH-20)/100;
		int progressBarWidth = multiplier*100;
		int progressStart = 0;
		int progressSize = 0;
		if(data->percent < 100) {
			progressStart = 0;
			progressSize = data->percent%100;
		}
		else if(data->percent >= 100 && data->percent < 200) {
			progressStart = (data->percent%100);
			progressSize = 100-progressStart;
		}
		else if(data->percent >= 200 && data->percent < 300) {
			progressStart = 100-(data->percent%100);
			progressSize = 100-progressStart;
		}
		else {
			progressStart = 0;
			progressSize = 100-(data->percent%100);
		}
		
		_DrawSimpleBox( (640/2 - progressBarWidth/2), y1+20,
				(multiplier*100), 20, 0, noColor, borderColor); 
		_DrawSimpleBox( (640/2 - progressBarWidth/2) + (progressStart*multiplier),
				y1+20,
				(multiplier*progressSize),
				20, 0, progressBarIndColor, noColor);
	}
	else {
		int multiplier = (PROGRESS_BOX_WIDTH-20)/100;
		int progressBarWidth = multiplier*100;
		_DrawSimpleBox( (640/2 - progressBarWidth/2), y1+20,
				(multiplier*100), 20, 0, noColor, borderColor); 
		_DrawSimpleBox( (640/2 - progressBarWidth/2), y1+20,
				(multiplier*data->percent), 20, 0, progressBarColor, noColor); 
		sprintf(fbTextBuffer,"%d%%", data->percent);
		bool displaySpeed = data->speed != 0;
		drawString(displaySpeed ? (x1 + 80) : (640/2), middleY+30, fbTextBuffer, 1.0f, true, defaultColor);
		if(displaySpeed) {
			if(data->speed >= 1048576) {
				sprintf(fbTextBuffer,"%.2f MB/s", (float)data->speed/(float)(1048576));
			}
			else if(data->speed >= 1024) {
				sprintf(fbTextBuffer,"%.2f KB/s", (float)data->speed/(float)(1024));
			}
			else {
				sprintf(fbTextBuffer,"%i B/s", data->speed);
			}
			drawString(x1 + 280, middleY+30, fbTextBuffer, 1.0f, true, defaultColor);
			sprintf(fbTextBuffer,"Elapsed: %02i:%02i:%02i", data->timestart / 3600, (data->timestart / 60)%60,  data->timestart % 60);
			drawString(x1 + 500, middleY+30, fbTextBuffer, 0.65f, true, defaultColor);
			sprintf(fbTextBuffer,"Remain: %02i:%02i:%02i", data->timeremain / 3600, (data->timeremain / 60)%60,  data->timeremain % 60);
			drawString(x1 + 500, middleY+45, fbTextBuffer, 0.65f, true, defaultColor);
		}
	}	
}

// External
uiDrawObj_t* DrawProgressBar(bool indeterminate, int percent, char *message) {
	drawProgressEvent_t *eventData = calloc(1, sizeof(drawProgressEvent_t));
	eventData->percent = percent;
	eventData->indeterminate = indeterminate;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_PROGRESS;
	event->data = eventData;
	if(message && strlen(message) > 0) {
		sprintf(txtbuffer, "%s", message);
		// Add child component(s) for label(s)
		char *tok = strtok(txtbuffer,"\n");
		int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
		int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));
		int middleY = (y2+y1)/2;
		while(tok != NULL) {
			DrawAddChild(event, DrawStyledLabel(640/2, middleY, tok, 1.0f, true, defaultColor));
			tok = strtok(NULL,"\n");
			middleY+=24;
		}
	}
	return event;
}

// Internal
static void _DrawMessageBox(uiDrawObj_t *evt) {
	int x1 = ((640/2) - (PROGRESS_BOX_WIDTH/2));
	int x2 = ((640/2) + (PROGRESS_BOX_WIDTH/2));
	int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
	int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));
	
  	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	_DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, fillColor, borderColor); 
}	

// External
uiDrawObj_t* DrawMessageBox(int type, char *msg) 
{
	drawMsgBoxEvent_t *eventData = calloc(1, sizeof(drawMsgBoxEvent_t));
	eventData->type = type;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_MSGBOX;
	event->data = eventData;
	
	// Add child component(s) for label(s)
	sprintf(txtbuffer, "%s", msg);
	char *tok = strtok(txtbuffer,"\n");
	int y1 = ((480/2) - (PROGRESS_BOX_HEIGHT/2));
	int y2 = ((480/2) + (PROGRESS_BOX_HEIGHT/2));
	int middleY = y2-y1 < 23 ? y1+3 : (y2+y1)/2-12;
	while(tok != NULL) {
		uiDrawObj_t *lineLabel = DrawStyledLabel(640/2, middleY, tok, 1.0f, true, defaultColor);
		tok = strtok(NULL,"\n");
		middleY+=24;
		DrawAddChild(event, lineLabel);
	}
	
	return event;
}

// Internal
static void _DrawSelectableButton(uiDrawObj_t *evt) {
	drawSelectableButtonEvent_t *data = (drawSelectableButtonEvent_t*)evt->data;
	int x1 = data->x1;
	int x2 = data->x2;
	GXColor selectColor = (GXColor) {96,107,164,GUI_MSGBOX_ALPHA}; //bluish
	GXColor noColor = (GXColor) {0,0,0,0}; //black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	int borderSize = 4;
	//determine length of the text ourselves if x2 == -1
	x1 = (x2 == -1) ? x1+2:x1;
	x2 = (x2 == -1) ? GetTextSizeInPixels(data->msg)+x1+(borderSize*2)+6 : x2;
	//Draw Text and backfill (if selected)
	if(data->mode==B_SELECTED) {
		_DrawSimpleBox( x1, data->y1, x2-x1, data->y2-data->y1+2, 0, selectColor, borderColor);
	}
	else {
		_DrawSimpleBox( x1, data->y1, x2-x1, data->y2-data->y1+2, 0, noColor, borderColor);
	}
	
	if(data->msg) {
		float scale = GetTextScaleToFitInWidth(data->msg, (x2-x1)-(borderSize*2));	
		drawString(data->x1 + borderSize+3, data->y1+2, data->msg, scale, false, defaultColor);
	}
}

// External
uiDrawObj_t* DrawSelectableButton(int x1, int y1, int x2, int y2, char *message, int mode) 
{	
	drawSelectableButtonEvent_t *eventData = calloc(1, sizeof(drawSelectableButtonEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	eventData->mode = mode;
	if(message) {
		eventData->msg = malloc(strlen(message));
		strcpy(eventData->msg, message);
	}
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_SELECTABLEBUTTON;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawTooltip(uiDrawObj_t *evt) {

	drawTooltipEvent_t *data = (drawTooltipEvent_t*)evt->data;

	if(data->tooltip/* && data->tooltiptime >= 50*/) {
		//int alpha = data->tooltiptime*4 > 255 ? 255 : data->tooltiptime;
		int alpha = 255;
		int borderSize = 4;
		GXColor borderColorTT = (GXColor) {255,255,255,alpha};
		GXColor backColorTT = (GXColor) {122,122,122,alpha}; //grey
		int numLines = 1;
		char *strPtr = data->tooltip;
		for (numLines=1; strPtr[numLines]; strPtr[numLines]=='\n' ? numLines++ : *strPtr++);
		int height = numLines*26;
		int tooltipY1 = (getVideoMode()->efbHeight / 2) - (height/2);
		// TODO centre on Y based on total size.
		int tooltipX1 = 25, tooltipX2 = getVideoMode()->fbWidth-25, tooltipY2 = tooltipY1+height;
		_DrawSimpleBox( tooltipX1, tooltipY1-6, tooltipX2-tooltipX1, (tooltipY2-tooltipY1)+6, 0, backColorTT, borderColorTT);
		
		// Write each line
		strPtr = data->tooltip;
		int curLine = 0;
		while(numLines) {
			float scale = GetTextScaleToFitInWidthWithMax(strPtr, (tooltipX2-tooltipX1)-(borderSize*2), 0.75f);
			drawString(tooltipX1 + borderSize+3, tooltipY1+2+(curLine*25), strPtr, scale, false, borderColorTT);
			numLines--;
			curLine++;
			// Increment to the next line if we have one.
			if(numLines > 0) {
				while(*strPtr != '\n') strPtr++;
				strPtr++;
			}
		}
	}
}

// External
uiDrawObj_t* DrawTooltip(char *tooltip) {
	drawTooltipEvent_t *eventData = calloc(1, sizeof(drawTooltipEvent_t));
	if(tooltip && strlen(tooltip) > 0) {
		eventData->tooltip = malloc(strlen(tooltip));
		strcpy(eventData->tooltip, tooltip);
	}
	else {
		eventData->tooltip = NULL;
	}
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_TOOLTIP;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawStyledLabel(uiDrawObj_t *evt) {
	drawStyledLabelEvent_t *data = (drawStyledLabelEvent_t*)evt->data;
	if(data->fadingDirection) {
		data->color.a += data->fadingDirection;
		if(data->color.a >= 255) data->fadingDirection = -1;
		else if(data->color.a <= 15) data->fadingDirection = 1;
	}
	drawString(data->x, data->y, data->string, data->size, data->centered, data->color);
}

// External
uiDrawObj_t* DrawStyledLabel(int x, int y, char *string, float size, bool centered, GXColor color) 
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = malloc(strlen(string));
		strcpy(eventData->string, string);
	}
	else {
		eventData->string = NULL;
	}
	eventData->size = size;
	eventData->centered = centered;
	eventData->color = color;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_STYLEDLABEL;
	event->data = eventData;
	return event;
}

// External
uiDrawObj_t* DrawLabel(int x, int y, char *string) 
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = malloc(strlen(string));
		strcpy(eventData->string, string);
	}
	else {
		eventData->string = NULL;
	}
	eventData->size = 1.0f;
	eventData->centered = false;
	eventData->color = (GXColor) {255, 255, 255, 255};
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_STYLEDLABEL;
	event->data = eventData;
	return event;
}

// External
uiDrawObj_t* DrawFadingLabel(int x, int y, char *string, float size) 
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = malloc(strlen(string));
		strcpy(eventData->string, string);
	}
	else {
		eventData->string = NULL;
	}
	eventData->size = size;
	eventData->centered = false;
	eventData->color = (GXColor) {255, 255, 255, 0};
	eventData->fadingDirection = 1;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_STYLEDLABEL;
	event->data = eventData;
	return event;
}

// External (this is used to tie objects together, think of it as an invisible panel)
uiDrawObj_t* DrawContainer()
{
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_CONTAINER;
	return event;
}

// Internal
static void _DrawFileBrowserButton(uiDrawObj_t *evt) {
	int borderSize = 4;	
	drawFileBrowserButtonEvent_t *data = (drawFileBrowserButtonEvent_t*)evt->data;
	
	// Not selected
	GXColor noColor 	= (GXColor) {0,0,0,0};
	GXColor selectColor = (GXColor) {46,57,104,GUI_MSGBOX_ALPHA}; 	//bluish
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver

	_DrawSimpleBox(data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 
				0, data->mode == B_SELECTED ? selectColor : noColor, borderColor);
	
	// Draw banner if there is one
	file_handle *file = data->file;
	if(file->meta && file->meta->banner) {
		GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
		GX_InvalidateTexAll();
		GX_LoadTexObj(&file->meta->bannerTexObj, GX_TEXMAP0);
		GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
			GX_Position3f32((float) data->x1+7,(float) data->y1+4, 0.0f );
			GX_Color4u8(255, 255, 255, 255);
			GX_TexCoord2f32(0.0f,0.0f);
			GX_Position3f32((float) (data->x1+7+96),(float) data->y1+4,0.0f );
			GX_Color4u8(255, 255, 255, 255);
			GX_TexCoord2f32(1.0f,0.0f);
			GX_Position3f32((float) (data->x1+7+96),(float) (data->y1+4+32),0.0f );
			GX_Color4u8(255, 255, 255, 255);
			GX_TexCoord2f32(1.0f,1.0f);
			GX_Position3f32((float) data->x1+7,(float) (data->y1+4+32),0.0f );
			GX_Color4u8(255, 255, 255, 255);
			GX_TexCoord2f32(0.0f,1.0f);
		GX_End();
	}
	if(file->meta && file->meta->regionTexId != -1 && file->meta->regionTexId != 0) {
		_DrawImageNow(file->meta->regionTexId, data->x2 - 37, data->y1+borderSize+2, 30,20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}
	
	float scale = GetTextScaleToFitInWidth(data->displayName, (data->x2-data->x1-96-35)-(borderSize*2));

	drawString(data->x1 + borderSize+5+96, data->y1+borderSize, data->displayName, scale, false, defaultColor);
	
	// Print specific stats
	if(file->fileAttrib==IS_FILE) {
		if(devices[DEVICE_CUR] == &__device_wode) {
			ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
			sprintf(fbTextBuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
		}
		else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
			sprintf(fbTextBuffer,"%.2fKB (%ul blocks)", (float)file->size/1024, file->size/8192);
		}
		else if(devices[DEVICE_CUR] == &__device_qoob) {
			sprintf(fbTextBuffer,"%.2fKB (%ul blocks)", (float)file->size/1024, file->size/0x10000);
		}
		else {
			sprintf(fbTextBuffer,"%.2f %s",file->size > (1024*1024) ? (float)file->size/(1024*1024):(float)file->size/1024,file->size > (1024*1024) ? "MB":"KB");
		}
		drawString(data->x2 - ((borderSize+3) + (GetTextSizeInPixels(fbTextBuffer)*0.45)), 
			data->y1+borderSize+24, fbTextBuffer, 0.45f, false, defaultColor);
	}
}

// External
uiDrawObj_t* DrawFileBrowserButton(int x1, int y1, int x2, int y2, char *message, file_handle *file, int mode) 
{
	drawFileBrowserButtonEvent_t *eventData = calloc(1, sizeof(drawFileBrowserButtonEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	eventData->displayName = calloc(1, PATHNAME_MAX);
	eventData->mode = mode;
	eventData->file = calloc(1, sizeof(file_handle));
	memcpy(eventData->file, file, sizeof(file_handle));
	if(file->meta) {
		eventData->file->meta = calloc(1, sizeof(file_meta));
		memcpy(eventData->file->meta, file->meta, sizeof(file_meta));
		if(file->meta->banner) {
			eventData->file->meta->banner = memalign(32, BannerSize);
			memcpy(eventData->file->meta->banner, file->meta->banner, BannerSize);
		}
	}
	strcpy(eventData->displayName, message);
	// Hide extension when rendering certain files
	if(file->fileAttrib == IS_FILE) {
		if(endsWith(eventData->displayName,".gcm") 
			|| endsWith(eventData->displayName,".iso")
			|| endsWith(eventData->displayName,".dol")
			|| endsWith(eventData->displayName,".dol+cli")) {
				if((u32)strrchr(eventData->displayName, '.')) {
				eventData->displayName[((u32)strrchr(eventData->displayName, '.'))-((u32)eventData->displayName)] = '\0';
			}
		}
	}
	
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_FILEBROWSERBUTTON;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawEmptyBox(uiDrawObj_t *evt) {
	drawBoxEvent_t *data = (drawBoxEvent_t*)evt->data;
	
	GXColor fillColor = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //Black
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	_DrawSimpleBox( data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, fillColor, borderColor);
}

// External
uiDrawObj_t* DrawEmptyBox(int x1, int y1, int x2, int y2) 
{
	int borderSize;
	borderSize = (y2-y1) <= 30 ? 3 : 10;
	x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;
	
	drawBoxEvent_t *eventData = calloc(1, sizeof(drawBoxEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_EMPTYBOX;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawTransparentBox(uiDrawObj_t *evt) {
	drawBoxEvent_t *data = (drawBoxEvent_t*)evt->data;
	
	GXColor noColor 	= (GXColor) {0,0,0,0};
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	_DrawSimpleBox( data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, noColor, borderColor);
}

// External
uiDrawObj_t* DrawTransparentBox(int x1, int y1, int x2, int y2) 
{
	int borderSize;
	borderSize = (y2-y1) <= 30 ? 3 : 10;
	x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;

	drawBoxEvent_t *eventData = calloc(1, sizeof(drawBoxEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_TRANSPARENTBOX;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawMenuButtons(uiDrawObj_t *evt) {
	
	drawMenuButtonsEvent_t *data = (drawMenuButtonsEvent_t*)evt->data;
	
	// Draw the buttons	
	_DrawImageNow(TEX_BTNDEVICE, 40+(0*116), 430, BTNDEVICE_WIDTH,BTNDEVICE_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNSETTINGS, 40+(1*116), 430, BTNSETTINGS_WIDTH,BTNSETTINGS_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNINFO, 40+(2*116), 430, BTNINFO_WIDTH,BTNINFO_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNREFRESH, 40+(3*116), 430, BTNREFRESH_WIDTH,BTNREFRESH_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNEXIT, 40+(4*116), 430, BTNEXIT_WIDTH,BTNEXIT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	
	// Highlight selected
	int i;
	for(i=0;i<5;i++)
	{
		if(data->selection==i)
			_DrawImageNow(TEX_BTNHILIGHT, 40+(i*116), 430, BTNHILIGHT_WIDTH,BTNHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
		else
			_DrawImageNow(TEX_BTNNOHILIGHT, 40+(i*116), 430, BTNNOHILIGHT_WIDTH,BTNNOHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}
}

// External
// Buttons
uiDrawObj_t* DrawMenuButtons(int selection) 
{
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	drawMenuButtonsEvent_t *eventData = calloc(1, sizeof(drawMenuButtonsEvent_t));
	eventData->selection = selection;
	event->type = EV_MENUBUTTONS;
	event->data = eventData;
	return event;
}

void DrawUpdateProgressBar(uiDrawObj_t *evt, int percent) {
	LWP_MutexLock(_videomutex);
	drawProgressEvent_t *data = (drawProgressEvent_t*)evt->data;
	data->percent = percent;
	LWP_MutexUnlock(_videomutex);
}

void DrawUpdateProgressBarDetail(uiDrawObj_t *evt, int percent, int speed, int timestart, int timeremain) {
	LWP_MutexLock(_videomutex);
	drawProgressEvent_t *data = (drawProgressEvent_t*)evt->data;
	data->percent = percent;
	data->speed = speed;
	data->timestart = timestart;
	data->timeremain = timeremain;
	LWP_MutexUnlock(_videomutex);
}

void DrawUpdateMenuButtons(u32 selection) {
	LWP_MutexLock(_videomutex);
	drawMenuButtonsEvent_t *data = (drawMenuButtonsEvent_t*)buttonPanel->data;
	data->selection = selection;
	LWP_MutexUnlock(_videomutex);
}

// Internal
static void _DrawVertScrollBar(uiDrawObj_t *evt) {
	drawVertScrollbarEvent_t *data = (drawVertScrollbarEvent_t*)evt->data;
	int x1 = data->x;
	int x2 = data->x+data->width;
	int y1 = data->y;
	int y2 = data->y+data->height;
	int scrollStartY = y1+3 + (int)(data->height*data->scrollPercent);

	if(scrollStartY + data->scrollHeight +3 > y2)
		scrollStartY = y2-data->scrollHeight-3;
	
	GXColor fillColor = (GXColor) {46,57,104,GUI_MSGBOX_ALPHA}; 	//bluish
  	GXColor noColor = (GXColor) {0,0,0,0}; //blank
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //silver
	
	_DrawSimpleBox( x1, y1, x2-x1, y2-y1, 0, noColor, borderColor);
	
	_DrawSimpleBox( x1, scrollStartY,
			data->width, data->scrollHeight, 0, fillColor, borderColor); 
}

// External
uiDrawObj_t* DrawVertScrollBar(int x, int y, int width, int height, float scrollPercent, int scrollHeight) {
	scrollHeight = scrollHeight < 10 ? 10:scrollHeight;
	drawVertScrollbarEvent_t *eventData = calloc(1, sizeof(drawVertScrollbarEvent_t));
	eventData->x = x;
	eventData->y = y;
	eventData->width = width;
	eventData->height = height;
	eventData->scrollPercent = scrollPercent;
	eventData->scrollHeight = scrollHeight;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_VERTSCROLLBAR;
	event->data = eventData;
	return event;
}

static uiDrawObj_t* drawParameterForArgsSelector(Parameter *param, int x, int y, int selected) {

	uiDrawObj_t* container = DrawContainer();
	char *name = &param->arg.name[0];
	char *selValue = &param->values[param->currentValueIdx].name[0];
	
	int chkWidth = 32, nameWidth = 300, gapWidth = 13, paramWidth = 120;
	// [32px 10px 250px 10px 5px 80px 5px]
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (param->enable || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	if(selected) DrawAddChild(container, DrawTransparentBox( x+chkWidth+gapWidth, y, getVideoMode()->fbWidth-52, y+30));
	DrawAddChild(container, DrawImage(param->enable ? TEX_CHECKED:TEX_UNCHECKED, x, y, 32, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	// Draw the parameter Name
	DrawAddChild(container, DrawStyledLabel(x+chkWidth+gapWidth+5, y+4, name, GetTextScaleToFitInWidth(name, nameWidth-10), false, fontColor));
	// If enabled, draw arrows indicating where in the param list we are
	if(selected && param->enable && param->num_values > 1) {
		if(param->currentValueIdx != 0) {
			DrawAddChild(container, DrawStyledLabel(x+(chkWidth+nameWidth+(gapWidth*4)), y+5, "<-", .8f, false, defaultColor));
		}
		if(param->currentValueIdx != param->num_values-1) {
			DrawAddChild(container, DrawStyledLabel(x+(chkWidth+nameWidth+paramWidth+(gapWidth*6)), y+5, "->", .8f, false, defaultColor));
		}
	}
	// Draw the current value
	DrawAddChild(container, DrawStyledLabel(x+chkWidth+nameWidth+(gapWidth*6), y+2, selValue, GetTextScaleToFitInWidth(selValue, paramWidth), false, fontColor));
	return container;
}

// External
void DrawArgsSelector(char *fileName) {
	Parameters* params = getParameters();
	int param_selection = 0;
	int params_per_page = 6;
	
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	uiDrawObj_t *container = NULL;
	while(1) {
		uiDrawObj_t *newPanel = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 460);
		sprintf(txtbuffer, "%s Parameters:", fileName);
		DrawAddChild(newPanel, DrawStyledLabel(25, 62, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, getVideoMode()->fbWidth-50), false, defaultColor));

		int j = 0;
		int current_view_start = MIN(MAX(0,param_selection-params_per_page/2),MAX(0,params->num_params-params_per_page));
		int current_view_end = MIN(params->num_params, MAX(param_selection+params_per_page/2,params_per_page));
	
		int scrollBarHeight = 90+(params_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)params->num_params);
		DrawAddChild(newPanel, DrawVertScrollBar(getVideoMode()->fbWidth-45, 120, 25, scrollBarHeight, (float)((float)param_selection/(float)(params->num_params-1)),scrollBarTabHeight));
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			DrawAddChild(newPanel, drawParameterForArgsSelector(&params->parameters[current_view_start], 25, 120+j*35, current_view_start==param_selection));
		}
		// Write about the default if there is any
		DrawAddChild(newPanel, DrawTransparentBox( 35, 350, getVideoMode()->fbWidth-35, 400));
		DrawAddChild(newPanel, DrawStyledLabel(33, 345, "Default values will be used by the DOL being loaded if a", 0.8f, false, defaultColor));
		DrawAddChild(newPanel, DrawStyledLabel(33, 365, "parameter is not enabled. Please check the documentation", 0.8f, false, defaultColor));
		DrawAddChild(newPanel, DrawStyledLabel(33, 385, "for this DOL if you are unsure of the default values.", 0.8f, false, defaultColor));
		DrawAddChild(newPanel, DrawStyledLabel(640/2, 440, "(A) Toggle Param - (Start) Load the DOL", 1.0f, true, defaultColor));
		
		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		
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
	DrawDispose(container);
}

static uiDrawObj_t* drawCheatForCheatsSelector(CheatEntry *cheat, int x, int y, int selected) {

	char *name = &cheat->name[0];
	uiDrawObj_t* container = DrawContainer();
	
	int chkWidth = 32, nameWidth = 525, gapWidth = 13;
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (cheat->enabled || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	DrawAddChild(container, DrawImage(cheat->enabled ? TEX_CHECKED:TEX_UNCHECKED, x, y, 32, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	if(selected) DrawAddChild(container, DrawTransparentBox( x+chkWidth+gapWidth, y, getVideoMode()->fbWidth-52, y+30));
	// Draw the cheat Name
	DrawAddChild(container, DrawStyledLabel(x+chkWidth+gapWidth+5, y+4, name, GetTextScaleToFitInWidth(name, nameWidth-10), false, fontColor));
	return container;
}

// External
void DrawCheatsSelector(char *fileName) {
	CheatEntries* cheats = getCheats();
	int cheat_selection = 0;
	int cheats_per_page = 6;

	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	uiDrawObj_t *container = NULL;		
	while(1) {
		uiDrawObj_t *newPanel = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 460);
		sprintf(txtbuffer, "%s Cheats:", fileName);
		DrawAddChild(newPanel, DrawStyledLabel(25, 62, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, getVideoMode()->fbWidth-50), false, defaultColor));

		int j = 0;
		int current_view_start = MIN(MAX(0,cheat_selection-cheats_per_page/2),MAX(0,cheats->num_cheats-cheats_per_page));
		int current_view_end = MIN(cheats->num_cheats, MAX(cheat_selection+cheats_per_page/2,cheats_per_page));
	
		int scrollBarHeight = 90+(cheats_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)cheats->num_cheats);
		DrawAddChild(newPanel, DrawVertScrollBar(getVideoMode()->fbWidth-45, 120, 25, scrollBarHeight, (float)((float)cheat_selection/(float)(cheats->num_cheats-1)),scrollBarTabHeight));
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			DrawAddChild(newPanel, drawCheatForCheatsSelector(&cheats->cheat[current_view_start], 25, 120+j*35, current_view_start==cheat_selection));
		}
		// Write about how many cheats are enabled
		DrawAddChild(newPanel, DrawTransparentBox( 35, 350, getVideoMode()->fbWidth-35, 410));
		
		float percent = (((float)getEnabledCheatsSize() / (float)kenobi_get_maxsize()) * 100.0f);
		sprintf(txtbuffer, "Space taken by cheats: %i/%i bytes (%.1f%% free)"
			, getEnabledCheatsSize(), kenobi_get_maxsize(), 100.0f-percent);
		DrawAddChild(newPanel, DrawStyledLabel(33, 345, txtbuffer, 0.8f, false, defaultColor));
		
		sprintf(txtbuffer, "Number of cheats enabled: %i", getEnabledCheatsCount());
		DrawAddChild(newPanel, DrawStyledLabel(33, 370, txtbuffer, 0.8f, false, defaultColor));
		
		sprintf(txtbuffer, "WiiRD Debug %s", swissSettings.wiirdDebug ? "Enabled":"Disabled");
		DrawAddChild(newPanel, DrawStyledLabel(33, 395, txtbuffer, 0.8f, false, defaultColor));
		DrawAddChild(newPanel, DrawStyledLabel(640/2, 440, "(A) Toggle Cheat - (X) WiiRD Debug - (B) Return", 0.9f, true, defaultColor));

		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		
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
	DrawDispose(container);
}

static void videoDrawEvent(uiDrawObj_t *videoEvent) {
	//print_gecko("Draw event: %08X (type %s)\r\n", (u32)videoEvent, typeStrings[videoEvent->type]);
	drawInit();
	switch(videoEvent->type) {
		case EV_TEXOBJ:
			_DrawTexObj(videoEvent);
			break;
		case EV_IMAGE:
			_DrawImage(videoEvent);
			break;
		case EV_MSGBOX:
			_DrawMessageBox(videoEvent);
			break;
		case EV_PROGRESS:
			_DrawProgressBar(videoEvent);
			break;
		case EV_SELECTABLEBUTTON:
			_DrawSelectableButton(videoEvent);
			break;
		case EV_EMPTYBOX:
			_DrawEmptyBox(videoEvent);
			break;
		case EV_TRANSPARENTBOX:
			_DrawTransparentBox(videoEvent);
			break;
		case EV_FILEBROWSERBUTTON:
			_DrawFileBrowserButton(videoEvent);
			break;
		case EV_VERTSCROLLBAR:
			_DrawVertScrollBar(videoEvent);
			break;
		case EV_STYLEDLABEL:
			_DrawStyledLabel(videoEvent);
			break;
		case EV_MENUBUTTONS:
			_DrawMenuButtons(videoEvent);
			break;
		case EV_TOOLTIP:
			_DrawTooltip(videoEvent);
			break;
		default:
			break;
	}
	if(videoEvent->child != NULL) {
		videoDrawEvent(videoEvent->child);
	}
}

static void markDisposed(uiDrawObj_t *evt)
{
	if(evt && evt->child && !evt->child->disposed) {
		markDisposed(evt->child);
	}
	if(evt) {
		evt->disposed = true;
	}
}

static void *videoUpdate(void *videoEventQueue) {
	GX_SetCurrentGXThread();
	
	//int frames = 0;
	//int framerate = 0;
	//u32 lasttime = gettick();
	while(threadAlive) {
		whichfb ^= 1;
		//frames++;
		LWP_MutexLock(_videomutex);
		// Mark events recursively as disposed
		uiDrawObjQueue_t *videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
		while(videoEventQueueEntry != NULL) {
			uiDrawObj_t *videoEvent = videoEventQueueEntry->event;
			if(videoEvent->disposed) {
				markDisposed(videoEvent);
			}
			videoEventQueueEntry = videoEventQueueEntry->next;
		}
		// Free events
		videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
		while(videoEventQueueEntry != NULL) {
			uiDrawObj_t *videoEvent = videoEventQueueEntry->event;
			// Remove any video events marked for disposal
			if(videoEvent->disposed) {
				disposeEvent(videoEvent);
				videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
				continue;
			}
			videoEventQueueEntry = videoEventQueueEntry->next;
		}
		
		// Draw out every event
		videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
		while(videoEventQueueEntry != NULL) {
			uiDrawObj_t *videoEvent = videoEventQueueEntry->event;
			videoDrawEvent(videoEvent);
			videoEventQueueEntry = videoEventQueueEntry->next;
		}
		//if(ticks_to_millisecs(gettick() - lasttime) >= 1000) {
		//	framerate = frames;
		//	frames = 0;
		//	lasttime = gettick();
		//}
		char fps[64];
		memset(fps,0,64);
		/*sprintf(fps, "fps %i", framerate);
		drawString(10, 10, fps, 1.0f, false, (GXColor){255,255,255,255});*/
		
		time_t curtime;
		time(&curtime);
		struct tm *info = localtime( &curtime );
		strftime(fps, 80, "%Y-%m-%d - %H:%M:%S", info);
		drawString(420, 30, fps, 0.55f, false, (GXColor){255,255,255,255});

		//Copy EFB->XFB
		GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
		GX_CopyDisp(xfb[whichfb],GX_TRUE);
		GX_Flush();

		LWP_MutexUnlock(_videomutex);
		VIDEO_SetNextFramebuffer(xfb[whichfb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}
	return NULL;
}

uiDrawObj_t* DrawPublish(uiDrawObj_t *evt)
{
	LWP_MutexLock(_videomutex);
	uiDrawObj_t* event = addVideoEvent(evt);
	LWP_MutexUnlock(_videomutex);
	return event;
}

void DrawAddChild(uiDrawObj_t *parent, uiDrawObj_t *child)
{
	LWP_MutexLock(_videomutex);
	//print_gecko("Added a new child event %08X (type %s)\r\n", (u32)child, typeStrings[child->type]);
	uiDrawObj_t *current = parent;
    while (current->child != NULL) {
        current = current->child;
    }
	current->child = child;
	child->disposed = false;
	//print_gecko("Add child %08X (type %s) to parent %08X (type %s)\r\n", 
	//	(u32)child, typeStrings[child->type], (u32)parent, typeStrings[parent->type]);
	LWP_MutexUnlock(_videomutex);
}

void DrawDispose(uiDrawObj_t *evt)
{
	LWP_MutexLock(_videomutex);
	evt->disposed = true;
	LWP_MutexUnlock(_videomutex);
}

void DrawInit() {
	init_textures();
	uiDrawObj_t *container = DrawContainer();
	DrawAddChild(container, DrawImage(TEX_BACKDROP, 0, 0, 640, 480, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	DrawAddChild(container, DrawStyledLabel(40,27, "Swiss v0.4", 1.5f, false, defaultColor));
	sprintf(fbTextBuffer, "commit: %s rev: %s", GITREVISION, GITVERSION);
	DrawAddChild(container, DrawStyledLabel(210,60, fbTextBuffer, 0.55f, false, defaultColor));
	buttonPanel = DrawMenuButtons(-1);
	DrawAddChild(container, buttonPanel);
	DrawPublish(container);
	LWP_MutexInit(&_videomutex, 0);
	threadAlive = 1;
	LWP_CreateThread(&video_thread, videoUpdate, videoEventQueue, video_thread_stack, VIDEO_STACK_SIZE, VIDEO_PRIORITY);
}

void DrawShutdown() {
	LWP_MutexDestroy(_videomutex);
	threadAlive = 0;
	LWP_JoinThread(video_thread, NULL);
	GX_SetCurrentGXThread();
}
