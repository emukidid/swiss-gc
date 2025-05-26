#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include "swiss.h"

#define DEFAULT_FIFO_SIZE (256*1024)//(64*1024) minimum
static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);

static GXRModeObj *vmode;		//Graphics Mode Object
u32 *xfb[2] = { NULL, NULL };	//Framebuffers
int whichfb = 0;				//Frame buffer toggle

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
#define DebugPalIntStr "NTSC 576i"
#define DebugPalDsStr  "NTSC 288p"
#define Eurgb60IntStr  "PAL 480i"
#define Eurgb60DsStr   "PAL 240p"
#define Eurgb60ProgStr "PAL 480p"
#define UnknownVideo   "Unknown"

char *getVideoModeString() {
	switch(getVideoMode()->viTVMode) {
		case VI_TVMODE_NTSC_INT:      return NtscIntStr;
		case VI_TVMODE_NTSC_DS:       return NtscDsStr;
		case VI_TVMODE_NTSC_PROG:     return NtscProgStr;
		case VI_TVMODE_PAL_INT:       return PalIntStr;
		case VI_TVMODE_PAL_DS:        return PalDsStr;
		case VI_TVMODE_PAL_PROG:      return PalProgStr;
		case VI_TVMODE_MPAL_INT:      return MpalIntStr;
		case VI_TVMODE_MPAL_DS:       return MpalDsStr;
		case VI_TVMODE_MPAL_PROG:     return MpalProgStr;
		case VI_TVMODE_DEBUG_PAL_INT: return DebugPalIntStr;
		case VI_TVMODE_DEBUG_PAL_DS:  return DebugPalDsStr;
		case VI_TVMODE_EURGB60_INT:   return Eurgb60IntStr;
		case VI_TVMODE_EURGB60_DS:    return Eurgb60DsStr;
		case VI_TVMODE_EURGB60_PROG:  return Eurgb60ProgStr;
		default:                      return UnknownVideo;
	}
}

int getTVFormat() {
	if(vmode == NULL) {
		volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
		int format = (vireg[1] >> 8) & 3;
		if(vireg[1] & 1) {
			switch(format) {
				case VI_NTSC:
					return swissSettings.sramVideo == SYS_VIDEO_PAL ? VI_EURGB60 : VI_NTSC;
				case VI_DEBUG:
					switch(swissSettings.sramVideo) {
						case SYS_VIDEO_PAL:  return swissSettings.sram60Hz ? VI_EURGB60 : VI_PAL;
						case SYS_VIDEO_MPAL: return vireg[55] & 1 ? VI_NTSC : VI_MPAL;
						default:             return VI_NTSC;
					}
			}
		} else {
			switch(swissSettings.sramVideo) {
				case SYS_VIDEO_PAL:  return swissSettings.sram60Hz ? VI_EURGB60 : VI_PAL;
				case SYS_VIDEO_MPAL: return vireg[55] & 1 ? VI_NTSC : VI_MPAL;
				default:             return VI_NTSC;
			}
		}
		return format;
	}
	switch(vmode->viTVMode >> 2) {
		case VI_PAL:
		case VI_DEBUG_PAL: return VI_PAL;
		case VI_EURGB60:   return VI_EURGB60;
		case VI_MPAL:      return VI_MPAL;
		default:           return VI_NTSC;
	}
}

int getScanMode() {
	if(vmode == NULL) {
		volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
		if(vireg[1] & 1) {
			if(vireg[54] & 1)
				return VI_PROGRESSIVE;
			else if((vireg[1] >> 2) & 1)
				return VI_NON_INTERLACE;
		} else if((vireg[55] & 1) && swissSettings.sramProgressive)
			return VI_PROGRESSIVE;
		
		return VI_INTERLACE;
	}
	return vmode->viTVMode & 3;
}

int getDTVStatus() {
	if(in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) && swissSettings.rt4kOptim) {
		return 1;
	} else if(!in_range(swissSettings.aveCompat, AVE_N_DOL_COMPAT, AVE_P_DOL_COMPAT)) {
		volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
		return (vireg[55] & 1) || swissSettings.forceDTVStatus;
	}
	return 0;
}

int getFontEncode() {
	volatile unsigned short* vireg = (volatile unsigned short*)0xCC002000;
	return (vireg[55] >> 1) & 1 ? SYS_FONTENC_SJIS : SYS_FONTENC_ANSI;
}

f32 getYScaleFactor(u16 efbHeight, u16 xfbHeight) {
	if(in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) && swissSettings.rt4kOptim)
		return 1.0f;
	return GX_GetYScaleFactor(efbHeight, xfbHeight);
}

GXRModeObj *getVideoModeFromSwissSetting(int uiVMode) {
	switch(uiVMode) {
		case 0:
			if(getDTVStatus()) {
				return swissSettings.sramVideo == SYS_VIDEO_PAL ? &TVEurgb60Hz480Prog : &TVNtsc480Prog;
			} else {
				switch(swissSettings.sramVideo) {
					case SYS_VIDEO_PAL:  return swissSettings.sram60Hz ? &TVEurgb60Hz480IntDf : &TVPal576IntDfScale;
					case SYS_VIDEO_MPAL: return &TVMpal480IntDf;
					default:             return &TVNtsc480IntDf;
				}
			}
		case 1:
			switch(swissSettings.sramVideo) {
				case SYS_VIDEO_PAL:  return &TVEurgb60Hz480IntDf;
				case SYS_VIDEO_MPAL: return getDTVStatus() ? &TVNtsc480IntDf : &TVMpal480IntDf;
				default:             return &TVNtsc480IntDf;
			}
		case 2:
			switch(swissSettings.sramVideo) {
				case SYS_VIDEO_PAL:  return &TVEurgb60Hz480Int;
				case SYS_VIDEO_MPAL: return getDTVStatus() ? &TVNtsc480Int : &TVMpal480Int;
				default:             return &TVNtsc480Int;
			}
		case 3:
			if(getDTVStatus()) {
				return swissSettings.sramVideo == SYS_VIDEO_PAL ? &TVEurgb60Hz480Prog : &TVNtsc480Prog;
			} else {
				switch(swissSettings.sramVideo) {
					case SYS_VIDEO_PAL:  return &TVEurgb60Hz480IntDf;
					case SYS_VIDEO_MPAL: return &TVMpal480IntDf;
					default:             return &TVNtsc480IntDf;
				}
			}
		case 4:
			return &TVPal576IntDfScale;
		case 5:
			return &TVPal576IntScale;
		case 6:
			return getDTVStatus() ? &TVPal576ProgScale : &TVPal576IntDfScale;
	}
	return getVideoMode();
}

static void ProperScanPADS(u32 retrace) {
	PAD_ScanPads();
}

GXRModeObj* getVideoMode() {
	if(vmode == NULL) {
		if(getScanMode() == VI_PROGRESSIVE) {
			switch(getTVFormat()) {
				case VI_PAL:
				case VI_DEBUG_PAL: return &TVPal576ProgScale;
				case VI_EURGB60:   return &TVEurgb60Hz480Prog;
				case VI_MPAL:      return &TVMpal480Prog;
				default:           return &TVNtsc480Prog;
			}
		} else {
			switch(getTVFormat()) {
				case VI_PAL:
				case VI_DEBUG_PAL: return &TVPal576IntDfScale;
				case VI_EURGB60:   return &TVEurgb60Hz480IntDf;
				case VI_MPAL:      return &TVMpal480IntDf;
				default:           return &TVNtsc480IntDf;
			}
		}
	}
	return vmode;
}

void updateVideoMode(GXRModeObj *m) {
	if(swissSettings.aveCompat == AVE_N_DOL_COMPAT) {
		switch(m->viTVMode) {
			case VI_TVMODE_PAL_INT: m->viTVMode = VI_TVMODE_DEBUG_PAL_INT; break;
			case VI_TVMODE_PAL_DS:  m->viTVMode = VI_TVMODE_DEBUG_PAL_DS;  break;
		}
	} else {
		switch(m->viTVMode) {
			case VI_TVMODE_DEBUG_PAL_INT: m->viTVMode = VI_TVMODE_PAL_INT; break;
			case VI_TVMODE_DEBUG_PAL_DS:  m->viTVMode = VI_TVMODE_PAL_DS;  break;
		}
	}
	if(in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT) && swissSettings.rt4kOptim) {
		m->viWidth = m->fbWidth;
		m->viXOrigin = 40;
	} else {
		m->viWidth = 704;
		m->viXOrigin = 8;
	}
	VIDEO_Init ();
	VIDEO_Configure (m);
	VIDEO_Flush ();
}

void setVideoMode(GXRModeObj *m) {
	updateVideoMode(m);
	GX_AbortFrame ();
	free(xfb[0]);
	free(xfb[1]);
	xfb[0] = (u32 *) SYS_AllocateFramebuffer (m);
	xfb[1] = (u32 *) SYS_AllocateFramebuffer (m);
	VIDEO_ClearFrameBuffer (m, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (m, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);
	VIDEO_SetPostRetraceCallback (ProperScanPADS);
	VIDEO_SetBlack (false);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
	VIDEO_WaitVSync ();
	
	// init GX
	GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);
	// clears the bg to color and clears the z buffer
	GX_SetCopyClear ((GXColor) {0, 0, 0, 0xFF}, GX_MAX_Z24);
	// init viewport
	GX_SetViewport (1.0f/24.0f, 1.0f/24.0f, m->fbWidth, m->efbHeight, 0.0f, 1.0f);
	// Set the correct y scaling for efb->xfb copy operation
	GX_SetDispCopyYScale (GX_GetYScaleFactor (m->efbHeight, m->xfbHeight));
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
}
