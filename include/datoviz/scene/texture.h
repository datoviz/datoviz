/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/* Texture                                                                                       */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEXTURE
#define DVZ_HEADER_TEXTURE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_atomic.h"
#include "_enums.h"
#include "_obj.h"
#include "datoviz_types.h"
#include "mvp.h"
#include "params.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define FIELD(t, f) offsetof(t, f), fsizeof(t, f)



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzTexture DvzTexture;

// Forward declarations.
typedef struct DvzBatch DvzBatch;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzTexture
{
    DvzObject obj;
    DvzBatch* batch;
    DvzTexDims dims;
    uvec3 shape;
    DvzFormat format;
    DvzFilter filter;
    DvzSamplerAddressMode address_mode;

    DvzId tex;
    DvzId sampler;

    int flags;
};



#endif
