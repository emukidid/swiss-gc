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
#define GRID_CELL 40
#define GRID_ICON_SIZE 32

// Shared layout (640x480 screen)
// DrawEmptyBox border expands 10px outward, so visible area is BOX coords ± 10px
#define BOX_X1 30
#define BOX_X2 (640 - BOX_X1)
#define BOX_INNER_W (BOX_X2 - BOX_X1)
#define PANEL_GAP 16

// Vertical layout — all positions cascade from BOX_TOP_Y
#define BOX_TOP_Y 52
#define TITLE_Y (BOX_TOP_Y + 8)
#define PANEL_TOP_Y (BOX_TOP_Y + 36)
#define GRID_TOP_Y (PANEL_TOP_Y + 18)
#define GRID_ROWS_VISIBLE_MAX 7
#define PANEL_BOTTOM_Y (GRID_TOP_Y + GRID_ROWS_VISIBLE_MAX * GRID_CELL + 2)
#define BOX_BOTTOM_Y (PANEL_BOTTOM_Y + 31)
#define HINTS_Y (PANEL_BOTTOM_Y + 17)

// Cards view: two equal-width panels
#define PANEL_WIDTH ((BOX_INNER_W - PANEL_GAP) / 2)

// Import
#define MAX_GCI_FILES 64

// --- Types ---

// Heap-allocated texture bundle — stable address, independent of card_entry lifetime
typedef struct {
	GXTexObj  tex;
	GXTlutObj tlut;
	u8       *pixels;
	u16       pixel_size;
	u8        fmt;
} cm_tex_snapshot;

typedef struct {
	u8 num_frames;
	u8 anim_type;	// CARD_ANIM_LOOP or CARD_ANIM_BOUNCE
	struct {
		cm_tex_snapshot *snap;
		u8 speed;
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
	u32 time;		// GC epoch: seconds since 2000-01-01
	u8 banner_fmt;
	u32 icon_addr;
	u16 icon_fmt;
	u16 icon_speed;
	cm_tex_snapshot *banner_snap;
	icon_anim *icon;
} card_entry;

// Panel source type
typedef enum {
	CM_SRC_PHYSICAL,	// Physical card via CARD_* API
	CM_SRC_VMC,			// Virtual memory card .raw file
} cm_source_type;

typedef struct {
	cm_source_type source;
	int slot;
	char label[32];		// Display label for panel header (e.g. "Slot A", "VMC: name")
	char vmc_path[PATHNAME_MAX];	// Path to .raw file (for CM_SRC_VMC)
	card_entry entries[128];
	int num_entries;
	int cursor;
	int prev_cursor;	// Previous cursor position (for animation reset)
	u32 anim_start;		// Tick when current icon was selected
	int scroll_row;
	s32 mem_size;
	s32 sector_size;
	bool card_present;
	bool has_animated_icons;
	bool needs_reload;
	bool loading;
	u8 activity;	// CM_ACTIVITY_*
} cm_panel;

// Panel activity states (for LED indicators)
#define CM_ACTIVITY_IDLE  0
#define CM_ACTIVITY_READ  1
#define CM_ACTIVITY_WRITE 2

// VMC file entry for picker
#define MAX_VMC_FILES 16
typedef struct {
	char path[PATHNAME_MAX];
	char label[32];
	u32 filesize;
	u32 total_blocks;
	u32 user_blocks;
} vmc_file_entry;

// GCI file entry for import picker (with parsed graphics)
typedef struct {
	char path[PATHNAME_MAX];
	u32 filesize;
	card_entry entry;	// Parsed metadata and first-frame icon
} gci_file_entry;

// Context menu action
typedef enum {
	CM_ACT_COPY,
	CM_ACT_EXPORT,
	CM_ACT_IMPORT,
	CM_ACT_DELETE,
	CM_ACT_COUNT
} cm_action;

// --- cm_card.c ---

void cm_parse_save_graphics(u8 *gfx_data, u32 gfx_len,
	u8 banner_fmt, u16 icon_fmt, u16 icon_speed, card_entry *entry);
void cm_parse_comment(u8 *file_data, u32 file_len, u32 comment_addr,
	card_entry *entry);
int card_manager_read_saves(int slot, card_entry *entries, int max_entries);
void cm_deep_copy_graphics(card_entry *dst, const card_entry *src);
void card_manager_free_graphics(card_entry *entries, int count);
cm_tex_snapshot *cm_snap_create(const u8 *pixels, u16 size, u8 fmt,
	u16 w, u16 h);
void cm_snap_retire(cm_tex_snapshot *snap);
void cm_retire_tick(void);
void cm_retire_flush(void);
void cm_gfx_cache_invalidate(const card_entry *entry);
bool cm_panel_add_from_gci(cm_panel *panel, GCI *gci, u8 *savedata, u32 save_len);
void cm_panel_remove_entry(cm_panel *panel, int index);

// --- cm_draw.c ---

void cm_draw_init(void);
uiDrawObj_t *cm_draw_message(const char *msg);
uiDrawObj_t *cm_draw_progress(const char *msg, int current, int total);
uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h);
uiDrawObj_t *cm_draw_highlight(int x, int y, int w, int h);
void cm_draw_panel_frame(uiDrawObj_t *container, bool is_vmc, int x, int y, int w, int h);
void cm_draw_title_bar(uiDrawObj_t *container, int view_mode);
void cm_draw_status_leds(uiDrawObj_t *container, cm_panel *panels[2]);
void cm_led_update_card_status(bool slot_a, bool slot_b);
void cm_led_scan_vmc(void);
void cm_led_begin(cm_panel *panels[2]);
void cm_led_end(void);
void cm_led_show(const char *msg);
void cm_led_hide(void);
void cm_led_pulse(void);
void cm_led_progress(const char *msg, int current, int total);
uiDrawObj_t *card_manager_draw(cm_panel *left, cm_panel *right,
	int active_panel, u32 anim_tick);
int cm_context_menu(const char *items[], const bool enabled[], int count);

// --- cm_transfer.c ---

bool cm_write_gci_to_sd(GCI *gci, u8 *savedata, u32 save_len);
bool card_manager_confirm_delete(const char *filename);
bool card_manager_delete_save(int slot, card_entry *entry);
bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size);
bool cm_read_physical_save(int slot, card_entry *entry, s32 sector_size,
	GCI *out_gci, u8 **out_data, u32 *out_len);
bool card_manager_import_gci_buf(int slot, GCI *gci, u8 *savedata,
	u32 save_len, s32 sector_size);
int card_manager_scan_gci_files(gci_file_entry *gci_files, int max_files);
void card_manager_free_gci_files(gci_file_entry *gci_files, int count);
bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size);
bool cm_backup_rename(gci_file_entry *gci);
bool cm_backup_delete(gci_file_entry *gci);
bool card_manager_backups(cm_panel *panel);

// --- cm_library.c ---

// Device indices — the 4 fixed sources the library tracks independently
#define LIB_DEV_PHYS_A  0
#define LIB_DEV_PHYS_B  1
#define LIB_DEV_VMC_A   2
#define LIB_DEV_VMC_B   3
#define LIB_NUM_DEVICES 4

// Per-device owned data (library loads independently from panels)
typedef struct {
	card_entry entries[128];
	int num_entries;
	bool present;
	bool loaded;
	bool needs_reload;
	int slot;					// CARD_SLOTA/B (physical only)
	char vmc_path[PATHNAME_MAX];
	char label[32];				// "Slot A", "MemoryCardA (USA)", etc.
	s32 mem_size;
	s32 sector_size;
	u8 activity;				// CM_ACTIVITY_*
} lib_device;

// Save source: where a save lives
typedef enum {
	LIB_SRC_SLOT_A,
	LIB_SRC_SLOT_B,
	LIB_SRC_VMC,
	LIB_SRC_GCI,		// Backup on SD
} lib_source_type;

// A single save in the unified library index
typedef struct {
	lib_source_type source;
	card_entry *entry;			// Points into lib_device.entries[] or gci_files[].entry
	char source_label[32];		// "Slot A", "Slot B", "MemoryCardA (USA)", "SD Backup"
	char display_name[48];		// Row label: filename for card/VMC, .gci basename for SD
	int device_idx;				// 0-3 = LIB_DEV_*, -1 = GCI
	int gci_idx;				// Index into gci_files array (-1 if not GCI)
} lib_save;

// A game group in the library
typedef struct {
	char gamecode[5];
	char game_name[33];
	card_entry *rep;			// Representative entry for icon display
	int first_save;				// Index into lib_saves array
	int num_saves;				// Number of saves for this game
} lib_game_group;

#define LIB_MAX_SAVES 640		// 128 per device × 4 + GCI
#define LIB_MAX_GROUPS 128
#define LIB_MAX_CARD_SAVES 64

// A unique save filename across card/VMC sources (grouped view)
typedef struct {
	char filename[CARD_FILENAMELEN + 1];
	u16 blocks;
	u32 time;				// Most recent modification time
	int save_idx[LIB_NUM_DEVICES];	// Index into all_saves per device (-1 if absent)
} lib_card_save;

// Library view state (managed from main loop, not a standalone page)
typedef struct {
	lib_device devices[LIB_NUM_DEVICES];
	gci_file_entry *gci_files;
	int num_gci;
	lib_save *saves;
	lib_game_group *groups;
	int num_saves, num_groups;
	int game_cursor, game_scroll;
	int save_cursor, save_scroll;
	int focus;				// 0=game list, 1=save list
	bool initialized;
	bool needs_rebuild;		// Deferred rebuild after cards-view mutations
	int save_indices[LIB_MAX_SAVES];
	int save_count;
	lib_game_group *sel_group;
	// Grouped card/VMC saves and separate GCI list for selected game
	lib_card_save card_saves[LIB_MAX_CARD_SAVES];
	int num_card_saves;
	int gci_save_indices[LIB_MAX_SAVES];
	int num_gci_saves;
	int total_rows;			// card rows + divider(1) + gci rows
} lib_state;

#define LIB_INPUT_NONE   0
#define LIB_INPUT_REDRAW 1
#define LIB_INPUT_EXIT   2

bool lib_state_init(lib_state *st);
void lib_state_rebuild(lib_state *st);
void lib_reload_device(lib_state *st, int dev_idx);
void lib_poll_cards(lib_state *st);
void lib_rebuild_index(lib_state *st);
uiDrawObj_t *lib_draw_view(lib_state *st, u32 anim_tick);
int lib_handle_input(lib_state *st, cm_panel *panels[2], u16 btns, s8 stickY);
void lib_state_free(lib_state *st);

// --- cm_vmc.c ---

int vmc_scan_files(vmc_file_entry *out, int max);
int vmc_read_saves(const char *vmc_path, card_entry *entries, int max_entries,
	s32 *out_mem_size, s32 *out_sector_size);
bool vmc_read_save(const char *vmc_path, card_entry *entry,
	GCI *out_gci, u8 **out_data, u32 *out_len);
bool vmc_export_save(const char *vmc_path, card_entry *entry);
bool vmc_delete_save(const char *vmc_path, card_entry *entry);
bool vmc_import_save_buf(const char *vmc_path, GCI *gci,
	u8 *savedata, u32 save_len);
bool vmc_import_save(const char *vmc_path, gci_file_entry *gci_entry);

#endif
