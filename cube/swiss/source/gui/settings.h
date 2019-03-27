#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "config.h"
#include "deviceHandler.h"

enum PAGES {
	PAGE_GLOBAL=0,
	PAGE_ADVANCED,
	PAGE_GAME
};
#define PAGE_MIN (PAGE_GLOBAL)
#define PAGE_MAX (PAGE_GAME)

enum SETTINGS_GLOBAL {
	SET_SYS_SOUND=0,
	SET_SCREEN_POS,
	SET_SYS_LANG,
	SET_EXI_SPEED,
	SET_SWISS_VIDEOMODE,
	SET_IGR,
	SET_CONFIG_DEV,
	SET_AVE_COMPAT,
	SET_PAGE_1_NEXT,
	SET_PAGE_1_SAVE,
	SET_PAGE_1_CANCEL
};
#define PAGE_GLOBAL_MAX (SET_PAGE_1_CANCEL)

enum SETTINGS_ADVANCED {
	SET_ENABLE_USBGECKODBG=0,
	SET_HIDE_UNK,
	SET_STOP_MOTOR,
	SET_WIIRDDBG,
	SET_FILE_MGMT,
	SET_ALL_CHEATS,
	SET_INIT_NET,
	SET_ENABLE_VIDPATCH,
	SET_ALTREADPATCH,
	SET_PAGE_2_BACK,
	SET_PAGE_2_NEXT,
	SET_PAGE_2_SAVE,
	SET_PAGE_2_CANCEL
};
#define PAGE_ADVANCED_MAX (SET_PAGE_2_CANCEL)

enum SETTINGS_GAME {
	SET_FORCE_VIDEOMODE=0,
	SET_HORIZ_SCALE,
	SET_VERT_OFFSET,
	SET_VERT_FILTER,
	SET_ALPHA_DITHER,
	SET_ANISO_FILTER,
	SET_WIDESCREEN,
	SET_TEXT_ENCODING,
	SET_INVERT_CAMERA,
	SET_PAGE_3_BACK,
	SET_PAGE_3_SAVE,
	SET_PAGE_3_CANCEL
};
#define PAGE_GAME_MAX (SET_PAGE_3_CANCEL)

extern char *uiVModeStr[];
extern char *gameVModeStr[];
extern char *forceHScaleStr[];
extern char *forceVFilterStr[];
extern char *forceWidescreenStr[];
extern char *forceEncodingStr[];
extern char *invertCStickStr[];
extern char *igrTypeStr[];
extern char *aveCompatStr[];
int show_settings(file_handle *file, ConfigEntry *config);
void refreshSRAM();

#endif