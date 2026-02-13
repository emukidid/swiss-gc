#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include <ogcsys.h>
#include "bba.h"
#include "exi.h"
#include "httpd.h"
#include "rt4k.h"
#include "swiss.h"
#include "wiiload.h"
#include "deviceHandler.h"

static s32 MX98730EC_GetExiSpeed(s32 chan, s32 dev);
static s32 ENC28J60_GetExiSpeed(s32 chan, s32 dev);
extern s32 W5500_GetExiSpeed(s32 chan, s32 dev);
extern s32 W6X00_GetExiSpeed(s32 chan, s32 dev);

/* Network Globals */
int net_initialized = 0;
struct in_addr bba_localip;
struct in_addr bba_netmask;
struct in_addr bba_gateway;
const char *bba_device_str = "Broadband Adapter";
u32 bba_location = LOC_UNK;
s32 (*bba_exi_speed)(s32 chan, s32 dev) = NULL;

static lwp_t net_thread = LWP_THREAD_NULL;

static void *net_thread_func(void *arg)
{
	inet_aton(swissSettings.bbaLocalIp, &bba_localip);
	bba_netmask.s_addr = INADDR_BROADCAST << (32 - swissSettings.bbaNetmask);
	inet_aton(swissSettings.bbaGateway, &bba_gateway);

	if (if_configex(&bba_localip, &bba_netmask, &bba_gateway, swissSettings.bbaUseDhcp) == -1) {
		net_initialized = 0;
		return NULL;
	}

	net_initialized = 1;
	inet_ntoa_r(bba_localip, swissSettings.bbaLocalIp, sizeof(swissSettings.bbaLocalIp));
	swissSettings.bbaNetmask = bba_netmask.s_addr ? (32 - __builtin_ctz(bba_netmask.s_addr)) : 0;
	inet_ntoa_r(bba_gateway, swissSettings.bbaGateway, sizeof(swissSettings.bbaGateway));

	char ifname[4];
	if (if_indextoname(1, ifname)) {
		switch (ifname[0]) {
			case 'e': bba_exi_speed = MX98730EC_GetExiSpeed; break;
			case 'E': bba_exi_speed = ENC28J60_GetExiSpeed;  break;
			case 'W': bba_exi_speed = W5500_GetExiSpeed;     break;
			case 'w': bba_exi_speed = W6X00_GetExiSpeed;     break;
		}

		if (strchr("EWw", ifname[0])) {
			switch (ifname[1]) {
				case 'A': bba_location = LOC_MEMCARD_SLOT_A; break;
				case 'B': bba_location = LOC_MEMCARD_SLOT_B; break;
				case '1': bba_location = LOC_SERIAL_PORT_1;  break;
				case '2': bba_location = LOC_SERIAL_PORT_2;  break;
			}
		} else if (ifname[0] == 'e')
			bba_location = LOC_SERIAL_PORT_1;

		switch (ifname[0]) {
			case 'e': bba_device_str = "Broadband Adapter";  break;
			case 'E': bba_device_str = "ENC28J60";           break;
			case 'W': bba_device_str = "WIZnet W5500";       break;
			case 'w': bba_device_str = "WIZnet W6X00";
				u32 id;
				if (getExiIdByLocation(bba_location, &id)) {
					switch (id) {
						case EXI_W5500_ID: bba_device_str = "WIZnet W6100"; break;
						case EXI_W6300_ID: bba_device_str = "WIZnet W6300"; break;
					}
				}
				break;
		}

		if (strchr("EWw", ifname[0]) && strchr("12AB", ifname[1])) {
			__device_flippy.emulable |= EMU_ETHERNET;
			__device_flippyflash.emulable |= EMU_ETHERNET;
			if (__device_gcloader.features & FEAT_PATCHES)
				__device_gcloader.emulable |= EMU_ETHERNET;
			__device_ata_c.emulable |= EMU_ETHERNET;
			__device_sd_c.emulable |= EMU_ETHERNET;
			__device_sd_a.emulable |= EMU_ETHERNET;
			__device_sd_b.emulable |= EMU_ETHERNET;
			__device_ata_a.emulable |= EMU_ETHERNET;
			__device_ata_b.emulable |= EMU_ETHERNET;
		}
	}

	deviceHandler_setDeviceAvailable(&__device_smb, deviceHandler_SMB_test());
	deviceHandler_setDeviceAvailable(&__device_ftp, deviceHandler_FTP_test());
	deviceHandler_setDeviceAvailable(&__device_fsp, deviceHandler_FSP_test());

	init_httpd_thread();
	init_wiiload_tcp_thread();
	rt4k_init();
	return NULL;
}

// Init the GC net interface (bba)
void init_network_async(void)
{
	if (!net_initialized && net_thread == LWP_THREAD_NULL)
		LWP_CreateThread(&net_thread, net_thread_func, NULL, NULL, 0, LWP_PRIO_LOWEST);
}

bool init_network(void)
{
	init_network_async();
	wait_network();
	return net_initialized && (bba_localip.s_addr = net_gethostip()) != INADDR_ANY;
}

void wait_network(void)
{
	lwp_t thread = net_thread;
	net_thread = LWP_THREAD_NULL;
	if (thread != LWP_THREAD_NULL) {
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Initialising Network"));
		LWP_JoinThread(thread, NULL);
		DrawDispose(msgBox);
	}
}

u32 bba_exists(u32 location)
{
	u32 id;

	if (location & bba_location)
		return bba_location;

	if ((location & LOC_SERIAL_PORT_1) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_2, &id)) {
		switch (id) {
			case EXI_W5500_ID:
			case EXI_MX98730EC_ID:
			case EXI_ENC28J60_ID:
			case EXI_W6300_ID:
				return LOC_SERIAL_PORT_1;
		}
	}
	if ((location & LOC_SERIAL_PORT_2) && EXI_GetID(EXI_CHANNEL_2, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_W5500_ID:
			case EXI_ENC28J60_ID:
			case EXI_W6300_ID:
				return LOC_SERIAL_PORT_2;
		}
	}
	if ((location & LOC_MEMCARD_SLOT_A) && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_W5500_ID:
			case EXI_ENC28J60_ID:
			case EXI_W6300_ID:
				return LOC_MEMCARD_SLOT_A;
		}
	}
	if ((location & LOC_MEMCARD_SLOT_B) && EXI_GetID(EXI_CHANNEL_1, EXI_DEVICE_0, &id)) {
		switch (id) {
			case EXI_W5500_ID:
			case EXI_ENC28J60_ID:
			case EXI_W6300_ID:
				return LOC_MEMCARD_SLOT_B;
		}
	}
	return LOC_UNK;
}

bool bba_requires_init(void)
{
	u32 location = bba_exists(LOC_ANY), id;

	if (location == LOC_SERIAL_PORT_1 && EXI_GetID(EXI_CHANNEL_0, EXI_DEVICE_2, &id))
		return id != EXI_MX98730EC_ID;

	return location != LOC_UNK;
}

const char *bba_address_str(void)
{
	static char string[18];

	if ((bba_localip.s_addr = net_gethostip()) == INADDR_ANY) {
		u8 mac_addr[6];

		if (net_get_mac_address(mac_addr) < 0)
			return "Not initialised";

		sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		return string;
	}

	sprintf(string, "%u.%u.%u.%u", ip4_addr1(&bba_localip), ip4_addr2(&bba_localip), ip4_addr3(&bba_localip), ip4_addr4(&bba_localip));
	return string;
}

static s32 MX98730EC_GetExiSpeed(s32 chan, s32 dev)
{
	return EXI_SPEED32MHZ;
}

static s32 ENC28J60_GetExiSpeed(s32 chan, s32 dev)
{
	if (chan == EXI_CHANNEL_0 && dev == EXI_DEVICE_2)
		return EXI_SPEED16MHZ;
	else
		return EXI_SPEED32MHZ;
}
