#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <stdbool.h>
#include <stdlib.h>

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

// N64 controller cmd 0x01 response, byte 0
#define N64_BTN_A      0x80
#define N64_BTN_B      0x40
#define N64_BTN_Z      0x20
#define N64_BTN_START  0x10
#define N64_DPAD_UP    0x08
#define N64_DPAD_DOWN  0x04
#define N64_DPAD_LEFT  0x02
#define N64_DPAD_RIGHT 0x01
// N64 controller cmd 0x01 response, byte 1
#define N64_BTN_L      0x20
#define N64_BTN_R      0x10
#define N64_BTN_CUP    0x08
#define N64_BTN_CDOWN  0x04
#define N64_BTN_CLEFT  0x02
#define N64_BTN_CRIGHT 0x01

#define SI_CMD_N64_POLL 0x01
#define N64_TYPE_ID     0x05000000  // upper 16 bits of N64 controller type

// GC controller status, populated by padsScan() each VBlank.
static PADStatus gc_status[PAD_CHANMAX];

// N64 controller state, populated lazily by n64_active() in main context.
static u16 n64_buttons[SI_MAX_CHAN];
static s8  n64_stick_x[SI_MAX_CHAN];
static s8  n64_stick_y[SI_MAX_CHAN];
static u32 n64_last_frame[SI_MAX_CHAN];

// SI scratch for cmd 0x01 polling. Aligned for DMA.
static u8  cmd_buf[SI_MAX_CHAN] ATTRIBUTE_ALIGN(32);
static u8  poll_resp[SI_MAX_CHAN][4] ATTRIBUTE_ALIGN(32);
static volatile u32 xfer_done_mask;

static void si_xfer_cb(s32 chan, u32 err)
{
	xfer_done_mask |= (1u << chan);
}

static bool si_xfer_sync(s32 chan, u8 cmd, void *resp, u32 resp_len)
{
	cmd_buf[chan] = cmd;
	u32 mask = (1u << chan);
	xfer_done_mask &= ~mask;
	if (!SI_Transfer(chan, &cmd_buf[chan], 1, resp, resp_len, si_xfer_cb, 65))
		return false;
	u64 deadline = gettime() + millisecs_to_ticks(5);
	while (!(xfer_done_mask & mask)) {
		if (gettime() > deadline) return false;
	}
	return true;
}

static u16 n64_translate(u8 b0, u8 b1)
{
	u16 out = 0;
	if (b0 & N64_BTN_A)      out |= PAD_BUTTON_A;
	if (b0 & N64_BTN_B)      out |= PAD_BUTTON_B;
	if (b0 & N64_BTN_Z)      out |= PAD_TRIGGER_Z;
	if (b0 & N64_BTN_START)  out |= PAD_BUTTON_START;
	if (b0 & N64_DPAD_UP)    out |= PAD_BUTTON_UP;
	if (b0 & N64_DPAD_DOWN)  out |= PAD_BUTTON_DOWN;
	if (b0 & N64_DPAD_LEFT)  out |= PAD_BUTTON_LEFT;
	if (b0 & N64_DPAD_RIGHT) out |= PAD_BUTTON_RIGHT;
	if (b1 & N64_BTN_L)      out |= PAD_TRIGGER_L;
	if (b1 & N64_BTN_R)      out |= PAD_TRIGGER_R;
	if (b1 & N64_BTN_CLEFT)  out |= PAD_BUTTON_Y;
	if (b1 & N64_BTN_CDOWN)  out |= PAD_BUTTON_X;
	return out;
}

// SI polling handler — same pattern as Enhanced mGBA's poll_cb. Runs at
// the SI sample rate (interrupt context). PAD_Read only — just a cached
// SI status read, no SI commands issued. PAD_Reset for hot-swap re-
// detection happens lazily from main context in padsCheck() below
// (PAD_Reset would call SI_DisablePolling from inside the SI polling
// iteration if we did it here, which crashes).
static void padsScan(u32 irq, void *ctx)
{
	(void)irq; (void)ctx;
	PAD_Read(gc_status);
}

// Lazy hot-swap helper called from main context on each pads*() query.
// libogc's PAD_Read fires SI_GetTypeAsync once on first NO_RESPONSE for
// a channel and disables itself afterward — meaning a GC controller
// plugged into a previously-empty port stays undetected. Selectively
// PAD_Reset those channels (skipping anything libogc thinks is N64) to
// re-arm detection without clobbering the N64 ports' si_type.
static void padsCheck(void)
{
	u32 reset_bits = 0;
	for (s32 c = 0; c < SI_MAX_CHAN; c++) {
		if (gc_status[c].err == PAD_ERR_NO_CONTROLLER
		    && (SI_GetType(c) & ~0xFFFF) != N64_TYPE_ID)
			reset_bits |= PAD_CHAN_BIT(c);
	}
	if (reset_bits) PAD_Reset(reset_bits);
}

// Lazy SI handler registration. Done on the first input query rather
// than at startup because libogc's SI interrupt handler reads VI state
// (VIDEO_GetCurrentLine) — registering before VIDEO_Init has run gives
// it garbage to read. The first padsButtonsHeld()/padsStick*() call
// from swiss-gc's UI necessarily happens after VIDEO_Init/DrawInit.
static void register_si_handler(void)
{
	static bool registered = false;
	if (!registered) {
		SI_RegisterPollingHandler(padsScan);
		registered = true;
	}
}

// Lazy poll — only runs from main context where synchronous SI_Transfer
// is safe. Returns true if `chan` currently holds an N64 controller and
// a fresh poll succeeded (or was already done this frame).
static bool n64_active(s32 chan)
{
	if ((SI_GetType(chan) & ~0xFFFF) != N64_TYPE_ID)
		return false;

	u32 frame = VIDEO_GetRetraceCount();
	if (n64_last_frame[chan] != frame) {
		n64_last_frame[chan] = frame;
		if (!si_xfer_sync(chan, SI_CMD_N64_POLL, poll_resp[chan], 4)) {
			// Transfer failed — controller may have been unplugged.
			// Trigger a libogc re-probe so si_type updates and future
			// frames don't keep entering this branch.
			PAD_Reset(PAD_CHAN_BIT(chan));
			return false;
		}
		n64_buttons[chan] = n64_translate(poll_resp[chan][0], poll_resp[chan][1]);
		n64_stick_x[chan] = (s8)poll_resp[chan][2];
		n64_stick_y[chan] = (s8)poll_resp[chan][3];
	}
	return true;
}

u16 padsButtonsHeld() {
	register_si_handler();
	padsCheck();
	u16 r = 0;
	for (s32 c = 0; c < SI_MAX_CHAN; c++) {
		if (n64_active(c))
			r |= n64_buttons[c];
		else if (gc_status[c].err == PAD_ERR_NONE)
			r |= gc_status[c].button;
	}
	return r;
}

s8 padsStickX() {
	register_si_handler();
	padsCheck();
	s8 v[SI_MAX_CHAN];
	for (s32 c = 0; c < SI_MAX_CHAN; c++) {
		if (n64_active(c))
			v[c] = n64_stick_x[c];
		else if (gc_status[c].err == PAD_ERR_NONE)
			v[c] = gc_status[c].stickX;
		else
			v[c] = 0;
	}
	return __chooseMaxMagnitiude(v[0], v[1], v[2], v[3]);
}

s8 padsStickY() {
	register_si_handler();
	padsCheck();
	s8 v[SI_MAX_CHAN];
	for (s32 c = 0; c < SI_MAX_CHAN; c++) {
		if (n64_active(c))
			v[c] = n64_stick_y[c];
		else if (gc_status[c].err == PAD_ERR_NONE)
			v[c] = gc_status[c].stickY;
		else
			v[c] = 0;
	}
	return __chooseMaxMagnitiude(v[0], v[1], v[2], v[3]);
}
