/*! \file ataconf.h \brief IDE-ATA interface driver configuration. */
//*****************************************************************************
//
// File Name	: 'ataconf.h'
// Title		: IDE-ATA interface driver for hard disks
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Date			: 11/22/2000
// Revised		: 12/29/2000
// Version		: 0.3
// Target MCU	: Atmel AVR Series)
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef ATA_IFCONF_H
#define ATA_IFCONF_H

// constants
#define SECTOR_BUFFER_ADDR			0

// ATA register base address
#define ATA_REG_BASE		0x00
// ATA register offset
#define ATA_REG_DATA		0x10    //1 0000b 
#define ATA_REG_ERROR		0x11    //1 0001b  
#define ATA_REG_FEATURES        0x11    //1 0001b 
#define ATA_REG_SECCOUNT	0x12    //1 0010b 
#define ATA_REG_STARTSEC	0x13    //1 0011b 
#define ATA_REG_CYLLO		0x14    //1 0100b 
#define ATA_REG_CYLHI		0x15    //1 0101b 
#define ATA_REG_HDDEVSEL	0x16    //1 0110b 
#define ATA_REG_DEVHEAD         0x16    //1 0110b
#define ATA_REG_COMMAND		0x17    //1 0111b 
#define ATA_REG_CMDSTATUS1	0x17    //1 0111b 
#define ATA_REG_CMDSTATUS2	0x18    //1 1000b
#define ATA_REG_CONTROL		0x0E    //0 1110b
#define ATA_REG_ACTSTATUS	0x0F    //0 1111b 
                                       
                                       
#endif












