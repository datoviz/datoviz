/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Polygon internals                                                                            */
/*************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "_scene.h"

#define DVZ_POLYGON_COMPOSITE_FILL_ROLE   "fill"
#define DVZ_POLYGON_COMPOSITE_STROKE_ROLE "stroke"
#define DVZ_POLYGON_SET_INITIAL_CAPACITY  4

bool _polygon_allocation_valid(uint32_t count, DvzSize item_size);

void _polygon_color_copy(DvzColor* dst, const DvzColor src);

uint32_t _polygon_stored_ring_count(const DvzPolygonStoredRing* ring);

void _polygon_ring_reset(DvzPolygonStoredRing* ring);

bool _polygon_ring_copy(const DvzPolygonRing* src, DvzPolygonStoredRing* dst);

void _polygon_set_item_reset(DvzPolygonSetItem* item);

void _polygon_set_item_default_style(DvzPolygonSetItem* item);

int _polygon_copy_desc(
    const DvzPolygonDesc* desc, DvzPolygonStoredRing* outer, DvzPolygonStoredRing** holes,
    uint32_t* hole_count);

bool _polygon_borrowed_desc(
    const DvzPolygon* polygon, DvzPolygonRing* outer, DvzPolygonRing** holes);

void _polygon_mark_composites_dirty(DvzPolygon* polygon, bool fill_dirty, bool stroke_dirty);

void _polygon_set_mark_composites_dirty(
    DvzPolygonSet* set, bool fill_dirty, bool stroke_dirty);

DvzPolygon* _scene_alloc_polygon(DvzScene* scene);

DvzPolygonSet* _scene_alloc_polygon_set(DvzScene* scene);

bool _polygon_set_reserve(DvzPolygonSet* set, uint32_t capacity);

int _polygon_set_item_set_geometry(DvzPolygonSetItem* item, const DvzPolygonDesc* desc);
