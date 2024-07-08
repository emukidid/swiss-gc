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
#include <errno.h>
#include "ff.h"
#include "diskio.h"
#include "wode/WodeInterface.h"
#include "bnr.h"

#define PATHNAME_MAX 1024

#define STATUS_NOT_MAPPED  0
#define STATUS_MAPPED      1
#define STATUS_HAS_MAPPING 2

typedef struct {
	u32 offset;
	u32 size;
	u64 fileNum  :  8;
	u64 devNum   :  8;
	u64 fileBase : 48;
} file_frag;

typedef struct {
	dvddiskid diskId;
	const char *displayName;
	GXTexObj *fileTypeTexObj;
	GXTexObj *regionTexObj;
	u8 *banner;
	u16 bannerSize;
	u16 bannerSum;
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
	vu32 lockCount;
	lwp_t thread;
} file_handle;	// Note: If the contents of this change, recompile pc/usbgecko/main.c

typedef struct {
	u64 freeSpace;
	u64 totalSpace;
	bool metric;
} device_info;

typedef struct {
	int textureId;
	u16 width;
	u16 height;
	u16 realWidth;
	u16 realHeight;
} textureImage;

typedef struct ExecutableFile ExecutableFile;

#define DEVICE_HANDLER_SEEK_SET 0
#define DEVICE_HANDLER_SEEK_CUR 1
#define DEVICE_HANDLER_SEEK_END 2

// Device struct
typedef bool (* _fn_test)(void);
typedef device_info* (* _fn_info)(file_handle*);
typedef s32 (* _fn_init)(file_handle*);
typedef s32 (* _fn_makeDir)(file_handle*);
typedef s32 (* _fn_readDir)(file_handle*, file_handle**, u32);
typedef s64 (* _fn_seekFile)(file_handle*, s64, u32);
typedef s32 (* _fn_readFile)(file_handle*, void*, u32);
typedef s32 (* _fn_writeFile)(file_handle*, const void*, u32);
typedef s32 (* _fn_closeFile)(file_handle*);
typedef s32 (* _fn_deleteFile)(file_handle*);
typedef s32 (* _fn_renameFile)(file_handle*, char*);
typedef s32 (* _fn_setupFile)(file_handle*, file_handle*, ExecutableFile*, int);
typedef s32 (* _fn_deinit)(file_handle*);
typedef u32 (* _fn_emulated)(void);
typedef char* (* _fn_status)(file_handle*);

// Device features
#define FEAT_READ				0x1
#define FEAT_WRITE				0x2
#define FEAT_BOOT_GCM 			0x4
#define FEAT_BOOT_DEVICE		0x8
#define FEAT_CONFIG_DEVICE		0x10
#define FEAT_AUTOLOAD_DOL		0x20
#define FEAT_THREAD_SAFE		0x40
#define FEAT_HYPERVISOR			0x80
#define FEAT_PATCHES			0x100
#define FEAT_AUDIO_STREAMING	0x200
#define FEAT_EXI_SPEED			0x400

// Device quirks
#define QUIRK_NONE						0x0
#define QUIRK_EXI_SPEED					0x1
#define QUIRK_GCLOADER_NO_DISC_2		0x2
#define QUIRK_GCLOADER_NO_PARTIAL_READ	0x4
#define QUIRK_GCLOADER_WRITE_CONFLICT	0x8

// Device emulated features
#define EMU_READ			0x80000000
#define EMU_AUDIO_STREAMING	0x40000000
#define EMU_READ_SPEED		0x20000000
#define EMU_ETHERNET		0x10000000
#define EMU_MEMCARD			0x8000000
#define EMU_BUS_ARBITER		0x1
#define EMU_NONE			0x0

// Device locations
#define LOC_UNK				0x0
#define LOC_MEMCARD_SLOT_A	0x1
#define LOC_MEMCARD_SLOT_B	0x2
#define LOC_DVD_CONNECTOR	0x4
#define LOC_SERIAL_PORT_1	0x8
#define LOC_SERIAL_PORT_2	0x10
#define LOC_HSP				0x20
#define LOC_SYSTEM			0x40
#define LOC_ANY				(LOC_MEMCARD_SLOT_A | LOC_MEMCARD_SLOT_B | LOC_DVD_CONNECTOR | LOC_SERIAL_PORT_1 | LOC_SERIAL_PORT_2 | LOC_HSP | LOC_SYSTEM)

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
#define DEVICE_ID_I			0x12
#define DEVICE_ID_J			0x13
#define DEVICE_ID_MAX		DEVICE_ID_J
#define DEVICE_ID_UNK		(DEVICE_ID_MAX + 1)

typedef struct DEVICEHANDLER_STRUCT DEVICEHANDLER_INTERFACE;

struct DEVICEHANDLER_STRUCT {
	u8 				deviceUniqueId;
	const char*		hwName;
	const char*		deviceName;
	const char*		deviceDescription;
	textureImage	deviceTexture;
	u32				features;
	u32				quirks;
	u32				emulable;
	u32				location;
	file_handle*	initial;
	_fn_test		test;
	_fn_info	 	info;
	_fn_init 		init;
	_fn_makeDir		makeDir;
	_fn_readDir		readDir;
	_fn_seekFile	seekFile;
	_fn_readFile	readFile;
	_fn_writeFile	writeFile;
	_fn_closeFile	closeFile;
	_fn_deleteFile	deleteFile;
	_fn_renameFile	renameFile;
	_fn_setupFile	setupFile;
	_fn_deinit		deinit;
	_fn_emulated	emulated;
	_fn_status		status;
} ;

enum DEVICE_SLOTS {
	DEVICE_CUR,
	DEVICE_PREV,
	DEVICE_DEST,
	DEVICE_CHEATS,
	DEVICE_CONFIG,
	DEVICE_PATCHES,
	MAX_DEVICE_SLOTS
};

// Common error types
enum DEV_ERRORS {
	E_NONET=1,
	E_CHECKCONFIG,
	E_CONNECTFAIL
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
#include "devices/aram/deviceHandler-ARAM.h"
#include "devices/flippydrive/deviceHandler-flippydrive.h"

extern void deviceHandler_setStatEnabled(int enable);
extern int deviceHandler_getStatEnabled();
extern bool deviceHandler_getDeviceAvailable(DEVICEHANDLER_INTERFACE *dev);
extern void deviceHandler_setDeviceAvailable(DEVICEHANDLER_INTERFACE *dev, bool availability);
extern void deviceHandler_setAllDevicesAvailable();

#define MAX_DEVICES 21

extern DEVICEHANDLER_INTERFACE* allDevices[MAX_DEVICES];
extern DEVICEHANDLER_INTERFACE* devices[MAX_DEVICE_SLOTS];

extern DEVICEHANDLER_INTERFACE* getDeviceByUniqueId(u8 id);
extern DEVICEHANDLER_INTERFACE* getDeviceByLocation(u32 location);
extern DEVICEHANDLER_INTERFACE* getDeviceFromPath(char *path);
extern bool getExiDeviceByLocation(u32 location, s32 *chan, s32 *dev);
extern vu32* getExiRegsByLocation(u32 location);
extern const char* getHwNameByLocation(u32 location);

#define MAX_FRAGS 40

extern bool getFragments(int deviceSlot, file_handle *file, file_frag **fragList, u32 *totFrags, u8 fileNum, u32 forceBaseOffset, u32 forceSize);
extern void print_frag_list(file_frag *fragList, u32 totFrags);

extern FILE* openFileStream(int deviceSlot, file_handle *file);

#endif

