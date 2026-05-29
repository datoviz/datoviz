/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual bindings */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "_visual_family.h"
#include "_visual_internal.h"
#include "bindings_internal.h"
#include "datoviz/scene.h"
#include "sample_profile.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return one mutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
DvzVisualBinding* _visual_binding(DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    _visual_family_state(visual)->bindings[idx].kind = kind;
    return &_visual_family_state(visual)->bindings[idx];
}



/**
 * Return one immutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
const DvzVisualBinding* _visual_binding_const(const DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    return &_visual_family_state(visual)->bindings[idx];
}



/**
 * Assign one visual binding and keep legacy convenience fields in sync.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @param slot_name the binding slot name, or NULL to clear
 * @param resource the bound resource, or NULL to clear
 * @param owned whether the visual owns the resource
 */
void _visual_binding_assign(
    DvzVisual* visual, DvzVisualBindingKind kind, const char* slot_name, void* resource,
    bool owned)
{
    ANN(visual);
    DvzVisualBinding* binding = _visual_binding(visual, kind);
    ANN(binding);
    binding->resource = resource;
    binding->owned = owned;
    dvz_memset(binding->slot, sizeof(binding->slot), 0, sizeof(binding->slot));
    if (slot_name != NULL && resource != NULL)
        dvz_strlcpy(binding->slot, slot_name, sizeof(binding->slot));

    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        _visual_family_state(visual)->field = (DvzSampledField*)resource;
        _visual_family_state(visual)->field_owned = owned;
        dvz_memset(_visual_family_state(visual)->field_slot, sizeof(_visual_family_state(visual)->field_slot), 0, sizeof(_visual_family_state(visual)->field_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(_visual_family_state(visual)->field_slot, slot_name, sizeof(_visual_family_state(visual)->field_slot));
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        _visual_family_state(visual)->buffer = (DvzSceneBuffer*)resource;
        dvz_memset(
            _visual_family_state(visual)->buffer_slot, sizeof(_visual_family_state(visual)->buffer_slot), 0, sizeof(_visual_family_state(visual)->buffer_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(_visual_family_state(visual)->buffer_slot, slot_name, sizeof(_visual_family_state(visual)->buffer_slot));
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        _visual_family_state(visual)->scale = (DvzScale*)resource;
        dvz_memset(_visual_family_state(visual)->scale_slot, sizeof(_visual_family_state(visual)->scale_slot), 0, sizeof(_visual_family_state(visual)->scale_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(_visual_family_state(visual)->scale_slot, slot_name, sizeof(_visual_family_state(visual)->scale_slot));
        break;
    default:
        break;
    }
}



/**
 * Clear one visual binding.
 *
 * @param visual the visual
 * @param kind the binding kind
 */
void _visual_binding_clear(DvzVisual* visual, DvzVisualBindingKind kind)
{
    _visual_binding_assign(visual, kind, NULL, NULL, false);
}


