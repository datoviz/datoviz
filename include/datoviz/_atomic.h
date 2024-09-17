/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Atomic operations                                                                            */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ATOMIC
#define DVZ_HEADER_ATOMIC



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include "_macros.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

// Atomic macro, for both C++ and C
#if defined(LANG_C) && !OS_WINDOWS
/*  C11 atomic is only used for C (not C++) and not on Windows                                   */
/*************************************************************************************************/
#define ATOMIC_C
#include <stdatomic.h>
// #define atomic(t, x) _Atomic t x
#define DvzAtomic_  _Atomic int32_t
#define ATOMIC_DECL static inline
#else
/*  C++ atomic                                                                                   */
/*************************************************************************************************/
#define ATOMIC_CPP
// #define atomic(t, x) std::atomic<t> x
// Forward reference.
typedef struct DvzAtomic_ DvzAtomic_;
#define ATOMIC_DECL DVZ_EXPORT
#endif

#define DvzAtomic DvzAtomic_*



/*************************************************************************************************/
/*  Atomic functions                                                                             */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize an atomic.
 *
 * @param atomic the atomic variable to initialize
 */
ATOMIC_DECL void dvz_atomic_init(DvzAtomic atomic)
#ifdef ATOMIC_C
{
    ANN(atomic);
    atomic_init(atomic, 0);
}
#else
    ;
#endif



/**
 * Create an atomic.
 *
 * @returns the atomic
 */
ATOMIC_DECL DvzAtomic dvz_atomic(void)
#ifdef ATOMIC_C
{
    DvzAtomic atomic = (DvzAtomic)calloc(1, sizeof(DvzAtomic_));
    dvz_atomic_init(atomic);
    return atomic;
}
#else
    ;
#endif



/**
 * Set an atomic variable to a given value.
 *
 * @param atomic the atomic variable
 * @param value the value
 */
ATOMIC_DECL void dvz_atomic_set(DvzAtomic atomic, int32_t value)
#ifdef ATOMIC_C
{
    ANN(atomic);
    atomic_store(atomic, value);
}
#else
    ;
#endif



/**
 * Get the value of an atomic variable.
 *
 * @param atomic the atomic variable
 * @returns the value
 */
ATOMIC_DECL int32_t dvz_atomic_get(DvzAtomic atomic)
#ifdef ATOMIC_C
{
    ANN(atomic);
    return atomic_load(atomic);
}
#else
    ;
#endif



/**
 * Destroy an atomic.
 *
 * @param atomic the atomic variable
 */
ATOMIC_DECL void dvz_atomic_destroy(DvzAtomic atomic)
#ifdef ATOMIC_C
{
    ANN(atomic);
    FREE(atomic);
}
#else
    ;
#endif



EXTERN_C_OFF

#endif
