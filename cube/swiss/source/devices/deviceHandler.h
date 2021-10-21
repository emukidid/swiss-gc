/* deviceHandler.h
	- devices interface (based on fileBrowser from wii64/wiiSX)
	by emu_kidid
 */

#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include <gccore.h>
#include <stdint.h>
#include <gctypes.h>
#include <stdio.h>
#include "ff.h"
#include "diskio.h"
#include "wode/WodeInterface.h"
#include "bnr.h"

#define MAX_DEVICES 19
#define PATHNAME_MAX 1024

typedef struct {
	dvddiskid diskId;
	GXTexObj *fileTypeTexObj;
	GXTexObj *regionTexObj;
	u8 *banner;
	int bannerSize;
	GXTexObj bannerTexObj;
	GXTlutObj bannerTlutObj;
	BNRDesc bannerDesc;
} file_meta;

typedef struct {
	char name[PATHNAME_MAX]; 		// File or Folder, absolute path goes here
	uint64_t fileBase;   	// Raw sector on device
	u32 offset;    			// Offset in the file
	u32 size;      			// size of the file
	s32 fileAttrib;        	// IS_FILE or IS_DIR
	s32 status;            	// is the device ok
	void *fp;				// file pointer
	FIL* ffsFp;				// file pointer (FATFS)
	file_meta *meta;
	u8 other[128];			// Store anything else we want here
	void* uiObj;			// UI associated with this file_handle
} file_handle;	// Note: If the contents of this change, recompile pc/usbgecko/main.c

typedef struct {
	u64 freeSpace;
	u64 totalSpace;
} device_info;

typedef struct {
	int textureId;
	u32 width;
	u32 height;
} textureImage;

#define DEVICE_HANDLER_SEEK_SET 0
#define DEVICE_HANDLER_SEEK_CUR 1

// Device struct
typedef bool (* _fn_test)(void);
typedef device_info* (* _fn_info)(file_handle*);
typedef s32 (* _fn_init)(file_handle*);
typedef s32 (* _fn_readDir)(file_handle*, file_handle**, u32);
typedef s32 (* _fn_readFile)(file_handle*, void*, u32);
typedef s32 (* _fn_writeFile)(file_handle*, void*, u32);
typedef s32 (* _fn_deleteFile)(file_handle*);
typedef s32 (* _fn_seekFile)(file_handle*,  u32, u32);
typedef s32 (* _fn_setupFile)(file_handle*, file_handle*, int);
typedef s32 (* _fn_closeFile)(file_handle*);
typedef s32 (* _fn_deinit)(file_handle*);
typedef u32 (* _fn_emulated)(void);

// Device features
#define FEAT_READ				0x1
#define FEAT_WRITE				0x2
#define FEAT_BOOT_GCM 			0x4
#define FEAT_BOOT_DEVICE		0x8
#define FEAT_CONFIG_DEVICE		0x10
#define FEAT_AUTOLOAD_DOL		0x20
#define FEAT_FAT_FUNCS			0x40
#define FEAT_HYPERVISOR			0x80
#define FEAT_PATCHES			0x100
#define FEAT_AUDIO_STREAMING	0x200

// Device emulated features
#define EMU_NONE			0x0
#define EMU_READ			0x1
#define EMU_READ_SPEED		0x2
#define EMU_AUDIO_STREAMING	0x4
#define EMU_MEMCARD			0x8

// Device locations
#define LOC_MEMCARD_SLOT_A 	0x1
#define LOC_MEMCARD_SLOT_B 	0x2
#define LOC_DVD_CONNECTOR	0x4
#define LOC_SERIAL_PORT_1	0x8
#define LOC_SERIAL_PORT_2	0x10
#define LOC_HSP				0x20
#define LOC_SYSTEM			0x40

// Device unique Id (used to store last used config device in SRAM - configDeviceId)
#define DEVICE_ID_0			0x00
#define DEVICE_ID_1			0x01
#define DEVICE_ID_2			0x02
#define DEVICE_ID_3			0x03
#define DEVICE_ID_4			0x04
#define DEVICE_ID_5			0x05
#define DEVICE_ID_6			0x06
#define DEVICE_ID_7			0x07
#define DEVICE_ID_8			0x08
#define DEVICE_ID_9			0x09
#define DEVICE_ID_A			0x0A
#define DEVICE_ID_B			0x0B
#define DEVICE_ID_C			0x0C
#define DEVICE_ID_D			0x0D
#define DEVICE_ID_E			0x0E
#define DEVICE_ID_F			0x0F
#define DEVICE_ID_G			0x10
#define DEVICE_ID_H			0x11
#define DEVICE_ID_MAX		DEVICE_ID_H
#define DEVICE_ID_UNK		(DEVICE_ID_MAX + 1)

struct DEVICEHANDLER_STRUCT {
	u8 				deviceUniqueId;
	const char*		hwName;
	const char*		deviceName;
	const char*		deviceDescription;
	textureImage	deviceTexture;
	u32				features;
	u32				emulable;
	u32				location;
	file_handle*	initial;
	_fn_test		test;
	_fn_info	 	info;
	_fn_init 		init;
	_fn_readDir		readDir;
	_fn_readFile	readFile;
	_fn_writeFile	writeFile;
	_fn_deleteFile	deleteFile;
	_fn_seekFile	seekFile;
	_fn_setupFile	setupFile;
	_fn_closeFile	closeFile;
	_fn_deinit		deinit;
	_fn_emulated	emulated;
} ;

typedef struct DEVICEHANDLER_STRUCT DEVICEHANDLER_INTERFACE;

enum DEVICE_SLOTS {
	DEVICE_CUR,
	DEVICE_DEST,
	DEVICE_TEMP,
	DEVICE_CONFIG,
	DEVICE_PATCHES,
	MAX_DEVICE_SLOTS
};

#include "devices/dvd/deviceHandler-DVD.h"
#include "devices/fat/deviceHandler-FAT.h"
#include "devices/qoob/deviceHandler-Qoob.h"
#include "devices/wode/deviceHandler-WODE.h"
#include "devices/memcard/deviceHandler-CARD.h"
#include "devices/smb/deviceHandler-SMB.h"
#include "devices/wiikeyfusion/deviceHandler-wiikeyfusion.h"
#include "devices/usbgecko/deviceHandler-usbgecko.h"
#include "devices/system/deviceHandler-SYS.h"
#include "devices/ftp/deviceHandler-FTP.h"
#include "devices/fsp/deviceHandler-FSP.h"
#include "devices/gcloader/deviceHandler-gcloader.h"

extern void deviceHandler_setStatEnabled(int enable);
extern int deviceHandler_getStatEnabled();
extern bool deviceHandler_getDeviceAvailable(DEVICEHANDLER_INTERFACE *dev);
extern void deviceHandler_setDeviceAvailable(DEVICEHANDLER_INTERFACE *dev, bool availability);
extern void deviceHandler_setAllDevicesAvailable();

extern DEVICEHANDLER_INTERFACE* allDevices[MAX_DEVICES];
extern DEVICEHANDLER_INTERFACE* devices[MAX_DEVICES];

extern int deviceHandler_test(DEVICEHANDLER_INTERFACE *device);
extern DEVICEHANDLER_INTERFACE* getDeviceByUniqueId(u8 id);
extern DEVICEHANDLER_INTERFACE* getDeviceByLocation(u32 location);
extern DEVICEHANDLER_INTERFACE* getDeviceFromPath(char *path);
extern const char* getHwNameByLocation(u32 location);

#define MAX_FRAGS 40

extern void print_frag_list(u32 (*fragList)[4]);

#endif

