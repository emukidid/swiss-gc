/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include <sdcard/gcsd.h>
#include "ata.h"
#include "wkf.h"
#include <ogc/dvd.h>
#include "ff_cache/cache.h"

const DISC_INTERFACE *driver[FF_VOLUMES] = {&__io_gcsda, &__io_gcsdb, &__io_gcsd2, &__io_ataa, &__io_atab, &__io_atac, &__io_wkf, &__io_gcode};
static bool disk_isInit[FF_VOLUMES] = {0,0,0,0,0,0,0,0};

// Disk caches
CACHE *cache[FF_VOLUMES] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	if (pdrv >= DEV_MAX)
		return STA_NOINIT;

	if (disk_isInit[pdrv]) {
		if (!driver[pdrv]->isInserted())
			return STA_NODISK | STA_NOINIT;
		return (driver[pdrv]->features & FEATURE_MEDIUM_CANWRITE ? 0 : STA_PROTECT);
	}

	// Disk isn't initialized.
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	if (pdrv >= DEV_MAX)
		return STA_NOINIT;

	if (!disk_isInit[pdrv]) {
		if (!driver[pdrv]->startup())
			return STA_NOINIT;
	}
	if (!driver[pdrv]->isInserted())
		return STA_NODISK | STA_NOINIT;

	// Initialize the disk cache.
	// libfat/source/common.h:
	// - DEFAULT_CACHE_PAGES = 4
	// - DEFAULT_SECTORS_PAGE = 64
	// NOTE: endOfPartition isn't usable, since this is a
	// per-disk cache, not per-partition. Use UINT_MAX.
	cache[pdrv] = _FAT_cache_constructor(4, 64, driver[pdrv], (sec_t)-1, 512);

	// Device initialized.
	disk_isInit[pdrv] = true;
	return (driver[pdrv]->features & FEATURE_MEDIUM_CANWRITE ? 0 : STA_PROTECT);
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (pdrv >= DEV_MAX || count == 0)
		return RES_PARERR;
	if (__builtin_add_overflow_p(sector, count, (sec_t)0))
		return RES_PARERR;

	// Read from the cache.
	bool ret;
	if (count == 1) {
		// Single sector.
		ret = _FAT_cache_readSector(cache[pdrv], buff, sector);
	} else {
		// Multiple sectors.
		ret = _FAT_cache_readSectors(cache[pdrv], sector, count, buff);
	}

	return (ret ? RES_OK : RES_ERROR);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (pdrv >= DEV_MAX || count == 0)
		return RES_PARERR;
	if (__builtin_add_overflow_p(sector, count, (sec_t)0))
		return RES_PARERR;

	// Write to the cache.
	bool ret;
	if (count == 1) {
		// Single sector.
		ret = _FAT_cache_writeSector(cache[pdrv], buff, sector);
	} else {
		// Multiple sectors.
		ret = _FAT_cache_writeSectors(cache[pdrv], sector, count, buff);
	}

	return (ret ? RES_OK : RES_ERROR);
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	int ret = RES_PARERR;
	if (pdrv >= DEV_MAX)
		return ret;

	switch (cmd) {
		case CTRL_SYNC:
			ret = (_FAT_cache_flush(cache[pdrv]) ? RES_OK : RES_ERROR);
			break;

		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			ret = RES_OK;
			break;

		default:
			break;
	}

	return ret;
}



// Get the current system time as a FAT timestamp.
DWORD get_fattime(void)
{
	time_t now;
	struct tm tm;

	if (time(&now) == (time_t)-1) {
		return 0;
	}
	if (!localtime_r(&now, &tm)) {
		return 0;
	}

	/**
	 * Convert to an MS-DOS timestamp.
	 * Reference: http://elm-chan.org/fsw/ff/en/fattime.html
	 * Bits 31-25: Year. (base is 1980) (0..127)
	 * Bits 24-21: Month. (1..12)
	 * Bits 20-16: Day. (1..31)
	 * Bits 15-11: Hour. (0..23)
	 * Bits 10-5:  Minute. (0..59)
	 * Bits 4-0:   Seconds/2. (0..29)
	 */
	return (((tm.tm_year - 80) & 0x7F) << 25) |	// tm.tm_year base is 1900, not 1980.
	       (((tm.tm_mon + 1) & 0xF) << 21) |	// tm.tm_mon starts at 0, not 1.
	       ((tm.tm_mday & 0x1F) << 16) |
	       ((tm.tm_hour & 0x1F) << 11) |
	       ((tm.tm_min & 0x3F) << 5) |
	       ((tm.tm_sec / 2) & 0x1F);
}

DRESULT disk_shutdown (BYTE pdrv)
{
	if (pdrv >= DEV_MAX)
		return RES_PARERR;
	if (!disk_isInit[pdrv])
		return RES_OK;

	if (cache[pdrv]) {
		// Flush and destroy the cache.
		_FAT_cache_destructor(cache[pdrv]);
		cache[pdrv] = NULL;
	}

	// Shut down the device.
	driver[pdrv]->shutdown();
	disk_isInit[pdrv] = false;
	return RES_OK;
}

DRESULT disk_flush (BYTE pdrv)
{
	if (pdrv >= DEV_MAX)
		return RES_PARERR;
	if (!disk_isInit[pdrv])
		return RES_OK;

	if (cache[pdrv]) {
		// Flush the cache.
		_FAT_cache_flush(cache[pdrv]);
	}

	return RES_OK;
}
