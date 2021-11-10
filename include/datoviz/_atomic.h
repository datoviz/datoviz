/*************************************************************************************************/
/*  Atomic operations                                                                            */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ATOMIC
#define DVZ_HEADER_ATOMIC



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_macros.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

// Atomic macro, for both C++ and C
// TODO: atomic wrapper type
#ifndef __cplusplus
#include <stdatomic.h>
#define atomic(t, x) _Atomic t x
#define DvzAtomic    _Atomic int32_t
#else
#include <atomic>
#define atomic(t, x) std::atomic<t> x
#define DvzAtomic    std::atomic<int32_t>
#endif



/*************************************************************************************************/
/*  Atomic functions                                                                             */
/*************************************************************************************************/

/**
 * Initialize an atomic.
 *
 * @param atomic the atomic variable to initialize
 */
static inline void dvz_atomic_init(DvzAtomic* atomic) { atomic_init(atomic, 0); }



/**
 * Set an atomic variable to a given value.
 *
 * @param atomic the atomic variable
 * @param value the value
 */
static inline void dvz_atomic_set(DvzAtomic* atomic, int32_t value)
{
    atomic_store(atomic, value);
}



/**
 * Get the value of an atomic variable.
 *
 * @param atomic the atomic variable
 * @returns the value
 */
static inline int32_t dvz_atomic_get(DvzAtomic* atomic) { return atomic_load(atomic); }



#endif
