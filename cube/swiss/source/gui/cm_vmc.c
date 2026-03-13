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
			cm_log("  FAT chain error: block=%u (iter %d, offset=%u/%u)",
				block, i, offset, filesize);
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

int vmc_read_saves(const char *vmc_path, card_entry *entries, int max_entries,
	s32 *out_mem_size, s32 *out_sector_size) {

	// Read entire system area (5 blocks = 40KB)
	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) return 0;

	// Get card size info from header block
	const u8 *hdr = sysarea + MC_BLOCK_HEADER * MC_BLOCK_SIZE;
	u16 card_mbits = *(const u16 *)(hdr + MC_HDR_CARD_SIZE);
	// Convert Mbits to bytes: Mbits * 1024 * 1024 / 8
	*out_mem_size = card_mbits * 131072;
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

	// Pick active directory and BAT
	int dir_blk = vmc_active_dir_block(sysarea);
	int bat_blk = vmc_active_bat_block(sysarea);
	const u8 *dir = sysarea + dir_blk * MC_BLOCK_SIZE;
	const u8 *bat = sysarea + bat_blk * MC_BLOCK_SIZE;

	u16 free_blocks = *(const u16 *)(bat + MC_BAT_FREE_BLOCKS);
	cm_log("VMC: dir=%c, bat=%c, total=%u blocks, free=%u",
		dir_blk == MC_BLOCK_DIR_A ? 'A' : 'B',
		bat_blk == MC_BLOCK_BAT_A ? 'A' : 'B',
		total_blocks, free_blocks);

	// Parse directory entries
	int count = 0;
	for (int i = 0; i < MC_DIR_ENTRIES && count < max_entries; i++) {
		const GCI *raw = (const GCI *)(dir + i * MC_DIR_ENTRY_SIZE);

		// Empty entry: gamecode starts with 0xFF
		if (raw->gamecode[0] == 0xFF)
			continue;
		// Also skip entries with all-zero gamecode (cleared)
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

		u16 first_block = raw->first_block;
		u32 comment_addr = raw->comment_addr;

		cm_log("VMC [%d] \"%s\" gc=%s sz=%u blk1=%u icon_addr=0x%X icon_fmt=0x%04X icon_spd=0x%04X bnr_fmt=0x%02X comment=0x%X",
			i, e->filename, e->gamecode, e->filesize, first_block,
			e->icon_addr, e->icon_fmt, e->icon_speed, e->banner_fmt, comment_addr);

		// Read file data for graphics and comment
		if (e->filesize > 0 && first_block >= MC_FIRST_DATA_BLOCK) {
			u8 *file_data = vmc_read_file_data(vmc_path, bat,
				first_block, e->filesize, total_blocks);
			if (file_data) {
				cm_log("  file_data OK (%u bytes), first 8: %02X %02X %02X %02X %02X %02X %02X %02X",
					e->filesize, file_data[0], file_data[1], file_data[2], file_data[3],
					file_data[4], file_data[5], file_data[6], file_data[7]);

				// Parse comment (game name + description)
				cm_parse_comment(file_data, e->filesize, comment_addr, e);

				// Parse graphics (banner + icons)
				if (e->icon_addr != (u32)-1 && e->icon_addr < e->filesize) {
					u8 *gfx = file_data + e->icon_addr;
					u32 gfx_len = e->filesize - e->icon_addr;
					cm_log("  gfx @ 0x%X len=%u, first 8: %02X %02X %02X %02X %02X %02X %02X %02X",
						e->icon_addr, gfx_len, gfx[0], gfx[1], gfx[2], gfx[3],
						gfx[4], gfx[5], gfx[6], gfx[7]);
					cm_parse_save_graphics(gfx, gfx_len,
						e->banner_fmt, e->icon_fmt, e->icon_speed, e);
					cm_log("  after parse: banner=%p icon=%p (frames=%d)",
						e->banner, e->icon, e->icon ? e->icon->num_frames : 0);
				} else {
					cm_log("  SKIP gfx: icon_addr=0x%X filesize=%u", e->icon_addr, e->filesize);
				}
				free(file_data);
			} else {
				cm_log("  vmc_read_file_data FAILED");
			}
		} else {
			cm_log("  SKIP read: filesize=%u first_block=%u", e->filesize, first_block);
		}

		count++;
	}

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

	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Reading VMC save...");
	DrawPublish(msgBox);

	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to read VMC.");
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
		msgBox = DrawMessageBox(D_FAIL, "Failed to read save data from VMC.");
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

	cm_log("VMC export: \"%s\" gc=%s sz=%u", entry->filename, entry->gamecode, entry->filesize);
	bool result = cm_write_gci_to_sd(&gci, filedata, data_len);
	free(filedata);
	return result;
}

// --- VMC delete ---

bool vmc_delete_save(const char *vmc_path, card_entry *entry) {
	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Deleting save from VMC...");
	DrawPublish(msgBox);

	u32 sysarea_size = MC_FIRST_DATA_BLOCK * MC_BLOCK_SIZE;
	u8 *sysarea = vmc_read_range(vmc_path, 0, sysarea_size);
	if (!sysarea) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to read VMC.");
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

	cm_log("VMC delete: \"%s\" fileno=%d first_block=%u blocks=%u",
		entry->filename, entry->fileno, first_block, num_blocks8);

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
	cm_log("  Freed %d blocks, new free count=%u", freed, *free_blocks);

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
		msgBox = DrawMessageBox(D_FAIL, "Failed to write VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	msgBox = DrawMessageBox(D_INFO, "Save deleted.");
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

	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Importing save to VMC...");
	DrawPublish(msgBox);

	u32 expected_len = gci->filesize8 * MC_BLOCK_SIZE;
	u16 need_blocks = gci->filesize8;

	if (save_len != expected_len) {
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "GCI size mismatch: got %dK, expected %dK",
			(int)(save_len / 1024), (int)(expected_len / 1024));
		msgBox = DrawMessageBox(D_FAIL, errmsg);
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
		msgBox = DrawMessageBox(D_FAIL, "Failed to read VMC.");
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
		msgBox = DrawMessageBox(D_WARN,
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
		msgBox = DrawMessageBox(D_INFO, "Importing save to VMC...");
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
		free(new_dir); free(new_bat); free(sysarea);
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "Not enough space.\nNeed %d blocks, have %d free.",
			need_blocks, *free_count);
		msgBox = DrawMessageBox(D_FAIL, errmsg);
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
		msgBox = DrawMessageBox(D_FAIL, "No free directory slots in VMC.");
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
		msgBox = DrawMessageBox(D_FAIL, "FAT allocation failed.");
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

	cm_log("VMC import: \"%s\" gc=%.4s dir_slot=%d first_block=%u blocks=%d",
		gci_filename, gci->gamecode, dir_slot, alloc_blocks[0], need_blocks);

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
		msgBox = DrawMessageBox(D_FAIL, "Failed to write VMC.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Imported \"%s\" to VMC.", gci_filename);
	msgBox = DrawMessageBox(D_INFO, done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
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
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Invalid GCI file.");
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
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
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
