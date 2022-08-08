/*************************************************************************************************/
/*  C++ atomic wrapper                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <atomic>
#include <cstddef>

#include "_atomic.h"
#include "_macros.h"



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
    atomic->atom = 0;
}



DvzAtomic dvz_atomic()
{
    DvzAtomic atomic = (DvzAtomic)calloc(1, sizeof(DvzAtomic));
    dvz_atomic_init(atomic);
    return atomic;
}



void dvz_atomic_set(DvzAtomic atomic, int32_t value)
{
    ANN(atomic);
    atomic->atom = value;
}



int32_t dvz_atomic_get(DvzAtomic atomic)
{
    ANN(atomic);
    int32_t value = atomic->atom;
    return value;
}



void dvz_atomic_destroy(DvzAtomic atomic)
{
    ANN(atomic);
    FREE(atomic);
}
