/*************************************************************************************************/
/*  C++ atomic wrapper                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <atomic>

#include "_alloc.h"
#include "_assertions.h"
#include "datoviz/thread/atomic.h"



/*************************************************************************************************/
/*  C struct wrapper                                                                             */
/*************************************************************************************************/

struct DvzAtomic_
{
    std::atomic<int32_t> atom;
};



/*************************************************************************************************/
/*  Atomic functions                                                                             */
/*************************************************************************************************/

void dvz_atomic_init(DvzAtomic atomic)
{
    ANN(atomic);
    atomic->atom.store(0, std::memory_order_relaxed);
}



DvzAtomic dvz_atomic()
{
    DvzAtomic atomic = (DvzAtomic)dvz_calloc(1, sizeof(DvzAtomic_));
    ANN(atomic);
    dvz_atomic_init(atomic);
    return atomic;
}



void dvz_atomic_set(DvzAtomic atomic, int32_t value)
{
    ANN(atomic);
    atomic->atom.store(value, std::memory_order_release);
}



int32_t dvz_atomic_get(DvzAtomic atomic)
{
    ANN(atomic);
    return atomic->atom.load(std::memory_order_acquire);
}



void dvz_atomic_destroy(DvzAtomic atomic)
{
    ANN(atomic);
    dvz_free(atomic);
}
