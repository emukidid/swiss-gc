#include <string.h>
#include <stdio.h>
#include "lock.h"

/* ************ Locking functions ***************** */

unsigned short client_get_key (FSP_LOCK *lock)
{
    return lock->share_key;
}

void client_set_key (FSP_LOCK *lock,unsigned short key)
{
    lock->share_key=key;
}

int client_init_key (FSP_LOCK *lock,
                            unsigned long server_addr,
			    unsigned short server_port)
{
    return 0;
}

void client_destroy_key(FSP_LOCK *lock)
{
    return;
}
