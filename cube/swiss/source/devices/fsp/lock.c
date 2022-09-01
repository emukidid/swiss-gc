#include "lock.h"

/* ************ Locking functions ***************** */

unsigned short client_get_key (FSP_LOCK *lock)
{
    LWP_MutexLock(lock->lock_mutex);
    return lock->share_key;
}

void client_set_key (FSP_LOCK *lock,unsigned short key)
{
    lock->share_key=key;
    LWP_MutexUnlock(lock->lock_mutex);
}

int client_init_key (FSP_LOCK *lock,
                            unsigned long server_addr,
			    unsigned short server_port)
{
    return LWP_MutexInit(&lock->lock_mutex,false);
}

void client_destroy_key(FSP_LOCK *lock)
{
    LWP_MutexDestroy(lock->lock_mutex);
}
