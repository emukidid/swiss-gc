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

#define GUI_MSGBOX_ALPHA 225

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
TPLFile loadingTPL;
GXTexObj loadingTexObj;
TPLFile mp3imgTPL;
GXTexObj mp3imgTexObj;
TPLFile dolimgTPL;
GXTexObj dolimgTexObj;
TPLFile dolcliimgTPL;
GXTexObj dolcliimgTexObj;
TPLFile elfimgTPL;
GXTexObj elfimgTexObj;
TPLFile fileimgTPL;
GXTexObj fileimgTexObj;
TPLFile dirimgTPL;
GXTexObj dirimgTexObj;
TPLFile gcloaderTPL;
GXTexObj gcloaderTexObj;
TPLFile m2loaderTPL;
GXTexObj m2loaderTexObj;

static char fbTextBuffer[256];

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
	int fadingDirection;
	bool showCaret;
	int caretPosition;
	GXColor caretColor;
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
	GXColor backfill;
} drawBoxEvent_t;

typedef struct drawFileBrowserButtonEvent {
	int x1;
	int y1;
	int x2;
	int y2;
	char *displayName;
	file_handle *file;
	int mode;
	bool isCarousel;	// Draw this as a full "card" style
	int distFromMiddle;	// 0 = full, -1 spine only but large then gradually getting smaller as dist increases from 0
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
	bool miniMode;
	int miniModePos;
	int miniModeAlpha;
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
	TPL_OpenTPLFromMemory(&loadingTPL, (void *)loading_16_tpl, loading_16_tpl_size);
	TPL_GetTexture(&loadingTPL,loading_16,&loadingTexObj);
	GX_InitTexObjWrapMode(&loadingTexObj, GX_MIRROR, GX_MIRROR);
	TPL_OpenTPLFromMemory(&mp3imgTPL, (void *)mp3img_tpl, mp3img_tpl_size);
	TPL_GetTexture(&mp3imgTPL,0,&mp3imgTexObj);
	TPL_OpenTPLFromMemory(&dolimgTPL, (void *)dolimg_tpl, dolimg_tpl_size);
	TPL_GetTexture(&dolimgTPL,0,&dolimgTexObj);
	TPL_OpenTPLFromMemory(&dolcliimgTPL, (void *)dolcliimg_tpl, dolcliimg_tpl_size);
	TPL_GetTexture(&dolcliimgTPL,0,&dolcliimgTexObj);
	TPL_OpenTPLFromMemory(&elfimgTPL, (void *)elfimg_tpl, elfimg_tpl_size);
	TPL_GetTexture(&elfimgTPL,0,&elfimgTexObj);
	TPL_OpenTPLFromMemory(&fileimgTPL, (void *)fileimg_tpl, fileimg_tpl_size);
	TPL_GetTexture(&fileimgTPL,0,&fileimgTexObj);
	TPL_OpenTPLFromMemory(&dirimgTPL, (void *)dirimg_tpl, dirimg_tpl_size);
	TPL_GetTexture(&dirimgTPL,0,&dirimgTexObj);
	TPL_OpenTPLFromMemory(&gcloaderTPL, (void *)gcloaderimg_tpl, gcloaderimg_tpl_size);
	TPL_GetTexture(&gcloaderTPL,gcloaderimg,&gcloaderTexObj);
	TPL_OpenTPLFromMemory(&m2loaderTPL, (void *)m2loaderimg_tpl, m2loaderimg_tpl_size);
	TPL_GetTexture(&m2loaderTPL,0,&m2loaderTexObj);
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
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 1);
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
		case TEX_CHECKED:
			GX_LoadTexObj(&checkedTexObj, GX_TEXMAP0);
			break;
		case TEX_UNCHECKED:
			GX_LoadTexObj(&uncheckedTexObj, GX_TEXMAP0);
			break;
		case TEX_GCLOADER:
			GX_LoadTexObj(&gcloaderTexObj, GX_TEXMAP0);
			break;
		case TEX_M2LOADER:
			GX_LoadTexObj(&m2loaderTexObj, GX_TEXMAP0);
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
static void _DrawTexObjNow(GXTexObj *texObj, int x, int y, int width, int height, int depth, float s1, float s2, float t1, float t2, int centered)
{
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_InvalidateTexAll();
	GXTlutObj *tlutObj = GX_GetTexObjUserData(texObj);
	if(tlutObj) GX_LoadTlut(tlutObj, GX_GetTexObjTlut(texObj));
	GX_LoadTexObj(texObj, GX_TEXMAP0);
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

// Internal
static void _DrawTexObj(uiDrawObj_t *evt)
{
	drawTexObjEvent_t *data = (drawTexObjEvent_t*)evt->data;
	_DrawTexObjNow(data->texObj, data->x, data->y, data->width, data->height, data->depth, data->s1, data->s2, data->t1, data->t2, 0);
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
	
	if(data->miniMode) {	
		int x = 30, y = 420;
		if(data->miniModePos == PROGRESS_BOX_TOPRIGHT) {
			x = 560; y = 100;
		}
		GXColor loadingColor = (GXColor) {255,255,255,data->miniModeAlpha};
		int numSegments = (data->percent*8)/100;
		data->percent += (data->percent + 2 > 200 ? -200 : 2);
		data->miniModeAlpha = (data->miniModeAlpha + 2 > 255 ? 255 : data->miniModeAlpha + 2);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
		GX_InvalidateTexAll();
		GX_LoadTexObj(&loadingTexObj, GX_TEXMAP0);
		_drawRect(x-8, y-8, 16, 16, 0, loadingColor, (float) (numSegments)/8, (float) (numSegments+1)/8, 0.0f, 1.0f);
		drawString(x+38, y, "Loading...", 0.55f, true, loadingColor);
		return;
	}
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
				sprintf(fbTextBuffer,"%.2fMB/s", (float)data->speed/(float)(1048576));
			}
			else if(data->speed >= 1024) {
				sprintf(fbTextBuffer,"%.2fKB/s", (float)data->speed/(float)(1024));
			}
			else {
				sprintf(fbTextBuffer,"%iB/s", data->speed);
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
uiDrawObj_t* DrawProgressBar(bool indeterminate, int percent, const char *message) {
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

uiDrawObj_t* DrawProgressLoading(int miniModePos) {
	drawProgressEvent_t *eventData = calloc(1, sizeof(drawProgressEvent_t));
	eventData->miniMode = true;
	eventData->miniModePos = miniModePos;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_PROGRESS;
	event->data = eventData;
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
uiDrawObj_t* DrawMessageBox(int type, const char *msg)
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
		// Adjust font when we can't fit vertically too
		int availHeight = data->y2 - data->y1 - 4;
		if(GetFontHeight(scale) > availHeight) {
			int fullHeight = GetFontHeight(1.0f);
			scale = (float)availHeight / (float)fullHeight;
		}
		drawString(data->x1 + borderSize+3, data->y1+2, data->msg, scale, false, defaultColor);
	}
}

// External
uiDrawObj_t* DrawSelectableButton(int x1, int y1, int x2, int y2, const char *message, int mode)
{	
	drawSelectableButtonEvent_t *eventData = calloc(1, sizeof(drawSelectableButtonEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	eventData->mode = mode;
	if(message) {
		eventData->msg = strdup(message);
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
uiDrawObj_t* DrawTooltip(const char *tooltip) {
	drawTooltipEvent_t *eventData = calloc(1, sizeof(drawTooltipEvent_t));
	if(tooltip && strlen(tooltip) > 0) {
		eventData->tooltip = strdup(tooltip);
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
	
	if(data->showCaret) {
		// blink the caret
		if(data->fadingDirection) {
			data->caretColor.a += (data->fadingDirection * 20);
			if(data->caretColor.a >= 255) { data->fadingDirection = -1; data->caretColor.a = 255; }
			else if(data->caretColor.a <= 15) { data->fadingDirection = 1; data->caretColor.a = 0; }
		}		
		drawStringWithCaret(data->x, data->y, data->string, data->size, data->centered, data->color, data->caretPosition, data->caretColor);
	}
	else {
		if(data->fadingDirection) {
			data->color.a += data->fadingDirection;
			if(data->color.a >= 255) data->fadingDirection = -1;
			else if(data->color.a <= 15) data->fadingDirection = 1;
		}
		drawString(data->x, data->y, data->string, data->size, data->centered, data->color);
	}
}

// External
uiDrawObj_t* DrawStyledLabel(int x, int y, const char *string, float size, bool centered, GXColor color)
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = strdup(string);
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
uiDrawObj_t* DrawStyledLabelWithCaret(int x, int y, const char *string, float size, bool centered, GXColor color, int caretPosition)
{	
	uiDrawObj_t *event = DrawStyledLabel(x, y, string, size, centered, color);
	drawStyledLabelEvent_t *eventData = (drawStyledLabelEvent_t*)event->data;
	eventData->caretPosition = caretPosition;
	eventData->showCaret = true;
	eventData->fadingDirection = 1;
	eventData->caretColor = eventData->color;
	return event;
}

// External
uiDrawObj_t* DrawLabel(int x, int y, const char *string)
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = strdup(string);
	}
	else {
		eventData->string = NULL;
	}
	eventData->size = 1.0f;
	eventData->centered = false;
	eventData->color = defaultColor;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_STYLEDLABEL;
	event->data = eventData;
	return event;
}

// External
uiDrawObj_t* DrawFadingLabel(int x, int y, const char *string, float size)
{	
	drawStyledLabelEvent_t *eventData = calloc(1, sizeof(drawStyledLabelEvent_t));
	eventData->x = x;
	eventData->y = y;
	if(string && strlen(string) > 0) {
		eventData->string = strdup(string);
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
	
	drawFileBrowserButtonEvent_t *data = (drawFileBrowserButtonEvent_t*)evt->data;
	int borderSize = 4;	
	if(data->isCarousel) {	
		// Not selected
		GXColor noColor 	= (GXColor) {0,0,0,128};
		GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
		// Large middle entry currently being displayed, verbose info
		if(data->distFromMiddle == 0) {

			_DrawSimpleBox(data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, noColor, borderColor);
			
			int x_mid = data->x2-((data->x2-data->x1)/2);
			int bnr_width = 96;
			int bnr_height = 32;
			file_handle *file = data->file;
			// Draw banner if there is one
			if(file->meta && (file->meta->banner || file->meta->fileTypeTexObj)) {
				GXTexObj *texObj = (file->meta->banner ? &file->meta->bannerTexObj : file->meta->fileTypeTexObj);
				bnr_width *= (file->meta->banner ? 2 : 1);
				bnr_height *= (file->meta->banner ? 2 : 1);
				GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
				GX_InvalidateTexAll();
				GXTlutObj *tlutObj = GX_GetTexObjUserData(texObj);
				if(tlutObj) GX_LoadTlut(tlutObj, GX_GetTexObjTlut(texObj));
				GX_LoadTexObj(texObj, GX_TEXMAP0);
				int bnr_x = x_mid - (bnr_width/2);
				GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
					GX_Position3f32((float) bnr_x,(float) data->y1+borderSize+40, 0.0f );
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(0.0f,0.0f);
					GX_Position3f32((float) (bnr_x+bnr_width),(float) data->y1+borderSize+40,0.0f );
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(1.0f,0.0f);
					GX_Position3f32((float) (bnr_x+bnr_width),(float) (data->y1+borderSize+40+bnr_height),0.0f );
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(1.0f,1.0f);
					GX_Position3f32((float) bnr_x,(float) (data->y1+borderSize+40+bnr_height),0.0f );
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(0.0f,1.0f);
				GX_End();
				
				// Company
				float scale = GetTextScaleToFitInWidth(file->meta->bannerDesc.fullCompany,(data->x2-data->x1)-(borderSize*2));
				drawString(x_mid, data->y1+(borderSize*2)+40+bnr_height+20, file->meta->bannerDesc.fullCompany, scale, true, defaultColor);
				
				// Description
				sprintf(fbTextBuffer, "%s", file->meta->bannerDesc.description);
				char* rest = &fbTextBuffer[0];
				char* tok;
				int line = 0;
				while ((tok = strtok_r (rest,"\r\n", &rest))) {
					scale = GetTextScaleToFitInWidthWithMax(tok,(data->x2-data->x1)-(borderSize*2), !line ? 1.0f : scale);
					drawString(x_mid, data->y1+(borderSize*2)+40+bnr_height+60+(line*scale*24), tok, scale, true, defaultColor);
					line++;
				}
			}
			// Region
			if(file->meta && file->meta->regionTexObj) {
				drawString(data->x2 - borderSize - 83, data->y2-(borderSize+46), "Region:", 0.45f, false, defaultColor);
				_DrawTexObjNow(file->meta->regionTexObj, data->x2 - 43, data->y2-(borderSize+50), 30, 20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
			}
			
			// fullGameName displays some titles with incorrect encoding, use displayName instead
			float scale = GetTextScaleToFitInWidth(data->displayName, (data->x2-data->x1)-(borderSize*2));
			drawString(x_mid, data->y1+(borderSize*2)+10, data->displayName, scale, true, defaultColor);
			
			// Print specific stats
			if(file->fileAttrib==IS_FILE) {
				if(devices[DEVICE_CUR] == &__device_wode) {
					ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
					sprintf(fbTextBuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
				}
				else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
					sprintf(fbTextBuffer,"Size: %.2fKB (%d blocks)", (float)file->size/1024, file->size/8192);
				}
				else if(devices[DEVICE_CUR] == &__device_qoob) {
					sprintf(fbTextBuffer,"Size: %.2fKB (%d blocks)", (float)file->size/1024, file->size/0x10000);
				}
				else {
					sprintf(fbTextBuffer,"Size: %.2f%s", file->size > (1024*1024*1024) ? (float)file->size/(1024*1024*1024) : file->size > (1024*1024) ? (float)file->size/(1024*1024) : (float)file->size/1024, file->size > (1024*1024*1024) ? "GB" : file->size > (1024*1024) ? "MB" : "KB");
				}
				drawString(data->x2 - (borderSize + (GetTextSizeInPixels(fbTextBuffer)*0.45f)), 
					data->y2-(borderSize+24), fbTextBuffer, 0.45f, false, defaultColor);
			}
		}
		else {
			// Vertical
			_DrawSimpleBox(data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, noColor, borderColor);
			int bnr_width = 72;
			int bnr_height = 24;
			int x_start = (data->x2-((data->x2-data->x1)/2)) - (bnr_height/2);
			// Draw banner if there is one
			file_handle *file = data->file;
			if(file->meta && (file->meta->banner || file->meta->fileTypeTexObj)) {
				GXTexObj *texObj = (file->meta->banner ? &file->meta->bannerTexObj : file->meta->fileTypeTexObj);
				GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
				GX_InvalidateTexAll();
				GXTlutObj *tlutObj = GX_GetTexObjUserData(texObj);
				if(tlutObj) GX_LoadTlut(tlutObj, GX_GetTexObjTlut(texObj));
				GX_LoadTexObj(texObj, GX_TEXMAP0);
				GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
					GX_Position3f32((float)x_start,(float) data->y2-borderSize, 0.0f ); // bottom left
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(0.0f,0.0f);
					GX_Position3f32((float)x_start,(float) data->y2-bnr_width-borderSize,0.0f );	// top left
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(1.0f,0.0f);
					GX_Position3f32((float)x_start+bnr_height,(float) data->y2-bnr_width-borderSize,0.0f );	// top right
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(1.0f,1.0f);
					GX_Position3f32((float)x_start+bnr_height,(float) data->y2 - borderSize,0.0f );	// bottom right
					GX_Color4u8(255, 255, 255, 255);
					GX_TexCoord2f32(0.0f,1.0f);
				GX_End();
			}
			// fullGameName displays some titles with incorrect encoding, use displayName instead
			drawStringEllipsis(data->x1 + (data->x2-data->x1)/2 - GetFontHeight(0.5f)/2, data->y2-bnr_width-5-borderSize, data->displayName, 0.5f, false, defaultColor, true, (data->y2-bnr_width-5-borderSize) - (data->y1 + (borderSize*2)));
		}
	}
	else {
		
		// Not selected
		GXColor noColor 	= (GXColor) {0,0,0,0};
		GXColor selectColor = (GXColor) {46,57,104,GUI_MSGBOX_ALPHA}; 	//bluish
		GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver

		_DrawSimpleBox(data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 
					0, data->mode == B_SELECTED ? selectColor : noColor, borderColor);
		
		// Draw banner if there is one
		file_handle *file = data->file;
		if(file->meta && (file->meta->banner || file->meta->fileTypeTexObj)) {
			GXTexObj *texObj = (file->meta->banner ? &file->meta->bannerTexObj : file->meta->fileTypeTexObj);
			GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
			GX_InvalidateTexAll();
			GXTlutObj *tlutObj = GX_GetTexObjUserData(texObj);
			if(tlutObj) GX_LoadTlut(tlutObj, GX_GetTexObjTlut(texObj));
			GX_LoadTexObj(texObj, GX_TEXMAP0);
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
		if(file->meta && file->meta->regionTexObj) {
			_DrawTexObjNow(file->meta->regionTexObj, data->x2 - 38, data->y1+borderSize+1, 30, 20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
		}

		// fullGameName displays some titles with incorrect encoding, use displayName instead
		if(data->mode == B_SELECTED) {
			float scale = GetTextScaleToFitInWidthWithMax(data->displayName, (data->x2-data->x1-96-40)-(borderSize*2), 0.6f);
			drawString(data->x1 + borderSize+8+96, data->y1+(data->y2-data->y1)/2-GetFontHeight(scale)/2, data->displayName, scale, false, defaultColor);
		} else {
			drawStringEllipsis(data->x1 + borderSize+8+96, data->y1+(data->y2-data->y1)/2-GetFontHeight(0.6f)/2, data->displayName, 0.6f, false, defaultColor, false, (data->x2-data->x1-96-40)-(borderSize*2));
		}
		
		// Print specific stats
		if(file->fileAttrib==IS_FILE) {
			if(devices[DEVICE_CUR] == &__device_wode) {
				ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
				sprintf(fbTextBuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
			}
			else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
				sprintf(fbTextBuffer,"%.2fKB (%d blocks)", (float)file->size/1024, file->size/8192);
			}
			else if(devices[DEVICE_CUR] == &__device_qoob) {
				sprintf(fbTextBuffer,"%.2fKB (%d blocks)", (float)file->size/1024, file->size/0x10000);
			}
			else {
				sprintf(fbTextBuffer,"%.2f%s", file->size > (1024*1024*1024) ? (float)file->size/(1024*1024*1024) : file->size > (1024*1024) ? (float)file->size/(1024*1024) : (float)file->size/1024, file->size > (1024*1024*1024) ? "GB" : file->size > (1024*1024) ? "MB" : "KB");
			}
			drawString(data->x2 - ((borderSize) + (GetTextSizeInPixels(fbTextBuffer)*0.45)), 
				data->y1+borderSize+21, fbTextBuffer, 0.45f, false, defaultColor);
		}
	}
}

// External
uiDrawObj_t* DrawFileBrowserButton(int x1, int y1, int x2, int y2, const char *message, file_handle *file, int mode)
{
	drawFileBrowserButtonEvent_t *eventData = calloc(1, sizeof(drawFileBrowserButtonEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	eventData->displayName = strdup(message);
	eventData->mode = mode;
	eventData->file = calloc(1, sizeof(file_handle));
	memcpy(eventData->file, file, sizeof(file_handle));
	if(file->meta) {
		eventData->file->meta = calloc(1, sizeof(file_meta));
		memcpy(eventData->file->meta, file->meta, sizeof(file_meta));
		if(file->meta->banner) {
			// Make a copy cause we want this one to be killed off when the display event is disposed
			eventData->file->meta->banner = memalign(32, eventData->file->meta->bannerSize);
			memcpy(eventData->file->meta->banner, file->meta->banner, eventData->file->meta->bannerSize);
			DCFlushRange(eventData->file->meta->banner, eventData->file->meta->bannerSize);
			GX_InitTexObjData(&eventData->file->meta->bannerTexObj, eventData->file->meta->banner);
			if(GX_GetTexObjUserData(&eventData->file->meta->bannerTexObj) == &file->meta->bannerTlutObj) {
				GX_InitTexObjUserData(&eventData->file->meta->bannerTexObj, &eventData->file->meta->bannerTlutObj);
			}
		}
	}
	// Hide extension when rendering certain files
	if(file->fileAttrib == IS_FILE) {
		if(endsWith(eventData->displayName,".dol")
			|| endsWith(eventData->displayName,".dol+cli")
			|| endsWith(eventData->displayName,".elf")
			|| endsWith(eventData->displayName,".gci")
			|| endsWith(eventData->displayName,".gcm")
			|| endsWith(eventData->displayName,".iso")
			|| endsWith(eventData->displayName,".mp3")
			|| endsWith(eventData->displayName,".tgc")) {
			if(endsWith(eventData->displayName,".nkit.iso")) {
				eventData->displayName[((u32)strrchr(eventData->displayName, '.'))-((u32)eventData->displayName)] = '\0';
			}
			eventData->displayName[((u32)strrchr(eventData->displayName, '.'))-((u32)eventData->displayName)] = '\0';
		}
	}
	
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_FILEBROWSERBUTTON;
	event->data = eventData;
	return event;
}

uiDrawObj_t* DrawFileCarouselEntry(int x1, int y1, int x2, int y2, const char *message, file_handle *file, int distFromMiddle) {
	uiDrawObj_t* event = DrawFileBrowserButton(x1, y1, x2, y2, message, file, B_SELECTED);
	drawFileBrowserButtonEvent_t *data = (drawFileBrowserButtonEvent_t*)event->data;
	data->isCarousel = true;
	data->distFromMiddle = distFromMiddle;
	//print_gecko("message %s dist = %i x: (%i -> %i) y: (%i -> %i)\r\n", message, distFromMiddle, x1, x2, y1, y2);
	return event;
}

// Internal
static void _DrawEmptyBox(uiDrawObj_t *evt) {
	drawBoxEvent_t *data = (drawBoxEvent_t*)evt->data;
	
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	_DrawSimpleBox(data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, data->backfill, borderColor);
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
	eventData->backfill = (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}; //Black
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_EMPTYBOX;
	event->data = eventData;
	return event;
}

// External
uiDrawObj_t* DrawEmptyColouredBox(int x1, int y1, int x2, int y2, GXColor colour) 
{
	int borderSize;
	borderSize = (y2-y1) <= 30 ? 3 : 10;
	x1-=borderSize;x2+=borderSize;y1-=borderSize;y2+=borderSize;
	
	drawBoxEvent_t *eventData = calloc(1, sizeof(drawBoxEvent_t));
	eventData->x1 = x1;
	eventData->y1 = y1;
	eventData->x2 = x2;
	eventData->y2 = y2;
	eventData->backfill = colour;
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_EMPTYBOX;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawTransparentBox(uiDrawObj_t *evt) {
	drawBoxEvent_t *data = (drawBoxEvent_t*)evt->data;
	
	GXColor borderColor = (GXColor) {200,200,200,GUI_MSGBOX_ALPHA}; //Silver
	
	_DrawSimpleBox( data->x1, data->y1, data->x2-data->x1, data->y2-data->y1, 0, data->backfill, borderColor);
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
	eventData->backfill = (GXColor) {0,0,0,0};
	uiDrawObj_t *event = calloc(1, sizeof(uiDrawObj_t));
	event->type = EV_TRANSPARENTBOX;
	event->data = eventData;
	return event;
}

// Internal
static void _DrawMenuButtons(uiDrawObj_t *evt) {
	
	drawMenuButtonsEvent_t *data = (drawMenuButtonsEvent_t*)evt->data;
	
	// Highlight selected
	int i;
	for(i=0;i<5;i++)
	{
		if(data->selection==i) 
			_DrawImageNow(TEX_BTNHILIGHT, 48+(i*119), 428, BTNHILIGHT_WIDTH,BTNHILIGHT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	}

	// Draw the buttons	
	_DrawImageNow(TEX_BTNDEVICE, 32+(0*119), 428, BTNDEVICE_WIDTH,BTNDEVICE_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNSETTINGS, 32+(1*119), 428, BTNSETTINGS_WIDTH,BTNSETTINGS_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNINFO, 32+(2*119), 428, BTNINFO_WIDTH,BTNINFO_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNREFRESH, 32+(3*119), 428, BTNREFRESH_WIDTH,BTNREFRESH_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	_DrawImageNow(TEX_BTNEXIT, 32+(4*119), 428, BTNEXIT_WIDTH,BTNEXIT_HEIGHT, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
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
void DrawArgsSelector(const char *fileName) {
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
void DrawCheatsSelector(const char *fileName) {
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
		
		sprintf(txtbuffer, "Enabled: %i Total: %i", getEnabledCheatsCount(), cheats->num_cheats);
		DrawAddChild(newPanel, DrawStyledLabel(33, 370, txtbuffer, 0.8f, false, defaultColor));
		
		sprintf(txtbuffer, "WiiRD Debug %s", swissSettings.wiirdDebug ? "Enabled":"Disabled");
		DrawAddChild(newPanel, DrawStyledLabel(33, 395, txtbuffer, 0.8f, false, defaultColor));
		DrawAddChild(newPanel, DrawStyledLabel(640/2, 440, "(A) Toggle Cheat - (X) WiiRD Debug - (B) Return", 0.9f, true, defaultColor));

		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		
		while (!(PAD_ButtonsHeld(0) & (PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_TRIGGER_L|PAD_TRIGGER_R)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & (PAD_BUTTON_UP|PAD_BUTTON_DOWN)) {
			cheat_selection = btns & PAD_BUTTON_UP ? 
				((--cheat_selection < 0) ? cheats->num_cheats-1 : cheat_selection)
				:((cheat_selection + 1) % cheats->num_cheats);
		}
		if(btns & (PAD_BUTTON_LEFT|PAD_TRIGGER_L)) {
			cheat_selection = (cheat_selection ? ((cheat_selection - cheats_per_page < 0) ? 0 : cheat_selection - cheats_per_page):(cheats->num_cheats-1));
		}
		if(btns & (PAD_BUTTON_RIGHT|PAD_TRIGGER_R)) {
			cheat_selection = cheat_selection == cheats->num_cheats-1 ? 0 : ((cheat_selection + cheats_per_page > cheats->num_cheats-1) ? cheats->num_cheats-1 : (cheat_selection + cheats_per_page) % cheats->num_cheats);
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
		while (PAD_ButtonsHeld(0) & (PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_TRIGGER_L|PAD_TRIGGER_R))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(container);
}


void DrawGetTextEntry(int mode, const char *label, void *src, int size) {
	
	print_gecko("DrawGetTextEntry Modes: Alpha [%s] Numeric [%s] IP [%s] Masked [%s] File [%s]\r\n", mode & ENTRYMODE_ALPHA ? "Y":"N", mode & ENTRYMODE_NUMERIC ? "Y":"N",
																		mode & ENTRYMODE_IP ? "Y":"N", mode & ENTRYMODE_MASKED ? "Y":"N", mode & ENTRYMODE_FILE ? "Y":"N");
	char *text = calloc(1, size);
	if(mode & (ENTRYMODE_ALPHA|ENTRYMODE_IP)) {
		strncpy(text, src, size);
	}
	else {
		u16 *src_int = (u16*)src;
		itoa(*src_int, text, 10);
	}
	print_gecko("Text is [%s] size %i\r\n", text, size);
	
	int caret = strlen(text);
	int cur_row = 0;
	int cur_col = 0;
	int num_rows = 0;
	int num_per_row[5] = {0,0,0,0,0};	// number of keys per row
	int pos_for_row[5] = {0,0,0,0,0};	// X pos to start drawing keys from
	int grid_gap = 45;
	int num_txt_modes = 0;	// Number of modes the text entry chars will have, e.g. upper case, lowercase etc.
	char *txt_modes_str[] = {"lowercase", "UPPERCASE"};
	// char arrays to grab from, exact order is important
	char *ip_mode_chars = "123456789.0\b";
	char *num_mode_chars = "1234567890\b";
	char *txt_mode_chars_lower = "1234567890-=\bqwertyuiop[]\\asdfghjkl;'zxcvbnm,./`!@\a#$%";
	char *txt_mode_chars_upper = "1234567890_+\bQWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?^&*\a()~";
	char *txt_mode_file_chars_lower = "1234567890-=\bqwertyuiop[]asdfghjkl;'zxcvbnm.`!\a#$%";
	char *txt_mode_file_chars_upper = "1234567890_+\bQWERTYUIOP{}ASDFGHJKL^&ZXCVBNM,@~\a()%";
	
	int cur_txt_mode = 0;
	char *gridText = NULL;
	
	// IP mode
	if(mode & ENTRYMODE_IP) {
		// 1  2  3
		// 4  5  6
		// 7  8  9
		//[.] 0  [backspace]
		num_rows = 4;
		grid_gap = 15;
		num_per_row[0] = 3;
		pos_for_row[0] = 240;
		num_per_row[1] = 3;
		pos_for_row[1] = 240;
		num_per_row[2] = 3;
		pos_for_row[2] = 240;
		num_per_row[3] = 3;
		pos_for_row[3] = 240;
		gridText = ip_mode_chars;
	}
	
	// Number only
	if((mode & ENTRYMODE_NUMERIC) && !(mode & ENTRYMODE_ALPHA)) {
		// 1  2  3
		// 4  5  6
		// 7  8  9
		// 0  [backspace]
		num_rows = 4;
		grid_gap = 15;
		num_per_row[0] = 3;
		pos_for_row[0] = 240;
		num_per_row[1] = 3;
		pos_for_row[1] = 240;
		num_per_row[2] = 3;
		pos_for_row[2] = 240;
		num_per_row[3] = 2;
		pos_for_row[3] = 240;
		gridText = num_mode_chars;
	}
	
	// Alpha only
	else if(!(mode & ENTRYMODE_NUMERIC) && (mode & ENTRYMODE_ALPHA)) {
		// TODO if we ever have to.
	}
	
	// Alphanumeric (not file)
	else if((mode & (ENTRYMODE_NUMERIC | ENTRYMODE_ALPHA)) && !(mode & ENTRYMODE_IP) && !(mode & ENTRYMODE_FILE)) {
		/* Mode 0:
		 1234567890-=<\b aka backspace>
		 qwertyuiop[]\
		 asdfghjkl;'
		 zxcvbnm,./
		 `!@<\a aka space>#$%
		
		 Mode 1:
		 1234567890_+<\b aka backspace>
		 QWERTYUIOP{}|
		 ASDFGHJKL:"
		 ZXCVBNM<>?
		 ^&*<\a aka space>()~
		*/
		
		num_txt_modes = 2;
		num_rows = 5;
		grid_gap = 10;
		num_per_row[0] = 13;
		pos_for_row[0] = 40;
		num_per_row[1] = 13;
		pos_for_row[1] = 60;
		num_per_row[2] = 11;
		pos_for_row[2] = 100;
		num_per_row[3] = 10;
		pos_for_row[3] = 120;
		num_per_row[4] = 7;
		pos_for_row[4] = 160;
	}
	
	// Alphanumeric (file)
	else if(mode & ENTRYMODE_FILE) {
		/* Mode 0:
		 1234567890-=<\b aka backspace>
		 qwertyuiop[]\
		 asdfghjkl;'
		 zxcvbnm
		 .`!<\a aka space>#$%
		
		 Mode 1:
		 1234567890_+<\b aka backspace>
		 QWERTYUIOP{}
		 ASDFGHJKL^&
		 ZXCVBNM
		 ,@~<\a aka space>()%
		*/
		
		num_txt_modes = 2;
		num_rows = 5;
		grid_gap = 10;
		num_per_row[0] = 13;
		pos_for_row[0] = 40;
		num_per_row[1] = 12;
		pos_for_row[1] = 60;
		num_per_row[2] = 11;
		pos_for_row[2] = 100;
		num_per_row[3] = 7;
		pos_for_row[3] = 120;
		num_per_row[4] = 7;
		pos_for_row[4] = 160;
	}
	
	// Wait for any A or Left/Right presses to finish
	while ((PAD_ButtonsHeld(0) & (PAD_BUTTON_A|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT))){ VIDEO_WaitVSync (); }
	uiDrawObj_t *container = NULL;
	while(1) {
		// Double box for extra darkness
		uiDrawObj_t *newPanel = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 440);
		DrawAddChild(newPanel, DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 440));
		sprintf(txtbuffer, "%s - Please enter a value", label);
		DrawAddChild(newPanel, DrawStyledLabel(25, 62, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, getVideoMode()->fbWidth-50), false, defaultColor));

		// Draw the text entry box (TODO: mask chars if the mode says to do so)
		DrawAddChild(newPanel, DrawEmptyBox(40, 100, getVideoMode()->fbWidth-40, 140));
		DrawAddChild(newPanel, DrawStyledLabelWithCaret(320, 120, text, GetTextScaleToFitInWidth(text, getVideoMode()->fbWidth-90), true, defaultColor, caret));
		DrawAddChild(newPanel, DrawStyledLabel(320, 160, "(L/R) Cursor | (Start) Accept | (B) Discard", 0.75f, true, defaultColor));

		// Alphanumeric has a little "mode" hint at the bottom (upper/lower case set switching)
		if(mode & ENTRYMODE_ALPHA) {
			gridText = cur_txt_mode == 0 ? 
				(mode & ENTRYMODE_FILE ? txt_mode_file_chars_lower : txt_mode_chars_lower)
				: 
				(mode & ENTRYMODE_FILE ? txt_mode_file_chars_upper : txt_mode_chars_upper);
			sprintf(txtbuffer, "Press X to change to [%s], current mode is [%s]",
													txt_modes_str[(cur_txt_mode + 1 >= num_txt_modes ? 0 : cur_txt_mode + 1)], txt_modes_str[cur_txt_mode]);
			DrawAddChild(newPanel, DrawStyledLabel(25, 420, txtbuffer, 0.65f, false, defaultColor));
		}
		
		// Draw the grid
		int y = 200, dx = 0, dy = 0, i = 0;
		int button_height = 30;
		for(dy = 0; dy < num_rows; dy++) {
			int x = pos_for_row[dy];
			for(dx = 0; dx < num_per_row[dy]; dx++) {
				// Space and Backspace are special, draw them in double the width with smaller fonts
				bool isSpecial = (gridText[i] == '\a' || gridText[i] == '\b');
				int button_width = isSpecial ? (60 + grid_gap) : 30;
				DrawAddChild(newPanel, DrawEmptyColouredBox(x, y, x+button_width, y+button_height, cur_col == dx && cur_row == dy ? (GXColor) {96,107,164,GUI_MSGBOX_ALPHA} : (GXColor) {0,0,0,GUI_MSGBOX_ALPHA}));
				float fontSize = isSpecial ? 0.65f : 1.0f;
				if(isSpecial)
					sprintf(txtbuffer, "%s", gridText[i] == '\a' ? "Space" : "(Y) Back");
				else
					sprintf(txtbuffer, "%c", gridText[i]);
				DrawAddChild(newPanel, DrawStyledLabel(x+(button_width/2), y+(button_height/2), txtbuffer, fontSize, true, cur_col == dx && cur_row == dy ? defaultColor : deSelectedColor));
				x += (grid_gap + button_width);
				i++;
			}
			y += (grid_gap + button_height);
		}
		
		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		
		while (!(PAD_ButtonsHeld(0) & (PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_BUTTON_START)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		// Key nav
		if(btns & PAD_BUTTON_DOWN) {
			cur_row = (cur_row + 1 >= num_rows ? 0 : cur_row + 1);
		}
		if(btns & PAD_BUTTON_UP) {
			cur_row = (cur_row == 0 ? num_rows - 1 : cur_row - 1);
		}
		if(btns & PAD_BUTTON_LEFT) {
			cur_col = (cur_col == 0 ? num_per_row[cur_row] - 1 : cur_col - 1);
		}
		if(btns & PAD_BUTTON_RIGHT) {
			cur_col = (cur_col + 1 >= num_per_row[cur_row] ? 0 : cur_col + 1);
		}
		// If we went off the end due to a row that has less than another
		if(cur_col >= num_per_row[cur_row]) cur_col = num_per_row[cur_row] - 1;
		// Key press handling
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_Y)) {
			int char_pos = cur_col;
			for(i = 0; i < cur_row; i++)
				char_pos += num_per_row[i];
			char pressed = gridText[char_pos];
			// Handle normal character presses
			if(pressed != '\b' && !(btns & PAD_BUTTON_Y)) {
				if(caret < size && strlen(text) < size) {
					//print_gecko("Pressed [%c]\r\n", pressed);
					if(pressed == '\a')
						pressed = ' ';
					// Shuffle everything forward (don't want overwrite functionality)
					for(i = size-1; i > caret; i--) {
						text[i] = text[i-1];
					}
					text[caret] = pressed;
					caret++;
				}
			}
			// Handle deletes via Y button or "backspace" button
			else if((btns & PAD_BUTTON_Y) || (pressed == '\b')) {
				// Delete a character from the caret
				if(caret-1 >= 0) {
					for(i = caret-1; i < size; i++) {
						text[i] = text[i+1];
						text[i+1] = '\0';
					}
					text[size] = '\0';
					if(caret > 0)
						caret --;
				}
			}
		}
		// Mode switching if the set allows it (only alpha does for upper/lower)
		if(btns & PAD_BUTTON_X) {
			if(num_txt_modes > 0) {
				cur_txt_mode = (cur_txt_mode + 1 >= num_txt_modes ? 0 : cur_txt_mode + 1);
			}
		}
		if(btns & PAD_TRIGGER_L) {
			if(caret > 0) caret--;
		}
		if(btns & PAD_TRIGGER_R) {
			if(caret < strlen(text)) caret++;
		}
		if(btns & PAD_BUTTON_B) {
			break;
		}
		if(btns & PAD_BUTTON_START) {
			if(mode & (ENTRYMODE_ALPHA|ENTRYMODE_IP)) {
				strcpy(src, text);
			}
			else {
				// TODO fix this stuff at some point, we're checking based on text size rather than max val of the data type.
				u16 *src_int = (u16*)src;
				*src_int = (u16)atoi(text);
			}
			break;
		}
		while (PAD_ButtonsHeld(0) & (PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_BUTTON_START))
			{ VIDEO_WaitVSync (); }
	}
	if(text) free(text);
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
		int count = 0;
		while(videoEventQueueEntry != NULL) {
			uiDrawObj_t *videoEvent = videoEventQueueEntry->event;
			videoDrawEvent(videoEvent);
			videoEventQueueEntry = videoEventQueueEntry->next;
			count++;
			// Hacky, draw this once after the background, not after everything.
			if(count == 1) {
				//if(ticks_to_millisecs(gettick() - lasttime) >= 1000) {
				//	framerate = frames;
				//	frames = 0;
				//	lasttime = gettick();
				//}
				char fps[64];
				memset(fps,0,64);
				/*sprintf(fps, "fps %i", framerate);
				drawString(10, 10, fps, 1.0f, false, (GXColor){255,255,255,255});*/
				
				s8 cputemp = SYS_GetCoreTemperature();
				if(cputemp >= 0) {
					sprintf(fps, "%i\260C", cputemp);
					drawString(390, 33, fps, 0.55f, false, (GXColor){255,255,255,255});
				}
				time_t curtime;
				time(&curtime);
				struct tm *info = localtime( &curtime );
				strftime(fps, 80, "%Y-%m-%d - %H:%M:%S", info);
				drawString(434, 33, fps, 0.55f, false, (GXColor){255,255,255,255});
			}
		}
		
		//Copy EFB->XFB
		GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
		GX_CopyDisp(xfb[whichfb],GX_TRUE);
		GX_DrawDone();

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
	DrawAddChild(container, DrawStyledLabel(40,28, "Swiss v0.6", 1.5f, false, defaultColor));
	sprintf(fbTextBuffer, "commit: %s rev: %s", GITREVISION, GITVERSION);
	DrawAddChild(container, DrawStyledLabel(425,50, fbTextBuffer, 0.55f, false, defaultColor));
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

void DrawVideoMode(GXRModeObj *videoMode)
{
	LWP_MutexLock(_videomutex);
	setVideoMode(videoMode);
	LWP_MutexUnlock(_videomutex);
}
