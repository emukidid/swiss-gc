#ifndef __DVD_H
#define __DVD_H

#include <gctypes.h>

#define CHUNK_SIZE    256*1024
#define DISC_SIZE   0x57058000
#define WII_D5_SIZE 0x118240000LL
#define WII_D9_SIZE 0x230480000LL

//Used in ISO9660 Parsing
#define NO_FILES -1
#define NO_ISO9660_DISC -2
#define FATAL_ERROR -3
#define MAXIMUM_ENTRIES_PER_DIR 2048

typedef struct
{
	char name[128];
	int flags;
	int sector, size;
} file_entry; 

typedef struct
{
	file_entry file[MAXIMUM_ENTRIES_PER_DIR];
} file_entries; 

struct pvd_s
{
	char id[8];
	char system_id[32];
	char volume_id[32];
	char zero[8];
	unsigned long total_sector_le, total_sect_be;
	char zero2[32];
	unsigned long volume_set_size, volume_seq_nr;
	unsigned short sector_size_le, sector_size_be;
	unsigned long path_table_len_le, path_table_len_be;
	unsigned long path_table_le, path_table_2nd_le;
	unsigned long path_table_be, path_table_2nd_be;
	unsigned char root_direntry[34];
	char volume_set_id[128], publisher_id[128], data_preparer_id[128], application_id[128];
	char copyright_file_id[37], abstract_file_id[37], bibliographical_file_id[37];
	// some additional dates, but we don't care for them :)
}  __attribute__((packed));

extern file_entries *DVDToc;

int dvd_read_directoryentries(u64 offset, int size);
unsigned int dvd_get_error(void);
void dvd_set_streaming(char stream);
unsigned int dvd_read_id();
void dvd_motor_off();
int read_safe(void* dst, int offset, int len);
extern void load_dol_fn_inmem(void* dol, int size);
int DVD_ReadID(dvddiskid *diskID);
int DVD_Read(void* dst, u64 offset, int len);
int read_sector(void* buffer, int numSectors, u32 sector);
int DVD_LowRead64(void* dst, unsigned int len, u64 offset);
void dvd_set_offset(u64 offset);
void xeno_disable();
void dvd_stop_laser();
void dvd_unlock();
void drive_version(u8 *buf);
void dvd_reset();
unsigned int npdp_inquiry(unsigned char *dst);
unsigned int npdp_getid(unsigned char *dst);
#endif
