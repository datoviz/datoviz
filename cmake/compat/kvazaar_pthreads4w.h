#ifndef DVZ_KVAZAAR_PTHREADS4W_H
#define DVZ_KVAZAAR_PTHREADS4W_H

#include <pthread.h>

static inline int _dvz_kvazaar_pthread_create(
    pthread_t* thread, const pthread_attr_t* attr, void*(__PTW32_CDECL* start)(void*), void* arg)
{
    (void)attr;
    return pthread_create(thread, NULL, start, arg);
}

#define pthread_create _dvz_kvazaar_pthread_create

#endif
