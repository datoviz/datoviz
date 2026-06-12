/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example visual style helpers                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_style.h"

#include "_assertions.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define GRAPHITE_CYAN_FRAME_BG_R 14u
#define GRAPHITE_CYAN_FRAME_BG_G 17u
#define GRAPHITE_CYAN_FRAME_BG_B 23u
#define GRAPHITE_CYAN_PANEL_BG_R 22u
#define GRAPHITE_CYAN_PANEL_BG_G 27u
#define GRAPHITE_CYAN_PANEL_BG_B 34u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Copy a color token into one RGBA array with an optional alpha override.
 *
 * @param out destination RGBA8 color
 * @param color source color
 * @param alpha alpha override
 */
static void _style_copy_color(uint8_t out[4], DvzColor color, uint8_t alpha)
{
    ANN(out);
    out[0] = color.r;
    out[1] = color.g;
    out[2] = color.b;
    out[3] = alpha;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return one color token from the graphite-cyan example palette.
 *
 * @param role color token role
 * @return RGBA8 color
 */
DvzColor example_graphite_cyan_color(ExampleStyleColorRole role)
{
    switch (role)
    {
    case EXAMPLE_STYLE_COLOR_FRAME_BG:
        return dvz_color_rgb(
            GRAPHITE_CYAN_FRAME_BG_R, GRAPHITE_CYAN_FRAME_BG_G, GRAPHITE_CYAN_FRAME_BG_B);
    case EXAMPLE_STYLE_COLOR_PANEL_BG:
        return dvz_color_rgb(
            GRAPHITE_CYAN_PANEL_BG_R, GRAPHITE_CYAN_PANEL_BG_G, GRAPHITE_CYAN_PANEL_BG_B);
    case EXAMPLE_STYLE_COLOR_GRID:
        return dvz_color_rgba(48, 54, 61, 160);
    case EXAMPLE_STYLE_COLOR_TEXT:
        return dvz_color_rgb(201, 209, 217);
    case EXAMPLE_STYLE_COLOR_MINOR_TICK:
        return dvz_color_rgba(140, 151, 165, 220);
    case EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY:
        return dvz_color_rgb(76, 201, 240);
    case EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY:
        return dvz_color_rgb(128, 255, 219);
    case EXAMPLE_STYLE_COLOR_WARNING:
        return dvz_color_rgb(255, 183, 3);
    case EXAMPLE_STYLE_COLOR_ERROR:
        return dvz_color_rgb(239, 71, 111);
    default:
        return dvz_color_rgb(255, 255, 255);
    }
}



/**
 * Create the shared graphite-cyan example colormap.
 *
 * @param scene target scene
 * @return scene-owned colormap, or NULL on error
 */
DvzColormap* example_graphite_cyan_colormap(DvzScene* scene)
{
    ANN(scene);
    DvzColor colors[] = {
        dvz_color_rgb(29, 43, 54),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY),
        example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_WARNING),
    };
    return dvz_colormap_custom(scene, "graphite_cyan", colors, DVZ_ARRAY_COUNT(colors));
}



/**
 * Create a continuous color scale using the shared graphite-cyan colormap.
 *
 * @param scene target scene
 * @param min scalar domain minimum
 * @param max scalar domain maximum
 * @return scene-owned scale, or NULL on error
 */
DvzScale* example_graphite_cyan_color_scale(DvzScene* scene, double min, double max)
{
    ANN(scene);
    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "value",
               });
    if (scale == NULL)
        return NULL;
    DvzColormap* colormap = example_graphite_cyan_colormap(scene);
    if (colormap == NULL)
        return NULL;
    dvz_scale_set_domain(scale, min, max);
    dvz_scale_set_colormap(scale, colormap);
    return scale;
}


/**
 * Return one semantic text style from the graphite-cyan example theme.
 *
 * @param role semantic text role
 * @return text style descriptor
 */
DvzTextStyle example_graphite_cyan_text_style(ExampleStyleTextRole role)
{
    DvzTextStyle style = dvz_text_style();
    style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    style.color[0] = 222u;
    style.color[1] = 236u;
    style.color[2] = 244u;
    style.color[3] = 245u;

    switch (role)
    {
    case EXAMPLE_STYLE_TEXT_TITLE:
        style.size_px = 21.0f;
        break;
    case EXAMPLE_STYLE_TEXT_PANEL_LABEL:
        style.size_px = 15.0f;
        break;
    case EXAMPLE_STYLE_TEXT_LEGEND_LABEL:
        style.size_px = 26.0f;
        style.color[3] = 235u;
        break;
    case EXAMPLE_STYLE_TEXT_DATA_LABEL:
        style.size_px = 20.0f;
        break;
    case EXAMPLE_STYLE_TEXT_ANNOTATION:
        style.size_px = 17.0f;
        break;
    case EXAMPLE_STYLE_TEXT_SMALL:
        style.size_px = 12.0f;
        style.color[3] = 220u;
        break;
    default:
        style.size_px = 15.0f;
        break;
    }
    return style;
}



/**
 * Return the graphite-cyan panel background color as normalized RGBA.
 *
 * @param out_rgba output normalized RGBA color
 */
void example_graphite_cyan_panel_background(float out_rgba[4])
{
    ANN(out_rgba);
    out_rgba[0] = (float)GRAPHITE_CYAN_PANEL_BG_R / 255.0f;
    out_rgba[1] = (float)GRAPHITE_CYAN_PANEL_BG_G / 255.0f;
    out_rgba[2] = (float)GRAPHITE_CYAN_PANEL_BG_B / 255.0f;
    out_rgba[3] = 1.0f;
}



/**
 * Apply the graphite-cyan panel background to one panel.
 *
 * @param panel target panel
 */
void example_graphite_cyan_set_panel_background(DvzPanel* panel)
{
    ANN(panel);
    float bg[4] = {0};
    example_graphite_cyan_panel_background(bg);
    dvz_panel_set_background_color(panel, bg[0], bg[1], bg[2], bg[3]);
}



/**
 * Return the default graphite-cyan axis style options for feature examples.
 *
 * @return axis style options
 */
ExampleAxisStyleOptions example_graphite_cyan_axis_options(void)
{
    return (ExampleAxisStyleOptions){
        .tick_size_px = 13.0f,
        .label_size_px = 16.0f,
        .tick_gap_px = 8.0f,
        .x_label_gap_px = 38.0f,
        .y_label_gap_px = 58.0f,
        .minor_tick_alpha = 220,
        .grid_alpha = 160,
    };
}



/**
 * Return a graphite-cyan axis style.
 *
 * @param vertical whether this is the vertical axis
 * @param options optional axis style options
 * @return axis style descriptor
 */
DvzAxisStyle example_graphite_cyan_axis_style(
    bool vertical, const ExampleAxisStyleOptions* options)
{
    ExampleAxisStyleOptions resolved =
        options != NULL ? *options : example_graphite_cyan_axis_options();

    DvzAxisStyle style = dvz_axis_style();
    style.spine_width = 1.5f;
    style.major_tick_width = 1.5f;
    style.minor_tick_width = 1.0f;
    style.grid_width = 1.0f;
    style.tick_size_px = resolved.tick_size_px;
    style.label_size_px = resolved.label_size_px;
    style.tick_gap_px = resolved.tick_gap_px;
    style.label_gap_px = vertical ? resolved.y_label_gap_px : resolved.x_label_gap_px;
    _style_copy_color(
        style.spine_color, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT), 255);
    _style_copy_color(
        style.major_tick_color, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT), 255);
    _style_copy_color(
        style.minor_tick_color, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK),
        resolved.minor_tick_alpha);
    _style_copy_color(
        style.grid_color, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID),
        resolved.grid_alpha);
    style.show_grid = true;
    return style;
}



/**
 * Apply a graphite-cyan axis style to one axis.
 *
 * @param axis target axis
 * @param vertical whether this is the vertical axis
 * @param options optional axis style options
 * @return true when the style update succeeds
 */
bool example_graphite_cyan_apply_axis_style(
    DvzAxis* axis, bool vertical, const ExampleAxisStyleOptions* options)
{
    ANN(axis);
    DvzAxisStyle style = example_graphite_cyan_axis_style(vertical, options);
    return dvz_axis_set_style(axis, &style);
}
