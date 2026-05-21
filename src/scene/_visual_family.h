/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family descriptors                                                              */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVisualFamilyAttrDesc DvzVisualFamilyAttrDesc;
struct DvzVisualFamilyAttrDesc
{
    const char* name;
    uint32_t item_size;
    uint32_t source_mask;
    bool instance;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const char* _visual_family_attr_storage_name(DvzVisualType type, const char* name);

const DvzVisualFamilyAttrDesc*
_visual_family_attr_desc(DvzVisualType type, const char* name);

const char* _visual_family_attr_expected(DvzVisualType type);

bool _visual_family_attr_source_supported(
    DvzVisualType type, const char* name, DvzVisualAttrSource source);

