/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Array struct                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/common/obj.h"
#include "datoviz/math/array.h"
#include "datoviz/math/types.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArray DvzArray;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArray
{
    DvzObject obj;
    DvzDataType dtype;
    uint32_t components; // number of components, ie 2 for vec2, 3 for dvec3, etc.
    DvzSize item_size;
    uint32_t item_count;
    DvzSize buffer_size;
    void* data;

    // 3D arrays
    uint32_t ndims; // 1, 2, or 3
    uvec3 shape;    // only for 3D arrays
};



/*************************************************************************************************/
/*  Inline functions                                                                             */
/*************************************************************************************************/

/**
 * Retrieve a single element from an array.
 *
 * @param array the array
 * @param idx the index of the element to retrieve
 * @returns a pointer to the requested element
 */
static inline void* dvz_array_item(DvzArray* array, uint32_t idx)
{
    if (array == NULL)
        return NULL;
    idx = CLIP(idx, 0, array->item_count - 1);
    return (void*)((int64_t)array->data + (int64_t)(idx * array->item_size));
}
