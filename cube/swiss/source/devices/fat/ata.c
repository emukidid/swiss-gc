/**
 * IDE EXI Driver for Gamecube & Wii
 *
 * Based loosely on code written by Dampro
 * Re-written by emu_kidid
**/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <ogc/exi.h>
#include <ata.h>
#include "main.h"
#include "swiss.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

u16 buffer[256] ATTRIBUTE_ALIGN (32);
static int __ata_init[2] = {0,0};

// Drive information struct
typeDriveInfo ataDriveInfo;

static inline u16 bswap16(u16 val)
{
	u16 tmp = val;
	return __lhbrx(&tmp,0);
}

// Returns 8 bits from the ATA Status register
static inline u8 ataReadStatusReg(int chn)
{
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u16 dat = 0x1700;
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,2,EXI_WRITE,NULL);
	EXI_Sync(chn);
	EXI_Imm(chn,&dat,1,EXI_READ,NULL);
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return *(u8*)&dat;
}

// Returns 8 bits from the ATA Error register
static inline u8 ataReadErrorReg(int chn)
{
	// read ATA_REG_ERROR | 0x00 (dummy)
	u16 dat = 0x1100;
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,2,EXI_WRITE,NULL);
	EXI_Sync(chn);
	EXI_Imm(chn,&dat,1,EXI_READ,NULL);
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return *(u8*)&dat;
}

// Writes 8 bits of data out to the specified ATA Register
static inline void ataWriteByte(int chn, u8 addr, u8 data)
{
	u32 dat = 0x80000000 | (addr << 24) | (data<<16);
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,3,EXI_WRITE,NULL);	
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}

// Writes 16 bits to the ATA Data register
static inline void ataWriteu16(int chn, u16 data) 
{
	// write 16 bit to ATA_REG_DATA | data LSB | data MSB | 0x00 (dummy)
	u32 dat = 0xD0000000 | (((data>>8) & 0xff)<<16) | ((data & 0xff)<<8);
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,4,EXI_WRITE,NULL);
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}


// Returns 16 bits from the ATA Data register
static inline u16 ataReadu16(int chn) 
{
	// read 16 bit from ATA_REG_DATA | 0x00 (dummy)
	u16 dat = 0x5000;  	
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,2,EXI_WRITE,NULL);
	EXI_Sync(chn);
	EXI_Imm(chn,&dat,2,EXI_READ,NULL); // read LSB & MSB
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    return dat;
}


// Initialises the 32 bit read mode
static inline void ataRead_init_mult(int chn, u16 numSectors) 
{
	u16 dwords = (numSectors<<7);
	// 011xxxxx  - read multiple (32bit words) | LSB (num dwords) | MSB (num dwords) | 0x00 (dummy)
	u32 dat = 0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8);
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,4,EXI_WRITE,NULL);
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
}

// Reads 32 bits of data from the HDD
static inline u32 ataRead32_mult(int chn) 
{
	u32 dat;
	EXI_Lock(chn, 0, NULL);
	EXI_Select(chn,0,GC_SD_SPEED);
	EXI_Imm(chn,&dat,4,EXI_READ,NULL);	// read LSB1 MSB1 LSB0 MSB0
	EXI_Sync(chn);
	EXI_Deselect(chn);
	EXI_Unlock(chn);
    // output 32 bit read from hdd
    return dat;
}

// Sends the IDENTIFY command to the HDD
// Returns 0 on success, -1 otherwise
u32 _ataDriveIdentify(int chn) {
  	u16 tmp,retries = 50;
  	u32 i = 0;

  	memset(&ataDriveInfo, 0, sizeof(typeDriveInfo));
  	
  	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, 0);
  	
	// Wait for drive to be ready (BSY to clear) - 5 sec timeout
	while((ataReadStatusReg(chn) & ATA_SR_BSY) && retries) {
		usleep(100000);	//sleep for 0.1 seconds
		retries--;
	}
	if(!retries) {
		return -1;
	}
    
  	// Write to the device register (single drive)
	ataWriteByte(chn, ATA_REG_DEVICE, 0);
	// Write the identify command
  	ataWriteByte(chn, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

  	// Wait for BSY to clear - 1 sec timeout
  	retries = 10;
  	do {
	  	tmp = ataReadStatusReg(chn);
	  	usleep(100000);	//sleep for 0.1 seconds
		retries--;
  	}
	while((tmp & ATA_SR_BSY) && retries);
	
	// If the error bit was set, fail.
	if(tmp & ATA_SR_ERR) {
		return -1;
	}

	// Wait for drive to request data transfer - 1 sec timeout
	retries = 10;
	while((!(ataReadStatusReg(chn) & ATA_SR_DRQ)) && retries) {
		usleep(100000);	//sleep for 0.1 seconds
		retries--;
	}
	if(!retries) {
		return -1;
	}
    
	u16 *ptr = (u16*)(&buffer[0]);
	
	// Read Identify data from drive
	for (i=0; i<256; i++) {
		tmp = ataReadu16(chn); // get data
	   *ptr++ = bswap16(tmp); // swap
	}
	
	// Get the info out of the Identify data buffer
	// From the command set, check if LBA48 is supported
	u16 commandSet = *(u16*)(&buffer[ATA_IDENT_COMMANDSET]);
	ataDriveInfo.lba48Support = (commandSet>>8) & ATA_IDENT_LBA48MASK;
	
	if(ataDriveInfo.lba48Support) {
		u16 lbaHi = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS+2]);
		u16 lbaMid = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS+1]);
		u16 lbaLo = *(u16*) (&buffer[ATA_IDENT_LBA48SECTORS]);
		ataDriveInfo.sizeInSectors = (u64)(((u64)lbaHi << 32) | (lbaMid << 16) | lbaLo);
		ataDriveInfo.sizeInGigaBytes = (u32)((ataDriveInfo.sizeInSectors<<9) / 1024 / 1024 / 1024);
	}
	else {
		ataDriveInfo.sizeInSectors =	((*(u16*) &buffer[ATA_IDENT_LBASECTORS+1])<<16) | 
										(*(u16*) &buffer[ATA_IDENT_LBASECTORS]);
		ataDriveInfo.sizeInGigaBytes = (u32)((ataDriveInfo.sizeInSectors<<9) / 1024 / 1024 / 1024);
	}
	
	i = 20;
	// copy serial string
	memcpy(&ataDriveInfo.serial[0], &buffer[ATA_IDENT_SERIAL],20);
	// cut off the string (usually has trailing spaces)
	while((ataDriveInfo.serial[i] == ' ' || !ataDriveInfo.serial[i]) && i >=0) {
		ataDriveInfo.serial[i] = 0;
		i--;
	}
	// copy model string
	memcpy(&ataDriveInfo.model[0], &buffer[ATA_IDENT_MODEL],40);
	// cut off the string (usually has trailing spaces)
	i = 40;
	while((ataDriveInfo.model[i] == ' ' || !ataDriveInfo.model[i]) && i >=0) {
		ataDriveInfo.model[i] = 0;
		i--;
	}
	
	// Return ok
	return 0;
}


// High-level drive init
// Returns 0 on success, err on failure
int ataDriveInit(int chn) {

    DrawFrameStart();
    DrawMessageBox(D_INFO,"Scanning IDE-EXI Device...");
    DrawFrameFinish();
	
    // Send the identify command
	if(_ataDriveIdentify(chn)) {
		return -1;
	}
	
	// process and print info / drive model & serial information	
 	DrawFrameStart();
	DrawEmptyBox(75,120, vmode->fbWidth-78, 400, COLOR_BLACK);
	sprintf(txtbuffer, "%d GB HDD Connected", ataDriveInfo.sizeInGigaBytes);
	WriteCentre(130,txtbuffer);
	sprintf(txtbuffer, "LBA 48-Bit Mode %s", ataDriveInfo.lba48Support ? "Supported" : "Not Supported");
	WriteCentre(250,txtbuffer);
	sprintf(txtbuffer, "Model: %s",ataDriveInfo.model);
	WriteCentre(310,txtbuffer);
	sprintf(txtbuffer, "Serial: %s",ataDriveInfo.serial); 
	WriteCentre(340,txtbuffer);
	DrawFrameFinish();
	sleep(2);
	
	return 0;
}

// Unlocks a ATA HDD with a password
// Returns 0 on success, -1 on failure.
int ataUnlock(int chn, int useMaster, char *password)
{
	u32 i;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, 0);

	// Write the appropriate unlock command
  	ataWriteByte(chn, ATA_REG_COMMAND, ATA_CMD_UNLOCK);

	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));
	
	// Fill an unlock struct
	unlockStruct unlock;
	memset(&unlock, 0, sizeof(unlockStruct));
	unlock.type = useMaster;
	memcpy(unlock.password, password, strlen(password));

	// write data to the drive 
	u16 *ptr = (u16*)&unlock;
	for (i=0; i<256; i++) {
		ataWriteu16(chn, ptr[i]);
	}
	
	// Wait for the write
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
	
	return !(ataReadErrorReg(chn) & ATA_ER_ABRT);
}
            		
// Reads sectors from the specified lba, for the specified slot
// Returns 0 on success, -1 on failure.
int _ataReadSectors(int chn, u64 lba, u16 numsectors, u32 *Buffer)
{
	u32 i, temp;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
  		
	// check if drive supports LBA 48-bit
	if(ataDriveInfo.lba48Support) {  		
		ataWriteByte(chn, ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));		// LBA Lo
		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>32) & 0xFF));	// LBA Mid
		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>40) & 0xFF));	// LBA Hi
		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)((numsectors>>8) & 0xFF));// Sector count
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));			// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));	// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));	// LBA Hi
  		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));// Sector count
	}
	else {
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));			// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));	// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));	// LBA Hi
  		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));// Sector count
	}

	// Write the appropriate read command
  	ataWriteByte(chn, ATA_REG_COMMAND, ataDriveInfo.lba48Support ? ATA_CMD_READSECTEXT : ATA_CMD_READSECT);

	// Wait for BSY to clear
  	do {
	  	temp = ataReadStatusReg(chn);
  	}
	while(temp & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		DrawFrameStart();
		sprintf(txtbuffer, "Error: %02X", ataReadErrorReg(chn));
    	DrawMessageBox(D_FAIL,txtbuffer);
    	DrawFrameFinish();
		sleep(5);
		return 1;
	}

	// Wait for drive to request data transfer
	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));

	// read data from drive 
	ataRead_init_mult(chn, numsectors); 		// initialise the 32 bit read mode

	for (i=0; i<(numsectors*128); i++) {
		*Buffer++ = ataRead32_mult(chn); 	// Read 4 bytes at a time
	}
	ataRead32_mult(chn); // waste an extra cycle - needed?
	return 0;
}

// Writes sectors to the specified lba, for the specified slot
// Returns 0 on success, -1 on failure.
int _ataWriteSectors(int chn, u64 lba, u16 numsectors, u32 *Buffer)
{
	u32 i, temp;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
  	
	// Select the device
  	ataWriteByte(chn, ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
  		
	// check if drive supports LBA 48-bit
	if(ataDriveInfo.lba48Support) {  		
		ataWriteByte(chn, ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));		// LBA Lo
		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>32) & 0xFF));	// LBA Mid
		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>40) & 0xFF));	// LBA Hi
		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)((numsectors>>8) & 0xFF));// Sector count
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));			// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));	// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));	// LBA Hi
  		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));// Sector count
	}
	else {
		ataWriteByte(chn, ATA_REG_LBALO, (u8)(lba & 0xFF));			// LBA Lo
  		ataWriteByte(chn, ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));	// LBA Mid
  		ataWriteByte(chn, ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));	// LBA Hi
  		ataWriteByte(chn, ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));// Sector count
	}

	// Write the appropriate write command
  	ataWriteByte(chn, ATA_REG_COMMAND, ataDriveInfo.lba48Support ? ATA_CMD_WRITESECTEXT : ATA_CMD_WRITESECT);

  	// Wait for BSY to clear
  	do {
	  	temp = ataReadStatusReg(chn);
  	}
	while(temp & ATA_SR_BSY);
	
	while(!(ataReadStatusReg(chn) & ATA_SR_DRQ));

	// write data to the drive 
	u16 *ptr = (u16*)Buffer;
	for (i=0; i<(numsectors*256); i++) {
		ataWriteu16(chn, ptr[i]);
	}
	
	// Wait for the write
	while(ataReadStatusReg(chn) & ATA_SR_BSY);
	
	return 0;
}

// Wrapper to read a number of sectors
// 0 on Success, -1 on Error
int ataReadSectors(int chn, u64 sector, unsigned int numSectors, unsigned char *dest) 
{
	while(numSectors > 127) {
		if(_ataReadSectors(chn,sector,127,(u32*)dest)) {
			return -1;
		}
		dest+=(127*512);
		sector+=127;
		numSectors-=127;
	}
	while(numSectors) {
		if(_ataReadSectors(chn,sector,1,(u32*)dest)) {
			return -1;
		}
		dest+=(512);
		sector++;
		numSectors-=1;
	}
	return 0;
}

// Wrapper to write a number of sectors
// 0 on Success, -1 on Error
int ataWriteSectors(int chn, u64 sector,unsigned int numSectors, unsigned char *src) 
{
	while(numSectors) {
		if(_ataWriteSectors(chn,sector,1,(u32*)src)) {
			return -1;
		}
		src+=(512);
		sector++;
		numSectors-=1;
	}
	return 0;
}

// Is an ATA device inserted?
bool ataIsInserted(int chn) {
	if(__ata_init[chn]) {
		return true;
	}
	if(_ataDriveIdentify(chn)) {
		return false;
	}
	__ata_init[chn] = 1;
	return true;
}

int ataShutdown(int chn) {
	__ata_init[chn] = 0;
	return 1;
}


static bool __ataa_startup(void)
{
	return ataIsInserted(0);
}

static bool __ataa_isInserted(void)
{
	return ataIsInserted(0);
}

static bool __ataa_readSectors(u32 sector, u32 numSectors, void *buffer)
{
	return !ataReadSectors(0, (u64)sector, numSectors, buffer);
}

static bool __ataa_writeSectors(u32 sector, u32 numSectors, void *buffer)
{
	return !ataWriteSectors(0, (u64)sector, numSectors, buffer);
}

static bool __ataa_clearStatus(void)
{
	return true;
}

static bool __ataa_shutdown(void)
{
	return true;
}

static bool __atab_startup(void)
{
	return ataIsInserted(1);
}

static bool __atab_isInserted(void)
{
	return ataIsInserted(1);
}

static bool __atab_readSectors(u32 sector, u32 numSectors, void *buffer)
{
	return !ataReadSectors(1, (u64)sector, numSectors, buffer);
}

static bool __atab_writeSectors(u32 sector, u32 numSectors, void *buffer)
{
	return !ataWriteSectors(1, (u64)sector, numSectors, buffer);
}

static bool __atab_clearStatus(void)
{
	return true;
}

static bool __atab_shutdown(void)
{
	return true;
}

const DISC_INTERFACE __io_ataa = {
	DEVICE_TYPE_GC_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_GAMECUBE_SLOTA,
	(FN_MEDIUM_STARTUP)&__ataa_startup,
	(FN_MEDIUM_ISINSERTED)&__ataa_isInserted,
	(FN_MEDIUM_READSECTORS)&__ataa_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__ataa_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__ataa_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__ataa_shutdown
} ;
const DISC_INTERFACE __io_atab = {
	DEVICE_TYPE_GC_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_GAMECUBE_SLOTB,
	(FN_MEDIUM_STARTUP)&__atab_startup,
	(FN_MEDIUM_ISINSERTED)&__atab_isInserted,
	(FN_MEDIUM_READSECTORS)&__atab_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__atab_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__atab_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__atab_shutdown
} ;