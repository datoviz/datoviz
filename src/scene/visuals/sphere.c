/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Sphere                                                                                   */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/sphere.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/scene.h"
#include "scene/texture.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Slots                                                                                        */
/*************************************************************************************************/

/*
   mvp 0
   viewport 1
*/
#define SPHERE_SLOT_LIGHT 2
#define SPHERE_SLOT_MATERIAL 3
#define SPHERE_SLOT_TEX 4



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_sphere(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, flags);
    ANN(visual);

    // Parse flags.
    int textured = ((flags & DVZ_SPHERE_FLAGS_TEXTURED) != 0);
    int rectangular = ((flags & DVZ_SPHERE_FLAGS_EQUAL_RECTANGULAR) != 0);
    int lighting = ((flags & DVZ_SPHERE_FLAGS_LIGHTING) != 0);
    int size_pixels = ((flags & DVZ_SPHERE_FLAGS_SIZE_PIXELS) != 0);
    log_trace(
        "create sphere visual, texture: %d, lighting: %d, size_pixels: %d, equal_rectangular: %d", //
        textured, lighting, size_pixels, rectangular);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_sphere");

    // Specialization constants.
    dvz_visual_specialization(visual, DVZ_SHADER_VERTEX, 0, sizeof(int), &size_pixels);

    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int), &textured);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int), &lighting);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int), &rectangular);


    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Vertex attributes.
    dvz_visual_attr(visual, 0, FIELD(DvzSphereVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, FIELD(DvzSphereVertex, color), DVZ_FORMAT_COLOR, 0);
    dvz_visual_attr(visual, 2, FIELD(DvzSphereVertex, size), DVZ_FORMAT_R32_SFLOAT, 0);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzSphereVertex));

    // Slots.
    _common_setup(visual);
    dvz_visual_slot(visual, SPHERE_SLOT_LIGHT, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, SPHERE_SLOT_MATERIAL, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, SPHERE_SLOT_TEX, DVZ_SLOT_TEX);

    // Lights.
    DvzParams* light_params = dvz_visual_params(visual, SPHERE_SLOT_LIGHT, sizeof(DvzSphereLight));
    dvz_params_attr(light_params, DVZ_LIGHT_PARAMS_POS, FIELD(DvzSphereLight, pos));
    dvz_params_attr(light_params, DVZ_LIGHT_PARAMS_COLOR, FIELD(DvzSphereLight, color));

    // Material.
    DvzParams* material_params =
        dvz_visual_params(visual, SPHERE_SLOT_MATERIAL, sizeof(DvzSphereMaterial));
    dvz_params_attr(material_params, DVZ_SPHERE_PARAMS_PARAMS, FIELD(DvzSphereMaterial, params));
    dvz_params_attr(material_params, DVZ_SPHERE_PARAMS_SHINE, FIELD(DvzSphereMaterial, shine));
    dvz_params_attr(material_params, DVZ_SPHERE_PARAMS_EMIT, FIELD(DvzSphereMaterial, emit));

    // Default texture to avoid Vulkan warning with unbound texture slot.
    dvz_visual_tex(
        visual, SPHERE_SLOT_TEX, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID,
        DVZ_ZERO_OFFSET);

    // Default light parameters.
    if (lighting > 0)
    {
        dvz_sphere_light_pos(visual, 0, DVZ_DEFAULT_LIGHT_POS);
        dvz_sphere_light_color(visual, 0, (DvzColor){DVZ_DEFAULT_LIGHT_COLOR});
        // dvz_sphere_light_pos(visual, 1, DVZ_DEFAULT_LIGHT1_POS);
        // dvz_sphere_light_pos(visual, 2, DVZ_DEFAULT_LIGHT2_POS);
        // dvz_sphere_light_pos(visual, 3, DVZ_DEFAULT_LIGHT3_POS);
        dvz_sphere_material_params(visual, 0, DVZ_DEFAULT_AMBIENT);
        dvz_sphere_material_params(visual, 1, DVZ_DEFAULT_DIFFUSE);
        dvz_sphere_material_params(visual, 2, DVZ_DEFAULT_SPECULAR);
        dvz_sphere_material_params(visual, 3, DVZ_DEFAULT_EMISSION);
        dvz_sphere_shine(visual, DVZ_DEFAULT_SHINE);
        dvz_sphere_emit(visual, DVZ_DEFAULT_EMIT);
    }

    return visual;
}



void dvz_sphere_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the sphere visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, item_count, 0);
}



void dvz_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, data);
}



void dvz_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, data);
}



void dvz_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* data, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, data);
}



void dvz_sphere_light_pos(DvzVisual* visual, uint32_t idx, vec4 pos)
{
    ANN(visual);
    if (!(visual->flags & DVZ_SPHERE_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the sphere flag "
            "DVZ_SPHERE_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = SPHERE_SLOT_LIGHT;
    uint32_t attr_idx = DVZ_LIGHT_PARAMS_POS;
    mat4* light_pos = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl
    light_pos[0][idx][0] = pos[0];
    light_pos[0][idx][1] = -pos[1]; // Flip the light y value.
    light_pos[0][idx][2] = pos[2];
    light_pos[0][idx][3] = pos[3];
    dvz_visual_param(visual, slot_idx, attr_idx, light_pos);
}



void dvz_sphere_light_color(DvzVisual* visual, uint32_t idx, DvzColor rgba)
{
    ANN(visual);
    if (!(visual->flags & DVZ_SPHERE_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the sphere flag "
            "DVZ_SPHERE_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = SPHERE_SLOT_LIGHT;
    uint32_t attr_idx = DVZ_LIGHT_PARAMS_COLOR;
    mat4* light_color = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl

    // Need to convert to float rgb as this is what the shader expects.
    light_color[0][idx][0] = ALPHA_D2F(rgba[0]);
    light_color[0][idx][1] = ALPHA_D2F(rgba[1]);
    light_color[0][idx][2] = ALPHA_D2F(rgba[2]);
    light_color[0][idx][3] = ALPHA_D2F(rgba[3]);

    dvz_visual_param(visual, slot_idx, attr_idx, light_color);
}



void dvz_sphere_material_params(DvzVisual* visual, uint32_t idx, vec3 params)
{
    ANN(visual);
    if (!(visual->flags & DVZ_SPHERE_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the sphere flag "
            "DVZ_SPHERE_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = SPHERE_SLOT_MATERIAL;
    uint32_t attr_idx = DVZ_SPHERE_PARAMS_PARAMS;
    mat4* material_params = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl
    material_params[0][idx][0] = params[0];
    material_params[0][idx][1] = params[1];
    material_params[0][idx][2] = params[2];
    material_params[0][idx][3] = 1.0; // params[3];
    dvz_visual_param(visual, slot_idx, attr_idx, material_params);
}



void dvz_sphere_shine(DvzVisual* visual, float shine)
{
    ANN(visual);
    dvz_visual_param(visual, SPHERE_SLOT_MATERIAL, DVZ_SPHERE_PARAMS_SHINE, &shine);
}



void dvz_sphere_emit(DvzVisual* visual, float emit)
{
    ANN(visual);
    dvz_visual_param(visual, SPHERE_SLOT_MATERIAL, DVZ_SPHERE_PARAMS_EMIT, &emit);
}



void dvz_sphere_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    if (!(visual->flags & DVZ_SPHERE_FLAGS_TEXTURED))
    {
        log_error("the sphere visual needs to be created with the DVZ_SPHERE_FLAGS_TEXTURED flag");
        return;
    }

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, SPHERE_SLOT_TEX, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);
}
