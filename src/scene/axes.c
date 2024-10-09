/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/axes.h"
#include "_cglm.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_types.h"
#include "scene/axis.h"
#include "scene/labels.h"
#include "scene/scene.h"
#include "scene/ticks.h"
#include "scene/transform.h"
#include "scene/viewset.h"
#include "scene/visuals.h"
#include "scene/visuals/glyph.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// TODO: customizable parameters
#define DVZ_AXES_FONT_SIZE          24
#define DVZ_AXES_DEFAULT_TICK_COUNT 8



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void axis_common_params(DvzAxis* axis)
{
    float font_size = DVZ_AXES_FONT_SIZE;

    DvzColor color_glyph = {0, 0, 0, DVZ_ALPHA_MAX};
    DvzColor color_lim = {0, 0, 0, DVZ_ALPHA_MAX};
    DvzColor color_grid = {0, 0, 0, DVZ_ALPHA_MAX};
    DvzColor color_major = {0, 0, 0, DVZ_ALPHA_MAX};
    DvzColor color_minor = {0, 0, 0, DVZ_ALPHA_MAX};

    float width_lim = 4;
    float width_grid = 2;
    float width_major = 4;
    float width_minor = 2;

    float length_lim = 1;
    float length_grid = 1;
    float length_major = 40;
    float length_minor = 20;

    dvz_axis_size(axis, font_size);
    dvz_axis_width(axis, width_lim, width_grid, width_major, width_minor);
    dvz_axis_length(axis, length_lim, length_grid, length_major, length_minor);
    dvz_axis_color(axis, color_glyph, color_lim, color_grid, color_major, color_minor);

    dvz_glyph_bgcolor(axis->glyph, (vec4){1, 1, 1, 1});
}

static void axis_horizontal_params(DvzAxis* axis)
{
    ANN(axis);

    vec2 hanchor = {+.5, 0};
    vec2 hoffset = {0, -80};

    dvz_axis_anchor(axis, hanchor);
    dvz_axis_offset(axis, hoffset);

    dvz_visual_fixed(axis->glyph, false, true, false);
    dvz_visual_fixed(axis->segment, false, true, false);

    dvz_visual_clip(axis->glyph, DVZ_VIEWPORT_CLIP_BOTTOM);
    dvz_visual_clip(axis->segment, DVZ_VIEWPORT_CLIP_BOTTOM);
}

static void axis_horizontal_pos(DvzAxis* axis, float x0, float x1, vec3 p0, vec3 p1, vec3 vector)
{
    ANN(axis);
    float a = 1.0;
    glm_vec3_copy((vec3){x0, -a, 0}, p0);
    glm_vec3_copy((vec3){x1, -a, 0}, p1);
    glm_vec3_copy((vec3){0, 1, 0}, vector);
}

static void axis_vertical_params(DvzAxis* axis)
{
    ANN(axis);

    vec2 vanchor = {+1, 0};
    vec2 voffset = {-50, -10};

    dvz_axis_anchor(axis, vanchor);
    dvz_axis_offset(axis, voffset);

    dvz_visual_fixed(axis->glyph, true, false, false);
    dvz_visual_fixed(axis->segment, true, false, false);

    dvz_visual_clip(axis->glyph, DVZ_VIEWPORT_CLIP_LEFT);
    dvz_visual_clip(axis->segment, DVZ_VIEWPORT_CLIP_LEFT);
}

static void axis_vertical_pos(DvzAxis* axis, float y0, float y1, vec3 p0, vec3 p1, vec3 vector)
{
    ANN(axis);
    float a = 1.0;
    glm_vec3_copy((vec3){-a, y0, 0}, p0);
    glm_vec3_copy((vec3){-a, y1, 0}, p1);
    glm_vec3_copy((vec3){1, 0, 0}, vector);
}

// NOTE: the caller must FREE the output.
static char* concatenate_strings(
    uint32_t glyph_count, uint32_t tick_count, char* string_labels, uint32_t* index)
{
    // Replace the concatenation of null-terminated strings by space-terminated strings,
    // to call freetype only once.
    uint32_t buffer_size = glyph_count + tick_count; // all glyphs + 1 space per glyph
    char* glyphs = (char*)calloc(buffer_size, sizeof(char));
    memcpy(glyphs, string_labels, buffer_size);
    // Replace the null termination by a space, except for the last group.
    uint32_t idx = 0;
    for (uint32_t i = 1; i < tick_count; i++)
    {
        idx = index[i];
        ASSERT(idx >= 1);
        idx--;
        // This character should be a null termination.
        ASSERT(glyphs[idx] == 0);
        // We replace it by a space.
        glyphs[idx] = ' ';
    }
    return glyphs;
}

static bool
compute_ticks(DvzAxes* axes, DvzTicksFlags which, double dmin, double dmax, float vmin, float vmax)
{
    ANN(axes);

    bool horizontal = which == DVZ_TICKS_FLAGS_HORIZONTAL;

    log_error("compute axis %d: %f %f, %f %f", !horizontal, dmin, dmax, vmin, vmax);

    DvzAxis* axis = horizontal ? axes->xaxis : axes->yaxis;
    DvzTicks* ticks = horizontal ? axes->xticks : axes->yticks;
    DvzLabels* labels = horizontal ? axes->xlabels : axes->ylabels;

    ANN(labels);
    ANN(axis);

    // Specify the axis positions.
    vec3 p0, p1, vector;
    if (horizontal)
        axis_horizontal_pos(axis, vmin, vmax, p0, p1, vector);
    else
        axis_vertical_pos(axis, vmin, vmax, p0, p1, vector);

    // TODO: dependent on viewport size?
    uint32_t requested_count = DVZ_AXES_DEFAULT_TICK_COUNT;
    // return axis_labels(axis, ticks, labels, p0, p1, vector, dmin, dmax, requested_count);

    double lmin = 0, lmax = 0, lstep = 0;

    // Calculate the tick positions.
    bool has_changed = dvz_ticks_compute(ticks, dmin, dmax, requested_count);

    // Skip if the ticks have not changed.
    if (!has_changed)
        return false;

    log_info("ticks have changed, updating tick visual");

    // Get the calculated number of ticks and lmin, lmax, lstep.
    uint32_t tick_count = dvz_ticks_range(ticks, &lmin, &lmax, &lstep);

    // Get the calculated format.
    // DvzTicksFormat format = dvz_ticks_format(ticks); // NOTE: unused for now

    // NOTE: disabled for now.
    uint32_t precision = 2;
    int32_t exponent = 0;
    double offset = 0.0;

    // Generate the tick labels.
    uint32_t glyph_count = dvz_labels_generate(
        labels, DVZ_TICKS_FORMAT_DECIMAL, precision, exponent, offset, lmin, lmax, lstep);

    // Obtain the arrays generated by the labels.
    char* string_labels = dvz_labels_string(labels);
    uint32_t* index = dvz_labels_index(labels);
    uint32_t* length = dvz_labels_length(labels);
    double* values = dvz_labels_values(labels);

    // Replace the concatenation of null-terminated strings by space-terminated strings,
    // to call freetype only once.
    char* glyphs = concatenate_strings(glyph_count, tick_count, string_labels, index);

    {
        // char* exponent = dvz_labels_exponent(labels); // NOTE: unused for now
        // char* offset = dvz_labels_offset(labels); // NOTE: unused for now

        // // DEBUG.
        // dvz_labels_print(labels);
        // printf("glyphs:\n");
        // for (uint32_t i = 0; i < buffer_size; i++)
        // {
        //     printf("%d %c | ", glyphs[i], glyphs[i]);
        // }
        // printf("\nindex:\n");
        // for (uint32_t i = 0; i < tick_count; i++)
        // {
        //     printf("%d ", index[i]);
        // }
        // printf("\nlength %d:\n", tick_count);
        // for (uint32_t i = 0; i < tick_count; i++)
        // {
        //     printf("%d ", length[i]);
        // }
        // printf("\nvalues:\n");
        // for (uint32_t i = 0; i < tick_count; i++)
        // {
        //     printf("%f ", values[i]);
        // }
    }

    DvzTickSpec spec = dvz_tick_spec(
        p0, p1, vector, dmin, dmax, tick_count, values, glyph_count, glyphs, index, length);
    dvz_axis_ticks(axis, &spec);

    FREE(glyphs);

    return true;
}



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

DvzAxes* dvz_axes(DvzPanel* panel, int flags)
{
    ANN(panel);
    ANN(panel->figure);
    ANN(panel->figure->scene);

    DvzBatch* batch = panel->figure->scene->batch;
    ANN(batch);

    DvzAxes* axes = (DvzAxes*)calloc(1, sizeof(DvzAxes));
    axes->panel = panel;
    axes->flags = flags;

    // Axis visuals.
    axes->xaxis = dvz_axis(batch, flags); // NOTE: axes flags passed to axis visual flags
    axes->yaxis = dvz_axis(batch, flags);

    // Ticks.
    axes->xticks = dvz_ticks(0);
    axes->yticks = dvz_ticks(0);

    // Labels.
    axes->xlabels = dvz_labels();
    axes->ylabels = dvz_labels();

    // Set the viewport size.
    dvz_axes_resize(axes);

    // Common parameters.
    axis_common_params(axes->xaxis);
    axis_common_params(axes->yaxis);

    // Axis-specific parameters.
    axis_horizontal_params(axes->xaxis);
    axis_vertical_params(axes->yaxis);

    // Initial.
    compute_ticks(axes, DVZ_TICKS_FLAGS_HORIZONTAL, -1, 1, -1, 1);
    compute_ticks(axes, DVZ_TICKS_FLAGS_VERTICAL, -1, 1, -1, 1);

    // TODO: margins.
    dvz_panel_margins(panel, 20, 20, 120, 120);
    dvz_axis_panel(axes->xaxis, panel);
    dvz_axis_panel(axes->yaxis, panel);

    return axes;
}



void dvz_axes_xref(DvzAxes* axes, dvec2 range)
{
    ANN(axes);
    // TODO
    // set the reference range corresponding to MVP id
}



void dvz_axes_yref(DvzAxes* axes, dvec2 range)
{
    ANN(axes);
    // TODO
    // set the reference range corresponding to MVP id
}



void dvz_axes_xget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc)
{
    ANN(axes);
    ANN(axes->panel);

    DvzMVP* mvp = dvz_transform_mvp(axes->panel->transform);
    dvz_axis_mvp(axes->xaxis, mvp, range_data, range_ndc);
}



void dvz_axes_yget(DvzAxes* axes, dvec2 range_data, vec2 range_ndc)
{
    ANN(axes);
    ANN(axes->panel);

    DvzMVP* mvp = dvz_transform_mvp(axes->panel->transform);
    dvz_axis_mvp(axes->yaxis, mvp, range_data, range_ndc);
}



bool dvz_axes_xset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc)
{
    // TODO: set the MVP so that the visible range is the one specified, given the ref
    ANN(axes);
    return compute_ticks(
        axes, DVZ_TICKS_FLAGS_HORIZONTAL, //
        range_data[0], range_data[1], range_ndc[0], range_ndc[1]);
}



bool dvz_axes_yset(DvzAxes* axes, dvec2 range_data, vec2 range_ndc)
{
    // TODO: set the MVP so that the visible range is the one specified, given the ref
    ANN(axes);
    return compute_ticks(
        axes, DVZ_TICKS_FLAGS_VERTICAL, //
        range_data[0], range_data[1], range_ndc[0], range_ndc[1]);
}



void dvz_axes_resize(DvzAxes* axes)
{
    // Size is given in framebuffer pixels.

    ANN(axes);

    DvzPanel* panel = axes->panel;
    ANN(panel);

    DvzView* view = panel->view;
    ANN(view);

    dvz_ticks_size(axes->xticks, view->shape[0], DVZ_AXES_FONT_SIZE);
    dvz_ticks_size(axes->yticks, view->shape[1], DVZ_AXES_FONT_SIZE);
}



void dvz_axes_update(DvzAxes* axes)
{
    ANN(axes);
    log_error("calling axes update()");

    // Compute the currently visible range.
    dvec2 xrange = {0};
    dvec2 yrange = {0};

    vec2 xrange_ndc = {0};
    vec2 yrange_ndc = {0};

    dvz_axes_xget(axes, xrange, xrange_ndc);
    dvz_axes_yget(axes, yrange, yrange_ndc);

    // Use the current viewport size in the axes before the computation of the ticks.
    dvz_axes_resize(axes);

    // Compute the ticks and update the visuals if the ticks have changed.
    bool xupdate = dvz_axes_xset(axes, xrange, xrange_ndc);
    bool yupdate = dvz_axes_xset(axes, yrange, yrange_ndc);
    if (!xupdate && !yupdate)
        return;

    log_error("axes update required!");
    dvz_axis_update(axes->xaxis);
    dvz_axis_update(axes->yaxis);

    dvz_atomic_set(axes->panel->figure->viewset->status, (int)DVZ_BUILD_DIRTY);
}



void dvz_axes_destroy(DvzAxes* axes)
{
    ANN(axes);

    if (axes->xaxis)
        dvz_axis_destroy(axes->xaxis);
    if (axes->yaxis)
        dvz_axis_destroy(axes->yaxis);

    if (axes->xticks)
        dvz_ticks_destroy(axes->xticks);
    if (axes->yticks)
        dvz_ticks_destroy(axes->yticks);

    if (axes->xlabels)
        dvz_labels_destroy(axes->xlabels);
    if (axes->ylabels)
        dvz_labels_destroy(axes->ylabels);

    FREE(axes);
}
