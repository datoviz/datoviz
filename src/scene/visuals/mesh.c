/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Mesh                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/mesh.h"
#include "_cglm.h"
#include "_map.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/baker.h"
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
#define MESH_SLOT_LIGHT 2
#define MESH_SLOT_MATERIAL 3
#define MESH_SLOT_CONTOUR 4
#define MESH_SLOT_TEX 5



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// See datoviz_enums.h



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);

    // NOTE: if indexing is used, count is item_count, so index_count/3 (number of faces).
    // We need to multiply by three to retrieve the number of elements to draw using
    // indexing.
    bool indexed = ((visual->flags & DVZ_VISUAL_FLAGS_INDEXED) != 0);
    if (indexed)
    {
        count *= 3;
    }

    dvz_visual_instance(visual, canvas, first, 0, count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_mesh(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Parse flags.
    int textured = (flags & DVZ_MESH_FLAGS_TEXTURED);
    int lighting = (flags & DVZ_MESH_FLAGS_LIGHTING);
    int contour = (flags & DVZ_MESH_FLAGS_CONTOUR);
    int isoline = (flags & DVZ_MESH_FLAGS_ISOLINE);
    log_trace("create mesh visual, texture: %d, lighting: %d", textured, lighting);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_mesh");

    // Enable depth test.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_ENABLE);
    dvz_visual_front(visual, DVZ_FRONT_FACE_COUNTER_CLOCKWISE);
    dvz_visual_cull(visual, DVZ_CULL_MODE_NONE);

    // Specialization constants.
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int), &textured);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int), &lighting);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 2, sizeof(int), &contour);
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 3, sizeof(int), &isoline);

    // Textured vertex.
    if (textured)
    {
        // Vertex attributes.
        dvz_visual_attr( //
            visual, 0, FIELD(DvzMeshTexturedVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 1, FIELD(DvzMeshTexturedVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 2, FIELD(DvzMeshTexturedVertex, texcoords), DVZ_FORMAT_R32G32B32A32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 3, FIELD(DvzMeshTexturedVertex, value), DVZ_FORMAT_R32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 4, FIELD(DvzMeshTexturedVertex, d_left), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 5, FIELD(DvzMeshTexturedVertex, d_right), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 6, FIELD(DvzMeshTexturedVertex, contour), DVZ_FORMAT_R8G8B8A8_SINT, 0);

        // Vertex stride.
        dvz_visual_stride(visual, 0, sizeof(DvzMeshTexturedVertex));
    }
    // Color vertex.
    else
    {
        // Vertex attributes.
        dvz_visual_attr( //
            visual, 0, FIELD(DvzMeshColorVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 1, FIELD(DvzMeshColorVertex, normal), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 2, FIELD(DvzMeshColorVertex, color), DVZ_FORMAT_COLOR, 0);
        dvz_visual_attr( //
            visual, 3, FIELD(DvzMeshColorVertex, value), DVZ_FORMAT_R32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 4, FIELD(DvzMeshColorVertex, d_left), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 5, FIELD(DvzMeshColorVertex, d_right), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
        dvz_visual_attr( //
            visual, 6, FIELD(DvzMeshColorVertex, contour), DVZ_FORMAT_R8G8B8A8_SINT, 0);

        // Vertex stride.
        dvz_visual_stride(visual, 0, sizeof(DvzMeshColorVertex));
    }

    // Slots.
    _common_setup(visual);
    dvz_visual_slot(visual, MESH_SLOT_LIGHT, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, MESH_SLOT_MATERIAL, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, MESH_SLOT_CONTOUR, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, MESH_SLOT_TEX, DVZ_SLOT_TEX);

    // Lights.
    DvzParams* light_params = dvz_visual_params(visual, MESH_SLOT_LIGHT, sizeof(DvzMeshLight));
    dvz_params_attr(light_params, DVZ_LIGHT_PARAMS_POS, FIELD(DvzMeshLight, pos));
    dvz_params_attr(light_params, DVZ_LIGHT_PARAMS_COLOR, FIELD(DvzMeshLight, color));

    // Material.
    DvzParams* material_params = dvz_visual_params(visual, MESH_SLOT_MATERIAL, sizeof(DvzMeshMaterial));
    dvz_params_attr(material_params, DVZ_MESH_PARAMS_PARAMS, FIELD(DvzMeshMaterial, params));
    dvz_params_attr(material_params, DVZ_MESH_PARAMS_SHINE, FIELD(DvzMeshMaterial, shine));
    dvz_params_attr(material_params, DVZ_MESH_PARAMS_EMIT, FIELD(DvzMeshMaterial, emit));

    // Contour.
    DvzParams* params = dvz_visual_params(visual, MESH_SLOT_CONTOUR, sizeof(DvzMeshContour));
    dvz_params_attr(params, DVZ_MESH_PARAMS_EDGECOLOR, FIELD(DvzMeshContour, edgecolor));
    dvz_params_attr(params, DVZ_MESH_PARAMS_LINEWIDTH, FIELD(DvzMeshContour, linewidth));
    dvz_params_attr(params, DVZ_MESH_PARAMS_ISOLINE_COUNT, FIELD(DvzMeshContour, isoline_count));

    // Default texture to avoid Vulkan warning with unbound texture slot.
    dvz_visual_tex(
        visual, MESH_SLOT_TEX, DVZ_SCENE_DEFAULT_TEX_ID, DVZ_SCENE_DEFAULT_SAMPLER_ID, DVZ_ZERO_OFFSET);

    // Default light parameters.
    if (lighting > 0)
    {
        dvz_mesh_light_pos(visual, 0, DVZ_DEFAULT_LIGHT0_POS);
        dvz_mesh_light_color(visual, 0, DVZ_DEFAULT_LIGHT0_COLOR);
        dvz_mesh_light_pos(visual, 1, DVZ_DEFAULT_LIGHT1_POS);
        dvz_mesh_light_color(visual, 1, DVZ_DEFAULT_LIGHT1_COLOR);
        dvz_mesh_light_pos(visual, 2, DVZ_DEFAULT_LIGHT2_POS);
        dvz_mesh_light_color(visual, 2, DVZ_DEFAULT_LIGHT2_COLOR);
        dvz_mesh_light_pos(visual, 3, DVZ_DEFAULT_LIGHT3_POS);
        dvz_mesh_light_color(visual, 3, DVZ_DEFAULT_LIGHT3_COLOR);
        dvz_mesh_material_params(visual, 0, DVZ_DEFAULT_AMBIENT);
        dvz_mesh_material_params(visual, 1, DVZ_DEFAULT_DIFFUSE);
        dvz_mesh_material_params(visual, 2, DVZ_DEFAULT_SPECULAR);
        dvz_mesh_material_params(visual, 3, DVZ_DEFAULT_EMISSION);
        dvz_mesh_shine(visual, DVZ_DEFAULT_SHINE);
        dvz_mesh_emit(visual, DVZ_DEFAULT_EMIT);
    }
    dvz_mesh_edgecolor(visual, (DvzColor){DVZ_DEFAULT_EDGECOLOR});
    dvz_mesh_linewidth(visual, DVZ_DEFAULT_LINEWIDTH);
    dvz_mesh_density(visual, 10);

    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_mesh_alloc(DvzVisual* visual, uint32_t vertex_count, uint32_t index_count)
{
    ANN(visual);
    ASSERT(vertex_count > 0);
    ASSERT(index_count % 3 == 0);
    log_debug("allocating the mesh visual, %d vertices, %d indices", vertex_count, index_count);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.

    // NOTE: by convention in this visual, 1 item = 1 triangle.
    // This is why item_count is index_count / 3 below.
    dvz_visual_alloc(visual, index_count / 3, vertex_count, index_count);
}



void dvz_mesh_index(DvzVisual* visual, uint32_t first, uint32_t count, DvzIndex* values, int flags)
{
    ANN(visual);
    if ((visual->flags & DVZ_VISUAL_FLAGS_INDEXED) == 0)
    {
        log_error(
            "mesh visual should be created with flag `DVZ_VISUAL_FLAGS_INDEXED` to use indices");
    }
    dvz_visual_index(visual, first, count, values);
}



void dvz_mesh_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, (void*)values);
}



void dvz_mesh_normal(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, (void*)values);
}



void dvz_mesh_color(DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    ANN(visual);
    if (visual->flags & DVZ_MESH_FLAGS_TEXTURED)
    {
        log_error("cannot use dvz_mesh_color() with a textured mesh");
        return;
    }
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_mesh_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* values, int flags)
{
    ANN(visual);
    if (!(visual->flags & DVZ_MESH_FLAGS_TEXTURED))
    {
        log_error("cannot use dvz_mesh_texcoords() with a color mesh");
        return;
    }
    dvz_visual_data(visual, 2, first, count, (void*)values);
}



void dvz_mesh_isoline(DvzVisual* visual, uint32_t first, uint32_t count, float* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 3, first, count, (void*)values);
}



void dvz_mesh_left(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 4, first, count, (void*)values);
}



void dvz_mesh_right(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 5, first, count, (void*)values);
}



void dvz_mesh_contour(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 6, first, count, (void*)values);
}



void dvz_mesh_texture(DvzVisual* visual, DvzTexture* texture)
{
    ANN(visual);
    ANN(texture);

    if (!(visual->flags & DVZ_MESH_FLAGS_TEXTURED))
    {
        log_error("the mesh visual needs to be created with the DVZ_MESH_FLAGS_TEXTURED flag");
        return;
    }

    dvz_texture_create(texture); // only create it if it is not already created
    dvz_visual_tex(visual, MESH_SLOT_TEX, texture->tex, texture->sampler, DVZ_ZERO_OFFSET);
}



void dvz_mesh_light_pos(DvzVisual* visual, uint32_t idx, vec4 pos)
{
    ANN(visual);
    if (!(visual->flags & DVZ_MESH_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the mesh flag DVZ_MESH_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = MESH_SLOT_LIGHT;
    uint32_t attr_idx = DVZ_LIGHT_PARAMS_POS;
    mat4* light_pos = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl
    light_pos[0][idx][0] = pos[0];
    light_pos[0][idx][1] = pos[1];
    light_pos[0][idx][2] = pos[2];
    light_pos[0][idx][3] = pos[3];
    dvz_visual_param(visual, slot_idx, attr_idx, light_pos);
}



void dvz_mesh_light_color(DvzVisual* visual, uint32_t idx, DvzColor rgba)
{
    ANN(visual);
    if (!(visual->flags & DVZ_MESH_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the mesh flag DVZ_MESH_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = MESH_SLOT_LIGHT;
    uint32_t attr_idx = DVZ_LIGHT_PARAMS_COLOR;
    mat4* light_color = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl

    // Need to convert to float rgb as this is what the shader expects.
#if DVZ_COLOR_CVEC4
    light_color[0][idx][0] = rgba[0] / 255.0;
    light_color[0][idx][1] = rgba[1] / 255.0;
    light_color[0][idx][2] = rgba[2] / 255.0;
    light_color[0][idx][3] = rgba[3] / 255.0;
#else
    light_color[0][idx][0] = rgba[0];
    light_color[0][idx][1] = rgba[1];
    light_color[0][idx][2] = rgba[2];
    light_color[0][idx][3] = rgba[3];
#endif

    dvz_visual_param(visual, slot_idx, attr_idx, light_color);
}



void dvz_mesh_material_params(DvzVisual* visual, uint32_t idx, vec3 params)
{
    ANN(visual);
    if (!(visual->flags & DVZ_MESH_FLAGS_LIGHTING))
    {
        log_error(
            "lighting support needs to be activated with the mesh flag DVZ_MESH_FLAGS_LIGHTING");
        return;
    }

    uint32_t slot_idx = MESH_SLOT_MATERIAL;
    uint32_t attr_idx = DVZ_MESH_PARAMS_PARAMS;
    mat4* material_params = _get_param(visual, slot_idx, attr_idx);

    // NOTE: matrix order is transposed between C and glsl
    material_params[0][idx][0] = params[0];
    material_params[0][idx][1] = params[1];
    material_params[0][idx][2] = params[2];
    material_params[0][idx][3] = 1.0;  //params[3];
    dvz_visual_param(visual, slot_idx, attr_idx, material_params);
}



void dvz_mesh_shine(DvzVisual* visual, float shine)
{
    ANN(visual);
    dvz_visual_param(visual, MESH_SLOT_MATERIAL, DVZ_MESH_PARAMS_SHINE, &shine);
}



void dvz_mesh_emit(DvzVisual* visual, float emit)
{
    ANN(visual);
    dvz_visual_param(visual, MESH_SLOT_MATERIAL, DVZ_MESH_PARAMS_EMIT, &emit);
}



void dvz_mesh_edgecolor(DvzVisual* visual, DvzColor rgba)
{
    ANN(visual);

    vec4 color = {0};
#if DVZ_COLOR_CVEC4
    color[0] = rgba[0] / 255.0;
    color[1] = rgba[1] / 255.0;
    color[2] = rgba[2] / 255.0;
    color[3] = rgba[3] / 255.0;
#else
    color[0] = rgba[0];
    color[1] = rgba[1];
    color[2] = rgba[2];
    color[3] = rgba[3];
#endif
    dvz_visual_param(visual, MESH_SLOT_CONTOUR, DVZ_MESH_PARAMS_EDGECOLOR, color);
}



void dvz_mesh_linewidth(DvzVisual* visual, float linewidth)
{
    ANN(visual);
    dvz_visual_param(visual, MESH_SLOT_CONTOUR, DVZ_MESH_PARAMS_LINEWIDTH, &linewidth);
}



void dvz_mesh_density(DvzVisual* visual, uint32_t count)
{
    ANN(visual);
    dvz_visual_param(visual, MESH_SLOT_CONTOUR, DVZ_MESH_PARAMS_ISOLINE_COUNT, &count);
}



DvzVisual* dvz_mesh_shape(DvzBatch* batch, DvzShape* shape, int flags)
{
    ANN(batch);
    ANN(shape);
    ANN(shape->pos);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    // NOTE: set the visual flag to indexed or non-indexed (default) depending on whether the shape
    // has an index buffer or not.
    flags |= (index_count > 0 ? DVZ_VISUAL_FLAGS_INDEXED : DVZ_VISUAL_FLAGS_DEFAULT);
    DvzVisual* visual = dvz_mesh(batch, flags);

    dvz_mesh_reshape(visual, shape);

    return visual;
}



void dvz_mesh_reshape(DvzVisual* visual, DvzShape* shape)
{
    ANN(visual);
    ANN(shape);
    ANN(shape->pos);

    uint32_t vertex_count = shape->vertex_count;
    uint32_t index_count = shape->index_count;
    ASSERT(vertex_count > 0);

    dvz_mesh_alloc(visual, vertex_count, index_count);

    dvz_mesh_position(visual, 0, vertex_count, shape->pos, 0);

    if (shape->normal)
        dvz_mesh_normal(visual, 0, vertex_count, shape->normal, 0);

    if (shape->color && !(visual->flags & DVZ_MESH_FLAGS_TEXTURED))
        dvz_mesh_color(visual, 0, vertex_count, shape->color, 0);

    if (shape->texcoords && (visual->flags & DVZ_MESH_FLAGS_TEXTURED))
        dvz_mesh_texcoords(visual, 0, vertex_count, shape->texcoords, 0);

    if (shape->isoline)
        dvz_mesh_isoline(visual, 0, vertex_count, shape->isoline, 0);

    if (shape->d_left)
        dvz_mesh_left(visual, 0, vertex_count, shape->d_left, 0);

    if (shape->d_right)
        dvz_mesh_right(visual, 0, vertex_count, shape->d_right, 0);

    if (shape->contour)
        dvz_mesh_contour(visual, 0, vertex_count, shape->contour, 0);

    if (shape->index_count > 0)
        dvz_mesh_index(visual, 0, index_count, shape->index, 0);
}
