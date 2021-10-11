#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include "swiss.h"

static GXRModeObj *vmode;		//Graphics Mode Object
void *gp_fifo = NULL;
u32 *xfb[2] = { NULL, NULL };	//Framebuffers
int whichfb = 0;				//Frame buffer toggle

#define DEFAULT_FIFO_SIZE    (256*1024)//(64*1024) minimum
//Video Modes (strings)
#define NtscIntStr     "NTSC 480i"
#define NtscDsStr      "NTSC 240p"
#define NtscProgStr    "NTSC 480p"
#define PalIntStr      "PAL 576i"
#define PalDsStr       "PAL 288p"
#define PalProgStr     "PAL 576p"
#define MpalIntStr     "PAL-M 480i"
#define MpalDsStr      "PAL-M 240p"
#define MpalProgStr    "PAL-M 480p"
#define Eurgb60IntStr  "PAL 480i"
#define Eurgb60DsStr   "PAL 240p"
#define Eurgb60ProgStr "PAL 480p"
#define UnknownVideo   "Unknown"

char *videoModeStr = NULL;

char *getVideoModeString() {
	return videoModeStr;
}

void setVideoModeString(GXRModeObj *v) {
	switch(v->viTVMode) {
		case VI_TVMODE_NTSC_INT:     videoModeStr = NtscIntStr;     break;
		case VI_TVMODE_NTSC_DS:      videoModeStr = NtscDsStr;      break;
		case VI_TVMODE_NTSC_PROG:    videoModeStr = NtscProgStr;    break;
		case VI_TVMODE_PAL_INT:      videoModeStr = PalIntStr;      break;
		case VI_TVMODE_PAL_DS:       videoModeStr = PalDsStr;       break;
		case VI_TVMODE_PAL_PROG:     videoModeStr = PalProgStr;     break;
		case VI_TVMODE_MPAL_INT:     videoModeStr = MpalIntStr;     break;
		case VI_TVMODE_MPAL_DS:      videoModeStr = MpalDsStr;      break;
		case VI_TVMODE_MPAL_PROG:    videoModeStr = MpalProgStr;    break;
		case VI_TVMODE_EURGB60_INT:  videoModeStr = Eurgb60IntStr;  break;
		case VI_TVMODE_EURGB60_DS:   videoModeStr = Eurgb60DsStr;   break;
		case VI_TVMODE_EURGB60_PROG: videoModeStr = Eurgb60ProgStr; break;
		default:                     videoModeStr = UnknownVideo;
	}
}

int getDTVStatus() {
	volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
	return swissSettings.forceDTVStatus || (vireg[55] & 1);
}

int getFontEncode() {
	volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
	return (vireg[55] >> 1) & 1;
}

GXRModeObj *getVideoModeFromSwissSetting(int uiVMode) {
	switch(uiVMode) {
		case 1:
			if(getDTVStatus()) {
				return &TVNtsc480IntDf;
			} else {
				switch(swissSettings.sramVideo) {
					case SYS_VIDEO_MPAL: return &TVMpal480IntDf;
					case SYS_VIDEO_PAL:  return &TVEurgb60Hz480IntDf;
					default:             return &TVNtsc480IntDf;
				}
			}
		case 2:
			if(getDTVStatus()) {
				return &TVNtsc480Prog;
			} else {
				switch(swissSettings.sramVideo) {
					case SYS_VIDEO_MPAL: return &TVMpal480IntDf;
					case SYS_VIDEO_PAL:  return &TVEurgb60Hz480IntDf;
					default:             return &TVNtsc480IntDf;
				}
			}
		case 3:
			return &TVPal576IntDfScale;
		case 4:
			if(getDTVStatus()) {
				return &TVPal576ProgScale;
			} else {
				return &TVPal576IntDfScale;
			}
	}
	return vmode;
}

static void ProperScanPADS() {
	PAD_ScanPads(); 
}

GXRModeObj* getVideoMode() {
	return vmode;
}

void setVideoMode(GXRModeObj *m) {
	m->viWidth = 704;
	m->viXOrigin = 8;
	VIDEO_Configure (m);
	if(xfb[0]) free(MEM_K1_TO_K0(xfb[0]));
	if(xfb[1]) free(MEM_K1_TO_K0(xfb[1]));
	xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (m));
	xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (m));
	VIDEO_ClearFrameBuffer (m, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (m, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);
	VIDEO_SetPostRetraceCallback (ProperScanPADS);
	VIDEO_SetBlack (0);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
	if (m->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
	else while (VIDEO_GetNextField())   VIDEO_WaitVSync();
	
	// setup the fifo and then init GX
	if(gp_fifo == NULL) {
		gp_fifo = MEM_K0_TO_K1 (memalign (32, DEFAULT_FIFO_SIZE));
		memset (gp_fifo, 0, DEFAULT_FIFO_SIZE);
		GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);
	}
	// clears the bg to color and clears the z buffer
	GX_SetCopyClear ((GXColor) {0, 0, 0, 0xFF}, GX_MAX_Z24);
	// init viewport
	GX_SetViewport (1.0f/24.0f, 1.0f/24.0f, m->fbWidth, m->efbHeight, 0.0f, 1.0f);
	// Set the correct y scaling for efb->xfb copy operation
	GX_SetDispCopyYScale ((f32) m->xfbHeight / (f32) m->efbHeight);
	GX_SetDispCopySrc (0, 0, m->fbWidth, m->efbHeight);
	GX_SetDispCopyDst (m->fbWidth, m->xfbHeight);
	GX_SetCopyFilter (m->aa, m->sample_pattern, GX_TRUE, m->vfilter);
	GX_SetFieldMode (m->field_rendering, ((m->viHeight == 2 * m->xfbHeight) ? GX_ENABLE : GX_DISABLE));
	if (m->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GX_SetCullMode (GX_CULL_NONE); // default in rsp init
	GX_CopyDisp (xfb[0], GX_TRUE); // This clears the efb
	GX_CopyDisp (xfb[0], GX_TRUE); // This clears the xfb
	
	vmode = m;
	setVideoModeString(vmode);
}
