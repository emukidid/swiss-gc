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
#include <ogc/semaphore.h>
#include <ogc/lwp_watchdog.h>
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

static char version[64];

// Video threading vars
#define VIDEO_STACK_SIZE (1024*8)
#define VIDEO_PRIORITY 100
static char  video_thread_stack[VIDEO_STACK_SIZE];
static lwp_t video_thread;
static sem_t _videosema = LWP_SEM_NULL;
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
	EV_MENUBUTTONS
};

char * typeStrings[] = {"TexObj", "MsgBox", "Image", "Progress", "SelectableButton", "EmptyBox", "TransparentBox",
						"FileBrowserButton", "VertScrollbar", "StyledLabel", "Container", "MenuButtons"};

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

typedef struct drawMsgBoxEvent {
	int type;
} drawMsgBoxEvent_t;

typedef struct drawProgressEvent {
	bool indeterminate;
	int percent;
	char *msg;
} drawProgressEvent_t;

typedef struct uiDrawObjQueue {
	struct uiDrawObj *event;
	struct uiDrawObjQueue *next;
} uiDrawObjQueue_t;

static uiDrawObjQueue_t *videoEventQueue = NULL;

// Add root level uiDrawObj_t
static uiDrawObj_t* addVideoEvent(uiDrawObj_t *event) {
	// First entry, make it root
	if(videoEventQueue == NULL) {
		if(_videosema != LWP_SEM_NULL)
			LWP_SemWait(_videosema);
		videoEventQueue = calloc(1, sizeof(uiDrawObjQueue_t));
		videoEventQueue->event = event;
		//print_gecko("Added first event %08X (type %s)\r\n", (u32)videoEventQueue, typeStrings[event->type]);
		return event;
	}
	
	uiDrawObjQueue_t *current = videoEventQueue;
    while (current->next != NULL) {
        current = current->next;
    }
	if(_videosema != LWP_SEM_NULL)
		LWP_SemWait(_videosema);
    current->next = calloc(1, sizeof(uiDrawObjQueue_t));
	current->next->event = event;
	event->disposed = false;
	//print_gecko("Added a new event %08X (type %s)\r\n", (u32)event, typeStrings[event->type]);
	if(_videosema != LWP_SEM_NULL)
		LWP_SemPost(_videosema);
	return event;
}

static void clearNestedEvent(uiDrawObj_t *event) {
	if(event->child) {
		clearNestedEvent(event->child);
	}
	//print_gecko("Dispose nested event %08X\r\n", (u32)event);
	if(event->data) {
		// Free any attached data
		if(event->type == EV_STYLEDLABEL) {
			if(((drawStyledLabelEvent_t*)event->data)->string) {
				free(((drawStyledLabelEvent_t*)event->data)->string);
			}
		}
		else if(event->type == EV_FILEBROWSERBUTTON) {
			if(((drawFileBrowserButtonEvent_t*)event->data)->displayName) {
				free(((drawFileBrowserButtonEvent_t*)event->data)->displayName);
			}
		}
		else if(event->type == EV_PROGRESS) {
			if(((drawProgressEvent_t*)event->data)->msg) {
				free(((drawProgressEvent_t*)event->data)->msg);
			}
		}
		free(event->data);
	}
	free(event);
}

static void disposeEvent(uiDrawObj_t *event) {
	if(videoEventQueue == NULL) {
		return;
	}

	//print_gecko("Dispose event %08X\r\n", (u32)event);
	// Remove any child uiDrawObj_t's first
	if(event->child) {
		clearNestedEvent(event->child);
		event->child = NULL;
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

static void drawInit()
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

static void drawRect(int x, int y, int width, int height, int depth, GXColor color, float s0, float s1, float t0, float t1)
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

// Internal
static void _DrawImage(uiDrawObj_t *evt) {
	drawImageEvent_t *data = (drawImageEvent_t*)evt->data;
	
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_InvalidateTexAll();

	switch(data->textureId)
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

static void _DrawImageNow(uiDrawObj_t *evt) {
	_DrawImage(evt);
	if(evt->data) {
		free(evt->data);
	}
	free(evt);
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

static void _DrawTexObjNow(uiDrawObj_t *evt) {
	_DrawTexObj(evt);
	if(evt->data) {
		free(evt->data);
	}
	free(evt);
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
	
	// Label
	int middleY = (y2+y1)/2;
	float scale = GetTextScaleToFitInWidth(data->msg, x2-x1);
	drawString(640/2, middleY, data->msg, scale, true, defaultColor);

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
		char *percentString = calloc(1, 8);
		sprintf(percentString,"%d%%", data->percent);
		drawString(640/2, middleY+30, percentString, 1.0f, true, defaultColor);
		free(percentString);
	}	
}

// External
uiDrawObj_t* DrawProgressBar(bool indeterminate, int percent, char *message) {
	drawProgressEvent_t *eventData = calloc(1, sizeof(drawProgressEvent_t));
	eventData->percent = percent;
	eventData->indeterminate = indeterminate;
	if(message && strlen(message) > 0) {
		eventData->msg = malloc(strlen(message));
		strcpy(eventData->msg, message);
	}
	else {
		eventData->msg = NULL;
	}
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_PROGRESS;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawMessageBox(uiDrawObj_t *evt) {
	drawMsgBoxEvent_t *data = (drawMsgBoxEvent_t*)evt->data;
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
	sprintf(txtbuffer, "%s", msg);	// TODO check txtbuffer usage
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
	
	//Draw Text and backfill (if selected)
	if(data->mode==B_SELECTED) {
		_DrawSimpleBox( x1, data->y1, x2-x1, data->y2-data->y1, 0, selectColor, borderColor);
	}
	else {
		_DrawSimpleBox( x1, data->y1, x2-x1, data->y2-data->y1, 0, noColor, borderColor);
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
	eventData->msg = message;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_SELECTABLEBUTTON;
	event->data = eventData;
	
	int borderSize = (mode==B_SELECTED) ? 6 : 4;
	int middleY = (((y2-y1)/2)-12)+y1;
	//determine length of the text ourselves if x2 == -1
	x1 = (x2 == -1) ? x1+2:x1;
	x2 = (x2 == -1) ? GetTextSizeInPixels(message)+x1+(borderSize*2)+6 : x2;

	if(middleY+24 > y2) {
		middleY = y1+3;
	}
	float scale = GetTextScaleToFitInWidth(message, (x2-x1)-(borderSize*2));
		
	DrawAddChild(event, DrawStyledLabel(x1 + borderSize+3, middleY, message, scale, false, defaultColor));
	return event;
}

// Internal
static void _DrawStyledLabel(uiDrawObj_t *evt) {
	drawStyledLabelEvent_t *data = (drawStyledLabelEvent_t*)evt->data;
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
		_DrawTexObjNow(DrawTexObj(&file->meta->bannerTexObj, data->x1+7, data->y1+4, 96, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	}
	if(file->meta && file->meta->regionTexId != -1 && file->meta->regionTexId != 0) {
		_DrawImageNow(DrawImage(file->meta->regionTexId, data->x2 - 37, data->y1+borderSize+2, 30,20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	}
	
	float scale = GetTextScaleToFitInWidth(data->displayName, (data->x2-data->x1-96-35)-(borderSize*2));

	drawString(data->x1 + borderSize+5+96, data->y1+borderSize, data->displayName, scale, false, defaultColor);
	
	// Print specific stats
	char* spaceTxt = calloc(1, 64);
	if(file->fileAttrib==IS_FILE) {
		if(devices[DEVICE_CUR] == &__device_wode) {
			ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
			sprintf(spaceTxt,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
		}
		else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
			sprintf(spaceTxt,"%.2fKB (%ld blocks)", (float)file->size/1024, file->size/8192);
		}
		else if(devices[DEVICE_CUR] == &__device_qoob) {
			sprintf(spaceTxt,"%.2fKB (%ld blocks)", (float)file->size/1024, file->size/0x10000);
		}
		else {
			sprintf(spaceTxt,"%.2f %s",file->size > (1024*1024) ? (float)file->size/(1024*1024):(float)file->size/1024,file->size > (1024*1024) ? "MB":"KB");
		}
		drawString(data->x2 - ((borderSize+3) + (GetTextSizeInPixels(spaceTxt)*0.45)), 
			data->y1+borderSize+24, spaceTxt, 0.45f, false, defaultColor);
		free(spaceTxt);
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
	eventData->file = file;
	strcpy(eventData->displayName, message);
	// Hide extension when rendering certain files
	if(file->fileAttrib == IS_FILE) {
		if(endsWith(eventData->displayName,".gcm") 
			|| endsWith(eventData->displayName,".iso")
			|| endsWith(eventData->displayName,".dol")
			|| endsWith(eventData->displayName,".dol+cli")) {
			eventData->displayName[((u32)strrchr(eventData->displayName, '.'))-((u32)eventData->displayName)] = '\0';
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
	_DrawImageNow(DrawImage(TEX_BTNDEVICE, 40+(0*116), 430, BTNDEVICE_WIDTH,BTNDEVICE_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	_DrawImageNow(DrawImage(TEX_BTNSETTINGS, 40+(1*116), 430, BTNSETTINGS_WIDTH,BTNSETTINGS_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	_DrawImageNow(DrawImage(TEX_BTNINFO, 40+(2*116), 430, BTNINFO_WIDTH,BTNINFO_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	_DrawImageNow(DrawImage(TEX_BTNREFRESH, 40+(3*116), 430, BTNREFRESH_WIDTH,BTNREFRESH_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	_DrawImageNow(DrawImage(TEX_BTNEXIT, 40+(4*116), 430, BTNEXIT_WIDTH,BTNEXIT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	
	// Highlight selected
	int i;
	for(i=0;i<5;i++)
	{
		if(data->selection==i)
			_DrawImageNow(DrawImage(TEX_BTNHILIGHT, 40+(i*116), 430, BTNHILIGHT_WIDTH,BTNHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		else
			_DrawImageNow(DrawImage(TEX_BTNNOHILIGHT, 40+(i*116), 430, BTNNOHILIGHT_WIDTH,BTNNOHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
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
	if(_videosema != LWP_SEM_NULL)
		LWP_SemWait(_videosema);
	drawProgressEvent_t *data = (drawProgressEvent_t*)evt->data;
	data->percent = percent;
	LWP_SemPost(_videosema);
}

void DrawUpdateMenuButtons(uiDrawObj_t *evt, int selection) {
	if(_videosema != LWP_SEM_NULL)
		LWP_SemWait(_videosema);
	drawMenuButtonsEvent_t *data = (drawMenuButtonsEvent_t*)evt->data;
	data->selection = selection;
	LWP_SemPost(_videosema);
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

static void drawParameterForArgsSelector(Parameter *param, int x, int y, int selected) {

	char *name = &param->arg.name[0];
	char *selValue = &param->values[param->currentValueIdx].name[0];
	
	int chkWidth = 32, nameWidth = 300, gapWidth = 13, paramWidth = 120;
	// [32px 10px 250px 10px 5px 80px 5px]
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (param->enable || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	if(selected) _DrawSimpleBox( x+chkWidth+gapWidth-5, y, nameWidth, 35, 0, deSelectedColor, defaultColor);
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

// External
void DrawArgsSelector(char *fileName) {
	return;
	Parameters* params = getParameters();
	int param_selection = 0;
	int params_per_page = 6;
	
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	while(1) {
		DrawEmptyBox(20,60, vmode->fbWidth-20, 460);
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

static void drawCheatForCheatsSelector(CheatEntry *cheat, int x, int y, int selected) {

	char *name = &cheat->name[0];
	
	int chkWidth = 32, nameWidth = 525, gapWidth = 13;
	// If not selected and not enabled, use greyed out font for everything
	GXColor fontColor = (cheat->enabled || selected) ? defaultColor : deSelectedColor;

	// If selected draw that it's selected
	if(selected) _DrawSimpleBox( x+chkWidth+gapWidth-5, y, nameWidth, 35, 0, deSelectedColor, defaultColor);
	DrawImage(cheat->enabled ? TEX_CHECKED:TEX_UNCHECKED, x, y, 32, 32, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	// Draw the cheat Name
	WriteFontStyled(x+chkWidth+gapWidth+5, y+4, name, GetTextScaleToFitInWidth(name, nameWidth-10), false, fontColor);
}

// External
void DrawCheatsSelector(char *fileName) {
	return;
	CheatEntries* cheats = getCheats();
	int cheat_selection = 0;
	int cheats_per_page = 6;

	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	while(1) {
		DrawEmptyBox(20,60, vmode->fbWidth-20, 460);
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
		_DrawSimpleBox( 33, 370, vmode->fbWidth-66, 20, 0, noColor, borderColor); 
		_DrawSimpleBox( 33, 370,	(int)((vmode->fbWidth-66)*multiplier), 20, 0, progressBarColor, noColor);
		sprintf(txtbuffer, "WiiRD Debug %s", swissSettings.wiirdDebug ? "Enabled":"Disabled");
		WriteFontStyled(33, 395, txtbuffer, 0.8f, false, defaultColor);
		WriteFontStyled(640/2, 440, "(A) Toggle Cheat - (X) WiiRD Debug - (B) Return", 0.9f, true, defaultColor);

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
		default:
			break;
	}
	if(videoEvent->child != NULL) {
		videoDrawEvent(videoEvent->child);
	}
}

static void *videoUpdate(void *videoEventQueue) {
	GX_SetCurrentGXThread();
	
	int frames = 0;
	int framerate = 0;
	u32 lasttime = gettick();
	while(threadAlive) {
		whichfb ^= 1;
		frames++;
		// Draw out every event
		LWP_SemWait(_videosema);
		uiDrawObjQueue_t *videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
		while(videoEventQueueEntry != NULL) {
			uiDrawObj_t *videoEvent = videoEventQueueEntry->event;
			// Remove any video events marked for disposal
			if(videoEvent->disposed) {
				disposeEvent(videoEvent);
				// not sure what to do here, start over?
				videoEventQueueEntry = (uiDrawObjQueue_t*)videoEventQueue;
				continue;
			}
			videoDrawEvent(videoEvent);
			videoEventQueueEntry = videoEventQueueEntry->next;
		}
		if(ticks_to_millisecs(gettick() - lasttime) >= 1000) {
			framerate = frames;
			frames = 0;
			lasttime = gettick();
		}
		char fps[64];
		memset(fps,0,64);
		sprintf(fps, "fps %i", framerate);
		drawString(10, 10, fps, 1.0f, false, (GXColor){255,255,255,255});
		
		time_t curtime;
		time(&curtime);
		struct tm *info = localtime( &curtime );
		strftime(fps, 80, "%Y-%m-%d - %H:%M:%S", info);
		drawString(420, 30, fps, 0.55f, false, (GXColor){255,255,255,255});

		//Copy EFB->XFB
		GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
		GX_CopyDisp(xfb[whichfb],GX_TRUE);
		GX_Flush();

		VIDEO_SetNextFramebuffer(xfb[whichfb]);
		VIDEO_Flush();
		LWP_SemPost(_videosema);
		VIDEO_WaitVSync();
	}
	return NULL;
}

uiDrawObj_t* DrawPublish(uiDrawObj_t *evt)
{
	return addVideoEvent(evt);
}

void DrawAddChild(uiDrawObj_t *parent, uiDrawObj_t *child)
{
	uiDrawObj_t *current = parent;
    while (current->child != NULL) {
        current = current->child;
    }
	if(_videosema != LWP_SEM_NULL)
		LWP_SemWait(_videosema);
	current->child = child;
	child->disposed = false;
	//print_gecko("Add child %08X (type %s) to parent %08X (type %s)\r\n", 
	//	(u32)child, typeStrings[child->type], (u32)parent, typeStrings[parent->type]);
	if(_videosema != LWP_SEM_NULL)
		LWP_SemPost(_videosema);
}

void DrawDispose(uiDrawObj_t *evt)
{
	evt->disposed = true;
}

void DrawInit() {
	uiDrawObj_t *container = DrawContainer();
	DrawAddChild(container, DrawImage(TEX_BACKDROP, 0, 0, 640, 480, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	DrawAddChild(container, DrawStyledLabel(40,27, "Swiss v0.4", 1.5f, false, defaultColor));
	sprintf(version, "commit: %s rev: %s", GITREVISION, GITVERSION);
	DrawAddChild(container, DrawStyledLabel(210,60, version, 0.55f, false, defaultColor));
	DrawPublish(container);
	LWP_SemInit(&_videosema, 1, 1);
	threadAlive = 1;
	LWP_CreateThread(&video_thread, videoUpdate, videoEventQueue, video_thread_stack, VIDEO_STACK_SIZE, VIDEO_PRIORITY);
}

void DrawShutdown() {
	threadAlive = 0;
	LWP_JoinThread(video_thread, NULL);
	GX_SetCurrentGXThread();
}
