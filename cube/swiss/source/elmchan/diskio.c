/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include <gccore.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ogc/exi.h>
#include <ogc/disc_io.h>
#include <sdcard/gcsd.h>
#include <math.h>
#include "swiss.h"
#include "main.h"
#include "dvd.h"
#include "ata.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

const DISC_INTERFACE* cardA = &__io_gcsda;
const DISC_INTERFACE* cardB = &__io_gcsdb;
int needsReInit = 0;
int dvd_init = 0;
int ide_init = 0;

void EXI_ResetSD(BYTE drv) {
	/* Wait for any pending transfers to complete */
	while ( *(vu32 *)0xCC00680C & 1 );
	while ( *(vu32 *)0xCC006820 & 1 );
	while ( *(vu32 *)0xCC006834 & 1 );
	*(vu32 *)0xCC00680C = 0xC0A;
	*(vu32 *)0xCC006820 = 0xC0A;
	*(vu32 *)0xCC006834 = 0x80A;
	/*** Needed to re-kick after insertion etc ***/
	EXI_ProbeEx(drv);
}

void reset_dvd() {
  DVD_Reset(DVD_RESETHARD);
  dvd_read_id();
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{ 
  DSTATUS res = 0;
  int i;
  
  //IDE on EXI read support
  if(curDevice == IDEEXI) {
	ataDriveInit(drv);    // init drive
	res = 1;
	ide_init = 1;
  }
  else if(drv==1)
  {
    cardB->shutdown();
    for(i=0; i<10; i++) //fails sometimes
    {
      res = cardB->startup();
      if(res) break;
    }
  }
  else {
    cardA->shutdown();
    for(i=0; i<10; i++) //fails sometimes
    {
      res = cardA->startup();
      if(res) break;
    }
  }
  
	if(res)
	  return RES_OK;
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
  DSTATUS res = -1;
  
  if(curDevice == IDEEXI) {
	if(ataIsInserted(drv)) {
	 	return RES_OK;
 	}
  }
  else if(drv==1) {
    res = cardB->isInserted();
  }
  else {
    res = cardA->isInserted();
  }

  if(res) {
	  return RES_OK;
  }
	  
  return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		  /* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
  DRESULT res = RES_ERROR;

  if(curDevice == IDEEXI) {
	if(!ataReadSectors(drv, (u64)sector, count, buff)) {
	  return RES_OK;
  	}
  }
  else if(drv==1) {
    res = cardB->readSectors(sector, count, buff);
  }
  else {
    res = cardA->readSectors(sector, count, buff);
  }

  if(res)
	  return RES_OK;

  return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	DWORD count			/* Number of sectors to write (1..255) */
)
{
  DRESULT res = RES_ERROR;
  
  if(curDevice == IDEEXI) {
	  if(!ataWriteSectors(drv, (u64)sector, count, (u8*)buff)) {
		  return RES_OK;
	  }
  }
  else if(drv==1) {
    res = cardB->writeSectors(sector, count, buff);
  }
  else {
    res = cardA->writeSectors(sector, count, buff);
  }

  if(res)
	  return RES_OK;
	  
	return RES_ERROR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  if(ctrl == GET_SECTOR_COUNT) {
    /* SD Spec 1.00 calculation.
     * Memory Capacity = BLOCKNR * BLOCK_LEN
     * BLOCKNR = (C_SIZE + 1) * MULT
     * MULT = 2^(C_SIZ_MULT+2) (C_SIZE_MULT << 8)
     * BLOCK_LEN = 2^READ_BL_LEN (READ_BL_LEN << 12)
    **/
    sdgecko_readCSD(drv);
    if((g_CSD[drv][0]>>6) & 3) { // high capacity spec
      u32 capacity = ((((*(u32*)(&g_CSD[drv][7])) >> 8)+1)*1024);
      memcpy(buff, &capacity, sizeof(u32));
    }
    else { // spec 1.00
      u32 c_size = ((*(u32*)(&g_CSD[drv][6])) >> 14) & 0xFFF;
      u32 c_size_mult = ((*(u16*)(&g_CSD[drv][9])) >> 7) & 7;
      u32 mult = pow(2,(c_size_mult+2));
      u32 blocknr = (c_size + 1) * mult;
      u32 capacity = blocknr;
      memcpy(buff, &capacity, sizeof(u32));
    }
    return RES_OK;
  }
  if(ctrl == GET_BLOCK_SIZE) {
    sdgecko_readCSD(drv);
    if((g_CSD[drv][0]>>6) & 3) { // high capacity spec
      u32 block_len = 512;
      memcpy(buff, &block_len, sizeof(u32));
      return RES_OK;
    }
    else {  //spec 1.00
      u32 read_bl_len = ((*(u16*)(&g_CSD[drv][4]))) & 0xF;
      u32 block_len = pow(2,read_bl_len);
      memcpy(buff, &block_len, sizeof(u32));
      return RES_OK;
    }
  }
	return RES_OK;
}

/****************************************************************************
* get_fattime
*
* bit31:25
*    Year from 1980 (0..127)
* bit24:21
*    Month (1..12)
* bit20:16
*    Date (1..31)
* bit15:11
*    Hour (0..23)
* bit10:5
*    Minute (0..59)
* bit4:0
*    Second/2 (0..29)
*
* I confess this is a mess - but it works!
****************************************************************************/
DWORD get_fattime( void )
{
  time_t rtc;
  char realtime[128];
  int year, month, day, hour, minute, second;
  char mth[4];
  char *p;
  char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  u32 ret = 0;

  rtc = time(NULL);
  strcpy(realtime, ctime(&rtc));

  /* ctime returns dates formatted to ASCII thus,
    DDD MMM 000 HH:MM:SS CCYY
  */
  year = atoi( realtime + 20 );
  day = atoi( realtime + 7 );
  hour = atoi( realtime + 11 );
  minute = atoi( realtime + 14 );
  second = atoi( realtime + 17 );

  memcpy(mth, realtime + 4, 3);
  mth[3] = 0;
  p = strstr(months, mth);
  if ( !p )
    month = 1;
  else
    month = (( p - months ) / 3 ) + 1;

  /* Convert to DOS time */
  /* YYYYYYY MMMM DDDDD HHHHH MMMMMM SSSSS */
  /* 1098765 4321 09876 54321 098765 43210 */

  year = year - 1980;
  ret = ( year & 0x7f ) << 25;
  ret |= ( month << 21 );
  ret |= ( day << 16 );
  ret |= ( hour << 11 );
  ret |= ( minute << 5 );
  ret |= ( second & 0x1f );

  return ret;

}

