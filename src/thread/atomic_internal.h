/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal atomic operations                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/

typedef struct DvzAtomic_ DvzAtomic_;
typedef DvzAtomic_* DvzAtomic;



EXTERN_C_ON

/*************************************************************************************************/
/*  Atomic functions                                                                             */
/*************************************************************************************************/

void dvz_atomic_init(DvzAtomic atomic);

DvzAtomic dvz_atomic(void);

void dvz_atomic_set(DvzAtomic atomic, int32_t value);

int32_t dvz_atomic_get(DvzAtomic atomic);

void dvz_atomic_destroy(DvzAtomic atomic);



EXTERN_C_OFF
