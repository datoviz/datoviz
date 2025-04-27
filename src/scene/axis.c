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
#include "scene/visual.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// TODO: customizable
#define TICK_DENSITY             0.2
#define MINOR_TICKS_PER_INTERVAL 4

#define ANCHOR_H        (vec2){+.5, 0}
#define OFFSET_H        (vec2){0, -35}
#define DIR_H           (vec2){0, -1}
#define POS_H           -1
#define FACTOR_OFFSET_H 70, -70
#define LABEL_OFFSET_H  0, -70

#define ANCHOR_V        (vec2){+1, 0}
#define OFFSET_V        (vec2){-50, -10}
#define DIR_V           (vec2){-1, 0}
#define POS_V           -1
#define FACTOR_OFFSET_V 20, +70
#define LABEL_OFFSET_V  0, 0

#define WIDTH_LIM   2
#define WIDTH_GRID  1
#define WIDTH_MAJOR 2
#define WIDTH_MINOR 1

#define LENGTH_LIM   1
#define LENGTH_GRID  1
#define LENGTH_MAJOR 16
#define LENGTH_MINOR 10

#define LABEL_COLOR   (DvzColor){0, 0, 0, 255}
#define LABEL_BGCOLOR (DvzColor){255, 255, 255, 255}

#define COLOR_GLYPH                                                                               \
    (DvzColor) { 0, 0, 0, DVZ_ALPHA_MAX }
#define COLOR_LIM {0, 0, 0, DVZ_ALPHA_MAX}
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
    dvz_axis_color(axis, COLOR_GLYPH, (cvec4)COLOR_LIM, COLOR_GRID, COLOR_MAJOR, COLOR_MINOR);

    dvz_glyph_bgcolor(axis->glyph, LABEL_BGCOLOR);

    if (axis->factor != NULL)
    {
        dvz_visual_fixed(axis->factor, true, true, true);
    }
    if (axis->label != NULL)
    {
        dvz_visual_fixed(axis->label, true, true, true);
    }
}



static void axis_horizontal_params(DvzAxis* axis)
{
    ANN(axis);

    dvz_axis_anchor(axis, ANCHOR_H);
    dvz_axis_offset(axis, OFFSET_H);
    dvz_axis_dir(axis, DIR_H);
    dvz_axis_pos(axis, POS_H);
    dvz_axis_factor_layout(axis, DVZ_ALIGN_HIGH, FACTOR_OFFSET_H);
    dvz_axis_label_layout(axis, DVZ_ALIGN_MIDDLE, LABEL_OFFSET_H);

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
    dvz_axis_factor_layout(axis, DVZ_ALIGN_HIGH, FACTOR_OFFSET_V);
    dvz_axis_label_layout(axis, DVZ_ALIGN_HIGH, LABEL_OFFSET_V);

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



static void create_spine(DvzVisual* spine, DvzDim dim)
{
    ANN(spine);
    // Set spine.
    dvz_segment_alloc(spine, 1);
    vec3 start = {0};
    vec3 end = {0};
    if (dim == DVZ_DIM_X)
    {
        start[0] = -1;
        start[1] = -1;
        end[0] = +2;
        end[1] = -1;
    }
    else if (dim == DVZ_DIM_Y)
    {
        start[0] = -1;
        start[1] = -1;
        end[0] = -1;
        end[1] = +2;
    }
    dvz_segment_position(spine, 0, 1, &start, &end, 0);
    dvz_segment_color(spine, 0, 1, (DvzColor[]){{0, 0, 0, 255}}, 0);
    dvz_segment_linewidth(spine, 0, 1, (float[]){1}, 0);
    dvz_visual_fixed(spine, true, true, true);
}



/*************************************************************************************************/
/*  Axis                                                                                         */
/*************************************************************************************************/

DvzAxis* dvz_axis(DvzBatch* batch, DvzAtlasFont* af, DvzDim dim, int flags)
{
    ANN(batch);

    DvzAxis* axis = (DvzAxis*)calloc(1, sizeof(DvzAxis));
    axis->batch = batch;
    axis->dim = dim;
    axis->flags = flags;

    // Create the glyph visual.
    axis->glyph = dvz_glyph(batch, 0);
    dvz_glyph_atlas_font(axis->glyph, af);

    // Create the segment visual.
    axis->segment = dvz_segment(batch, 0);

    // Create the glyph visual for the exponent and offset (factorized formats only).
    axis->factor = dvz_glyph(batch, 0);
    dvz_glyph_atlas_font(axis->factor, af);

    // Create the label visual.
    axis->label = dvz_glyph(batch, 0);
    dvz_glyph_atlas_font(axis->label, af);

    // Create the spine visual.
    axis->spine = dvz_segment(batch, 0);
    create_spine(axis->spine, dim);


    // Create the ticks.
    axis->ticks = dvz_ticks(0);

    // HACK: add the visual with an empty string because empty visuals cannot be added to a panel
    // at the moment.
    dvz_glyph_strings(
        axis->factor, 1, (char*[]){" "}, (vec3[]){{0, 0, 0}}, NULL, LABEL_COLOR, (vec2){0},
        (vec2){0});
    dvz_glyph_strings(
        axis->label, 1, (char*[]){" "}, (vec3[]){{0, 0, 0}}, NULL, LABEL_COLOR, (vec2){0},
        (vec2){0});

    return axis;
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

    dvz_glyph_strings(
        axis->glyph, tick_count, labels, positions, NULL, LABEL_COLOR, axis->spec.offset,
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
        sprintf(label, "1e%d  %s", exponent, sign);
    }
    else
    {
        sprintf(label, "1e%d  %s%g", exponent, sign, offset);
    }

    vec3 pos = {0};

    for (uint32_t i = 0; i < 2; i++)
    {
        compute_layout(axis->factor_layout.align[i], axis->factor_layout.pos[i], &pos[i]);
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
        anchor[1] = +1;
    }

    dvz_glyph_strings(
        axis->factor, 1, (char*[]){label}, &pos, NULL, LABEL_COLOR, axis->factor_layout.offset,
        anchor);
}



void dvz_axis_label(DvzAxis* axis, char* text, float margin, DvzOrientation orientation)
{
    ANN(axis);
    if (axis->factor == NULL)
    {
        log_trace("skip setting of axis label as axis->label visual is not set (NULL)");
        return;
    }

    vec3 pos = {0};
    for (uint32_t i = 0; i < 2; i++)
    {
        compute_layout(axis->label_layout.align[i], axis->label_layout.pos[i], &pos[i]);
    }

    vec2 anchor = {0, 1};
    dvz_glyph_strings(
        axis->label, 1, (char*[]){text}, &pos, NULL, LABEL_COLOR, axis->label_layout.offset,
        anchor);
}



bool dvz_axis_update(DvzAxis* axis, DvzRef* ref, double dmin, double dmax)
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
    ASSERT(ticks->glyph_size > 0);
    uint32_t req_count = TICK_DENSITY * ticks->range_size / ticks->glyph_size;
    // log_debug("request %d ticks on [%g, %g]", req_count, dmin, dmax);
    if (req_count == 0)
    {
        log_trace(
            "requesting 0 ticks on range [%g, %g] (range size %g, glyph size %g), skipping "
            "dvz_axis_update()",
            dmin, dmax, ticks->range_size, ticks->glyph_size);
        return false;
    }
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
    dvz_ref_transform_1D(ref, axis->dim, tick_count, tick_pos, positions);

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



bool dvz_axis_on_panzoom(DvzAxis* axis, DvzPanzoom* pz, DvzRef* ref)
{
    ANN(axis);
    ANN(pz);
    ANN(ref);

    DvzTicks* ticks = axis->ticks;
    ANN(ticks);

    // Find the extent.
    DvzBox box = {0};
    dvz_panzoom_extent(pz, &box);
    dvec3 pos = {0};

    // Minimum value.
    vec3 pos_tr = {0};
    if (axis->dim == DVZ_DIM_X)
    {
        pos_tr[0] = box.xmin;
        pos_tr[1] = 0;
        pos_tr[2] = 0;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        pos_tr[0] = 0;
        pos_tr[1] = box.ymin;
        pos_tr[2] = 0;
    }
    dvz_ref_inverse(ref, pos_tr, &pos);
    double vmin = pos[axis->dim];


    // Maximum value.
    if (axis->dim == DVZ_DIM_X)
    {
        pos_tr[0] = box.xmax;
        pos_tr[1] = 0;
        pos_tr[2] = 0;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        pos_tr[0] = 0;
        pos_tr[1] = box.ymax;
        pos_tr[2] = 0;
    }
    dvz_ref_inverse(ref, pos_tr, &pos);
    double vmax = pos[axis->dim];


    // If the extent is the same, do not recompute the ticks.
    if ((fabs(vmin - ticks->dmin) < 1e-12) && (fabs(vmax - ticks->dmax) < 1e-12))
    {
        return false;
    }

    // Otherwise, recompute the ticks and only update the axes if the ticks have changed.
    bool updated = dvz_axis_update(axis, ref, vmin, vmax);

    return updated;
}



void dvz_axis_panel(DvzAxis* axis, DvzPanel* panel)
{
    ANN(axis);
    ANN(panel);

    if (axis->panel != NULL)
    {
        log_trace("avoid adding a panel to an axis twice");
        return;
    }

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, axis->glyph, 0);
    dvz_panel_visual(panel, axis->segment, 0);
    dvz_panel_visual(panel, axis->factor, 0);
    dvz_panel_visual(panel, axis->label, 0);
    dvz_panel_visual(panel, axis->spine, 0);

    axis->panel = panel;
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



void dvz_axis_label_layout(DvzAxis* axis, DvzAlign align, float xoffset, float yoffset)
{
    ANN(axis);

    if (axis->dim == DVZ_DIM_X)
    {
        axis->label_layout.align[0] = align;
        axis->label_layout.align[1] = DVZ_ALIGN_LOW;
    }
    else if (axis->dim == DVZ_DIM_Y)
    {
        axis->label_layout.align[0] = DVZ_ALIGN_LOW;
        axis->label_layout.align[1] = align;
    }

    axis->label_layout.offset[0] = xoffset;
    axis->label_layout.offset[1] = yoffset;
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
