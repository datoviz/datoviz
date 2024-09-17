/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Params                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PARAMS
#define DVZ_HEADER_PARAMS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_enums.h"
#include "../_log.h"
#include "datoviz_math.h"
#include "dual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzParamsAttr DvzParamsAttr;
typedef struct DvzParams DvzParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_PARAMS_MAX_ATTRS 16



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzParamsAttr
{
    uint32_t attr_idx;
    DvzSize offset;
    DvzSize item_size;
};



struct DvzParams
{
    DvzBatch* batch;
    // DvzSize struct_size;
    DvzDual dual;
    DvzParamsAttr attrs[DVZ_PARAMS_MAX_ATTRS];
    bool is_shared; // if false, the visual will destroy it, otherwise the scene will
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Params                                                                                       */
/*************************************************************************************************/

/**
 *
 */
DvzParams* dvz_params(DvzBatch* batch, DvzSize struct_size, bool is_shared);



/**
 *
 */
void dvz_params_attr(DvzParams* params, uint32_t idx, DvzSize offset, DvzSize item_size);



/**
 *
 */
void dvz_params_data(DvzParams* params, void* data);



/**
 *
 */
void dvz_params_set(DvzParams* params, uint32_t idx, void* item);



/**
 *
 */
void* dvz_params_get(DvzParams* params, uint32_t idx);



/**
 *
 */
void dvz_params_bind(DvzParams* params, DvzId graphics_id, uint32_t slot_idx);



/**
 *
 */
void dvz_params_update(DvzParams* params);



/**
 *
 */
void dvz_params_destroy(DvzParams* params);



EXTERN_C_OFF

#endif
