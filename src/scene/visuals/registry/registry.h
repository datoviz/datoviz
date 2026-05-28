/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual family registry                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"
#include "_visual_pipeline.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzVisualFamilyOps DvzVisualFamilyOps;
typedef struct DvzVisualLowering DvzVisualLowering;

typedef bool (*DvzVisualFamilyLoweringFn)(
    const DvzVisual* visual, DvzVisualLowering* out);

typedef bool (*DvzVisualFamilyPassCapsFn)(
    const DvzVisual* visual, const DvzPanelAttach* attach, const DvzVisualLowering* lowering,
    DvzSceneVisualPassCaps* out);

struct DvzVisualFamilyOps
{
    DvzVisualType type;
    const char* name;
    DvzVisualFamilyLoweringFn resolve_lowering;
    DvzVisualFamilyPassCapsFn resolve_pass_caps;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

const DvzVisualFamilyOps* _scene_visual_family_ops(DvzVisualType type);

uint32_t _scene_visual_family_ops_count(void);

const DvzVisualFamilyOps* _scene_visual_family_ops_at(uint32_t index);

bool _scene_visual_family_ops_registered(DvzVisualType type);
