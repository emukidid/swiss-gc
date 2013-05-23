#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/dvd.h>	
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <stdint.h>
#include "main.h"
#include "dvd.h"
#include "drivecodes.h"

/* Simple DVD functions */
int is_unicode,files;
static int last_current_dir = -1;
u32 inquiryBuf[2048] __attribute__((aligned(32)));
file_entries *DVDToc = NULL; //Dynamically allocate this
volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;

void dvd_reset()
{
	dvd[1] = 2;
	volatile unsigned long v = *(volatile unsigned long*)0xcc003024;
	*(volatile unsigned long*)0xcc003024 = (v & ~4) | 1;
	sleep(1);
	*(volatile unsigned long*)0xcc003024 = v | 5;
}

unsigned int dvd_read_id()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0x80000000;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	
	while(( dvd[7] & 1) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)){ VIDEO_WaitVSync (); }
	
	if (dvd[0] & 0x4){
		return 1;
	}
	return 0;
}

void dvd_set_streaming(char stream)
{
	if(stream)
	{
		dvd[0] = 0x2E;
		dvd[2] = 0xE4010000;
		dvd[7] = 1;
		while (dvd[7] & 1);
	}
	else
	{		
		dvd[0] = 0x2E;
		dvd[2] = 0xE4000000;
		dvd[7] = 1;
		while (dvd[7] & 1);
	}
}

unsigned int dvd_get_error(void)
{
	dvd[2] = 0xE0000000;
	dvd[8] = 0;
	dvd[7] = 1;
	while (dvd[7] & 1);
	return dvd[8];
}

void dvd_motor_off()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0xe3000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

void dvd_set_offset(u64 offset)
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = (is_gamecube())?(0x32000000):(0xD9000000);
	dvd[3] = ((offset>>2)&0xFFFFFFFF);
	dvd[4] = 0;
	dvd[7] = 1;
	while (dvd[7] & 1);
}

int dvd_seek(int offset)
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xAB000000;
	dvd[3] = offset >> 2;
	dvd[4] = 0;

	dvd[7] = 1;
	while (dvd[7] & 1);

	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

void dvd_setstatus()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0xee060300;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

void dvd_setextension()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0x55010000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

void dvd_unlock()
{
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF014D41;
	dvd[3] = 0x54534849;	//MATS
	dvd[4] = 0x54410200;	//HITA
	dvd[7] = 1;
	while (dvd[7] & 1);
	
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF004456;
	dvd[3] = 0x442D4741;	//DVD-
	dvd[4] = 0x4D450300;	//GAME
	dvd[7] = 1;
	while (dvd[7] & 1);
}

int dvd_writemem_32(u32 addr, u32 dat)
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xFE010100;	
	dvd[3] = addr;
	dvd[4] = 0x00040000;	
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 3;
	while (dvd[7] & 1);

	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = dat;
	dvd[7] = 1;
	while (dvd[7] & 1);

	return 0;
}

int dvd_writemem_array(u32 addr, void* buf, u32 size)
{
	u32* ptr = (u32*)buf;
	int rem = size;

	while (rem > 0)
	{
		dvd_writemem_32(addr, *ptr++);
		addr += 4;
		rem -= 4;
	}
	return 0;
}

#define DEBUG_STOP_DRIVE 0
#define DEBUG_START_DRIVE 0x100
#define DEBUG_ACCEPT_COPY 0x4000
#define DEBUG_DISC_CHECK 0x8000

void dvd_motor_on_extra()
{
	dvd[0] = 0x2e;
	dvd[1] = 1;
	dvd[2] = (0xfe110000 | DEBUG_ACCEPT_COPY | DEBUG_START_DRIVE);
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while (dvd[7] & 1);
}

void* drive_patch_ptr(u32 driveVersion)
{
	if(driveVersion == 0x20020402)
		return &Drive04;
	if(driveVersion == 0x20010608)
		return &Drive06;
	if(driveVersion == 0x20010831)
		return &DriveQ;
	if(driveVersion == 0x20020823)
		return &Drive08;
	return 0;
}

void dvd_enable_patches() 
{
	// Get the drive date
	u8* driveDate = (u8*)memalign(32,32);
	memset(driveDate,0,32);
	drive_version(driveDate);
	
	u32 driveVersion = *(u32*)&driveDate[0];
	free(driveDate);
	
	if(!driveVersion) return;	// Unsupported drive

	void* patchCode = drive_patch_ptr(driveVersion);
	
	print_gecko("Drive date %08X\r\nUnlocking DVD\r\n",driveVersion);
	dvd_unlock();
	print_gecko("Unlocking DVD - done\r\nWrite patch\r\n");
	dvd_writemem_array(0xff40d000, patchCode, 0x1F0);
	dvd_writemem_32(0x804c, 0x00d04000);
	print_gecko("Write patch - done\r\nSet extension %08X\r\n",dvd_get_error());
	dvd_setextension();
	print_gecko("Set extension - done\r\nUnlock again %08X\r\n",dvd_get_error());
	dvd_unlock();
	print_gecko("Unlock again - done\r\nDebug Motor On %08X\r\n",dvd_get_error());
	dvd_motor_on_extra();
	print_gecko("Debug Motor On - done\r\nSet Status %08X\r\n",dvd_get_error());
	dvd_setstatus();
	print_gecko("Set Status - done %08X\r\n",dvd_get_error());
	dvd_read_id();
	print_gecko("Read ID %08X\r\n",dvd_get_error());
}


void dvd_stop_laser()
{
	dvd[0] = 0x2e;
	dvd[1] = 1;
	dvd[2] = 0xfe080000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while (dvd[7] & 1);
	dvd_seek(0);
}

void drive_version(u8 *buf)
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0x12000000;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (long)(&inquiryBuf[0]);
	dvd[6] = 0x20;
	dvd[7] = 3;
	
	int retries = 1000000;
	while(( dvd[7] & 1) && retries) {
		retries --;
	}
	DCFlushRange((void *)inquiryBuf, 0x20);
	
	if(!retries) {
		buf[0] = 0;
	}
	else {
		memcpy(buf,&inquiryBuf[1],8);
	}
}


/* 
* NPDP Section 
*/

unsigned int npdp_inquiry(unsigned char *dst)
{
	static unsigned char inq[256] __attribute__((__aligned__(32)));
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0x12000000;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (unsigned long)inq;
	dvd[6] = 0x20;
	dvd[7] = 3;
	
	while (dvd[7] & 1);
	DCFlushRange((void *)inq, 0x20);
	
	if (dvd[6])
		return 1;
	memcpy(dst, inq, 0x20);
	return 0;
}

unsigned int npdp_getid(unsigned char *dst)
{
	static unsigned char id[256] __attribute__((__aligned__(32)));
	DCInvalidateRange(id, 0x40);

	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0x4E504450;	//"NPDP"
	dvd[3] = 0x2D494400;	//"-ID\0"
	dvd[4] = 0x40;
	dvd[5] = (unsigned long)id;
	dvd[6] = 0x40;
	dvd[7] = 3;
	while (dvd[7] & 1);
	
	if (dvd[6])
		return 1;
	memcpy(dst, id, 0x40);
	return 0;
}


/* 
DVD_LowRead64(void* dst, unsigned int len, uint64_t offset)
  Read Raw, needs to be on sector boundaries (on a real DVD Drive)
  Has 8,796,093,020,160 byte limit (8 TeraBytes)
  Synchronous function.
    return -1 if offset is out of range
*/
int DVD_LowRead64(void* dst, unsigned int len, uint64_t offset)
{
  volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
  if(offset>>2 > 0xFFFFFFFF)
    return -1;
    
  if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
		dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000000;
	dvd[3] = offset >> 2;
	dvd[4] = len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
	DCInvalidateRange(dst, len);
	while (dvd[7] & 1);

	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

/* 
DVD_Read(void* dst, uint64_t offset, int len)
  Reads from any offset and handles alignment from device
  Synchronous function.
    return -1 if offset is out of range
*/
int DVD_Read(void* dst, uint64_t offset, int len)
{
	int ol = len;
	int ret = 0;	
  char *sector_buffer = (char*)memalign(32,2048);
	while (len)
	{
		uint32_t sector = offset / 2048;
		ret |= DVD_LowRead64(sector_buffer, 2048, sector * 2048);
		uint32_t off = offset & 2047;

		int rl = 2048 - off;
		if (rl > len)
			rl = len;
		memcpy(dst, sector_buffer + off, rl);	

		offset += rl;
		len -= rl;
		dst += rl;
	}
	free(sector_buffer);
	if(ret)
		return -1;

	return ol;
}

/* 
DVD_ReadID()
  Read the GameID (first 32 bytes) of a Game
    returns 0 on success
  Synchronous function.
*/
int DVD_ReadID(dvddiskid *diskID)
{
  volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (unsigned int)diskID;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	while (dvd[7] & 1);
	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

int read_sector(void* buffer, int numSectors, uint32_t sector){
	return DVD_Read(buffer, sector*2048, numSectors*2048);
}

void xeno_disable() {
  char *readBuf = (char*)memalign(32,64*1024);
  if(!readBuf) {
    return;
  }
  DVD_LowRead64(readBuf, 64*1024, 0);           //xeno GC enable patching
  DVD_LowRead64(readBuf, 64*1024, 0x1000000);   //xeno GC disable patching
  free(readBuf);
}

int read_direntry(unsigned char* direntry)
{
       int nrb = *direntry++;
       ++direntry;

       int sector;

       direntry += 4;
       sector = (*direntry++) << 24;
       sector |= (*direntry++) << 16;
       sector |= (*direntry++) << 8;
       sector |= (*direntry++);        

       int size;

       direntry += 4;

       size = (*direntry++) << 24;
       size |= (*direntry++) << 16;
       size |= (*direntry++) << 8;
       size |= (*direntry++);

       direntry += 7; // skip date

       int flags = *direntry++;
       ++direntry; ++direntry; direntry += 4;

       int nl = *direntry++;

       char* name = DVDToc->file[files].name;

       DVDToc->file[files].sector = sector;
       DVDToc->file[files].size = size;
       DVDToc->file[files].flags = flags;

       if ((nl == 1) && (direntry[0] == 1)) // ".."
       {
               DVDToc->file[files].name[0] = 0;
               if (last_current_dir != sector)
                       files++;
       }
       else if ((nl == 1) && (direntry[0] == 0))
       {
               last_current_dir = sector;
       }
       else
       {
               if (is_unicode)
               {
                       int i;
                       for (i = 0; i < (nl / 2); ++i)
                               name[i] = direntry[i * 2 + 1];
                       name[i] = 0;
                       nl = i;
               }
               else
               {
                       memcpy(name, direntry, nl);
                       name[nl] = 0;
               }

               if (!(flags & 2))
               {
                       if (name[nl - 2] == ';')
                               name[nl - 2] = 0;

                       int i = nl;
                       while (i >= 0)
                               if (name[i] == '.')
                                       break;
                               else
                                       --i;

                       ++i;

               }
               else
               {
                       name[nl++] = '/';
                       name[nl] = 0;
               }

               files++;
       }

       return nrb;
}



void read_directory(int sector, int len)
{
  int ptr = 0;
  unsigned char *sector_buffer = (unsigned char*)memalign(32,2048);
  read_sector(sector_buffer, 1, sector);
  
  files = 0;
  memset(DVDToc,0,sizeof(file_entries));
  while (len > 0)
  {
    ptr += read_direntry(sector_buffer + ptr);
    if (ptr >= 2048 || !sector_buffer[ptr])
    {
      len -= 2048;
      sector++;
      read_sector(sector_buffer, 1, sector);
      ptr = 0;
    }
  }
  free(sector_buffer);
}


int dvd_read_directoryentries(uint64_t offset, int size) {
  int sector = 16;
  unsigned char *bufferDVD = (unsigned char*)memalign(32,2048);
  struct pvd_s* pvd = 0;
  struct pvd_s* svd = 0;
  
  if(DVDToc)
  {
    free(DVDToc);
    DVDToc = NULL;
  }
  DVDToc = memalign(32,sizeof(file_entries));
  
  while (sector < 32)
  {
    if (read_sector(bufferDVD, 1, sector)!=2048)
    {
      free(bufferDVD);
      free(DVDToc);
      DVDToc = NULL;
      return FATAL_ERROR;
    }
    if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\2CD001\1", 8))
    {
      svd = (void*)bufferDVD;
      break;
    }
    ++sector;
  }
  
  
  if (!svd)
  {
    sector = 16;
    while (sector < 32)
    {
      if (read_sector(bufferDVD, 1, sector)!=2048)
      {
        free(bufferDVD);
        free(DVDToc);
        DVDToc = NULL;
        return FATAL_ERROR;
      }
      
      if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\1CD001\1", 8))
      {
        pvd = (void*)bufferDVD;
        break;
      }
      ++sector;
    }
  }
  
  if ((!pvd) && (!svd))
  {
    free(bufferDVD);
    free(DVDToc);
    DVDToc = NULL;
    return NO_ISO9660_DISC;
  }
  
  files = 0;
  if (svd)
  {
    is_unicode = 1;
    read_direntry(svd->root_direntry);
  }
  else
  {
    is_unicode = 0;
    read_direntry(pvd->root_direntry);
  }
  
  if((size + offset) == 0)  // enter root
    read_directory(DVDToc->file[0].sector, DVDToc->file[0].size);
  else
    read_directory(offset>>11, size);

  free(bufferDVD);
  if(files>0)
    return files;
  return NO_FILES;
}
