/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene interaction internals                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/input/pointer.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_ITEM_INTERACTION_QUERY_HOVER 1u
#define DVZ_ITEM_INTERACTION_QUERY_SELECTION 2u

bool _scene_item_interaction_pointer(DvzItemInteraction* interaction, const DvzPointerEvent* ev);

void _scene_item_interaction_pointer_leave(DvzItemInteraction* interaction);

void _scene_item_interaction_apply_query_result(
    DvzItemInteraction* interaction, uint32_t query_kind, const DvzQueryResult* query);
