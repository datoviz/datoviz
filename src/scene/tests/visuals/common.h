/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <float.h>
#include <math.h>

#include "scene_graph_utils.h"
#include "_frame_plan_runtime_internal.h"
#include "core/figure_emit_internal.h"
#include "datoviz/geom.h"
#include "datoviz/vk/memory_interop.h"
#include "datoviz/vklite/sync.h"
#include "domain/field_internal.h"
#include "domain/polygon_internal.h"
#include "registry/registry.h"
#include "scene_emit/internal.h"
#include "visuals/bounds_internal.h"
#include "_visual_internal.h"



/**
 * Assert one bounds object exactly enough for deterministic test inputs.
 *
 * @param bounds the bounds object
 * @param dims expected dimension count
 * @param min0 expected lower x coordinate
 * @param min1 expected lower y coordinate
 * @param min2 expected lower z coordinate
 * @param max0 expected upper x coordinate
 * @param max1 expected upper y coordinate
 * @param max2 expected upper z coordinate
 * @return 0 on success, 1 on assertion failure
 */
static inline int _scene_visuals_bounds_expect(
    const DvzBounds* bounds, uint32_t dims, double min0, double min1, double min2, double max0,
    double max1, double max2)
{
    ANN(bounds);
    AT(bounds->valid);
    AT(bounds->dims == dims);
    AC(bounds->min[0], min0, 1e-6);
    AC(bounds->min[1], min1, 1e-6);
    AC(bounds->min[2], min2, 1e-6);
    AC(bounds->max[0], max0, 1e-6);
    AC(bounds->max[1], max1, 1e-6);
    AC(bounds->max[2], max2, 1e-6);
    return 0;
}



/**
 * Apply a Phong material while preserving the current visual alpha mode.
 *
 * @param visual the visual
 * @param light_direction material light direction
 * @param ambient ambient coefficient
 * @param diffuse diffuse coefficient
 * @param specular specular coefficient
 * @param shininess shininess exponent
 * @return 0 on success, -1 on error
 */
static inline int _scene_visuals_set_phong_material(
    DvzVisual* visual, const float light_direction[3], float ambient, float diffuse,
    float specular, float shininess)
{
    ANN(visual);
    ANN(light_direction);
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.alpha_mode = dvz_visual_alpha_mode(visual);
    material.light_direction[0] = light_direction[0];
    material.light_direction[1] = light_direction[1];
    material.light_direction[2] = light_direction[2];
    material.phong.ambient = ambient;
    material.phong.diffuse = diffuse;
    material.phong.specular = specular;
    material.phong.shininess = shininess;
    return dvz_visual_set_material(visual, &material);
}
