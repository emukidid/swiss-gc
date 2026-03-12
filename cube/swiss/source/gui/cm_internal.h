/* cm_internal.h
	- Card Manager internal shared types, defines, and prototypes
	by Swiss contributors
 */

#ifndef CM_INTERNAL_H
#define CM_INTERNAL_H

#include <gccore.h>
#include <ogc/card.h>
#include "swiss.h"
#include "devices/memcard/deviceHandler-CARD.h"

// --- Constants ---

#define CARD_POLL_INTERVAL 30
#define ICON_SPEED_FAST_TICKS 4
#define ICON_SPEED_MIDDLE_TICKS 8
#define ICON_SPEED_SLOW_TICKS 12

// Grid layout
#define GRID_COLS 6
#define GRID_CELL 42
#define GRID_ICON_SIZE 32

// Dual-panel layout (640x480 screen)
// DrawEmptyBox border expands 10px outward, so visible area is BOX coords ± 10px
#define PANEL_OUTER_X 30
#define PANEL_GAP 20
#define PANEL_WIDTH 280
#define GRID_ROWS_VISIBLE_MAX 6
// Vertical layout — all positions cascade from BOX_TOP_Y
#define BOX_TOP_Y 52
#define TITLE_Y (BOX_TOP_Y + 8)
#define PANEL_TOP_Y (BOX_TOP_Y + 36)
#define GRID_TOP_Y (PANEL_TOP_Y + 18)
#define PANEL_BOTTOM_Y (GRID_TOP_Y + GRID_ROWS_VISIBLE_MAX * GRID_CELL + 2)
#define DETAIL_TOP_Y (PANEL_BOTTOM_Y + 8)
#define HINTS_Y (DETAIL_TOP_Y + 38)
#define BOX_BOTTOM_Y (HINTS_Y + 18)
#define DETAIL_BANNER_W 96
#define DETAIL_BANNER_H 32

// Import
#define MAX_GCI_FILES 64

// --- Types ---

typedef struct {
	u8 num_frames;
	u8 anim_type;	// CARD_ANIM_LOOP or CARD_ANIM_BOUNCE
	struct {
		u8 *data;
		u16 data_size;
		u8 fmt;
		u8 speed;
		GXTexObj tex;
		GXTlutObj tlut;
	} frames[CARD_MAXICONS];
} icon_anim;

typedef struct {
	char filename[CARD_FILENAMELEN + 1];
	char gamecode[5];
	char company[3];
	char game_name[33];
	char game_desc[33];
	u32 filesize;
	u16 blocks;
	u32 fileno;
	u8 permissions;
	u8 banner_fmt;
	u32 icon_addr;
	u16 icon_fmt;
	u16 icon_speed;
	u8 *banner;
	u16 banner_size;
	GXTexObj banner_tex;
	GXTlutObj banner_tlut;
	icon_anim *icon;
} card_entry;

typedef struct {
	int slot;
	card_entry entries[128];
	int num_entries;
	int cursor;
	int scroll_row;
	s32 mem_size;
	s32 sector_size;
	bool card_present;
	bool has_animated_icons;
	bool needs_reload;
	bool loading;
} cm_panel;

typedef struct {
	char path[PATHNAME_MAX];
	char display[48];
	u32 filesize;
} gci_file_entry;

// --- cm_log.c ---

void cm_log_open(void);
void cm_log(const char *fmt, ...);
void cm_log_close(void);
void cm_log_card_info(int slot, s32 mem_size, s32 sector_size, int num_entries);
void cm_log_entry(int idx, card_entry *entry);

// --- cm_card.c ---

int card_manager_read_saves(int slot, card_entry *entries, int max_entries);
void card_manager_free_graphics(card_entry *entries, int count);

// --- cm_draw.c ---

void cm_draw_init(void);
uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h);
uiDrawObj_t *card_manager_draw(cm_panel *left, cm_panel *right,
	int active_panel, u32 anim_tick);

// --- cm_transfer.c ---

bool card_manager_confirm_delete(const char *filename);
bool card_manager_delete_save(int slot, card_entry *entry);
bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size);
int card_manager_scan_gci_files(gci_file_entry *gci_files, int max_files);
int card_manager_pick_gci(gci_file_entry *files, int num_files);
bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size);

#endif
