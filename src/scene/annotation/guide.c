/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene guide lines and spans                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "core/generated_visual_policy.h"
#include "prepare_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_GUIDE_LINE_DESC_KNOWN_FLAGS 0u
#define DVZ_GUIDE_SPAN_DESC_KNOWN_FLAGS 0u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _guide_orientation_valid(DvzGuideOrientation orientation)
{
    return orientation == DVZ_GUIDE_ORIENTATION_HORIZONTAL ||
           orientation == DVZ_GUIDE_ORIENTATION_VERTICAL;
}



static bool _guide_cap_valid(DvzSegmentCap cap)
{
    return cap == DVZ_SEGMENT_CAP_NONE || cap == DVZ_SEGMENT_CAP_ROUND ||
           cap == DVZ_SEGMENT_CAP_TRIANGLE_IN || cap == DVZ_SEGMENT_CAP_TRIANGLE_OUT ||
           cap == DVZ_SEGMENT_CAP_SQUARE || cap == DVZ_SEGMENT_CAP_BUTT;
}



static bool _guide_line_desc_validate(const DvzGuideLineDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzGuideLineDesc, DVZ_GUIDE_LINE_DESC_KNOWN_FLAGS))
    {
        log_error("invalid guide-line descriptor ABI");
        return false;
    }
    if (!_guide_orientation_valid(desc->orientation))
    {
        log_error("invalid guide-line orientation");
        return false;
    }
    if (!isfinite(desc->value))
    {
        log_error("invalid guide-line value");
        return false;
    }
    if (!isfinite(desc->stroke_width_px) || desc->stroke_width_px < 0.0f)
    {
        log_error("invalid guide-line stroke width");
        return false;
    }
    if (!_guide_cap_valid(desc->cap_start) || !_guide_cap_valid(desc->cap_end))
    {
        log_error("invalid guide-line cap");
        return false;
    }
    return true;
}



static bool _guide_span_desc_validate(const DvzGuideSpanDesc* desc)
{
    if (desc == NULL)
        return false;
    if (!DVZ_STRUCT_VALID(desc, DvzGuideSpanDesc, DVZ_GUIDE_SPAN_DESC_KNOWN_FLAGS))
    {
        log_error("invalid guide-span descriptor ABI");
        return false;
    }
    if (!_guide_orientation_valid(desc->orientation))
    {
        log_error("invalid guide-span orientation");
        return false;
    }
    if (!isfinite(desc->min_value) || !isfinite(desc->max_value))
    {
        log_error("invalid guide-span values");
        return false;
    }
    if (fabs(desc->max_value - desc->min_value) <= DBL_EPSILON)
    {
        log_error("guide-span values must differ");
        return false;
    }
    if (!isfinite(desc->outline_width_px) || desc->outline_width_px < 0.0f)
    {
        log_error("invalid guide-span outline width");
        return false;
    }
    return true;
}



static bool _guide_panel_valid(DvzPanel* panel, DvzScene** out_scene)
{
    if (out_scene != NULL)
        *out_scene = NULL;
    if (panel == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    if (out_scene != NULL)
        *out_scene = panel->figure->scene;
    return true;
}



static void _guide_attach_visual(
    DvzPanel* panel, DvzVisual* visual, DvzGeneratedVisualRole role, int32_t z_offset)
{
    ANN(panel);
    ANN(visual);
    if (visual->visible)
    {
        if (_scene_panel_add_generated_visual(panel, visual, role, z_offset) != 0)
            log_error("failed to attach guide visual");
    }
}



static void
_guide_apply_visual_defaults(DvzVisual* visual, DvzGeneratedVisualRole role, uint8_t alpha)
{
    ANN(visual);
    DvzGeneratedVisualPolicy policy = _scene_generated_visual_policy(role);
    (void)_scene_generated_visual_apply_defaults(visual, &policy, alpha);
}



static bool _guide_data_to_panel_pixels(
    const DvzPanelFrameResolved* snapshot, double x, double y, float* out_x, float* out_y)
{
    ANN(snapshot);
    ANN(out_x);
    ANN(out_y);

    double x0 = snapshot->visible_data_x[0];
    double x1 = snapshot->visible_data_x[1];
    double y0 = snapshot->visible_data_y[0];
    double y1 = snapshot->visible_data_y[1];
    if (fabs(x1 - x0) <= DBL_EPSILON || fabs(y1 - y0) <= DBL_EPSILON)
        return false;

    const double tx = (x - x0) / (x1 - x0);
    const double ty = (y - y0) / (y1 - y0);
    *out_x = snapshot->plot_px.x - snapshot->panel_px.x + (float)tx * snapshot->plot_px.width;
    *out_y =
        snapshot->plot_px.y - snapshot->panel_px.y + (1.0f - (float)ty) * snapshot->plot_px.height;
    return true;
}



static void _guide_label_update(
    DvzAnnotation* label, const DvzPanelFrameResolved* snapshot, double x, double y)
{
    if (label == NULL)
        return;

    float px = 0.0f;
    float py = 0.0f;
    if (!_guide_data_to_panel_pixels(snapshot, x, y, &px, &py))
        return;

    label->placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label->placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label->placement.position[0] = px;
    label->placement.position[1] = py;
    label->placement.position[2] = 0.0;
    label->dirty_flags |= DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    label->version++;
}



static DvzAnnotation* _guide_label_create(DvzPanel* panel, const char* text)
{
    if (text == NULL || text[0] == '\0')
        return NULL;

    DvzLabelDesc label_desc = dvz_label_desc();
    label_desc.text = text;
    label_desc.placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    label_desc.placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    label_desc.placement.text_anchor[0] = 0.5f;
    label_desc.placement.text_anchor[1] = 0.5f;
    label_desc.placement.has_text_anchor = true;
    return dvz_annotation_label(panel, &label_desc);
}



static bool _guide_line_upload(DvzGuideLine* guide)
{
    ANN(guide);
    ANN(guide->panel);
    ANN(guide->line_visual);

    DvzPanelFrameResolved snapshot = {0};
    if (!_scene_panel_frame_snapshot(guide->panel, &snapshot) || !snapshot.has_valid_visible_x ||
        !snapshot.has_valid_visible_y)
        return false;
    double x0 = snapshot.visible_data_x[0];
    double x1 = snapshot.visible_data_x[1];
    double y0 = snapshot.visible_data_y[0];
    double y1 = snapshot.visible_data_y[1];

    vec3 position_start[1] = {{0}};
    vec3 position_end[1] = {{0}};
    if (guide->desc.orientation == DVZ_GUIDE_ORIENTATION_HORIZONTAL)
    {
        position_start[0][0] = (float)x0;
        position_start[0][1] = (float)guide->desc.value;
        position_end[0][0] = (float)x1;
        position_end[0][1] = (float)guide->desc.value;
        _guide_label_update(guide->label, &snapshot, 0.5 * (x0 + x1), guide->desc.value);
    }
    else
    {
        position_start[0][0] = (float)guide->desc.value;
        position_start[0][1] = (float)y0;
        position_end[0][0] = (float)guide->desc.value;
        position_end[0][1] = (float)y1;
        _guide_label_update(guide->label, &snapshot, guide->desc.value, 0.5 * (y0 + y1));
    }

    DvzColor colors[1] = {guide->desc.color};
    float widths[1] = {guide->desc.stroke_width_px};
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position_start", .data = position_start, .item_count = 1},
        {.attr_name = "position_end", .data = position_end, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "stroke_width_px", .data = widths, .item_count = 1},
    };
    return dvz_visual_set_data_many(guide->line_visual, updates, 4) == 0;
}



static bool _guide_span_upload(DvzGuideSpan* span)
{
    ANN(span);
    ANN(span->panel);
    ANN(span->fill_visual);

    DvzPanelFrameResolved snapshot = {0};
    if (!_scene_panel_frame_snapshot(span->panel, &snapshot) || !snapshot.has_valid_visible_x ||
        !snapshot.has_valid_visible_y)
        return false;
    double x0 = snapshot.visible_data_x[0];
    double x1 = snapshot.visible_data_x[1];
    double y0 = snapshot.visible_data_y[0];
    double y1 = snapshot.visible_data_y[1];

    double v0 = span->desc.min_value;
    double v1 = span->desc.max_value;
    if (v1 < v0)
    {
        double tmp = v0;
        v0 = v1;
        v1 = tmp;
    }

    double sx0 = x0, sx1 = x1, sy0 = y0, sy1 = y1;
    if (span->desc.orientation == DVZ_GUIDE_ORIENTATION_HORIZONTAL)
    {
        sy0 = v0;
        sy1 = v1;
        _guide_label_update(span->label, &snapshot, 0.5 * (x0 + x1), 0.5 * (v0 + v1));
    }
    else
    {
        sx0 = v0;
        sx1 = v1;
        _guide_label_update(span->label, &snapshot, 0.5 * (v0 + v1), 0.5 * (y0 + y1));
    }

    const vec3 positions[6] = {
        {(float)sx0, (float)sy0, 0.0f},
        {(float)sx1, (float)sy0, 0.0f},
        {(float)sx1, (float)sy1, 0.0f},
        {(float)sx0, (float)sy0, 0.0f},
        {(float)sx1, (float)sy1, 0.0f},
        {(float)sx0, (float)sy1, 0.0f},
    };
    DvzColor fill_colors[6] = {0};
    for (uint32_t i = 0; i < 6; i++)
        fill_colors[i] = span->desc.fill_color;

    DvzVisualDataUpdate fill_updates[] = {
        {.attr_name = "position", .data = positions, .item_count = 6},
        {.attr_name = "color", .data = fill_colors, .item_count = 6},
    };
    if (dvz_visual_set_data_many(span->fill_visual, fill_updates, 2) != 0)
        return false;

    if (span->outline_visual != NULL)
    {
        const vec3 starts[4] = {
            {(float)sx0, (float)sy0, 0.0f},
            {(float)sx1, (float)sy0, 0.0f},
            {(float)sx1, (float)sy1, 0.0f},
            {(float)sx0, (float)sy1, 0.0f},
        };
        const vec3 ends[4] = {
            {(float)sx1, (float)sy0, 0.0f},
            {(float)sx1, (float)sy1, 0.0f},
            {(float)sx0, (float)sy1, 0.0f},
            {(float)sx0, (float)sy0, 0.0f},
        };
        DvzColor outline_colors[4] = {0};
        float widths[4] = {0};
        for (uint32_t i = 0; i < 4; i++)
        {
            outline_colors[i] = span->desc.outline_color;
            widths[i] = span->desc.outline_width_px;
        }
        DvzVisualDataUpdate outline_updates[] = {
            {.attr_name = "position_start", .data = starts, .item_count = 4},
            {.attr_name = "position_end", .data = ends, .item_count = 4},
            {.attr_name = "color", .data = outline_colors, .item_count = 4},
            {.attr_name = "stroke_width_px", .data = widths, .item_count = 4},
        };
        if (dvz_visual_set_data_many(span->outline_visual, outline_updates, 4) != 0)
            return false;
    }

    return true;
}



/*************************************************************************************************/
/*  Public API                                                                                   */
/*************************************************************************************************/

DvzGuideLineDesc dvz_guide_line_desc(void)
{
    return (DvzGuideLineDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGuideLineDesc),
        .orientation = DVZ_GUIDE_ORIENTATION_HORIZONTAL,
        .value = 0.0,
        .stroke_width_px = 1.5f,
        .cap_start = DVZ_SEGMENT_CAP_BUTT,
        .cap_end = DVZ_SEGMENT_CAP_BUTT,
        .color = {255, 255, 255, 220},
    };
}



DvzGuideSpanDesc dvz_guide_span_desc(void)
{
    return (DvzGuideSpanDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGuideSpanDesc),
        .orientation = DVZ_GUIDE_ORIENTATION_VERTICAL,
        .min_value = -0.5,
        .max_value = +0.5,
        .fill_color = {255, 255, 255, 48},
        .outline_color = {255, 255, 255, 160},
        .outline_width_px = 0.0f,
    };
}



DvzGuideLine* dvz_guide_line(DvzPanel* panel, const DvzGuideLineDesc* desc)
{
    DvzScene* scene = NULL;
    if (!_guide_panel_valid(panel, &scene))
        return NULL;

    DvzGuideLineDesc resolved = desc != NULL ? *desc : dvz_guide_line_desc();
    if (!_guide_line_desc_validate(&resolved))
        return NULL;
    if (scene->guide_line_count >= DVZ_SCENE_MAX_GUIDE_LINES)
    {
        log_error("maximum guide-line count reached");
        return NULL;
    }

    DvzVisual* visual = dvz_segment(scene, 0);
    if (visual == NULL)
        return NULL;
    (void)dvz_segment_set_caps(visual, resolved.cap_start, resolved.cap_end);
    _guide_apply_visual_defaults(visual, DVZ_GENERATED_VISUAL_GUIDE_LINE, resolved.color.a);

    DvzGuideLine* guide = &scene->guide_lines[scene->guide_line_count++];
    dvz_memset(guide, sizeof(DvzGuideLine), 0, sizeof(DvzGuideLine));
    guide->scene = scene;
    guide->panel = panel;
    guide->desc = resolved;
    guide->active = true;
    guide->dirty = true;
    guide->version = 1;
    guide->line_visual = visual;
    guide->label = _guide_label_create(panel, resolved.label);

    _guide_attach_visual(panel, visual, DVZ_GENERATED_VISUAL_GUIDE_LINE, resolved.z_layer);
    _scene_notify_request_frame(panel->figure);
    return guide;
}



DvzVisual* dvz_guide_line_visual(DvzGuideLine* guide, DvzPlotRole role)
{
    if (guide == NULL || !guide->active)
        return NULL;
    return role == DVZ_PLOT_ROLE_LINE ? guide->line_visual : NULL;
}


DvzResult dvz_guide_line_set_value(DvzGuideLine* guide, double value)
{
    if (guide == NULL || !guide->active)
        return -1;
    if (!isfinite(value))
    {
        log_error("invalid guide-line value");
        return -1;
    }

    guide->desc.value = value;
    guide->dirty = true;
    guide->version++;
    _scene_notify_request_frame(guide->panel != NULL ? guide->panel->figure : NULL);
    return 0;
}



DvzGuideSpan* dvz_guide_span(DvzPanel* panel, const DvzGuideSpanDesc* desc)
{
    DvzScene* scene = NULL;
    if (!_guide_panel_valid(panel, &scene))
        return NULL;

    DvzGuideSpanDesc resolved = desc != NULL ? *desc : dvz_guide_span_desc();
    if (!_guide_span_desc_validate(&resolved))
        return NULL;
    if (scene->guide_span_count >= DVZ_SCENE_MAX_GUIDE_SPANS)
    {
        log_error("maximum guide-span count reached");
        return NULL;
    }

    DvzVisual* fill = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
    if (fill == NULL)
        return NULL;
    _guide_apply_visual_defaults(fill, DVZ_GENERATED_VISUAL_GUIDE_FILL, resolved.fill_color.a);

    DvzVisual* outline = NULL;
    if (resolved.outline_width_px > 0.0f && resolved.outline_color.a > 0)
    {
        outline = dvz_segment(scene, 0);
        if (outline == NULL)
            return NULL;
        (void)dvz_segment_set_caps(outline, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT);
        _guide_apply_visual_defaults(
            outline, DVZ_GENERATED_VISUAL_GUIDE_OUTLINE, resolved.outline_color.a);
    }

    DvzGuideSpan* span = &scene->guide_spans[scene->guide_span_count++];
    dvz_memset(span, sizeof(DvzGuideSpan), 0, sizeof(DvzGuideSpan));
    span->scene = scene;
    span->panel = panel;
    span->desc = resolved;
    span->active = true;
    span->dirty = true;
    span->version = 1;
    span->fill_visual = fill;
    span->outline_visual = outline;
    span->label = _guide_label_create(panel, resolved.label);

    _guide_attach_visual(panel, fill, DVZ_GENERATED_VISUAL_GUIDE_FILL, resolved.z_layer);
    if (outline != NULL)
        _guide_attach_visual(panel, outline, DVZ_GENERATED_VISUAL_GUIDE_OUTLINE, resolved.z_layer);
    _scene_notify_request_frame(panel->figure);
    return span;
}



DvzVisual* dvz_guide_span_visual(DvzGuideSpan* span, DvzPlotRole role)
{
    if (span == NULL || !span->active)
        return NULL;
    switch (role)
    {
    case DVZ_PLOT_ROLE_FILL:
        return span->fill_visual;
    case DVZ_PLOT_ROLE_OUTLINE:
        return span->outline_visual;
    default:
        return NULL;
    }
}


DvzResult dvz_guide_span_set_range(DvzGuideSpan* span, double min_value, double max_value)
{
    if (span == NULL || !span->active)
        return -1;
    if (!isfinite(min_value) || !isfinite(max_value))
    {
        log_error("invalid guide-span values");
        return -1;
    }
    if (fabs(max_value - min_value) <= DBL_EPSILON)
    {
        log_error("guide-span values must differ");
        return -1;
    }

    span->desc.min_value = min_value;
    span->desc.max_value = max_value;
    span->dirty = true;
    span->version++;
    _scene_notify_request_frame(span->panel != NULL ? span->panel->figure : NULL);
    return 0;
}



/*************************************************************************************************/
/*  Preparation                                                                                  */
/*************************************************************************************************/

void _scene_prepare_guide_visuals(DvzFigure* figure)
{
    if (figure == NULL || figure->scene == NULL)
        return;

    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->guide_line_count; i++)
    {
        DvzGuideLine* guide = &scene->guide_lines[i];
        if (!guide->active || guide->panel == NULL || guide->panel->figure != figure)
            continue;
        if (!_guide_line_upload(guide))
            log_error("failed to prepare guide-line visual %u", i);
        guide->dirty = false;
    }
    for (uint32_t i = 0; i < scene->guide_span_count; i++)
    {
        DvzGuideSpan* span = &scene->guide_spans[i];
        if (!span->active || span->panel == NULL || span->panel->figure != figure)
            continue;
        if (!_guide_span_upload(span))
            log_error("failed to prepare guide-span visual %u", i);
        span->dirty = false;
    }
}
