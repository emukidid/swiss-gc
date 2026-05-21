#include <gctypes.h>
#include <ogc/n64.h>
#include <ogc/pad.h>
#include <ogc/si.h>
#include <ogc/system.h>
#include <stdlib.h>

static u8 status[PAD_CHANMAX];

static void resetCallback(void)
{
	u32 mask = 0;

	for (s32 chan = PAD_CHAN0; chan < PAD_CHANMAX; chan++) {
		u32 type;

		if (PAD_GetType(chan, &type)) {
			mask |= PAD_CHAN_BIT(chan);
		} else {
			switch (SI_DecodeType(type)) {
				case SI_N64_CONTROLLER:
					N64_ResetAsync(chan, &status[chan], NULL);
					break;
			}
		}
	}

	PAD_Recalibrate(mask);
}

__attribute((constructor))
static void initPads(void)
{
	PAD_Init();
	SYS_SetResetCallback(resetCallback);
}

s8 __chooseMaxMagnitiude(s8 p0, s8 p1, s8 p2, s8 p3)
{
	s8 p0a = abs(p0);
	s8 p1a = abs(p1);
	s8 p2a = abs(p2);
	s8 p3a = abs(p3);

	s8 res = p0;
	s8 maxa = p0a;

	if (p1a > maxa) {
		res = p1;
		maxa = p1a;
	}
	if (p2a > maxa) {
		res = p2;
		maxa = p2a;
	}
	if (p3a > maxa) {
		res = p3;
		maxa = p3a;
	}

	return res;
}

u16 padsButtonsHeld() {
	return (
		PAD_ButtonsHeld(PAD_CHAN0) |
		PAD_ButtonsHeld(PAD_CHAN1) |
		PAD_ButtonsHeld(PAD_CHAN2) |
		PAD_ButtonsHeld(PAD_CHAN3)
	);
}

s8 padsStickX() {
	return __chooseMaxMagnitiude(
		PAD_StickX(PAD_CHAN0),
		PAD_StickX(PAD_CHAN1),
		PAD_StickX(PAD_CHAN2),
		PAD_StickX(PAD_CHAN3)
	);
}

s8 padsStickY() {
	return __chooseMaxMagnitiude(
		PAD_StickY(PAD_CHAN0),
		PAD_StickY(PAD_CHAN1),
		PAD_StickY(PAD_CHAN2),
		PAD_StickY(PAD_CHAN3)
	);
}
