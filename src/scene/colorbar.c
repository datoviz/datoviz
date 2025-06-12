/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene/colorbar.h"
#include "datoviz.h"
#include "datoviz/scene/axis.h"
#include "datoviz/scene/ref.h"
#include "datoviz/scene/visual.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define FONT_SIZE  16
#define DIM        DVZ_DIM_Y
#define POS_X      0
#define POS_Y      0
#define WIDTH      1
#define HEIGHT     2
#define TEXWIDTH   1
#define TEXHEIGHT  256
#define XANCHOR    -1
#define YANCHOR    0
#define COLORMAP   DVZ_CMAP_HSV
#define GLYPH_SIZE FONT_SIZE



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzColor* create_colormap_data(uint32_t width, uint32_t height)
{
    ASSERT(width > 0);
    ASSERT(height > 0);
    // Typically vmin = 0 and vmax = 1.
    DvzColor* imgdata = (DvzColor*)calloc(width * height, sizeof(DvzColor));
    return imgdata;
}



static void update_colormap_data(
    DvzColormap cmap, uint32_t width, uint32_t height, double vmin, double vmax, DvzColor* imgdata)
{
    ASSERT(width > 0);
    ASSERT(height > 0);
    ANN(imgdata);

    for (uint32_t i = 0; i < height; i++)
    {
        for (uint32_t j = 0; j < width; j++)
        {
            ASSERT(width * i + j < width * height);
            dvz_colormap_scale(cmap, i / (float)(height - 1), vmin, vmax, imgdata[width * i + j]);
        }
    }
}



static DvzTexture*
create_colormap_texture(DvzBatch* batch, uint32_t width, uint32_t height, DvzColor* imgdata)
{
    ANN(batch);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ANN(imgdata);

    DvzTexture* texture = dvz_texture_2D(
        batch, DVZ_FORMAT_COLOR, DVZ_FILTER_NEAREST, DVZ_SAMPLER_ADDRESS_MODE_REPEAT, //
        width, height, imgdata, 0);
    return texture;
}



static DvzVisual* create_colormap_visual(DvzBatch* batch)
{
    ANN(batch);

    DvzVisual* image = dvz_image(batch, DVZ_IMAGE_FLAGS_SIZE_NDC | DVZ_IMAGE_FLAGS_MODE_RGBA);
    ANN(image);

    dvz_image_alloc(image, 1);
    dvz_image_texcoords(image, 0, 1, (vec4[]){{0, 0, +1, +1}}, 0);
    // dvz_visual_fixed(image, true, true, true);
    return image;
}



static void update_colorbar(DvzColorbar* colorbar)
{
    ANN(colorbar);
    ANN(colorbar->image);

    // Create the colormap.
    if (colorbar->imgdata == NULL)
    {
        colorbar->imgdata = create_colormap_data(TEXWIDTH, TEXHEIGHT);
    }
    update_colormap_data(colorbar->cmap, TEXWIDTH, TEXHEIGHT, 0, 1, colorbar->imgdata);


    // NOTE: at the moment, the texture size cannot be changed once created.
    if (colorbar->texture == NULL)
    {
        colorbar->texture =
            create_colormap_texture(colorbar->batch, TEXWIDTH, TEXHEIGHT, colorbar->imgdata);
    }
}



/*************************************************************************************************/
/*  Colorbar                                                                                     */
/*************************************************************************************************/

DvzColorbar* dvz_colorbar(DvzBatch* batch, int flags)
{
    ANN(batch);

    DvzColorbar* colorbar = (DvzColorbar*)calloc(1, sizeof(DvzColorbar));
    colorbar->flags = flags;
    colorbar->batch = batch;

    dvz_atlas_font(FONT_SIZE, &colorbar->af);

    double dmin = 0;
    double dmax = 1;

    colorbar->ref = dvz_ref(0);
    dvz_ref_set(colorbar->ref, DIM, dmin, dmax);

    colorbar->axis = dvz_axis(batch, &colorbar->af, DIM, 0);
    dvz_axis_extra(colorbar->axis, 0);
    dvz_axis_size(colorbar->axis, 600, GLYPH_SIZE);
    dvz_axis_anchor(colorbar->axis, (vec2){+1, 0});
    dvz_axis_offset(colorbar->axis, (vec2){-10, 0});
    dvz_axis_dir(colorbar->axis, (vec2){-1, 0});
    dvz_axis_pos(colorbar->axis, 0);
    dvz_visual_show(colorbar->axis->spine, false);

    dvz_axis_update(colorbar->axis, colorbar->ref, dmin, dmax);

    // Create the visual.
    if (colorbar->image == NULL)
    {
        colorbar->image = create_colormap_visual(colorbar->batch);
    }

    // Default values.
    dvz_colorbar_colormap(colorbar, COLORMAP);
    dvz_colorbar_position(colorbar, (vec2){POS_X, POS_Y});
    dvz_colorbar_size(colorbar, (vec2){WIDTH, HEIGHT});
    dvz_colorbar_anchor(colorbar, (vec2){XANCHOR, YANCHOR});
    dvz_colorbar_update(colorbar);
    dvz_image_texture(colorbar->image, colorbar->texture);

    return colorbar;
}



void dvz_colorbar_colormap(DvzColorbar* colorbar, DvzColormap cmap)
{
    ANN(colorbar);
    colorbar->cmap = cmap;
}



void dvz_colorbar_position(DvzColorbar* colorbar, vec2 position)
{
    ANN(colorbar);
    colorbar->position[0] = position[0];
    colorbar->position[1] = position[1];
    dvz_image_position(colorbar->image, 0, 1, (vec3[]){{position[0], position[1], 0}}, 0);
}



void dvz_colorbar_size(DvzColorbar* colorbar, vec2 size)
{
    ANN(colorbar);
    colorbar->size[0] = size[0];
    colorbar->size[1] = size[1];
    dvz_image_size(colorbar->image, 0, 1, (vec2[]){{colorbar->size[0], colorbar->size[1]}}, 0);
}



void dvz_colorbar_anchor(DvzColorbar* colorbar, vec2 anchor)
{
    ANN(colorbar);
    colorbar->anchor[0] = anchor[0];
    colorbar->anchor[1] = anchor[1];
    dvz_image_anchor(
        colorbar->image, 0, 1, (vec2[]){{colorbar->anchor[0], colorbar->anchor[1]}}, 0);
}



void dvz_colorbar_panel(DvzColorbar* colorbar, DvzPanel* panel)
{
    ANN(colorbar);
    ANN(panel);

    // Add the image visual to the panel.
    dvz_panel_visual(panel, colorbar->image, 0);

    // Add the axis visual.
    dvz_axis_panel(colorbar->axis, panel);
}



void dvz_colorbar_update(DvzColorbar* colorbar)
{
    ANN(colorbar);
    update_colorbar(colorbar);
}



void dvz_colorbar_destroy(DvzColorbar* colorbar)
{
    ANN(colorbar);

    if (colorbar->image != NULL)
    {
        dvz_visual_destroy(colorbar->image);
    }

    if (colorbar->texture != NULL)
    {
        dvz_texture_destroy(colorbar->texture);
    }

    if (colorbar->imgdata != NULL)
    {
        FREE(colorbar->imgdata);
    }

    dvz_axis_destroy(colorbar->axis);
    dvz_ref_destroy(colorbar->ref);
    dvz_font_destroy(colorbar->af.font);
    dvz_atlas_destroy(colorbar->af.atlas);

    FREE(colorbar);
}
