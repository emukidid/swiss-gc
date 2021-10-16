#ifndef EXI_H
#define EXI_H

#include "common.h"

#define EXI_STATE_BUSY_DMA 0x01
#define EXI_STATE_BUSY_IMM 0x02
#define EXI_STATE_BUSY     (EXI_STATE_BUSY_DMA | EXI_STATE_BUSY_IMM)
#define EXI_STATE_SELECTED 0x04
#define EXI_STATE_ATTACHED 0x08
#define EXI_STATE_LOCKED   0x10

enum {
	EXI_READ = 0,
	EXI_WRITE,
	EXI_READ_WRITE,
};

enum {
	EXI_CHANNEL_0 = 0,
	EXI_CHANNEL_1,
	EXI_CHANNEL_2,
	EXI_CHANNEL_MAX
};

enum {
	EXI_DEVICE_0 = 0,
	EXI_DEVICE_1,
	EXI_DEVICE_2,
	EXI_DEVICE_MAX
};

enum {
	EXI_SPEED_1MHZ = 0,
	EXI_SPEED_2MHZ,
	EXI_SPEED_4MHZ,
	EXI_SPEED_8MHZ,
	EXI_SPEED_16MHZ,
	EXI_SPEED_32MHZ,
};

typedef void (*EXICallback)(s32 chan, u32 dev);

typedef struct EXIControl {
	EXICallback exiCallback;
	EXICallback tcCallback;
	EXICallback extCallback;
	u32 state;
	s32 immLen;
	void *immBuf;
	u32 dev;
	u32 id;
	s32 idTime;
	s32 items;
	struct {
		u32 dev;
		EXICallback callback;
	} queue[EXI_DEVICE_MAX];
} EXIControl;

extern s32 (*EXILock)(s32 chan, u32 dev, EXICallback unlockedCallback);
extern s32 (*EXIUnlock)(s32 chan);

s32 WriteUARTN(const void *buf, u32 len);

#endif /* EXI_H */
