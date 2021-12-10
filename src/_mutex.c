/*************************************************************************************************/
/*  Mutex                                                                                        */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_mutex.h"
#include "_macros.h"
#include "_time.h"

#include "tinycthread.h"



/*************************************************************************************************/
/*  Mutex functions                                                                              */
/*************************************************************************************************/


int dvz_mutex_init(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    // NOTE: tct_thrd_success is 1, not 0 (!?)
    return tct_mtx_init(mutex, 0) != tct_thrd_success;
}



DvzMutex dvz_mutex()
{
    INIT(DvzMutex, mutex);
    dvz_mutex_init(&mutex);
    return mutex;
}



int dvz_mutex_lock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return tct_mtx_lock(mutex) != tct_thrd_success;
}



int dvz_mutex_unlock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return tct_mtx_unlock(mutex) != tct_thrd_success;
}



void dvz_mutex_destroy(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    tct_mtx_destroy(mutex);
}



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

int dvz_cond_init(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return tct_cnd_init(cond);
}



DvzCond dvz_cond()
{

    INIT(DvzCond, cond);
    dvz_cond_init(&cond);
    return cond;
}



int dvz_cond_signal(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return tct_cnd_signal(cond);
}



int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex)
{
    ASSERT(cond != NULL);
    return tct_cnd_wait(cond, mutex);
}



int dvz_cond_timedwait(DvzCond* cond, DvzMutex* mutex, struct timespec* wait)
{
    ASSERT(cond != NULL);
    return tct_cnd_timedwait(cond, mutex, wait);
}



void dvz_cond_destroy(DvzCond* cond)
{
    ASSERT(cond != NULL);
    tct_cnd_destroy(cond);
}
