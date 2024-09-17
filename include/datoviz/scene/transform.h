/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Transform                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TRANSFORM
#define DVZ_HEADER_TRANSFORM



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz_types.h"
#include "dual.h"
#include "mvp.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTransform DvzTransform;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
// typedef struct DvzDual DvzDual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_TRANSFORM_FLAGS_LINEAR = 0x0000,
    DVZ_TRANSFORM_FLAGS_PANEL = 0x0010,
} DvzTransformFlags;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTransform
{
    int flags;
    DvzDual dual;       // Dual array with a DvzMVP struct.
    DvzTransform* next; // NOTE: transform chaining not implemented yet
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Transform                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DvzTransform* dvz_transform(DvzBatch* batch, int flags);



/**
 *
 */
void dvz_transform_set(DvzTransform* tr, DvzMVP* mvp);



/**
 *
 */
void dvz_transform_update(DvzTransform* tr);



/**
 *
 */
void dvz_transform_next(DvzTransform* tr, DvzTransform* next);



/**
 *
 */
DvzMVP* dvz_transform_mvp(DvzTransform* tr);



/**
 *
 */
void dvz_transform_destroy(DvzTransform* tr);



EXTERN_C_OFF

#endif
