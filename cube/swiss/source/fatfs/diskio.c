/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

#include <sdcard/gcsd.h>
#include "ata.h"
#include "wkf.h"
#include <ogc/dvd.h>
#include "aram.h"
#include <dvm.h>

static DISC_INTERFACE *iface[FF_VOLUMES] = {&__io_gcsda, &__io_gcsdb, &__io_gcsd2, &__io_ataa, &__io_atab, &__io_atac, &__io_wkf, &__io_gcode, &__io_aram};
static DvmDisc *disc[FF_VOLUMES] = {NULL};


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	if (pdrv >= DEV_MAX)
		return STA_NODISK | STA_NOINIT;
	if (!disc[pdrv])
		return STA_NOINIT;

	return (disc[pdrv]->features & FEATURE_MEDIUM_CANWRITE) ? 0 : STA_PROTECT;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
	if (pdrv >= DEV_MAX)
		return STA_NODISK | STA_NOINIT;

	if (disc[pdrv])
		disc[pdrv]->vt->destroy(disc[pdrv]);

	disc[pdrv] = dvmDiscCreate(iface[pdrv]);

	if (!disc[pdrv])
		return STA_NOINIT;

	switch (pdrv) {
		case DEV_ARAM:
			disc[pdrv] = dvmDiscCacheCreate(disc[pdrv], 2, 8);
			break;

		default:
			disc[pdrv] = dvmDiscCacheCreate(disc[pdrv], 128, 8);
			break;
	}

	return (disc[pdrv]->features & FEATURE_MEDIUM_CANWRITE) ? 0 : STA_PROTECT;
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
	if (!disc[pdrv])
		return RES_NOTRDY;

	return disc[pdrv]->vt->read_sectors(disc[pdrv], buff, sector, count) ? RES_OK : RES_ERROR;
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
	if (!disc[pdrv])
		return RES_NOTRDY;
	if (!(disc[pdrv]->features & FEATURE_MEDIUM_CANWRITE))
		return RES_WRPRT;

	return disc[pdrv]->vt->write_sectors(disc[pdrv], buff, sector, count) ? RES_OK : RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	if (pdrv >= DEV_MAX)
		return RES_PARERR;
	if (!disc[pdrv])
		return RES_NOTRDY;

	switch (ctrl) {
		case CTRL_SYNC:
			disc[pdrv]->vt->flush(disc[pdrv]);
			res = RES_OK;
			break;

		case GET_SECTOR_COUNT:
			*(LBA_t*)buff = disc[pdrv]->num_sectors;
			res = RES_OK;
			break;

		case GET_SECTOR_SIZE:
			*(WORD*)buff = disc[pdrv]->sector_sz;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
			break;
	}

	return res;
}



// Get the current system time as a FAT timestamp.
DWORD get_fattime(void)
{
	time_t now;
	struct tm tm;

	if (time(&now) == (time_t)-1) {
		return (((FF_NORTC_YEAR - 1980) & 0x7F) << 25) |
		       ((FF_NORTC_MON & 0xF) << 21) |
		       ((FF_NORTC_MDAY & 0x1F) << 16);
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

DRESULT disk_shutdown (
	BYTE pdrv
)
{
	if (pdrv >= DEV_MAX)
		return RES_PARERR;
	if (!disc[pdrv])
		return RES_NOTRDY;

	disc[pdrv]->vt->destroy(disc[pdrv]);
	disc[pdrv] = NULL;
	return RES_OK;
}
