#ifndef __BBA_H
#define __BBA_H
#include <network.h>

extern int net_initialized;
extern struct in_addr bba_localip;
extern struct in_addr bba_netmask;
extern struct in_addr bba_gateway;
extern const char *bba_device_str;
extern u32 bba_location;
extern s32 (*bba_exi_speed)(s32 chan, s32 dev);

void wait_network();
bool init_network();
void init_network_async();
u32 bba_exists(u32 location);
bool bba_requires_init();
const char *bba_address_str();

#endif
