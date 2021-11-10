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
extern u8 reboot_bin[];
extern u32 reboot_bin_size;
extern u8 stub_bin[];
extern u32 stub_bin_size;
extern u8 ideexi_v1_bin[];
extern u32 ideexi_v1_bin_size;
extern u8 ideexi_v1_card_bin[];
extern u32 ideexi_v1_card_bin_size;
extern u8 ideexi_v1_dtk_bin[];
extern u32 ideexi_v1_dtk_bin_size;
extern u8 ideexi_v2_bin[];
extern u32 ideexi_v2_bin_size;
extern u8 ideexi_v2_card_bin[];
extern u32 ideexi_v2_card_bin_size;
extern u8 ideexi_v2_dtk_bin[];
extern u32 ideexi_v2_dtk_bin_size;
extern u8 sd_bin[];
extern u32 sd_bin_size;
extern u8 sd_card_bin[];
extern u32 sd_card_bin_size;
extern u8 sd_dtk_bin[];
extern u32 sd_dtk_bin_size;
extern u8 usbgecko_bin[];
extern u32 usbgecko_bin_size;
extern u8 wkf_bin[];
extern u32 wkf_bin_size;
extern u8 wkf_card_bin[];
extern u32 wkf_card_bin_size;
extern u8 wkf_dtk_bin[];
extern u32 wkf_dtk_bin_size;
extern u8 dvd_bin[];
extern u32 dvd_bin_size;
extern u8 dvd_card_bin[];
extern u32 dvd_card_bin_size;
extern u8 fsp_bin[];
extern u32 fsp_bin_size;
extern u8 gcloader_v1_bin[];
extern u32 gcloader_v1_bin_size;
extern u8 gcloader_v1_card_bin[];
extern u32 gcloader_v1_card_bin_size;
extern u8 gcloader_v2_bin[];
extern u32 gcloader_v2_bin_size;
extern u8 gcloader_v2_card_bin[];
extern u32 gcloader_v2_card_bin_size;

/* SDK patches */
extern u8 DVDLowTestAlarmHook[];
extern u32 DVDLowTestAlarmHook_length;
extern u8 GXAdjustForOverscanPatch[];
extern u32 GXAdjustForOverscanPatch_length;
extern u8 GXCopyDispHook[];
extern u32 GXCopyDispHook_length;
extern u8 GXInitTexObjLODHook[];
extern u32 GXInitTexObjLODHook_length;
extern u8 GXSetBlendModePatch1[];
extern u32 GXSetBlendModePatch1_length;
extern u8 GXSetBlendModePatch2[];
extern u32 GXSetBlendModePatch2_length;
extern u8 GXSetBlendModePatch3[];
extern u32 GXSetBlendModePatch3_length;
extern u8 GXSetCopyFilterPatch[];
extern u32 GXSetCopyFilterPatch_length;
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
extern u8 WriteUARTN_bin[];
extern u32 WriteUARTN_bin_size;
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
extern u8 VIConfigure540p50[];
extern u32 VIConfigure540p50_length;
extern u8 VIConfigure540p60[];
extern u32 VIConfigure540p60_length;
extern u8 VIConfigure576i[];
extern u32 VIConfigure576i_length;
extern u8 VIConfigure576p[];
extern u32 VIConfigure576p_length;
extern u8 VIConfigure960i[];
extern u32 VIConfigure960i_length;
extern u8 VIConfigure1080i50[];
extern u32 VIConfigure1080i50_length;
extern u8 VIConfigure1080i60[];
extern u32 VIConfigure1080i60_length;
extern u8 VIConfigure1152i[];
extern u32 VIConfigure1152i_length;
extern u8 VIConfigureHook1[];
extern u32 VIConfigureHook1_length;
extern u8 VIConfigureHook1GCVideo[];
extern u32 VIConfigureHook1GCVideo_length;
extern u8 VIConfigureHook2[];
extern u32 VIConfigureHook2_length;
extern u8 VIConfigurePanHook[];
extern u32 VIConfigurePanHook_length;
extern u8 VIConfigurePanHookD[];
extern u32 VIConfigurePanHookD_length;
extern u8 VIGetRetraceCountHook[];
extern u32 VIGetRetraceCountHook_length;
extern u8 VIRetraceHandlerHook[];
extern u32 VIRetraceHandlerHook_length;

enum patchIds {
	DVD_LOWTESTALARMHOOK = 0,
	GX_COPYDISPHOOK,
	GX_INITTEXOBJLODHOOK,
	GX_SETPROJECTIONHOOK,
	GX_SETSCISSORHOOK,
	MTX_FRUSTUMHOOK,
	MTX_LIGHTFRUSTUMHOOK,
	MTX_LIGHTPERSPECTIVEHOOK,
	MTX_ORTHOHOOK,
	MTX_PERSPECTIVEHOOK,
	OS_RESERVED,
	VI_CONFIGURE240P,
	VI_CONFIGURE288P,
	VI_CONFIGURE480I,
	VI_CONFIGURE480P,
	VI_CONFIGURE540P50,
	VI_CONFIGURE540P60,
	VI_CONFIGURE576I,
	VI_CONFIGURE576P,
	VI_CONFIGURE960I,
	VI_CONFIGURE1080I50,
	VI_CONFIGURE1080I60,
	VI_CONFIGURE1152I,
	VI_CONFIGUREHOOK1,
	VI_CONFIGUREHOOK1_GCVIDEO,
	VI_CONFIGUREHOOK2,
	VI_CONFIGUREPANHOOK,
	VI_CONFIGUREPANHOOKD,
	VI_GETRETRACECOUNTHOOK,
	VI_RETRACEHANDLERHOOK,
	PATCHES_MAX
};

#define SWISS_MAGIC 0x53574953 /* "SWIS" */

#define LO_RESERVE 0x80000C00

/* Function jump locations for the hypervisor */
#define INIT			(u32 *)(LO_RESERVE + 0x104)
#define SET_IRQ_HANDLER	(u32 *)(LO_RESERVE + 0x108)
#define MASK_IRQ		(u32 *)(LO_RESERVE + 0x10C)
#define UNMASK_IRQ		(u32 *)(LO_RESERVE + 0x110)
#define IDLE_THREAD		(u32 *)(LO_RESERVE + 0x114)
#define CHECK_PAD		(u32 *)(LO_RESERVE + 0x118)
#define FINI			(u32 *)(LO_RESERVE + 0x11C)

/* Types of files we may patch */
enum patchTypes {
	PATCH_APPLOADER = 0,
	PATCH_BS2,
	PATCH_DOL,
	PATCH_DOL_PRS,
	PATCH_ELF,
	PATCH_OTHER,
	PATCH_OTHER_PRS
};

int Patch_Hypervisor(u32 *data, u32 length, int dataType);
void Patch_VideoMode(u32 *data, u32 length, int dataType);
void Patch_Widescreen(u32 *data, u32 length, int dataType);
int Patch_TexFilt(u32 *data, u32 length, int dataType);
int Patch_FontEncode(u32 *data, u32 length);
int Patch_GameSpecific(void *data, u32 length, const char *gameID, int dataType);
int Patch_GameSpecificFile(void *data, u32 length, const char *gameID, const char *fileName);
int Patch_GameSpecificHypervisor(void *data, u32 length, const char *gameID, int dataType);
void Patch_GameSpecificVideo(void *data, u32 length, const char *gameID, int dataType);
int Patch_Miscellaneous(u32 *data, u32 length, int dataType);
void *Calc_ProperAddress(void *data, int dataType, u32 offsetFoundAt);
void *Calc_Address(void *data, int dataType, u32 properAddress);
int Patch_CheatsHook(u8 *data, u32 length, u32 type);
void *installPatch(int patchId);
void *installPatch2(void *patchLocation, u32 patchSize);
void *getPatchAddr(int patchId);
void setTopAddr(u32 addr);
u32 getTopAddr();
int install_code(int final);


#endif

