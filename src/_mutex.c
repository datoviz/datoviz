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
    // NOTE: thrd_success is 1, not 0 (!?)
    return mtx_init(mutex, 0) != thrd_success;
}



DvzMutex dvz_mutex()
{
#ifdef LANG_CPP
    DvzMutex mutex = {};
#else
    DvzMutex mutex = {0};
#endif
    dvz_mutex_init(&mutex);
    return mutex;
}



int dvz_mutex_lock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_lock(mutex) != thrd_success;
}



int dvz_mutex_unlock(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    return mtx_unlock(mutex) != thrd_success;
}



void dvz_mutex_destroy(DvzMutex* mutex)
{
    ASSERT(mutex != NULL);
    mtx_destroy(mutex);
}



/*************************************************************************************************/
/*  Cond functions                                                                               */
/*************************************************************************************************/

int dvz_cond_init(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return cnd_init(cond);
}



DvzCond dvz_cond()
{

#ifdef LANG_CPP
    DvzCond cond = {};
#else
    DvzCond cond = {0};
#endif
    dvz_cond_init(&cond);
    return cond;
}



int dvz_cond_signal(DvzCond* cond)
{
    ASSERT(cond != NULL);
    return cnd_signal(cond);
}



int dvz_cond_wait(DvzCond* cond, DvzMutex* mutex)
{
    ASSERT(cond != NULL);
    return cnd_wait(cond, mutex);
}



int dvz_cond_timedwait(DvzCond* cond, DvzMutex* mutex, struct timespec* wait)
{
    ASSERT(cond != NULL);
    return cnd_timedwait(cond, mutex, wait);
}



void dvz_cond_destroy(DvzCond* cond)
{
    ASSERT(cond != NULL);
    cnd_destroy(cond);
}
