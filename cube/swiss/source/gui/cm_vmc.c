/* cm_vmc.c
	- Virtual Memory Card (.raw) reader/writer for Card Manager
	  Pure binary parsing — no CARD_* API dependency.
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "cm_internal.h"
#include "config.h"
#include "IPLFontWrite.h"

// --- Raw memory card layout constants ---

// Block size is always 8192 bytes for GameCube memory cards
#define MC_BLOCK_SIZE		8192

// System area: 5 blocks (header, dirA, dirB, batA, batB)
#define MC_BLOCK_HEADER		0
#define MC_BLOCK_DIR_A		1
#define MC_BLOCK_DIR_B		2
#define MC_BLOCK_BAT_A		3
#define MC_BLOCK_BAT_B		4
#define MC_FIRST_DATA_BLOCK	5

// Directory block: 127 entries × 64 bytes, then control footer
#define MC_DIR_ENTRIES		127
#define MC_DIR_ENTRY_SIZE	64

// Directory control area (last 64 bytes of directory block)
// Offset within the directory block
#define MC_DIR_CTRL_OFF		(MC_DIR_ENTRIES * MC_DIR_ENTRY_SIZE)  // 0x1FC0
// Update counter at offset 0x3A within the control area
#define MC_DIR_UPDATE_OFF	(MC_DIR_CTRL_OFF + 0x3A)

// BAT block layout
#define MC_BAT_CHECKSUM1	0x0000	// u16
#define MC_BAT_CHECKSUM2	0x0002	// u16
#define MC_BAT_UPDATE		0x0004	// u16
#define MC_BAT_FREE_BLOCKS	0x0006	// u16
#define MC_BAT_LAST_ALLOC	0x0008	// u16
#define MC_BAT_FAT_START	0x000A	// u16 entries, indexed by block number

// FAT entry special values
#define MC_FAT_FREE			0x0000
#define MC_FAT_LAST			0xFFFF

// Card header: encoding field at offset 0x0024
#define MC_HDR_CARD_SIZE	0x0022	// u16, card size in Mbits
#define MC_HDR_ENCODING		0x0024	// u16, 0=ANSI, 1=SJIS

// --- Helpers ---
// GameCube is big-endian, same as .raw file format.
// GCI struct fields and FAT entries can be read directly without byte swapping.

// Forward declaration (defined below)
static u8 *vmc_read_range(const char *path, u32 offset, u32 len);

// --- VMC file scanning ---

int vmc_scan_files(vmc_file_entry *out, int max) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return 0;

	file_handle *dir = calloc(1, sizeof(file_handle));
	if (!dir) return 0;
	concat_path(dir->name, dev->initial->name, "swiss/saves");

	if (dev->statFile(dir)) {
		free(dir);
		return 0;
	}

	file_handle *dirEntries = NULL;
	int count = 0;

	s32 num_dir = dev->readDir(dir, &dirEntries, -1);
	if (num_dir > 0 && dirEntries) {
		for (int i = 0; i < num_dir && count < max; i++) {
			char *name = getRelativeName(dirEntries[i].name);
			int len = strlen(name);
			if (len > 4 && !strcasecmp(name + len - 4, ".raw")) {
				memset(&out[count], 0, sizeof(vmc_file_entry));
				strlcpy(out[count].path, dirEntries[i].name, PATHNAME_MAX);
				strlcpy(out[count].filename, name, sizeof(out[count].filename));
				out[count].filesize = dirEntries[i].size;

				// Build a human-readable label from filename
				// e.g. "MemoryCardA.USA.raw" → "MemoryCardA (USA)"
				char *dot1 = strchr(name, '.');
				char *dot2 = dot1 ? strchr(dot1 + 1, '.') : NULL;
				if (dot1 && dot2) {
					int base_len = dot1 - name;
					int region_len = dot2 - dot1 - 1;
					if (base_len > 24) base_len = 24;
					if (region_len > 6) region_len = 6;
					snprintf(out[count].label, sizeof(out[count].label),
						"%.*s (%.*s)", base_len, name, region_len, dot1 + 1);
					snprintf(out[count].region, sizeof(out[count].region),
						"%.*s", region_len, dot1 + 1);
					// Mark non-USA as SJIS/foreign encoding
					if (!(region_len == 3 && strncasecmp(dot1 + 1, "USA", 3) == 0))
						out[count].encoding = 1;
				} else {
					snprintf(out[count].label, sizeof(out[count].label),
						"%.31s", name);
				}

				// Compute block count from file size
				if (out[count].filesize >= MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE) {
					out[count].total_blocks = out[count].filesize / MC_BLOCK_SIZE;
					out[count].user_blocks = out[count].total_blocks - MC_FIRST_DATA_BLOCK;
				}

					count++;
			}
		}
		free(dirEntries);
	}
	free(dir);
	return count;
}

// --- VMC reading ---

// Read a range of bytes from a .raw file
static u8 *vmc_read_range(const char *path, u32 offset, u32 len) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return NULL;

	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return NULL;
	strlcpy(f->name, path, PATHNAME_MAX);

	if (dev->statFile(f)) { free(f); return NULL; }
	if (offset + len > f->size) { free(f); return NULL; }

	u8 *buf = (u8 *)memalign(32, len);
	if (!buf) { free(f); return NULL; }

	dev->seekFile(f, offset, DEVICE_HANDLER_SEEK_SET);
	s32 ret = dev->readFile(f, buf, len);
	dev->closeFile(f);
	free(f);

	if (ret != (s32)len) { free(buf); return NULL; }
	return buf;
}

// Pick active directory block (higher update counter)
static int vmc_active_dir_block(const u8 *sysarea) {
	const u16 *counterA = (const u16 *)(sysarea + MC_BLOCK_DIR_A * MC_BLOCK_SIZE + MC_DIR_UPDATE_OFF);
	const u16 *counterB = (const u16 *)(sysarea + MC_BLOCK_DIR_B * MC_BLOCK_SIZE + MC_DIR_UPDATE_OFF);
	// Higher counter wins; on tie, A is default
	// Handle wrap-around: if difference > 0x8000, the "lower" one wrapped
	if (*counterA == *counterB) return MC_BLOCK_DIR_A;
	u16 diff = *counterB - *counterA;
	return (diff > 0 && diff < 0x8000) ? MC_BLOCK_DIR_B : MC_BLOCK_DIR_A;
}

// Pick active BAT block
static int vmc_active_bat_block(const u8 *sysarea) {
	const u16 *counterA = (const u16 *)(sysarea + MC_BLOCK_BAT_A * MC_BLOCK_SIZE + MC_BAT_UPDATE);
	const u16 *counterB = (const u16 *)(sysarea + MC_BLOCK_BAT_B * MC_BLOCK_SIZE + MC_BAT_UPDATE);
	if (*counterA == *counterB) return MC_BLOCK_BAT_A;
	u16 diff = *counterB - *counterA;
	return (diff > 0 && diff < 0x8000) ? MC_BLOCK_BAT_B : MC_BLOCK_BAT_A;
}

// Read a file's contiguous data by following the FAT chain
static u8 *vmc_read_file_data(const char *vmc_path, const u8 *bat,
	u16 first_block, u32 filesize, u32 total_blocks) {

	if (first_block < MC_FIRST_DATA_BLOCK || first_block >= total_blocks)
		return NULL;
	if (filesize == 0 || filesize > (total_blocks - MC_FIRST_DATA_BLOCK) * MC_BLOCK_SIZE)
		return NULL;

	u8 *data = (u8 *)memalign(32, filesize);
	if (!data) return NULL;

	// FAT entries are indexed relative to first data block:
	// fat[0] = entry for block 5, fat[1] = block 6, etc.
	const u16 *fat = (const u16 *)(bat + MC_BAT_FAT_START);
	u16 block = first_block;
	u32 offset = 0;
	int max_iter = filesize / MC_BLOCK_SIZE + 1;

	for (int i = 0; i < max_iter && offset < filesize; i++) {
		if (block < MC_FIRST_DATA_BLOCK || block >= total_blocks) {
			free(data);
			return NULL;
		}

		u32 read_len = MC_BLOCK_SIZE;
		if (offset + read_len > filesize)
			read_len = filesize - offset;

		u8 *blk_data = vmc_read_range(vmc_path, block * MC_BLOCK_SIZE, MC_BLOCK_SIZE);
		if (!blk_data) {
			free(data);
			return NULL;
		}
		memcpy(data + offset, blk_data, read_len);
		free(blk_data);

		offset += read_len;
		if (offset >= filesize) break;

		// FAT is indexed by (block - SYSAREA), values are absolute block numbers
		u16 fat_idx = block - MC_FIRST_DATA_BLOCK;
		u16 next_block = fat[fat_idx];
		if (next_block == MC_FAT_LAST) break;
		block = next_block;
	}

	if (offset < filesize) {
		// Incomplete read — FAT chain ended early
		free(data);
		return NULL;
	}

	return data;
}

// Fast phase: read sysarea + parse directory (no per-file I/O)
// Caller must free *out_sysarea when incremental loading is complete.
int vmc_read_dir(const char *vmc_path, card_entry *entries, int max_entries,
	s32 *out_mem_size, s32 *out_sector_size,
	u8 **out_sysarea, u32 *out_total_blocks) {

	*out_sysarea = NULL;
	*out_total_blocks = 0;

	// Read entire system area (5 blocks = 40KB)
	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) return 0;

	// Get card size info from header block
	const u8 *hdr = sysarea + MC_BLOCK_HEADER * MC_BLOCK_SIZE;
	u16 card_mbits = *(const u16 *)(hdr + MC_HDR_CARD_SIZE);
	*out_mem_size = card_mbits;	// Mbits, matches CARD_ProbeEx format
	*out_sector_size = MC_BLOCK_SIZE;

	// Determine total blocks from file size (more reliable than header)
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	u32 total_blocks = 0;
	if (dev) {
		file_handle *f = calloc(1, sizeof(file_handle));
		if (f) {
			strlcpy(f->name, vmc_path, PATHNAME_MAX);
			if (!dev->statFile(f))
				total_blocks = f->size / MC_BLOCK_SIZE;
			free(f);
		}
	}
	if (total_blocks < MC_FIRST_DATA_BLOCK + 1) {
		free(sysarea);
		return 0;
	}

	// Pick active directory
	int dir_blk = vmc_active_dir_block(sysarea);
	const u8 *dir = sysarea + dir_blk * MC_BLOCK_SIZE;

	// Parse directory entries (metadata only, no file data reads)
	int count = 0;
	for (int i = 0; i < MC_DIR_ENTRIES && count < max_entries; i++) {
		const GCI *raw = (const GCI *)(dir + i * MC_DIR_ENTRY_SIZE);

		if (raw->gamecode[0] == 0xFF)
			continue;
		if (raw->gamecode[0] == 0x00)
			continue;

		card_entry *e = &entries[count];
		memset(e, 0, sizeof(card_entry));

		memcpy(e->gamecode, raw->gamecode, 4);
		e->gamecode[4] = '\0';
		memcpy(e->company, raw->company, 2);
		e->company[2] = '\0';
		snprintf(e->filename, CARD_FILENAMELEN + 1, "%.*s",
			CARD_FILENAMELEN, raw->filename);

		e->banner_fmt = raw->banner_fmt;
		e->time = raw->time;
		e->icon_addr = raw->icon_addr;
		e->icon_fmt = raw->icon_fmt;
		e->icon_speed = raw->icon_speed;
		e->permissions = raw->permission;
		e->filesize = raw->filesize8 * MC_BLOCK_SIZE;
		e->blocks = e->filesize / MC_BLOCK_SIZE;
		e->fileno = i;

		count++;
	}

	*out_sysarea = sysarea;
	*out_total_blocks = total_blocks;
	return count;
}

// Slow phase: read one save's file data for comment + graphics
void vmc_read_save_detail(const char *vmc_path, card_entry *entry,
	const u8 *sysarea, u32 total_blocks) {

	int bat_blk = vmc_active_bat_block(sysarea);
	int dir_blk = vmc_active_dir_block(sysarea);
	const u8 *bat = sysarea + bat_blk * MC_BLOCK_SIZE;
	const u8 *dir = sysarea + dir_blk * MC_BLOCK_SIZE;

	// Get first_block and comment_addr from directory entry
	const GCI *raw = (const GCI *)(dir + entry->fileno * MC_DIR_ENTRY_SIZE);
	u16 first_block = raw->first_block;
	u32 comment_addr = raw->comment_addr;

	if (entry->filesize > 0 && first_block >= MC_FIRST_DATA_BLOCK) {
		u8 *file_data = vmc_read_file_data(vmc_path, bat,
			first_block, entry->filesize, total_blocks);
		if (file_data) {
			cm_parse_comment(file_data, entry->filesize, comment_addr, entry);

			if (entry->icon_addr != (u32)-1 && entry->icon_addr < entry->filesize) {
				u8 *gfx = file_data + entry->icon_addr;
				u32 gfx_len = entry->filesize - entry->icon_addr;
				cm_parse_save_graphics(gfx, gfx_len,
					entry->banner_fmt, entry->icon_fmt, entry->icon_speed, entry);
			}
			free(file_data);
		}
	}
}

// Convenience wrapper for callers that don't need incremental loading
int vmc_read_saves(const char *vmc_path, card_entry *entries, int max_entries,
	s32 *out_mem_size, s32 *out_sector_size) {

	u8 *sysarea;
	u32 total_blocks;
	int count = vmc_read_dir(vmc_path, entries, max_entries,
		out_mem_size, out_sector_size, &sysarea, &total_blocks);
	for (int i = 0; i < count; i++)
		vmc_read_save_detail(vmc_path, &entries[i], sysarea, total_blocks);
	free(sysarea);
	return count;
}

// --- VMC write support ---

// GC memory card checksum (matches libogc __card_checksum)
static void vmc_checksum(const u16 *data, int num_words, u16 *cs1, u16 *cs2) {
	u16 c1 = 0, c2 = 0;
	for (int i = 0; i < num_words; i++) {
		c1 += data[i];
		c2 += data[i] ^ 0xFFFF;
	}
	if (c1 == 0xFFFF) c1 = 0;
	if (c2 == 0xFFFF) c2 = 0;
	*cs1 = c1;
	*cs2 = c2;
}

// Directory block: checksum covers bytes 0-0x1FFB, stored at 0x1FFC-0x1FFF
#define MC_DIR_CHECKSUM_OFF  0x1FFC

static void vmc_update_dir_checksum(u8 *dir_block) {
	u16 cs1, cs2;
	vmc_checksum((const u16 *)dir_block, MC_DIR_CHECKSUM_OFF / 2, &cs1, &cs2);
	*(u16 *)(dir_block + MC_DIR_CHECKSUM_OFF) = cs1;
	*(u16 *)(dir_block + MC_DIR_CHECKSUM_OFF + 2) = cs2;
}

// BAT block: checksum covers bytes 0x0004-0x1FFF, stored at 0x0000-0x0003
static void vmc_update_bat_checksum(u8 *bat_block) {
	u16 cs1, cs2;
	vmc_checksum((const u16 *)(bat_block + 4), (MC_BLOCK_SIZE - 4) / 2, &cs1, &cs2);
	*(u16 *)(bat_block + MC_BAT_CHECKSUM1) = cs1;
	*(u16 *)(bat_block + MC_BAT_CHECKSUM2) = cs2;
}

// Get total blocks in a VMC file
static u32 vmc_get_total_blocks(const char *vmc_path) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return 0;
	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return 0;
	strlcpy(f->name, vmc_path, PATHNAME_MAX);
	u32 blocks = 0;
	if (!dev->statFile(f))
		blocks = f->size / MC_BLOCK_SIZE;
	free(f);
	return blocks;
}

// Safe block-level rewrite of a VMC .raw file.
// Copies the file block-by-block to a temp file, substituting modified blocks,
// then deletes the original and renames temp to original.
static bool vmc_rewrite(const char *vmc_path,
	int num_mods, const u32 *mod_blocks, u8 *const *mod_data) {

	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	file_handle *src = calloc(1, sizeof(file_handle));
	if (!src) return false;
	strlcpy(src->name, vmc_path, PATHNAME_MAX);
	if (dev->statFile(src)) { free(src); return false; }
	u32 total_size = src->size;
	u32 nblocks = total_size / MC_BLOCK_SIZE;

	// Save original path for rename
	char orig_path[PATHNAME_MAX];
	strlcpy(orig_path, vmc_path, PATHNAME_MAX);

	file_handle *dst = calloc(1, sizeof(file_handle));
	if (!dst) { free(src); return false; }
	snprintf(dst->name, PATHNAME_MAX, "%s.tmp", vmc_path);

	u8 *buf = (u8 *)memalign(32, MC_BLOCK_SIZE);
	if (!buf) { free(src); free(dst); return false; }

	bool ok = true;
	dev->seekFile(src, 0, DEVICE_HANDLER_SEEK_SET);

	for (u32 b = 0; b < nblocks && ok; b++) {
		// Check if this block has a modification
		u8 *data = NULL;
		for (int m = 0; m < num_mods; m++) {
			if (mod_blocks[m] == b) { data = mod_data[m]; break; }
		}

		if (data) {
			// Write modified block, skip read
			dev->seekFile(src, (b + 1) * MC_BLOCK_SIZE, DEVICE_HANDLER_SEEK_SET);
			if (dev->writeFile(dst, data, MC_BLOCK_SIZE) != MC_BLOCK_SIZE)
				ok = false;
		} else {
			// Copy unmodified block
			if (dev->readFile(src, buf, MC_BLOCK_SIZE) != MC_BLOCK_SIZE) {
				ok = false;
			} else {
				if (dev->writeFile(dst, buf, MC_BLOCK_SIZE) != MC_BLOCK_SIZE)
					ok = false;
			}
		}
	}
	dev->closeFile(src);
	dev->closeFile(dst);
	free(buf);

	if (!ok) {
		// Clean up failed temp file
		dev->deleteFile(dst);
		free(src); free(dst);
		return false;
	}

	// Replace original with temp
	dev->deleteFile(src);
	dev->renameFile(dst, orig_path);

	free(src);
	free(dst);
	return true;
}

// Prepare updated directory and BAT blocks for writing.
// Copies active block contents to the inactive (backup) block,
// increments the backup's update counter (making it the new active),
// and recomputes its checksum.
// Returns the block numbers of the new dir and bat blocks.
static bool vmc_prepare_write(u8 *sysarea,
	u8 **out_dir, int *out_dir_blk, u8 **out_bat, int *out_bat_blk) {

	int active_dir = vmc_active_dir_block(sysarea);
	int active_bat = vmc_active_bat_block(sysarea);
	int new_dir_blk = (active_dir == MC_BLOCK_DIR_A) ? MC_BLOCK_DIR_B : MC_BLOCK_DIR_A;
	int new_bat_blk = (active_bat == MC_BLOCK_BAT_A) ? MC_BLOCK_BAT_B : MC_BLOCK_BAT_A;

	// Copy active to new (inactive) block
	u8 *new_dir = (u8 *)memalign(32, MC_BLOCK_SIZE);
	u8 *new_bat = (u8 *)memalign(32, MC_BLOCK_SIZE);
	if (!new_dir || !new_bat) {
		free(new_dir); free(new_bat);
		return false;
	}
	memcpy(new_dir, sysarea + active_dir * MC_BLOCK_SIZE, MC_BLOCK_SIZE);
	memcpy(new_bat, sysarea + active_bat * MC_BLOCK_SIZE, MC_BLOCK_SIZE);

	// Increment update counters
	u16 *dir_counter = (u16 *)(new_dir + MC_DIR_UPDATE_OFF);
	(*dir_counter)++;
	u16 *bat_counter = (u16 *)(new_bat + MC_BAT_UPDATE);
	(*bat_counter)++;

	*out_dir = new_dir;
	*out_dir_blk = new_dir_blk;
	*out_bat = new_bat;
	*out_bat_blk = new_bat_blk;
	return true;
}

// --- VMC export ---

// --- Read save from VMC as in-memory GCI ---

bool vmc_read_save(const char *vmc_path, card_entry *entry,
	GCI *out_gci, u8 **out_data, u32 *out_len) {

	uiDrawObj_t *msgBox = cm_draw_message("Reading save...");
	DrawPublish(msgBox);

	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) {
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Failed to read VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	int dir_blk = vmc_active_dir_block(sysarea);
	int bat_blk = vmc_active_bat_block(sysarea);
	const u8 *dir = sysarea + dir_blk * MC_BLOCK_SIZE;
	const u8 *bat = sysarea + bat_blk * MC_BLOCK_SIZE;
	const GCI *raw = (const GCI *)(dir + entry->fileno * MC_DIR_ENTRY_SIZE);

	u32 total_blocks = vmc_get_total_blocks(vmc_path);

	u8 *filedata = vmc_read_file_data(vmc_path, bat,
		raw->first_block, entry->filesize, total_blocks);
	if (!filedata) {
		free(sysarea);
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Failed to read save data from VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	DrawDispose(msgBox);

	GCI gci;
	memcpy(&gci, raw, sizeof(GCI));
	gci.reserved01 = 0xFF;
	gci.reserved02 = 0xFFFF;
	gci.first_block = 0;

	*out_gci = gci;
	*out_data = filedata;
	*out_len = entry->filesize;
	free(sysarea);
	return true;
}

bool vmc_export_save(const char *vmc_path, card_entry *entry) {
	GCI gci;
	u8 *filedata;
	u32 data_len;
	if (!vmc_read_save(vmc_path, entry, &gci, &filedata, &data_len))
		return false;

	bool result = cm_write_gci_to_sd(&gci, filedata, data_len);
	free(filedata);
	return result;
}

// --- VMC delete ---

bool vmc_delete_save(const char *vmc_path, card_entry *entry) {
	uiDrawObj_t *msgBox = cm_draw_message("Deleting save...");
	DrawPublish(msgBox);

	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) {
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Failed to read VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Prepare new directory and BAT blocks
	u8 *new_dir, *new_bat;
	int new_dir_blk, new_bat_blk;
	if (!vmc_prepare_write(sysarea, &new_dir, &new_dir_blk, &new_bat, &new_bat_blk)) {
		free(sysarea);
		DrawDispose(msgBox);
		return false;
	}

	// Find the directory entry and get first block
	GCI *dir_entry = (GCI *)(new_dir + entry->fileno * MC_DIR_ENTRY_SIZE);
	u16 first_block = dir_entry->first_block;
	u16 num_blocks8 = dir_entry->filesize8;

	// Clear directory entry
	dir_entry->gamecode[0] = 0xFF;

	// Free FAT chain
	u16 *fat = (u16 *)(new_bat + MC_BAT_FAT_START);
	u16 *free_blocks = (u16 *)(new_bat + MC_BAT_FREE_BLOCKS);
	u32 total_blocks = vmc_get_total_blocks(vmc_path);
	u16 block = first_block;
	int freed = 0;

	for (int i = 0; i < num_blocks8 && block >= MC_FIRST_DATA_BLOCK && block < total_blocks; i++) {
		u16 fat_idx = block - MC_FIRST_DATA_BLOCK;
		u16 next_block = fat[fat_idx];
		fat[fat_idx] = MC_FAT_FREE;
		freed++;
		if (next_block == MC_FAT_LAST) break;
		block = next_block;
	}
	*free_blocks += freed;

	// Update checksums
	vmc_update_dir_checksum(new_dir);
	vmc_update_bat_checksum(new_bat);

	// Write back
	u32 mod_blocks[2] = { new_dir_blk, new_bat_blk };
	u8 *mod_data[2] = { new_dir, new_bat };
	bool ok = vmc_rewrite(vmc_path, 2, mod_blocks, mod_data);

	free(new_dir);
	free(new_bat);
	free(sysarea);
	DrawDispose(msgBox);

	if (!ok) {
		msgBox = cm_draw_message("Failed to write VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	msgBox = cm_draw_message("Save deleted.");
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
}

// --- VMC import ---

// --- Import in-memory GCI to VMC ---

bool vmc_import_save_buf(const char *vmc_path, GCI *gci,
	u8 *savedata, u32 save_len) {

	uiDrawObj_t *msgBox = cm_draw_message("Writing save...");
	DrawPublish(msgBox);

	u32 expected_len = gci->filesize8 * MC_BLOCK_SIZE;
	u16 need_blocks = gci->filesize8;

	if (save_len != expected_len) {
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "GCI size mismatch: got %dK, expected %dK",
			(int)(save_len / 1024), (int)(expected_len / 1024));
		msgBox = cm_draw_message(errmsg);
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Read VMC system area
	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) {
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Failed to read VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	u32 total_blocks = vmc_get_total_blocks(vmc_path);

	// Prepare new directory and BAT blocks
	u8 *new_dir, *new_bat;
	int new_dir_blk, new_bat_blk;
	if (!vmc_prepare_write(sysarea, &new_dir, &new_dir_blk, &new_bat, &new_bat_blk)) {
		free(sysarea);
		DrawDispose(msgBox);
		return false;
	}

	u16 *fat = (u16 *)(new_bat + MC_BAT_FAT_START);
	u16 *free_count = (u16 *)(new_bat + MC_BAT_FREE_BLOCKS);
	u16 *last_alloc = (u16 *)(new_bat + MC_BAT_LAST_ALLOC);
	u32 user_blocks = total_blocks - MC_FIRST_DATA_BLOCK;

	// Check if a file with the same name/gamecode already exists
	char gci_filename[CARD_FILENAMELEN + 1];
	memset(gci_filename, 0, sizeof(gci_filename));
	memcpy(gci_filename, gci->filename, CARD_FILENAMELEN);
	int existing_slot = -1;
	for (int i = 0; i < MC_DIR_ENTRIES; i++) {
		GCI *de = (GCI *)(new_dir + i * MC_DIR_ENTRY_SIZE);
		if (de->gamecode[0] == 0xFF || de->gamecode[0] == 0x00) continue;
		if (memcmp(de->gamecode, gci->gamecode, 4) == 0
			&& memcmp(de->company, gci->company, 2) == 0
			&& strncmp(de->filename, gci->filename, CARD_FILENAMELEN) == 0) {
			existing_slot = i;
			break;
		}
	}

	if (existing_slot >= 0) {
		// Confirm overwrite
		DrawDispose(msgBox);
		msgBox = cm_draw_message(
			"Save already exists in VMC.\n \nA: Overwrite  |  B: Cancel");
		DrawPublish(msgBox);
		int choice = 0;
		while (!choice) {
			u16 btn = padsButtonsHeld();
			if (btn & PAD_BUTTON_A) choice = 1;
			if (btn & PAD_BUTTON_B) choice = 2;
			VIDEO_WaitVSync();
		}
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		if (choice == 2) {
			free(new_dir); free(new_bat); free(sysarea);
			return false;
		}
		msgBox = cm_draw_message("Importing save to VMC...");
		DrawPublish(msgBox);

		// Free old blocks first
		GCI *old_entry = (GCI *)(new_dir + existing_slot * MC_DIR_ENTRY_SIZE);
		u16 block = old_entry->first_block;
		for (int i = 0; i < old_entry->filesize8 && block >= MC_FIRST_DATA_BLOCK && block < total_blocks; i++) {
			u16 fat_idx = block - MC_FIRST_DATA_BLOCK;
			u16 next_block = fat[fat_idx];
			fat[fat_idx] = MC_FAT_FREE;
			(*free_count)++;
			if (next_block == MC_FAT_LAST) break;
			block = next_block;
		}
	}

	// Check free space
	if (*free_count < need_blocks) {
		u16 avail = *free_count;
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "Not enough space.\nNeed %d blocks, have %d free.",
			need_blocks, avail);
		msgBox = cm_draw_message(errmsg);
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Find free directory slot
	int dir_slot = existing_slot;
	if (dir_slot < 0) {
		for (int i = 0; i < MC_DIR_ENTRIES; i++) {
			GCI *de = (GCI *)(new_dir + i * MC_DIR_ENTRY_SIZE);
			if (de->gamecode[0] == 0xFF || de->gamecode[0] == 0x00) {
				dir_slot = i;
				break;
			}
		}
	}
	if (dir_slot < 0) {
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		msgBox = cm_draw_message("No free directory slots in VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Allocate blocks in FAT and build chain
	u16 *alloc_blocks = (u16 *)calloc(need_blocks, sizeof(u16));
	if (!alloc_blocks) {
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		return false;
	}

	int allocated = 0;
	// Start searching after last allocated block for locality (lastalloc is absolute)
	u16 search_start = *last_alloc + 1;
	if (search_start < MC_FIRST_DATA_BLOCK) search_start = MC_FIRST_DATA_BLOCK;
	for (u32 i = 0; i < user_blocks && allocated < need_blocks; i++) {
		u32 idx = (search_start - MC_FIRST_DATA_BLOCK + i) % user_blocks;
		if (fat[idx] == MC_FAT_FREE) {
			alloc_blocks[allocated++] = idx + MC_FIRST_DATA_BLOCK;
		}
	}

	if (allocated < need_blocks) {
		free(alloc_blocks);
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		msgBox = cm_draw_message("FAT allocation failed.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Build FAT chain (values are absolute block numbers)
	for (int i = 0; i < need_blocks; i++) {
		u16 fat_idx = alloc_blocks[i] - MC_FIRST_DATA_BLOCK;
		if (i < need_blocks - 1)
			fat[fat_idx] = alloc_blocks[i + 1];
		else
			fat[fat_idx] = MC_FAT_LAST;
	}

	// Update BAT metadata
	*free_count -= need_blocks;
	*last_alloc = alloc_blocks[need_blocks - 1];

	// Write directory entry
	GCI *new_entry = (GCI *)(new_dir + dir_slot * MC_DIR_ENTRY_SIZE);
	memcpy(new_entry, gci, sizeof(GCI));
	new_entry->first_block = alloc_blocks[0];

	// Update checksums
	vmc_update_dir_checksum(new_dir);
	vmc_update_bat_checksum(new_bat);

	// Build modification list: 2 system blocks + N data blocks
	int num_mods = 2 + need_blocks;
	u32 *mod_block_nums = (u32 *)calloc(num_mods, sizeof(u32));
	u8 **mod_block_data = (u8 **)calloc(num_mods, sizeof(u8 *));
	if (!mod_block_nums || !mod_block_data) {
		free(mod_block_nums); free(mod_block_data);
		free(alloc_blocks);
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		return false;
	}

	mod_block_nums[0] = new_dir_blk;
	mod_block_data[0] = new_dir;
	mod_block_nums[1] = new_bat_blk;
	mod_block_data[1] = new_bat;

	// Prepare data blocks (each must be a full MC_BLOCK_SIZE buffer)
	u8 **data_bufs = (u8 **)calloc(need_blocks, sizeof(u8 *));
	bool alloc_ok = data_bufs != NULL;
	for (int i = 0; i < need_blocks && alloc_ok; i++) {
		data_bufs[i] = (u8 *)memalign(32, MC_BLOCK_SIZE);
		if (!data_bufs[i]) { alloc_ok = false; break; }
		u32 offset = i * MC_BLOCK_SIZE;
		u32 copy_len = (offset + MC_BLOCK_SIZE <= expected_len)
			? MC_BLOCK_SIZE : (expected_len - offset);
		memset(data_bufs[i], 0, MC_BLOCK_SIZE);
		memcpy(data_bufs[i], savedata + offset, copy_len);
		mod_block_nums[2 + i] = alloc_blocks[i];
		mod_block_data[2 + i] = data_bufs[i];
	}

	bool ok = false;
	if (alloc_ok) {
		ok = vmc_rewrite(vmc_path, num_mods, mod_block_nums, mod_block_data);
	}

	// Cleanup
	if (data_bufs) {
		for (int i = 0; i < need_blocks; i++) free(data_bufs[i]);
		free(data_bufs);
	}
	free(mod_block_nums);
	free(mod_block_data);
	free(alloc_blocks);
	free(new_dir);
	free(new_bat);
	free(sysarea);
	DrawDispose(msgBox);

	if (!ok) {
		msgBox = cm_draw_message("Failed to write VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Imported \"%s\" successfully.", gci_filename);
	msgBox = cm_draw_message(done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
}

// --- VMC Creation ---

// Create a new blank .raw VMC file.
// Returns true and fills out_filename with the new filename on success.
// Format a blank memory card system area (header + 2 dir blocks + 2 BAT blocks).
// total_blocks = total blocks in the card image (including 5 system blocks).
// encoding = 0 (ANSI/USA) or 1 (SJIS/JPN).
static void vmc_format_sysarea(u8 *sysarea, u32 total_blocks, u16 encoding) {
	u32 user_blocks = total_blocks - MC_FIRST_DATA_BLOCK;

	// Card size in Mbits: total_blocks * 8192 * 8 / 1048576 = total_blocks / 16
	u16 card_mbits = total_blocks / 16;
	if (card_mbits < 1) card_mbits = 1;

	memset(sysarea, 0, MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE);

	// --- Header block (block 0) ---
	u8 *hdr = sysarea;
	// Serial number (bytes 0x00-0x11): random/unique; fill with non-zero pattern
	for (int i = 0; i < 18; i++)
		hdr[i] = (u8)(i * 0x13 + 0x42);
	// Device ID (0x0012, u16): 0x0000 for standard card
	// Card size in Mbits at 0x0022
	*(u16 *)(hdr + MC_HDR_CARD_SIZE) = card_mbits;
	// Encoding at 0x0024
	*(u16 *)(hdr + MC_HDR_ENCODING) = encoding;
	// Header checksum: covers 0x0000-0x01FB, stored at 0x01FC-0x01FF
	{
		u16 cs1, cs2;
		vmc_checksum((const u16 *)hdr, 0x01FC / 2, &cs1, &cs2);
		*(u16 *)(hdr + 0x01FC) = cs1;
		*(u16 *)(hdr + 0x01FE) = cs2;
	}

	// --- Directory blocks (blocks 1 & 2) ---
	// Empty directory: all entry bytes = 0xFF, then valid checksum
	for (int d = 0; d < 2; d++) {
		u8 *dir = sysarea + (MC_BLOCK_DIR_A + d) * MC_BLOCK_SIZE;
		memset(dir, 0xFF, MC_BLOCK_SIZE);
		// Update counter at 0x1FFA (u16): block A=0, block B=0
		*(u16 *)(dir + MC_DIR_UPDATE_OFF) = 0;
		// Checksum placeholder (will be computed below)
		*(u16 *)(dir + MC_DIR_CHECKSUM_OFF) = 0;
		*(u16 *)(dir + MC_DIR_CHECKSUM_OFF + 2) = 0;
		vmc_update_dir_checksum(dir);
	}

	// --- BAT blocks (blocks 3 & 4) ---
	for (int b = 0; b < 2; b++) {
		u8 *bat = sysarea + (MC_BLOCK_BAT_A + b) * MC_BLOCK_SIZE;
		memset(bat, 0, MC_BLOCK_SIZE);
		// Update counter
		*(u16 *)(bat + MC_BAT_UPDATE) = 0;
		// Free blocks count
		*(u16 *)(bat + MC_BAT_FREE_BLOCKS) = (u16)user_blocks;
		// Last allocated block (points to last user block, or first data block - 1)
		*(u16 *)(bat + MC_BAT_LAST_ALLOC) = MC_FIRST_DATA_BLOCK - 1;
		// FAT entries: all free (0x0000) — already zeroed by memset
		vmc_update_bat_checksum(bat);
	}
}

bool cm_vmc_create(int game_region, char *out_filename, int out_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	const char *region_str = "USA";
	static const char *regions[] = {"JPN", "USA", "EUR", "ALL", "KOR"};
	if (game_region >= 0 && game_region <= 4) region_str = regions[game_region];

	// Auto-generate a unique filename: Card_001.USA.raw, Card_002.USA.raw, etc.
	char filename[64];
	for (int n = 1; n <= 999; n++) {
		snprintf(filename, sizeof(filename), "Card_%03d.%s.raw", n, region_str);

		file_handle *check = calloc(1, sizeof(file_handle));
		if (!check) return false;
		concatf_path(check->name, dev->initial->name, "swiss/saves/%s", filename);
		int exists = !dev->statFile(check);
		free(check);
		if (!exists) break;
		if (n == 999) return false;
	}

	// Size picker — covers the full range of real memory card sizes
	// total_blocks includes 5 system blocks; user_blocks = total - 5
	#define NUM_VMC_SIZES 6
	int total_blocks[] = {64, 128, 256, 512, 1024, 2048};
	const char *size_labels[] = {
		"59 blocks (512KB)",
		"123 blocks (1MB)",
		"251 blocks (2MB)",
		"507 blocks (4MB)",
		"1019 blocks (8MB)",
		"2043 blocks (16MB)"
	};
	int sel = NUM_VMC_SIZES - 1;

	uiDrawObj_t *overlay = NULL;
	bool needs_redraw = true;
	bool debounce = true;

	while (1) {
		if (needs_redraw) {
			char msg[128];
			snprintf(msg, sizeof(msg), "Create: %s\n\nSize: < %s >\n\nPress A to create, B to cancel",
				filename, size_labels[sel]);
			uiDrawObj_t *fresh = cm_draw_message(msg);
			if (overlay) DrawDispose(overlay);
			overlay = fresh;
			DrawPublish(fresh);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		u16 btns = padsButtonsHeld();

		if (debounce) {
			if (!(btns & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT)))
				debounce = false;
			continue;
		}

		if ((btns & PAD_TRIGGER_L) || (btns & PAD_BUTTON_LEFT)) {
			if (sel > 0) sel--;
			needs_redraw = true;
			debounce = true;
		}
		if ((btns & PAD_TRIGGER_R) || (btns & PAD_BUTTON_RIGHT)) {
			if (sel < NUM_VMC_SIZES - 1) sel++;
			needs_redraw = true;
			debounce = true;
		}
		if (btns & PAD_BUTTON_B) {
			while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
			DrawDispose(overlay);
			return false;
		}
		if (btns & PAD_BUTTON_A) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			DrawDispose(overlay);
			break;
		}
	}

	// Create the file with a properly formatted system area
	uiDrawObj_t *prog = cm_draw_message("Creating memory card...");
	DrawPublish(prog);

	u16 encoding = (game_region == 0 || game_region == 4) ? 1 : 0; // JPN/KOR = SJIS

	// Allocate and format the system area (5 blocks)
	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = (u8 *)memalign(32, sysarea_size);
	if (!sysarea) { DrawDispose(prog); return false; }
	vmc_format_sysarea(sysarea, total_blocks[sel], encoding);

	file_handle *newFile = calloc(1, sizeof(file_handle));
	if (!newFile) { free(sysarea); DrawDispose(prog); return false; }
	concatf_path(newFile->name, dev->initial->name, "swiss/saves/%s", filename);

	int dev_slot = devices[DEVICE_CONFIG] ? DEVICE_CONFIG : DEVICE_CUR;
	ensure_path(dev_slot, "swiss/saves", NULL, false);

	// Write system area first, then extend to full file size
	u32 file_size = (u32)total_blocks[sel] * MC_BLOCK_SIZE;
	dev->writeFile(newFile, sysarea, sysarea_size);
	dev->seekFile(newFile, file_size, DEVICE_HANDLER_SEEK_SET);
	dev->writeFile(newFile, NULL, 0);
	dev->closeFile(newFile);
	free(newFile);
	free(sysarea);

	DrawDispose(prog);

	strlcpy(out_filename, filename, out_size);
	return true;
}

// --- VMC Picker ---

// Map GCMDisk.RegionCode to string for comparison with VMC region tag
static const char *vmc_region_string(int region_code) {
	static const char *regions[] = {"JPN", "USA", "EUR", "ALL", "KOR"};
	if (region_code >= 0 && region_code <= 4) return regions[region_code];
	return "UNK";
}

static char s_picked_filename[64];

int cm_vmc_picker(const char *current_filename, int game_region) {
	vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
	if (!vmcs) return VMC_PICK_CANCEL;

	int num = vmc_scan_files(vmcs, MAX_VMC_FILES);

	// Items: Auto + N VMCs + Create New
	int total_items = 1 + num + 1;
	const char *game_region_str = vmc_region_string(game_region);

	// Build display strings
	char auto_label[48];
	snprintf(auto_label, sizeof(auto_label), "Auto (%s)", game_region_str);

	// Item labels with block info
	char item_labels[MAX_VMC_FILES][48];
	for (int i = 0; i < num; i++) {
		if (vmcs[i].user_blocks > 0) {
			bool mismatch = vmcs[i].region[0] &&
				strcasecmp(vmcs[i].region, game_region_str) != 0;
			snprintf(item_labels[i], sizeof(item_labels[i]),
				"%s  %lu/%lu%s",
				vmcs[i].label,
				(unsigned long)(vmcs[i].user_blocks),
				(unsigned long)(vmcs[i].user_blocks),
				mismatch ? "  !" : "");
		} else {
			strlcpy(item_labels[i], vmcs[i].label, sizeof(item_labels[i]));
		}
	}

	int cursor = 0;
	// Pre-select current filename if set
	if (current_filename && current_filename[0]) {
		for (int i = 0; i < num; i++) {
			if (!strcasecmp(vmcs[i].filename, current_filename)) {
				cursor = 1 + i;
				break;
			}
		}
	}

	// Menu dimensions
	int item_h = 22;
	int pad = 10;
	int menu_w = 320;
	int menu_h = total_items * item_h + pad * 2;
	if (menu_h > 400) menu_h = 400;
	int menu_x = (640 - menu_w) / 2;
	int menu_y = (480 - menu_h) / 2;

	int max_visible = (menu_h - pad * 2) / item_h;
	int scroll = 0;
	if (cursor >= max_visible) scroll = cursor - max_visible + 1;

	uiDrawObj_t *overlay = NULL;
	bool needs_redraw = true;
	bool debounce = true; // Start debounced to avoid instant selection

	int result = VMC_PICK_CANCEL;

	while (1) {
		cm_retire_tick();

		if (needs_redraw) {
			uiDrawObj_t *fresh = DrawEmptyBox(menu_x, menu_y,
				menu_x + menu_w, menu_y + menu_h);

			for (int vi = 0; vi < max_visible && scroll + vi < total_items; vi++) {
				int idx = scroll + vi;
				int iy = menu_y + pad + vi * item_h + item_h / 2;

				const char *label;
				GXColor color;
				bool is_mismatch = false;

				if (idx == 0) {
					label = auto_label;
				} else if (idx <= num) {
					int vi2 = idx - 1;
					label = item_labels[vi2];
					is_mismatch = vmcs[vi2].region[0] &&
						strcasecmp(vmcs[vi2].region, game_region_str) != 0;
				} else {
					label = "Create New...";
				}

				if (idx == cursor) {
					color = (GXColor){96, 107, 164, 255};
					DrawAddChild(fresh, cm_draw_highlight(
						menu_x + 6, menu_y + pad + vi * item_h,
						menu_w - 12, item_h));
				} else if (is_mismatch) {
					color = (GXColor){180, 160, 80, 255};
				} else {
					color = defaultColor;
				}

				DrawAddChild(fresh, DrawStyledLabel(menu_x + 16, iy,
					label, 0.5f, ALIGN_LEFT, color));
			}

			if (overlay) DrawDispose(overlay);
			overlay = fresh;
			DrawPublish(fresh);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		u16 btns = padsButtonsHeld();
		s8 stickY = padsStickY();

		if (debounce) {
			if (!(btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT)))
				debounce = false;
			continue;
		}

		if ((btns & PAD_BUTTON_UP) || stickY > 16) {
			if (cursor > 0) {
				cursor--;
				if (cursor < scroll) scroll = cursor;
				needs_redraw = true;
			}
			debounce = true;
		}
		if ((btns & PAD_BUTTON_DOWN) || stickY < -16) {
			if (cursor < total_items - 1) {
				cursor++;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				needs_redraw = true;
			}
			debounce = true;
		}
		if (btns & PAD_BUTTON_A) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			if (cursor == 0) {
				result = VMC_PICK_AUTO;
			} else if (cursor <= num) {
				result = cursor - 1; // Index into vmcs array
			} else {
				result = VMC_PICK_CREATE;
			}
			break;
		}
		if (btns & PAD_BUTTON_B) {
			while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
			result = VMC_PICK_CANCEL;
			break;
		}
	}

	DrawDispose(overlay);

	// If a VMC was selected, copy its filename to a static buffer
	// so the caller can retrieve it after vmcs is freed
	if (result >= 0 && result < num) {
		strlcpy(s_picked_filename, vmcs[result].filename, sizeof(s_picked_filename));
	}
	free(vmcs);

	return result;
}

const char *cm_vmc_picker_filename(void) {
	return s_picked_filename;
}

// --- Import GCI file from SD to VMC ---

bool vmc_import_save(const char *vmc_path, gci_file_entry *gci_entry) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	file_handle *inFile = calloc(1, sizeof(file_handle));
	if (!inFile) return false;
	strlcpy(inFile->name, gci_entry->path, PATHNAME_MAX);

	if (dev->statFile(inFile) || inFile->size <= sizeof(GCI)) {
		free(inFile);
		uiDrawObj_t *msgBox = cm_draw_message("Invalid GCI file.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	u32 total_file = inFile->size;
	u8 *filebuf = (u8 *)memalign(32, total_file);
	if (!filebuf) {
		free(inFile);
		uiDrawObj_t *msgBox = cm_draw_message("Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	dev->readFile(inFile, filebuf, total_file);
	dev->closeFile(inFile);
	free(inFile);

	GCI *gci = (GCI *)filebuf;
	u8 *savedata = filebuf + sizeof(GCI);
	u32 save_len = total_file - sizeof(GCI);

	bool result = vmc_import_save_buf(vmc_path, gci, savedata, save_len);
	free(filebuf);
	return result;
}

// --- Game config scanning and picker for "Assign to game..." ---

int cm_scan_game_configs(cm_game_entry *out, int max) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return 0;

	file_handle *dir = calloc(1, sizeof(file_handle));
	if (!dir) return 0;
	concat_path(dir->name, dev->initial->name, "swiss/settings/game");

	if (dev->statFile(dir)) {
		free(dir);
		return 0;
	}

	file_handle *dirEntries = NULL;
	int count = 0;

	s32 num_dir = dev->readDir(dir, &dirEntries, -1);
	if (num_dir > 0 && dirEntries) {
		for (int i = 0; i < num_dir && count < max; i++) {
			char *name = getRelativeName(dirEntries[i].name);
			int len = strlen(name);
			// Only look at .ini files with 4-char game IDs (e.g. "GALE.ini")
			if (len == 8 && !strcasecmp(name + 4, ".ini")) {
				// Read the file to extract the Name field
				file_handle *iniFile = calloc(1, sizeof(file_handle));
				if (!iniFile) continue;
				strlcpy(iniFile->name, dirEntries[i].name, PATHNAME_MAX);

				if (dev->statFile(iniFile) || iniFile->size == 0) {
					free(iniFile);
					continue;
				}

				char *buf = calloc(1, iniFile->size + 1);
				if (!buf) { free(iniFile); continue; }
				dev->readFile(iniFile, buf, iniFile->size);
				dev->closeFile(iniFile);
				free(iniFile);

				// Extract game ID from filename
				memset(&out[count], 0, sizeof(cm_game_entry));
				memcpy(out[count].game_id, name, 4);
				out[count].game_id[4] = '\0';

				// Parse Name= from ini content
				char *line, *ctx = NULL;
				char *bufcopy = strdup(buf);
				line = strtok_r(bufcopy, "\r\n", &ctx);
				while (line) {
					if (!strncmp(line, "Name=", 5)) {
						strlcpy(out[count].game_name, line + 5, sizeof(out[count].game_name));
						break;
					}
					line = strtok_r(NULL, "\r\n", &ctx);
				}
				free(bufcopy);
				free(buf);

				// Use game ID as fallback name
				if (!out[count].game_name[0])
					strlcpy(out[count].game_name, out[count].game_id, sizeof(out[count].game_name));

				count++;
			}
		}
		free(dirEntries);
	}
	free(dir);

	// Merge in recent games that aren't already in the list
	for (int r = 0; r < RECENT_MAX && count < max; r++) {
		if (!swissSettings.recent[r][0]) continue;

		DEVICEHANDLER_INTERFACE *rdev = getDeviceFromPath(swissSettings.recent[r]);
		if (!rdev) continue;

		// Read disc header to get game ID and name
		file_handle *rf = calloc(1, sizeof(file_handle));
		if (!rf) continue;
		strlcpy(rf->name, swissSettings.recent[r], PATHNAME_MAX);
		rf->device = rdev;

		if (rdev->statFile(rf) || rf->size < 0x60) {
			free(rf);
			continue;
		}

		u8 *hdr = (u8 *)memalign(32, 0x60);
		if (!hdr) { free(rf); continue; }
		rdev->seekFile(rf, 0, DEVICE_HANDLER_SEEK_SET);
		rdev->readFile(rf, hdr, 0x60);
		rdev->closeFile(rf);
		free(rf);

		char rid[5] = {0};
		memcpy(rid, hdr, 4);
		rid[4] = '\0';

		// Validate: first byte should be a console ID letter
		if (rid[0] < 'A' || rid[0] > 'Z') { free(hdr); continue; }

		// Check for duplicate
		bool dup = false;
		for (int d = 0; d < count; d++) {
			if (!strcmp(out[d].game_id, rid)) { dup = true; break; }
		}
		if (!dup) {
			memset(&out[count], 0, sizeof(cm_game_entry));
			strlcpy(out[count].game_id, rid, sizeof(out[count].game_id));
			// Game name starts at offset 0x20 in disc header
			char gname[33] = {0};
			memcpy(gname, hdr + 0x20, 32);
			gname[32] = '\0';
			if (gname[0])
				strlcpy(out[count].game_name, gname, sizeof(out[count].game_name));
			else
				strlcpy(out[count].game_name, rid, sizeof(out[count].game_name));
			count++;
		}
		free(hdr);
	}

	// Sort alphabetically by game name
	for (int i = 0; i < count - 1; i++) {
		for (int j = i + 1; j < count; j++) {
			if (strcasecmp(out[i].game_name, out[j].game_name) > 0) {
				cm_game_entry tmp = out[i];
				out[i] = out[j];
				out[j] = tmp;
			}
		}
	}

	return count;
}

int cm_game_picker(cm_game_entry *games, int count) {
	if (count == 0) return -1;

	int item_h = 22;
	int pad = 10;
	int menu_w = 380;
	int hint_h = 16;
	int menu_h = (count < 14 ? count : 14) * item_h + pad * 2 + hint_h;
	if (menu_h > 400) menu_h = 400;
	int menu_x = (640 - menu_w) / 2;
	int menu_y = (480 - menu_h) / 2;
	int max_visible = (menu_h - pad * 2 - hint_h) / item_h;

	int cursor = 0, scroll = 0;
	uiDrawObj_t *overlay = NULL;
	bool needs_redraw = true;
	bool debounce = true;

	// Build display labels: "GAME NAME (GALE)"
	char labels[MAX_GAME_ENTRIES][72];
	for (int i = 0; i < count; i++) {
		snprintf(labels[i], sizeof(labels[i]), "%.60s (%.4s)",
			games[i].game_name, games[i].game_id);
	}

	while (1) {
		cm_retire_tick();

		if (needs_redraw) {
			uiDrawObj_t *fresh = DrawEmptyBox(menu_x, menu_y,
				menu_x + menu_w, menu_y + menu_h);

			// Hint at bottom
			DrawAddChild(fresh, DrawStyledLabel(
				menu_x + menu_w / 2, menu_y + menu_h - pad - 2,
				"Also configurable in per-game settings",
				0.4f, ALIGN_CENTER, (GXColor){160, 160, 160, 255}));

			for (int vi = 0; vi < max_visible && scroll + vi < count; vi++) {
				int idx = scroll + vi;
				int iy = menu_y + pad + vi * item_h + item_h / 2;
				GXColor color;

				if (idx == cursor) {
					color = (GXColor){96, 107, 164, 255};
					DrawAddChild(fresh, cm_draw_highlight(
						menu_x + 6, menu_y + pad + vi * item_h,
						menu_w - 12, item_h));
				} else {
					color = defaultColor;
				}

				DrawAddChild(fresh, DrawStyledLabel(menu_x + 16, iy,
					labels[idx], 0.5f, ALIGN_LEFT, color));
			}

			if (overlay) DrawDispose(overlay);
			overlay = fresh;
			DrawPublish(fresh);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		u16 btns = padsButtonsHeld();
		s8 stickY = padsStickY();

		if (debounce) {
			if (!(btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_A | PAD_BUTTON_B)))
				debounce = false;
			continue;
		}

		if ((btns & PAD_BUTTON_UP) || stickY > 16) {
			if (cursor > 0) {
				cursor--;
				if (cursor < scroll) scroll = cursor;
				needs_redraw = true;
			}
			debounce = true;
		}
		if ((btns & PAD_BUTTON_DOWN) || stickY < -16) {
			if (cursor < count - 1) {
				cursor++;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				needs_redraw = true;
			}
			debounce = true;
		}
		if (btns & PAD_BUTTON_A) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			DrawDispose(overlay);
			return cursor;
		}
		if (btns & PAD_BUTTON_B) {
			while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
			DrawDispose(overlay);
			return -1;
		}
	}
}

int cm_vmc_find_assignments(const char *vmc_filename, cm_game_entry *out, int max) {
	cm_game_entry *all = calloc(MAX_GAME_ENTRIES, sizeof(cm_game_entry));
	if (!all) return 0;
	int num = cm_scan_game_configs(all, MAX_GAME_ENTRIES);
	int found = 0;

	for (int i = 0; i < num && found < max; i++) {
		ConfigEntry cfg;
		memset(&cfg, 0, sizeof(cfg));
		memcpy(cfg.game_id, all[i].game_id, 4);
		cfg.game_id[4] = '\0';
		config_find(&cfg);

		if ((cfg.memoryCardA[0] && strcmp(cfg.memoryCardA, vmc_filename) == 0)
			|| (cfg.memoryCardB[0] && strcmp(cfg.memoryCardB, vmc_filename) == 0)) {
			memcpy(&out[found], &all[i], sizeof(cm_game_entry));
			found++;
		}
	}
	free(all);
	return found;
}

void cm_vmc_clear_assignments(const char *vmc_filename) {
	cm_game_entry games[MAX_GAME_ENTRIES];
	int n = cm_vmc_find_assignments(vmc_filename, games, MAX_GAME_ENTRIES);

	for (int i = 0; i < n; i++) {
		ConfigEntry cfg;
		memset(&cfg, 0, sizeof(cfg));
		memcpy(cfg.game_id, games[i].game_id, 4);
		cfg.game_id[4] = '\0';
		config_find(&cfg);

		if (cfg.memoryCardA[0] && strcmp(cfg.memoryCardA, vmc_filename) == 0)
			cfg.memoryCardA[0] = '\0';
		if (cfg.memoryCardB[0] && strcmp(cfg.memoryCardB, vmc_filename) == 0)
			cfg.memoryCardB[0] = '\0';

		ConfigEntry defaults;
		memset(&defaults, 0, sizeof(defaults));
		memcpy(defaults.game_id, games[i].game_id, 4);
		config_defaults(&defaults);
		config_update_game(&cfg, &defaults, true);
	}
}

bool cm_assign_vmc_to_game(const char *vmc_filename, const char *game_id, int slot) {
	ConfigEntry entry;
	memset(&entry, 0, sizeof(entry));
	memcpy(entry.game_id, game_id, 4);
	entry.game_id[4] = '\0';

	// Load existing config for this game (fills defaults + any saved overrides)
	config_find(&entry);

	// Set the VMC filename for the requested slot
	if (slot == 0)
		strlcpy(entry.memoryCardA, vmc_filename, sizeof(entry.memoryCardA));
	else
		strlcpy(entry.memoryCardB, vmc_filename, sizeof(entry.memoryCardB));

	// Save — need defaults to diff against
	ConfigEntry defaults;
	memset(&defaults, 0, sizeof(defaults));
	memcpy(defaults.game_id, game_id, 4);
	config_defaults(&defaults);

	return config_update_game(&entry, &defaults, true) != 0;
}
