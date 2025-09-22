#ifndef PATCHER_H
#define PATCHER_H

#include "reservedarea.h"

typedef struct FuncPattern
{
	u32 Length;
	u32 Loads;
	u32 Stores;
	u32 FCalls;
	u32 Branch;
	u32 Moves;
	const void *Patch;
	u32 PatchLength;
	const char *Name;
	u32 offsetFoundAt;
	u32 *properAddress;
} FuncPattern;

/* the SDGecko/IDE-EXI patches */
#include "sd_v1_bin.h"
#include "sd_v1_card_bin.h"
#include "sd_v1_dtk_bin.h"
#include "sd_v1_enc28j60_eth_bin.h"
#include "sd_v1_w5500_eth_bin.h"
#include "sd_v1_w6100_eth_bin.h"
#include "sd_v1_w6300_eth_bin.h"
#include "sd_v2_bin.h"
#include "sd_v2_card_bin.h"
#include "sd_v2_dtk_bin.h"
#include "sd_v2_enc28j60_eth_bin.h"
#include "sd_v2_w5500_eth_bin.h"
#include "sd_v2_w6100_eth_bin.h"
#include "sd_v2_w6300_eth_bin.h"
#include "ideexi_v1_bin.h"
#include "ideexi_v1_card_bin.h"
#include "ideexi_v1_dtk_bin.h"
#include "ideexi_v1_enc28j60_eth_bin.h"
#include "ideexi_v1_w5500_eth_bin.h"
#include "ideexi_v1_w6100_eth_bin.h"
#include "ideexi_v1_w6300_eth_bin.h"
#include "ideexi_v2_bin.h"
#include "ideexi_v2_card_bin.h"
#include "ideexi_v2_dtk_bin.h"
#include "ideexi_v2_enc28j60_eth_bin.h"
#include "ideexi_v2_w5500_eth_bin.h"
#include "ideexi_v2_w6100_eth_bin.h"
#include "ideexi_v2_w6300_eth_bin.h"
#include "dvd_bin.h"
#include "dvd_card_bin.h"
#include "usbgecko_bin.h"
#include "wkf_bin.h"
#include "wkf_card_bin.h"
#include "wkf_dtk_bin.h"
#include "fsp_bin.h"
#include "fsp_dtk_bin.h"
#include "fsp_eth_bin.h"
#include "gcloader_v1_bin.h"
#include "gcloader_v1_card_bin.h"
#include "gcloader_v2_bin.h"
#include "gcloader_v2_card_bin.h"
#include "gcloader_v2_dtk_bin.h"
#include "gcloader_v2_enc28j60_eth_bin.h"
#include "gcloader_v2_w5500_eth_bin.h"
#include "gcloader_v2_w6100_eth_bin.h"
#include "gcloader_v2_w6300_eth_bin.h"
#include "flippy_bin.h"
#include "flippy_card_bin.h"
#include "flippy_enc28j60_eth_bin.h"
#include "flippy_w5500_eth_bin.h"
#include "flippy_w6100_eth_bin.h"
#include "flippy_w6300_eth_bin.h"

/* SDK patches */
#include "backwards_memcpy_bin.h"
#include "memcpy_bin.h"
#define memcpyPatch memcpy_bin
#define memcpyPatchLength (memcpy_bin_size / sizeof(u32))
extern u8 DVDLowTestAlarmHook[];
extern u32 DVDLowTestAlarmHook_size;
#include "WriteUARTN_bin.h"
#define WriteUARTNPatch WriteUARTN_bin
#define WriteUARTNPatchLength (WriteUARTN_bin_size / sizeof(u32))
extern u8 GXAdjustForOverscanPatch[];
extern u32 GXAdjustForOverscanPatchLength;
extern u8 GXCopyDispHook[];
extern u32 GXCopyDispHook_size;
extern u8 GXInitTexObjLODHook[];
extern u32 GXInitTexObjLODHook_size;
extern u8 GXPeekARGBPatch[];
extern u32 GXPeekARGBPatchLength;
extern u8 GXPeekZPatch[];
extern u32 GXPeekZPatchLength;
extern u8 GXPokeARGBPatch[];
extern u32 GXPokeARGBPatchLength;
extern u8 GXPokeZPatch[];
extern u32 GXPokeZPatchLength;
extern u8 GXSetBlendModePatch1[];
extern u32 GXSetBlendModePatch1Length;
extern u8 GXSetBlendModePatch2[];
extern u32 GXSetBlendModePatch2Length;
extern u8 GXSetBlendModePatch3[];
extern u32 GXSetBlendModePatch3Length;
extern u8 GXSetCopyFilterPatch[];
extern u32 GXSetCopyFilterPatchLength;
extern u8 GXSetDispCopyYScalePatch1[];
extern u32 GXSetDispCopyYScalePatch1Length;
extern u8 GXSetDispCopyYScalePatch2[];
extern u32 GXSetDispCopyYScalePatch2Length;
extern u8 GXSetDispCopyYScaleStub1[];
extern u32 GXSetDispCopyYScaleStub1Length;
extern u8 GXSetDispCopyYScaleStub2[];
extern u32 GXSetDispCopyYScaleStub2Length;
extern u8 GXSetProjectionHook[];
extern u32 GXSetProjectionHook_size;
extern u8 GXSetScissorHook[];
extern u32 GXSetScissorHook_size;
extern u8 __GXSetViewportPatch[];
extern u32 __GXSetViewportPatchLength;
extern u8 __GXSetViewportTAAPatch[];
extern u32 __GXSetViewportTAAPatchLength;
extern u8 GXSetViewportJitterPatch1[];
extern u32 GXSetViewportJitterPatch1Length;
extern u8 GXSetViewportJitterPatch2[];
extern u32 GXSetViewportJitterPatch2Length;
extern u8 GXSetViewportJitterPatch3[];
extern u32 GXSetViewportJitterPatch3Length;
extern u8 GXSetViewportJitterTAAPatch1[];
extern u32 GXSetViewportJitterTAAPatch1Length;
extern u8 GXSetViewportJitterTAAPatch3[];
extern u32 GXSetViewportJitterTAAPatch3Length;
extern u8 GXSetViewportPatch1[];
extern u32 GXSetViewportPatch1Length;
extern u8 GXSetViewportPatch2[];
extern u32 GXSetViewportPatch2Length;
extern u8 GXSetViewportPatch3[];
extern u32 GXSetViewportPatch3Length;
extern u8 GXSetViewportTAAPatch3[];
extern u32 GXSetViewportTAAPatch3Length;
extern u8 MTXFrustumHook[];
extern u32 MTXFrustumHook_size;
extern u8 MTXLightFrustumHook[];
extern u32 MTXLightFrustumHook_size;
extern u8 MTXLightPerspectiveHook[];
extern u32 MTXLightPerspectiveHook_size;
extern u8 MTXOrthoHook[];
extern u32 MTXOrthoHook_size;
extern u8 MTXPerspectiveHook[];
extern u32 MTXPerspectiveHook_size;
#include "CallAlarmHandler_bin.h"
#include "CheckStatus_bin.h"
#include "CheckStatusGCDigital_bin.h"
extern u8 getTimingPatch[];
extern u32 getTimingPatchLength;
extern u8 VIConfigure240p[];
extern u32 VIConfigure240p_size;
extern u8 VIConfigure288p[];
extern u32 VIConfigure288p_size;
extern u8 VIConfigure480i[];
extern u32 VIConfigure480i_size;
extern u8 VIConfigure480p[];
extern u32 VIConfigure480p_size;
extern u8 VIConfigure540p50[];
extern u32 VIConfigure540p50_size;
extern u8 VIConfigure540p60[];
extern u32 VIConfigure540p60_size;
extern u8 VIConfigure576i[];
extern u32 VIConfigure576i_size;
extern u8 VIConfigure576p[];
extern u32 VIConfigure576p_size;
extern u8 VIConfigure960i[];
extern u32 VIConfigure960i_size;
extern u8 VIConfigure1080i50[];
extern u32 VIConfigure1080i50_size;
extern u8 VIConfigure1080i60[];
extern u32 VIConfigure1080i60_size;
extern u8 VIConfigure1152i[];
extern u32 VIConfigure1152i_size;
extern u8 VIConfigureAutop[];
extern u32 VIConfigureAutop_size;
extern u8 VIConfigureFieldMode[];
extern u32 VIConfigureFieldMode_size;
extern u8 VIConfigureHook1[];
extern u32 VIConfigureHook1_size;
extern u8 VIConfigureHook1GCVideo[];
extern u32 VIConfigureHook1GCVideo_size;
extern u8 VIConfigureHook1RT4K[];
extern u32 VIConfigureHook1RT4K_size;
extern u8 VIConfigureHook2[];
extern u32 VIConfigureHook2_size;
extern u8 VIConfigureNoYScale[];
extern u32 VIConfigureNoYScale_size;
extern u8 VIConfigurePanHook[];
extern u32 VIConfigurePanHook_size;
extern u8 VIConfigurePanHookD[];
extern u32 VIConfigurePanHookD_size;
extern u8 VIGetRetraceCountHook[];
extern u32 VIGetRetraceCountHook_size;
extern u8 VIRetraceHandlerHook[];
extern u32 VIRetraceHandlerHook_size;

enum patchIds {
	BACKWARDS_MEMCPY = 0,
	DVD_LOWTESTALARMHOOK,
	GX_COPYDISPHOOK,
	GX_INITTEXOBJLODHOOK,
	GX_SETPROJECTIONHOOK,
	GX_SETSCISSORHOOK,
	MTX_FRUSTUMHOOK,
	MTX_LIGHTFRUSTUMHOOK,
	MTX_LIGHTPERSPECTIVEHOOK,
	MTX_ORTHOHOOK,
	MTX_PERSPECTIVEHOOK,
	OS_CALLALARMHANDLER,
	OS_RESERVED,
	PAD_CHECKSTATUS,
	PAD_CHECKSTATUS_GCDIGITAL,
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
	VI_CONFIGUREAUTOP,
	VI_CONFIGUREFIELDMODE,
	VI_CONFIGUREHOOK1,
	VI_CONFIGUREHOOK1_GCVIDEO,
	VI_CONFIGUREHOOK1_RT4K,
	VI_CONFIGUREHOOK2,
	VI_CONFIGURENOYSCALE,
	VI_CONFIGUREPANHOOK,
	VI_CONFIGUREPANHOOKD,
	VI_GETRETRACECOUNTHOOK,
	VI_RETRACEHANDLERHOOK,
	PATCHES_MAX
};

#define LO_RESERVE 0x80000C00
#define HI_RESERVE 0x80003000

/* Function jump locations for the hypervisor */
#define DSI_EXCEPTION_VECTOR (u32 *)(LO_RESERVE + 0x01C)
#define INIT                 (u32 *)(LO_RESERVE + 0x104)
#define DISPATCH_INTERRUPT   (u32 *)(LO_RESERVE + 0x108)
#define IDLE_THREAD          (u32 *)(LO_RESERVE + 0x10C)
#define FINI                 (u32 *)(LO_RESERVE + 0x110)

/* Types of files we may patch */
enum patchTypes {
	PATCH_APPLOADER = 0,
	PATCH_BS2,
	PATCH_EXEC,
	PATCH_BIN,
	PATCH_DOL,
	PATCH_DOL_PRS,
	PATCH_ELF,
	PATCH_OTHER,
	PATCH_OTHER_PRS
};

int Patch_Hypervisor(u32 *data, u32 length, int dataType);
void Patch_Video(u32 *data, u32 length, int dataType);
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
int Patch_ExecutableFile(void **buffer, u32 *sizeToRead, const char *gameID, int type);
void *installPatch(int patchId);
void *installPatch2(const void *patch, u32 patchSize);
void *getPatchAddr(int patchId);
void setTopAddr(u32 addr);
u32 getTopAddr();
int install_code(int final);
u32 branchResolve(u32 *data, int dataType, u32 offsetFoundAt);

#endif
