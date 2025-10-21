/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Assertions                                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Assertions                                                                                   */
/*************************************************************************************************/

#ifndef ASSERT
#if DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) dvz_assert(x, #x, __FILE_NAME__, __LINE__)
#endif
#endif



// ASSERT NOT NULL
#ifndef ANN
#define ANN(x)                                                                                    \
    do                                                                                            \
    {                                                                                             \
        assert((x) != NULL);                                                                      \
        ASSERT((x) != NULL);                                                                      \
        DVZ_ASSUME((x) != NULL);                                                                  \
    } while (0)
#endif



EXTERN_C_ON

DVZ_EXPORT extern void
dvz_assert(bool assertion, const char* message, const char* filename, int line);

EXTERN_C_OFF
