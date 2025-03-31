/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/axis.h"
#include "_cglm.h"
#include "_macros.h"
#include "datoviz.h"
#include "datoviz_types.h"
#include "scene/ref.h"
#include "scene/ticks.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// TODO: customizable
#define TICK_DENSITY             0.2
#define MINOR_TICKS_PER_INTERVAL 4

#define ANCHOR_H (vec2){+.5, 0}
#define OFFSET_H (vec2){0, -50}
#define DIR_H    (vec2){0, -1}
#define POS_H    -1

#define ANCHOR_V (vec2){+1, 0}
#define OFFSET_V (vec2){-50, -10}
#define DIR_V    (vec2){-1, 0}
#define POS_V    -1

#define WIDTH_LIM   2
#define WIDTH_GRID  1
#define WIDTH_MAJOR 2
#define WIDTH_MINOR 1

#define LENGTH_LIM   1
#define LENGTH_GRID  1
#define LENGTH_MAJOR 20
#define LENGTH_MINOR 10

#define LABEL_COLOR   (cvec4){0, 0, 0, 255}
#define LABEL_BGCOLOR (vec4){1, 1, 1, 1}

#define COLOR_GLYPH                                                                               \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }
#define COLOR_LIM                                                                                 \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }
#define COLOR_GRID                                                                                \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }
#define COLOR_MAJOR                                                                               \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }
#define COLOR_MINOR                                                                               \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// NOTE: to be called in axis.c instead?
// Free tick labels
static void free_tick_labels(uint32_t count, char** strings)
{
    if (!strings)
        return;

    for (uint32_t i = 0; i < count; i++)
    {
        FREE(strings[i]);
    }

    FREE(strings);
}



static inline uint32_t _minor_tick_count(uint32_t tick_count)
{
    return (tick_count - 1) * MINOR_TICKS_PER_INTERVAL;
}



/*************************************************************************************************/
/*  Segment helpers                                                                              */
/*************************************************************************************************/

static inline void set_segment_pos(DvzVisual* segment, uint32_t tick_count, vec3* positions)
{
    ANN(segment);
    ANN(positions);
    ASSERT(tick_count > 0);

    uint32_t n_major = tick_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Concatenation of major and minor ticks.
    vec3* pos = (vec3*)calloc(n_total, sizeof(vec3));
    memcpy(pos, positions, n_major * sizeof(vec3));

    // Generate the minor ticks.
    uint32_t major = 0;
    uint32_t minor = 0;
    vec3* target = &pos[n_major];

    float dx = (positions[1][0] - positions[0][0]) / (MINOR_TICKS_PER_INTERVAL + 1);
    float dy = (positions[1][1] - positions[0][1]) / (MINOR_TICKS_PER_INTERVAL + 1);
    float dz = (positions[1][2] - positions[0][2]) / (MINOR_TICKS_PER_INTERVAL + 1);

    for (uint32_t i = 0; i < n_minor; i++)
    {
        major = i / MINOR_TICKS_PER_INTERVAL;
        minor = i % MINOR_TICKS_PER_INTERVAL;

        target[i][0] = positions[major][0] + (minor + 1) * dx;
        target[i][1] = positions[major][1] + (minor + 1) * dy;
        target[i][2] = positions[major][2] + (minor + 1) * dz;
    }

    dvz_segment_position(segment, 0, n_total, pos, pos, 0);
    FREE(pos);
}



static inline void set_segment_color(
    DvzVisual* segment, uint32_t tick_count, DvzColor color_major, DvzColor color_minor)
{
    ANN(segment);
    ASSERT(tick_count > 0);

    uint32_t n_major = tick_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Colors of the major and minor ticks.
    DvzColor* colors = (DvzColor*)calloc(n_total, sizeof(DvzColor));
    for (uint32_t i = 0; i < n_major; i++)
    {
        memcpy(colors[i], color_major, sizeof(DvzColor));
    }

    for (uint32_t i = 0; i < n_minor; i++)
    {
        memcpy(colors[n_major + i], color_minor, sizeof(DvzColor));
    }

    dvz_segment_color(segment, 0, n_total, colors, 0);
    FREE(colors);
}



static inline void
set_segment_shift(DvzVisual* segment, uint32_t tick_count, vec2 tick_length, vec2 tick_dir)
{
    ANN(segment);
    ASSERT(tick_count > 0);

    uint32_t n_major = tick_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Tick length.
    float major_length = tick_length[2];
    float minor_length = tick_length[3];

    float a = tick_dir[0];
    float b = tick_dir[1];

    // Major and minor ticks.
    vec4* shift = (vec4*)calloc(n_total, sizeof(vec4));
    for (uint32_t i = 0; i < n_major; i++)
    {
        shift[i][2] = a * major_length;
        shift[i][3] = b * major_length;
    }

    for (uint32_t i = 0; i < n_minor; i++)
    {
        shift[n_major + i][2] = a * minor_length;
        shift[n_major + i][3] = b * minor_length;
    }

    // NOTE: this only works in 2D. In 3D, need to use end positions and shift=0.
    dvz_segment_shift(segment, 0, n_total, shift, 0);
    FREE(shift);
}



static inline void set_segment_width(DvzVisual* segment, uint32_t tick_count, vec2 tick_width)
{
    ANN(segment);
    ASSERT(tick_count > 0);

    uint32_t n_major = tick_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    // Widths of the major and minor ticks.
    float* width = (float*)calloc(n_total, sizeof(float));
    for (uint32_t i = 0; i < n_major; i++)
    {
        width[i] = tick_width[2]; // major
    }
    for (uint32_t i = 0; i < n_minor; i++)
    {
        width[n_major + i] = tick_width[3]; // minor
    }

    dvz_segment_linewidth(segment, 0, n_total, width, 0);
    FREE(width);
}



/*************************************************************************************************/
/*  Axis helpers                                                                                 */
/*************************************************************************************************/

static void axis_common_params(DvzAxis* axis)
{
    dvz_axis_width(axis, WIDTH_LIM, WIDTH_GRID, WIDTH_MAJOR, WIDTH_MINOR);
    dvz_axis_length(axis, LENGTH_LIM, LENGTH_GRID, LENGTH_MAJOR, LENGTH_MINOR);
    dvz_axis_color(axis, COLOR_GLYPH, COLOR_LIM, COLOR_GRID, COLOR_MAJOR, COLOR_MINOR);

    dvz_glyph_bgcolor(axis->glyph, LABEL_BGCOLOR);

    if (axis->factor != NULL)
    {
        dvz_visual_fixed(axis->factor, true, true, true);
    }
}



static void axis_horizontal_params(DvzAxis* axis)
{
    ANN(axis);

    dvz_axis_anchor(axis, ANCHOR_H);
    dvz_axis_offset(axis, OFFSET_H);
    dvz_axis_dir(axis, DIR_H);
    dvz_axis_pos(axis, POS_H);

    dvz_visual_fixed(axis->glyph, false, true, false);
    dvz_visual_fixed(axis->segment, false, true, false);

    dvz_visual_clip(axis->glyph, DVZ_VIEWPORT_CLIP_BOTTOM);
    dvz_visual_clip(axis->segment, DVZ_VIEWPORT_CLIP_BOTTOM);
}



static void axis_vertical_params(DvzAxis* axis)
{
    ANN(axis);

    dvz_axis_anchor(axis, ANCHOR_V);
    dvz_axis_offset(axis, OFFSET_V);
    dvz_axis_dir(axis, DIR_V);
    dvz_axis_pos(axis, POS_V);

    dvz_visual_fixed(axis->glyph, true, false, false);
    dvz_visual_fixed(axis->segment, true, false, false);

    dvz_visual_clip(axis->glyph, DVZ_VIEWPORT_CLIP_LEFT);
    dvz_visual_clip(axis->segment, DVZ_VIEWPORT_CLIP_LEFT);
}



static void compute_layout(
    // float viewport_size, // size of the INNER viewport in pixels along a given axis
    DvzAlign align, // alignment of the factor visual along a given axis
    float pos,      // position of the factor visual along a given axis
    // float offset,        // offset in pixels
    float* out_pos // out position in NDC
    // float* out_offset   // out offset in NDC
)
{
    ANN(out_pos);

    switch (align)
    {
    case DVZ_ALIGN_NONE:
        *out_pos = pos;
        // NOTE: no out_offset in this case.
        break;

    case DVZ_ALIGN_LOW:
        // ANN(out_offset);
        *out_pos = -1;
        // *out_offset = 2.0 * offset / viewport_size;
        break;

    case DVZ_ALIGN_MIDDLE:
        *out_pos = 0;
        // NOTE: no out_offset in this case.
        break;

    case DVZ_ALIGN_HIGH:
        // ANN(out_offset);
        *out_pos = +1;
        // *out_offset = -2.0 * offset / viewport_size;
        break;

    default:
        break;
    }
}



/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/

DvzAxis* dvz_axis(DvzVisual* glyph, DvzVisual* segment, DvzVisual* factor, DvzDim dim, int flags)
{
    ANN(glyph);
    ANN(segment);

    DvzAxis* axis = (DvzAxis*)calloc(1, sizeof(DvzAxis));
    axis->glyph = glyph;
    axis->segment = segment;
    axis->factor = factor; // used only with factored formats: offset and exponent
    axis->dim = dim;
    axis->flags = flags;

    axis->ticks = dvz_ticks(0);

    // HACK: add the visual with an empty string because empty visuals cannot be added to a panel
    // at the moment.
    dvz_glyph_strings(
        axis->factor, 1, (char*[]){" "}, (vec3[]){{0, 0, 0}}, LABEL_COLOR, (vec2){0}, (vec2){0});

    return axis;
}



void dvz_axis_ref(DvzAxis* axis, DvzRef* ref)
{
    ANN(axis);
    ANN(ref);

    // the axis needs the ref to do the normalization of the glyph and ticks, which are positioned
    // in [lmin, lmax] and then normalized to positions via DvzRef
    axis->ref = ref;
}



void dvz_axis_size(DvzAxis* axis, double range_size, double glyph_size)
{
    ANN(axis);
    dvz_ticks_size(axis->ticks, range_size, glyph_size);
}



void dvz_axis_glyph(DvzAxis* axis, uint32_t tick_count, char** labels, vec3* positions)
{
    ANN(axis);
    ANN(axis->glyph);
    ANN(axis->ref);

    dvz_glyph_strings(
        axis->glyph, tick_count, labels, positions, LABEL_COLOR, axis->spec.offset,
        axis->spec.anchor);
}



void dvz_axis_segment(DvzAxis* axis, uint32_t tick_count, vec3* positions)
{
    ANN(axis);

    DvzVisual* segment = axis->segment;
    ANN(segment);

    DvzAxisSpec* spec = &axis->spec;
    ANN(spec);

    uint32_t n_major = tick_count;
    uint32_t n_minor = _minor_tick_count(n_major);
    uint32_t n_total = n_major + n_minor;

    dvz_segment_alloc(segment, n_total);

    set_segment_pos(segment, tick_count, positions);
    set_segment_color(segment, tick_count, spec->color_major, spec->color_minor);
    set_segment_shift(segment, tick_count, spec->tick_length, spec->tick_dir);
    set_segment_width(segment, tick_count, spec->tick_width);
}



void dvz_axis_factor(DvzAxis* axis, int32_t exponent, double offset)
{
    ANN(axis);
    if (axis->factor == NULL)
    {
        log_trace("skip setting of axis factor as axis->factor visual is not set (NULL)");
        return;
    }
    const char* sign = offset > 0 ? "+" : "";
    const char* s1e = exponent != 0 ? "1e" : "";
    char label[64] = {0};
    if (exponent == 0)
    {
        sprintf(label, "%s%g", sign, offset);
    }
    else if (offset == 0)
    {
        sprintf(label, "1e%d %s", exponent, sign);
    }
    else
    {
        sprintf(label, "1e%d %s%g", exponent, sign, offset);
    }

    vec3 pos = {0};

    for (uint32_t i = 0; i < 2; i++)
    {
        compute_layout(
            axis->factor_layout.align[i], axis->factor_layout.pos[i], //
            &pos[i]);
    }

    vec2 anchor = {0};
    if (axis->dim == DVZ_DIM_X)
    {
        anchor[0] = 1;
        anchor[1] = 1;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        anchor[0] = -1;
        anchor[1] = -1;
    }
    dvz_glyph_strings(
        axis->factor, 1, (char*[]){label}, &pos, LABEL_COLOR, axis->factor_layout.offset, anchor);
}



bool dvz_axis_update(DvzAxis* axis, double dmin, double dmax)
{
    ANN(axis);
    if (dmin >= dmax)
    {
        log_error("dmin should be strictly lower than dmax");
        return false;
    }

    DvzTicks* ticks = axis->ticks;
    ANN(ticks);

    // req_count depends on the viewport size vs glyph size.
    uint32_t req_count = TICK_DENSITY * ticks->range_size / ticks->glyph_size;
    // log_debug("request %d ticks on [%g, %g]", req_count, dmin, dmax);
    ASSERT(req_count > 0);

    bool has_changed = dvz_ticks_compute(ticks, dmin, dmax, req_count);
    if (!has_changed)
        return false;

    double lmin = 0, lmax = 0, lstep = 0;
    dvz_ticks_range(ticks, &lmin, &lmax, &lstep);

    // NOTE: extend left and right for aesthetical purposes.
    // WARNING: extra should be a multiple of step
    double extra = 3 * lstep;
    lmin -= extra;
    lmax += extra;

    uint32_t tick_count = get_tick_count(lmin, lmax, lstep);
    if (tick_count < 2)
        return false;
    log_debug("found %d ticks", tick_count);

    double* tick_pos = (double*)calloc(tick_count, sizeof(double));
    ANN(tick_pos);

    char** labels = (char**)calloc(tick_count, sizeof(char*));
    ANN(labels);

    // Generate the tick positions and labels.
    dvz_ticks_linspace(&ticks->spec, tick_count, lmin, lmax, lstep, labels, tick_pos);

    // Normalize the tick positions using the reference frame.
    vec3* positions = calloc(tick_count, sizeof(vec3));
    ANN(positions);
    dvz_ref_transform1D(axis->ref, axis->dim, tick_count, tick_pos, positions);

    // Fixed dimension.
    uint32_t fixed_dim = 0;
    if (axis->dim == DVZ_DIM_X)
    {
        fixed_dim = 1;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        fixed_dim = 0;
    }
    for (uint32_t i = 0; i < tick_count; i++)
    {
        positions[i][fixed_dim] = axis->spec.pos;
    }

    // Now update the glyph visual.
    dvz_axis_glyph(axis, tick_count, labels, positions);

    // Update the segment visual.
    dvz_axis_segment(axis, tick_count, positions);

    // Show offset and exponent in factored formats.

    bool is_factored = _is_format_factored(&axis->ticks->spec);
    if (is_factored)
    {
        dvz_axis_factor(axis, ticks->spec.exponent, ticks->spec.offset);
    }
    if (axis->factor != NULL)
    {
        dvz_visual_show(axis->factor, is_factored);
    }

    // Free the labels array and all labels in it.
    free_tick_labels(tick_count, labels);

    FREE(positions);
    FREE(tick_pos);

    return true;
}



bool dvz_axis_onpanzoom(DvzAxis* axis, DvzPanzoom* pz)
{
    ANN(axis);
    ANN(pz);

    DvzTicks* ticks = axis->ticks;
    ANN(ticks);

    DvzRef* ref = axis->ref;
    ANN(ref);

    // Find the extent.
    DvzBox box = {0};
    dvz_panzoom_extent(pz, &box);
    dvec3 pos = {0};

    dvz_ref_inverse(ref, (vec3){box.xmin, 0, 0}, &pos);
    double xmin = pos[0];

    dvz_ref_inverse(ref, (vec3){box.xmax, 0, 0}, &pos);
    double xmax = pos[0];

    // If the extent is the same, do not recompute the ticks.
    if ((fabs(xmin - ticks->dmin) < 1e-12) && (fabs(xmax - ticks->dmax) < 1e-12))
    {
        return false;
    }

    // Otherwise, recompute the ticks and only update the axes if the ticks have changed.
    bool updated = dvz_axis_update(axis, xmin, xmax);

    return updated;
}



void dvz_axis_destroy(DvzAxis* axis)
{
    ANN(axis);
    FREE(axis);
}



/*************************************************************************************************/
/*  Axis spec                                                                                    */
/*************************************************************************************************/

void dvz_axis_pos(DvzAxis* axis, float pos)
{
    ANN(axis);
    // x0 for y axis, y0 for x axis
    axis->spec.pos = pos;
}



void dvz_axis_width(DvzAxis* axis, float lim, float grid, float major, float minor)
{
    ANN(axis);
    axis->spec.tick_width[0] = lim;
    axis->spec.tick_width[1] = grid;
    axis->spec.tick_width[2] = major;
    axis->spec.tick_width[3] = minor;
}



void dvz_axis_length(DvzAxis* axis, float lim, float grid, float major, float minor)
{
    ANN(axis);
    axis->spec.tick_length[0] = lim;
    axis->spec.tick_length[1] = grid;
    axis->spec.tick_length[2] = major;
    axis->spec.tick_length[3] = minor;
}



void dvz_axis_color(
    DvzAxis* axis, DvzColor glyph, DvzColor lim, DvzColor grid, DvzColor major, DvzColor minor)
{
    ANN(axis);
    memcpy(axis->spec.color_glyph, glyph, sizeof(DvzColor));
    memcpy(axis->spec.color_lim, lim, sizeof(DvzColor));
    memcpy(axis->spec.color_grid, grid, sizeof(DvzColor));
    memcpy(axis->spec.color_major, major, sizeof(DvzColor));
    memcpy(axis->spec.color_minor, minor, sizeof(DvzColor));
}



void dvz_axis_anchor(DvzAxis* axis, vec2 anchor)
{
    ANN(axis);
    glm_vec2_copy(anchor, axis->spec.anchor);
}



void dvz_axis_offset(DvzAxis* axis, vec2 offset)
{
    ANN(axis);
    glm_vec2_copy(offset, axis->spec.offset);
}



void dvz_axis_dir(DvzAxis* axis, vec2 tick_dir)
{
    ANN(axis);
    glm_vec2_copy(tick_dir, axis->spec.tick_dir);
}



void dvz_axis_factor_layout(DvzAxis* axis, DvzAlign align, float xoffset, float yoffset)
{
    ANN(axis);

    if (axis->dim == DVZ_DIM_X)
    {
        axis->factor_layout.align[0] = align;
        axis->factor_layout.align[1] = DVZ_ALIGN_LOW;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        axis->factor_layout.align[0] = DVZ_ALIGN_LOW;
        axis->factor_layout.align[1] = align;
    }

    axis->factor_layout.offset[0] = xoffset;
    axis->factor_layout.offset[1] = yoffset;
}



/*************************************************************************************************/
/*  Axis helpers                                                                                 */
/*************************************************************************************************/

void dvz_axis_horizontal(DvzAxis* axis, int flags)
{
    ANN(axis);

    axis_common_params(axis);
    axis_horizontal_params(axis);
}



void dvz_axis_vertical(DvzAxis* axis, int flags)
{
    ANN(axis);

    axis_common_params(axis);
    axis_vertical_params(axis);
}



/*************************************************************************************************/
/*  Axes                                                                                         */
/*************************************************************************************************/

DvzAxes* dvz_axes(DvzBatch* batch, int flags)
{
    ANN(batch);
    DvzAxes* axes = (DvzAxes*)calloc(1, sizeof(DvzAxes));

    // TODO: create glyph and segment visuals for each axis
    // TODO: flags to create axes only on 1, 2, or 3 dimensions
    // axes->axis_xyz[0] = dvz_axis(glyph_x, segment_x, DVZ_DIM_X, flags);
    // axes->axis_xyz[1] = dvz_axis(glyph_y, segment_y, DVZ_DIM_Y, flags);
    // axes->axis_xyz[2] = dvz_axis(glyph_z, segment_z, DVZ_DIM_Z, flags);

    return axes;
}



DvzAxis* dvz_axes_get(DvzAxes* axes, DvzDim dim)
{
    ANN(axes);
    ASSERT((uint32_t)dim <= 2);
    return axes->axis_xyz[(uint32_t)dim];
}



void dvz_axes_destroy(DvzAxes* axes)
{
    ANN(axes);

    // Destroy the axes that were created by dvz_axes().
    for (uint32_t i = 0; i < DVZ_DIM_COUNT; i++)
    {
        if (axes->axis_xyz[i])
        {
            dvz_axis_destroy(axes->axis_xyz[i]);

            // Destroy the glyph and segment visuals.
            // TODO
        }
    }
    FREE(axes);
}



/*************************************************************************************************/
/*  Panel axes                                                                                   */
/*************************************************************************************************/

DvzAxes* dvz_panel_axes2D(DvzPanel* panel, int flags)
{
    ANN(panel);
    // TODO
    // TODO: margins
    /*
    create DvzAxes*

    register a frame callback (or define panel callback, called whenever something changes in the
    panel)

    if panel has not changed, do nothing

        for axis in x, y:

            get new bound box in data coords (dmin, dmax)

            dvz_axis_update(dmin, dmax) // only update if tick spec has changed

    */
    return NULL;
}
