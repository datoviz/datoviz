/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/visuals/image.h"
#include "../src/resources_utils.h"
#include "_map.h"
#include "datoviz.h"
#include "datoviz_protocol.h"
#include "datoviz_types.h"
#include "fileio.h"
#include "scene/graphics.h"
#include "scene/viewset.h"
#include "scene/visual.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Internal functions                                                                           */
/*************************************************************************************************/

static void _visual_callback(
    DvzVisual* visual, DvzId canvas, //
    uint32_t first, uint32_t count,  //
    uint32_t first_instance, uint32_t instance_count)
{
    ANN(visual);
    ASSERT(count > 0);
    dvz_visual_instance(visual, canvas, 6 * first, 0, 6 * count, first_instance, instance_count);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual* dvz_image(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, flags);
    ANN(visual);

    // Visual shaders.
    dvz_visual_shader(visual, "graphics_image");

    // Vertex attributes.
    int af = DVZ_ATTR_FLAGS_REPEAT_X6;
    dvz_visual_attr(visual, 0, FIELD(DvzImageVertex, pos), DVZ_FORMAT_R32G32B32_SFLOAT, af);
    dvz_visual_attr(visual, 1, FIELD(DvzImageVertex, size), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 2, FIELD(DvzImageVertex, anchor), DVZ_FORMAT_R32G32_SFLOAT, af);
    dvz_visual_attr(visual, 3, FIELD(DvzImageVertex, uv), DVZ_FORMAT_R32G32_SFLOAT, 0);
    dvz_visual_attr(visual, 4, FIELD(DvzImageVertex, color), DVZ_FORMAT_COLOR, af);

    // Vertex stride.
    dvz_visual_stride(visual, 0, sizeof(DvzImageVertex));

    // Slots.
    dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 2, DVZ_SLOT_DAT);
    dvz_visual_slot(visual, 3, DVZ_SLOT_TEX);


    // Params.
    DvzParams* params = dvz_visual_params(visual, 2, sizeof(DvzImageParams));
    dvz_params_attr(params, DVZ_IMAGE_PARAMS_EDGECOLOR, FIELD(DvzImageParams, edgecolor));
    dvz_params_attr(params, DVZ_IMAGE_PARAMS_LINEWIDTH, FIELD(DvzImageParams, linewidth));
    dvz_params_attr(params, DVZ_IMAGE_PARAMS_RADIUS, FIELD(DvzImageParams, radius));
    dvz_params_attr(params, DVZ_IMAGE_PARAMS_COLORMAP, FIELD(DvzImageParams, cmap));


    // Vertex shader specialization constants.

    // Size specialization constant.
    int size_ndc = (flags & DVZ_IMAGE_FLAGS_SIZE_NDC) > 0;
    dvz_visual_specialization(visual, DVZ_SHADER_VERTEX, 0, sizeof(int), &size_ndc);

    // Rescale specialization constant.
    int rescale = 0;
    if ((flags & DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO) > 0)
        rescale = 1;
    if ((flags & DVZ_IMAGE_FLAGS_RESCALE) > 0)
        rescale = 2;
    dvz_visual_specialization(visual, DVZ_SHADER_VERTEX, 1, sizeof(int), &rescale);


    // Fragment shader specialization constants.

    // Color mode: FILL, RGBA, or COLORMAP.
    // Fragment shader: 0 = fill color, 1 = RGBA texture, 2 = R texture with colormap
    int mode = 1; // default = RGBA because DVZ_IMAGE_FLAGS_MODE_RGBA = 0
    if ((flags & DVZ_IMAGE_FLAGS_MODE_FILL) > 0)
        mode = 0;
    else if ((flags & DVZ_IMAGE_FLAGS_MODE_COLORMAP) > 0)
        mode = 2;
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 0, sizeof(int), &mode);

    // Border specialization constant.
    int border = (flags & DVZ_IMAGE_FLAGS_BORDER) > 0;
    dvz_visual_specialization(visual, DVZ_SHADER_FRAGMENT, 1, sizeof(int), &border);


    // Visual draw callback.
    dvz_visual_callback(visual, _visual_callback);

    return visual;
}



void dvz_image_alloc(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    log_debug("allocating the image visual");

    DvzBatch* batch = visual->batch;
    ANN(batch);

    // Create the visual.
    dvz_visual_alloc(visual, item_count, 6 * item_count, 0);
}



void dvz_image_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 0, first, count, values);
}



void dvz_image_size(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 1, first, count, values);
}



void dvz_image_anchor(DvzVisual* visual, uint32_t first, uint32_t count, vec2* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 2, first, count, values);
}



void dvz_image_texcoords(DvzVisual* visual, uint32_t first, uint32_t count, vec4* tl_br, int flags)
{
    ANN(visual);
    dvz_visual_quads(visual, 3, first, count, tl_br);
}



void dvz_image_color(
    DvzVisual* visual, uint32_t first, uint32_t count, DvzColor* values, int flags)
{
    ANN(visual);
    dvz_visual_data(visual, 4, first, count, values);
}



void dvz_image_edgecolor(DvzVisual* visual, DvzColor color)
{
    ANN(visual);
    if (!(visual->flags & DVZ_IMAGE_FLAGS_BORDER))
    {
        log_warn(
            "The image visual must be created with the DVZ_IMAGE_FLAGS_BORDER flag if the "
            "edgecolor is set");
        return;
    }

#if DVZ_COLOR_CVEC4
    // NOTE: convert from cvec4 into vec4 as GLSL uniforms do not support cvec4 (?)
    float r = color[0] / 255.0;
    float g = color[1] / 255.0;
    float b = color[2] / 255.0;
    float a = color[3] / 255.0;

    dvz_visual_param(visual, 2, DVZ_IMAGE_PARAMS_EDGECOLOR, (vec4){r, g, b, a});
#else
    dvz_visual_param(visual, 2, DVZ_IMAGE_PARAMS_EDGECOLOR, color);
#endif
}



void dvz_image_linewidth(DvzVisual* visual, float width)
{
    ANN(visual);
    if (!(visual->flags & DVZ_IMAGE_FLAGS_BORDER))
    {
        log_warn(
            "The image visual must be created with the DVZ_IMAGE_FLAGS_BORDER flag if the "
            "linewidth is set");
        return;
    }
    dvz_visual_param(visual, 2, DVZ_IMAGE_PARAMS_LINEWIDTH, &width);
}



void dvz_image_radius(DvzVisual* visual, float radius)
{
    ANN(visual);
    dvz_visual_param(visual, 2, DVZ_IMAGE_PARAMS_RADIUS, &radius);
}



void dvz_image_colormap(DvzVisual* visual, DvzColormap cmap)
{
    ANN(visual);
    if (!(visual->flags & DVZ_IMAGE_FLAGS_MODE_COLORMAP))
    {
        log_warn(
            "The image visual must be created with the DVZ_IMAGE_FLAGS_MODE_COLORMAP flag if the "
            "colormap is set");
        return;
    }
    dvz_visual_param(visual, 2, DVZ_IMAGE_PARAMS_COLORMAP, &cmap);
}



void dvz_image_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode)
{
    ANN(visual);

    DvzBatch* batch = visual->batch;
    ANN(batch);

    DvzId sampler = dvz_create_sampler(batch, filter, address_mode).id;

    // Bind texture to the visual.
    dvz_visual_tex(visual, 3, tex, sampler, DVZ_ZERO_OFFSET);
}



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

DvzId dvz_tex_image(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, void* data, int flags)
{
    ANN(batch);
    ASSERT(width > 0);
    ASSERT(height > 0);

    uvec3 shape = {width, height, 1};
    DvzSize size = width * height * _format_size(format);
    DvzId tex = dvz_create_tex(batch, DVZ_TEX_2D, format, shape, flags).id;

    if (data != NULL)
        dvz_upload_tex(batch, tex, DVZ_ZERO_OFFSET, shape, size, data, 0);

    return tex;
}
