#ifndef PATCHER_H
#define PATCHER_H

#include "../../reservedarea.h"

typedef struct FuncPattern
{
	u32 Length;
	u32 Loads;
	u32 Stores;
	u32 FCalls;
	u32 Branch;
	u32 Moves;
	u8 *Patch;
	u32 PatchLength;
	char *Name;
	u32 offsetFoundAt;
} FuncPattern;

/* the SDGecko/IDE-EXI patches */
extern u8 ideexi_v1_bin[];
extern u32 ideexi_v1_bin_size;
extern u8 ideexi_v2_bin[];
extern u32 ideexi_v2_bin_size;
extern u8 sd_bin[];
extern u32 sd_bin_size;
extern u8 usbgecko_bin[];
extern u32 usbgecko_bin_size;
extern u8 wkf_bin[];
extern u32 wkf_bin_size;
extern u8 dvd_bin[];
extern u32 dvd_bin_size;
extern u8 bba_bin[];
extern u32 bba_bin_size;

/* SDK patches */
extern u8 GXAdjustForOverscan[];
extern u32 GXAdjustForOverscan_length;
extern u8 GXAdjustForOverscanPatch[];
extern u32 GXAdjustForOverscanPatch_length;
extern u8 GXCopyDispHook[];
extern u32 GXCopyDispHook_length;
extern u8 GXInitTexObjLODHook[];
extern u32 GXInitTexObjLODHook_length;
extern u8 GXSetDispCopyYScalePatch1[];
extern u32 GXSetDispCopyYScalePatch1_length;
extern u8 GXSetDispCopyYScalePatch2[];
extern u32 GXSetDispCopyYScalePatch2_length;
extern u8 GXSetProjectionHook[];
extern u32 GXSetProjectionHook_length;
extern u8 GXSetScissorHook[];
extern u32 GXSetScissorHook_length;
extern u8 GXSetViewportJitterPatch[];
extern u32 GXSetViewportJitterPatch_length;
extern u8 GXSetViewportPatch[];
extern u32 GXSetViewportPatch_length;
extern u8 MTXFrustumHook[];
extern u32 MTXFrustumHook_length;
extern u8 MTXLightFrustumHook[];
extern u32 MTXLightFrustumHook_length;
extern u8 MTXLightPerspectiveHook[];
extern u32 MTXLightPerspectiveHook_length;
extern u8 MTXOrthoHook[];
extern u32 MTXOrthoHook_length;
extern u8 MTXPerspectiveHook[];
extern u32 MTXPerspectiveHook_length;
extern u8 getTimingPatch[];
extern u32 getTimingPatch_length;
extern u8 VIConfigure240p[];
extern u32 VIConfigure240p_length;
extern u8 VIConfigure288p[];
extern u32 VIConfigure288p_length;
extern u8 VIConfigure480i[];
extern u32 VIConfigure480i_length;
extern u8 VIConfigure480p[];
extern u32 VIConfigure480p_length;
extern u8 VIConfigure576i[];
extern u32 VIConfigure576i_length;
extern u8 VIConfigure576p[];
extern u32 VIConfigure576p_length;
extern u8 VIConfigure1080i50[];
extern u32 VIConfigure1080i50_length;
extern u8 VIConfigure1080i60[];
extern u32 VIConfigure1080i60_length;
extern u8 VIConfigureHook1[];
extern u32 VIConfigureHook1_length;
extern u8 VIConfigureHook2[];
extern u32 VIConfigureHook2_length;
extern u8 VIConfigurePanHook[];
extern u32 VIConfigurePanHook_length;
extern u8 VIRetraceHandlerHook[];
extern u32 VIRetraceHandlerHook_length;
extern u8 MajoraSaveRegs[];
extern u32 MajoraSaveRegs_length;
extern u8 MajoraAudioStream[];
extern u32 MajoraAudioStream_length;
extern u8 MajoraLoadRegs[];
extern u32 MajoraLoadRegs_length;

enum patchIds {
	GX_COPYDISPHOOK = 0,
	GX_INITTEXOBJLODHOOK,
	GX_SETPROJECTIONHOOK,
	GX_SETSCISSORHOOK,
	MTX_FRUSTUMHOOK,
	MTX_LIGHTFRUSTUMHOOK,
	MTX_LIGHTPERSPECTIVEHOOK,
	MTX_ORTHOHOOK,
	MTX_PERSPECTIVEHOOK,
	VI_CONFIGURE240P,
	VI_CONFIGURE288P,
	VI_CONFIGURE480I,
	VI_CONFIGURE480P,
	VI_CONFIGURE576I,
	VI_CONFIGURE576P,
	VI_CONFIGURE1080I50,
	VI_CONFIGURE1080I60,
	VI_CONFIGUREHOOK1,
	VI_CONFIGUREHOOK2,
	VI_CONFIGUREPANHOOK,
	VI_RETRACEHANDLERHOOK,
	MAJORA_SAVEREGS,
	MAJORA_AUDIOSTREAM,
	MAJORA_LOADREGS,
	PATCHES_MAX
};

#define SWISS_MAGIC 0x53574953 /* "SWIS" */

#define LO_RESERVE 		0x80001000
#define LO_RESERVE_DVD 	0x80001800

/* Function jump locations for the SD/IDE/USBGecko patch */
#define PATCHED_MEMCPY			(void*)(LO_RESERVE)
#define CALC_SPEED				(void*)(LO_RESERVE | 0x04)
#define STOP_DI_IRQ				(void*)(LO_RESERVE | 0x08)
#define READ_TRIGGER_INTERRUPT	(void*)(LO_RESERVE | 0x0C)
#define DSP_HANDLER_HOOK		(void*)(LO_RESERVE | 0x10)
#define PATCHED_MEMCPY_DBG		(void*)(LO_RESERVE | 0x14)
#define IGR_CHECK				(void*)(LO_RESERVE | 0x18)

/* Function jump locations for the DVD patch */
#define ENABLE_BACKUP_DISC 		(void*)(LO_RESERVE_DVD | 0x00)
#define READ_REAL_OR_PATCHED	(void*)(LO_RESERVE_DVD | 0x04)
#define IGR_CHECK_DVD			(void*)(LO_RESERVE_DVD | 0x08)

/* Function jump locations for the WKF/WASP patch */
#define PATCHED_MEMCPY_WKF		(void*)(LO_RESERVE)
#define PATCHED_MEMCPY_DBG_WKF	(void*)(LO_RESERVE | 0x04)
#define ADJUST_LBA_OFFSET	 	(void*)(LO_RESERVE | 0x08)
#define IGR_CHECK_WKF			(void*)(LO_RESERVE | 0x0C)

/* Function jump locations for the USBGecko patch */
#define PATCHED_MEMCPY_USB		(void*)(LO_RESERVE)
#define PATCHED_MEMCPY_DBG_USB	(void*)(LO_RESERVE | 0x04)
#define PERFORM_READ_USBGECKO	(void*)(LO_RESERVE | 0x08)

#define READ_PATCHED_ALL 		(0x111)

/* Types of files we may patch */
#define PATCH_DOL		0
#define PATCH_ELF		1
#define PATCH_LOADER	2

/* The device patches for a particular game were written to */
// -1 no device, 0 slot a, 1 slot b.
extern int savePatchDevice;

u32 Patch_DVDLowLevelReadForUSBGecko(void *addr, u32 length, int dataType);
u32 Patch_DVDLowLevelReadForWKF(void *addr, u32 length, int dataType);
u32 Patch_DVDLowLevelReadForDVD(void *addr, u32 length, int dataType);
u32 Patch_DVDLowLevelRead(void *addr, u32 length, int dataType);
void Patch_VideoMode(u32 *data, u32 length, int dataType);
void Patch_Widescreen(u32 *data, u32 length, int dataType);
int Patch_TexFilt(u32 *data, u32 length, int dataType);
int Patch_FontEnc(void *addr, u32 length);
int Patch_Fwrite(void *addr, u32 length);
int Patch_DVDReset(void *addr,u32 length);
int Patch_GameSpecific(void *addr, u32 length, const char* gameID, int dataType);
int Patch_IGR(void *data, u32 length, int dataType);
int PatchDetectLowMemUsage( u8 *dst, u32 Length, int dataType );
void *Calc_ProperAddress(void *data, int dataType, u32 offsetFoundAt);
void *Calc_Address(void *data, int dataType, u32 properAddress);
int Patch_CheatsHook(u8 *data, u32 length, u32 type);
int install_code();


#endif

