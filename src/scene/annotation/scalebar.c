/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene scale-bar annotations                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scale_ticks.h"
#include "_scene.h"
#include "core/generated_visual_policy.h"
#include "annotation/text_visual_bridge.h"
#include "core/format_state_internal.h"
#include "core/scene_notify_internal.h"
#include "core/units_internal.h"
#include "core/panel_layout_internal.h"
#include "datoviz/ffi.h"
#include "datoviz/scene.h"
#include "text_internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCALEBAR_DESC_KNOWN_FLAGS 0u

#define DVZ_SCALEBAR_TARGET_LENGTH_PX 120.0f
#define DVZ_SCALEBAR_MIN_LENGTH_PX    70.0f
#define DVZ_SCALEBAR_MAX_LENGTH_PX    180.0f
#define DVZ_SCALEBAR_OFFSET_PX        16.0f
#define DVZ_SCALEBAR_TICK_LENGTH_PX   8.0f
#define DVZ_SCALEBAR_LINE_WIDTH_PX    1.0f
#define DVZ_SCALEBAR_LABEL_GAP_PX     6.0f
#define DVZ_SCALEBAR_LABEL_SIZE_PX    12.0f
#define DVZ_SCALEBAR_LABEL_RESERVED_GLYPHS 12u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _scalebar_desc_validate(const DvzScaleBarDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzScaleBarDesc, DVZ_SCALEBAR_DESC_KNOWN_FLAGS))
    {
        log_error("invalid scale-bar descriptor ABI");
        return false;
    }
    return true;
}


/**
 * Return the default scale-bar annotation descriptor.
 *
 * @return default scale-bar descriptor
 */
DvzScaleBarDesc dvz_scale_bar_desc(void)
{
    return (DvzScaleBarDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
    };
}


bool dvz_ffi_scale_bar_desc(DvzScaleBarDesc* out)
{
    if (out == NULL)
        return false;
    *out = dvz_scale_bar_desc();
    return true;
}


/**
 * Return a positive finite scale-bar descriptor value or a fallback.
 *
 * @param value input value
 * @param fallback fallback value
 * @return resolved value
 */
static float _scalebar_positive_or_default(float value, float fallback)
{
    return isfinite(value) && value > 0.0f ? value : fallback;
}


/**
 * Return the data-to-canonical factor used by a scale bar.
 *
 * @param annotation scale-bar annotation
 * @return finite positive conversion factor
 */
static double _scalebar_data_to_canonical(const DvzAnnotation* annotation)
{
    ANN(annotation);
    if (annotation->scalebar_units_format != NULL &&
        isfinite(annotation->scalebar_units_format->data_to_canonical) &&
        annotation->scalebar_units_format->data_to_canonical > 0.0)
        return fabs(annotation->scalebar_units_format->data_to_canonical);
    const DvzScaleBarDesc* desc = &annotation->scalebar;
    return desc->data_to_unit != 0.0 && isfinite(desc->data_to_unit) ?
               fabs(desc->data_to_unit) :
               1.0;
}


/**
 * Create the compatibility units object implied by a scale-bar descriptor.
 *
 * @param annotation scale-bar annotation
 * @return units object, or NULL on allocation failure
 */
static DvzUnits* _scalebar_descriptor_units(DvzAnnotation* annotation)
{
    ANN(annotation);
    ANN(annotation->scene);
    const DvzScaleBarDesc* desc = &annotation->scalebar;
    double factor = desc->data_to_unit != 0.0 && isfinite(desc->data_to_unit) ?
                        fabs(desc->data_to_unit) :
                        1.0;
    const char* unit = desc->unit != NULL ? desc->unit : "";
    if (unit[0] == '\0')
        return dvz_units_builtin(annotation->scene, DVZ_UNIT_LADDER_RAW, factor);
    if (strcmp(unit, "m") == 0)
        return dvz_units_builtin(annotation->scene, DVZ_UNIT_LADDER_METRIC_LENGTH, factor);

    DvzUnitLadder* ladder = dvz_unit_ladder_create(annotation->scene, unit);
    if (ladder == NULL)
        return NULL;
    if (dvz_unit_ladder_add(ladder, 1.0, unit) != 0)
        return NULL;
    DvzUnits* units = dvz_units_create(annotation->scene);
    if (units == NULL)
        return NULL;
    if (dvz_units_set_data_to_canonical(units, factor) != 0 ||
        dvz_units_set_ladder(units, ladder) != 0)
        return NULL;
    return units;
}



/**
 * Resolve a scale-bar line color, defaulting to opaque white.
 *
 * @param desc scale-bar descriptor
 * @param out output color
 */
static void _scalebar_line_color(const DvzScaleBarDesc* desc, DvzColor* out)
{
    ANN(desc);
    ANN(out);
    *out = dvz_color_rgba(
        desc->line_color[0], desc->line_color[1], desc->line_color[2], desc->line_color[3]);
    if (out->a == 0 && out->r == 0 && out->g == 0 && out->b == 0)
    {
        *out = dvz_color_rgb(255, 255, 255);
    }
}



/**
 * Resolve the panel-local anchor point for a scale bar.
 *
 * @param anchor scene anchor
 * @param width panel width in logical pixels
 * @param height panel height in logical pixels
 * @param out_x output x coordinate
 * @param out_y output y coordinate
 */
static void _scalebar_anchor_px(
    DvzSceneAnchor anchor, float width, float height, float* out_x, float* out_y)
{
    ANN(out_x);
    ANN(out_y);
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
        *out_x = 0.5f * width;
        *out_y = 0.0f;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
        *out_x = width;
        *out_y = 0.0f;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        *out_x = 0.0f;
        *out_y = 0.5f * height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
        *out_x = 0.5f * width;
        *out_y = 0.5f * height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        *out_x = width;
        *out_y = 0.5f * height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
        *out_x = 0.0f;
        *out_y = height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        *out_x = 0.5f * width;
        *out_y = height;
        break;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        *out_x = width;
        *out_y = height;
        break;
    case DVZ_SCENE_ANCHOR_NONE:
    case DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT:
    default:
        *out_x = 0.0f;
        *out_y = 0.0f;
        break;
    }
}



/**
 * Return whether one anchor is on the right edge.
 *
 * @param anchor scene anchor
 * @return whether the anchor is right-aligned
 */
static bool _scalebar_anchor_right(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT;
}



/**
 * Return whether one anchor is horizontally centered.
 *
 * @param anchor scene anchor
 * @return whether the anchor is centered
 */
static bool _scalebar_anchor_center_x(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_TOP ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_CENTER ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
}



/**
 * Return whether one anchor is on the bottom edge.
 *
 * @param anchor scene anchor
 * @return whether the anchor is bottom-aligned
 */
static bool _scalebar_anchor_bottom(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT;
}



/**
 * Ensure scale-bar derived segment and text visuals exist and are attached.
 *
 * @param annotation scale-bar annotation
 * @return whether visuals exist
 */
static bool _scalebar_ensure_visuals(DvzAnnotation* annotation)
{
    ANN(annotation);
    ANN(annotation->scene);
    ANN(annotation->panel);
    if (annotation->scalebar_visual == NULL)
    {
        annotation->scalebar_visual = dvz_segment(annotation->scene, 0);
        if (annotation->scalebar_visual == NULL)
            return false;
        annotation->scalebar_visual->visible = false;
        if (dvz_segment_set_caps(
                annotation->scalebar_visual, DVZ_SEGMENT_CAP_BUTT, DVZ_SEGMENT_CAP_BUTT) != 0)
            return false;
        if (_scene_panel_add_generated_visual(
                annotation->panel, annotation->scalebar_visual,
                DVZ_GENERATED_VISUAL_SCALEBAR_LINE, 0) != 0)
            return false;
    }
    if (annotation->visual == NULL)
    {
        annotation->visual = _scene_text_visual(annotation->scene, 0);
        if (annotation->visual == NULL)
            return false;
        _visual_family_state(annotation->visual)->text.reserved_glyph_vertices =
            DVZ_SCALEBAR_LABEL_RESERVED_GLYPHS * 6u;
        annotation->visual->visible = false;
        if (_scene_panel_add_generated_visual(
                annotation->panel, annotation->visual, DVZ_GENERATED_VISUAL_SCALEBAR_TEXT, 0) != 0)
            return false;
    }
    return true;
}


static const DvzPanelAttach* _scalebar_panel_attach(DvzPanel* panel, const DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual == visual)
            return &panel->visuals[i];
    }
    return NULL;
}



/**
 * Hide all derived visuals for one scale bar.
 *
 * @param annotation scale-bar annotation
 */
static void _scalebar_hide(DvzAnnotation* annotation)
{
    if (annotation == NULL)
        return;
    if (annotation->scalebar_visual != NULL)
        dvz_visual_set_visible(annotation->scalebar_visual, false);
    if (annotation->visual != NULL)
        dvz_visual_set_visible(annotation->visual, false);
    annotation->scalebar_realization.valid = false;
}



/**
 * Resolve the world reference direction for a 3D scale bar.
 *
 * @param desc scale-bar descriptor
 * @param out output normalized direction
 * @return whether a finite direction was resolved
 */
static bool _scalebar_reference_direction(const DvzScaleBarDesc* desc, vec3 out)
{
    ANN(desc);
    ANN(out);

    out[0] = (float)desc->reference_direction[0];
    out[1] = (float)desc->reference_direction[1];
    out[2] = (float)desc->reference_direction[2];
    if (!isfinite(out[0]) || !isfinite(out[1]) || !isfinite(out[2]))
        return false;

    float norm = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
    if (norm <= 0.0f)
    {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        switch (desc->dimension)
        {
        case DVZ_DIM_Y:
            out[1] = 1.0f;
            break;
        case DVZ_DIM_Z:
            out[2] = 1.0f;
            break;
        case DVZ_DIM_X:
        default:
            out[0] = 1.0f;
            break;
        }
        return true;
    }

    out[0] /= norm;
    out[1] /= norm;
    out[2] /= norm;
    return true;
}



/**
 * Project one world point to panel-local pixels.
 *
 * @param panel panel owning the scale bar
 * @param mvp active panel MVP
 * @param point input world position
 * @param out_x output panel-local x coordinate in pixels
 * @param out_y output panel-local y coordinate in pixels
 * @return whether projection succeeded
 */
static bool _scalebar_project_world_to_panel_px(
    const DvzPanel* panel, DvzMVP* mvp, const vec3 point, float* out_x, float* out_y)
{
    ANN(panel);
    ANN(mvp);
    ANN(out_x);
    ANN(out_y);

    vec4 p = {point[0], point[1], point[2], 1.0f};
    vec4 tmp0 = {0};
    vec4 tmp1 = {0};
    vec4 clip = {0};
    glm_mat4_mulv(mvp->model, p, tmp0);
    glm_mat4_mulv(mvp->view, tmp0, tmp1);
    glm_mat4_mulv(mvp->proj, tmp1, clip);
    if (!isfinite(clip[0]) || !isfinite(clip[1]) || !isfinite(clip[3]) ||
        fabsf(clip[3]) <= 1e-12f)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    if (panel_width <= 0.0f || panel_height <= 0.0f)
        return false;

    float ndc_x = clip[0] / clip[3];
    float ndc_y = clip[1] / clip[3];
    if (!isfinite(ndc_x) || !isfinite(ndc_y))
        return false;
    *out_x = 0.5f * (ndc_x + 1.0f) * panel_width;
    *out_y = 0.5f * (1.0f - ndc_y) * panel_height;
    return isfinite(*out_x) && isfinite(*out_y);
}



/**
 * Strip model rotation while preserving translation and uniform scale.
 *
 * @param src source model matrix
 * @param dst destination model matrix
 */
static void _scalebar_model_without_rotation(mat4 src, mat4 dst)
{
    ANN(dst);

    glm_mat4_identity(dst);
    float sx = glm_vec3_norm((vec3){src[0][0], src[0][1], src[0][2]});
    float sy = glm_vec3_norm((vec3){src[1][0], src[1][1], src[1][2]});
    float sz = glm_vec3_norm((vec3){src[2][0], src[2][1], src[2][2]});
    float scale = 1.0f;
    uint32_t count = 0;
    if (isfinite(sx) && sx > 0.0f)
    {
        scale += sx;
        count++;
    }
    if (isfinite(sy) && sy > 0.0f)
    {
        scale += sy;
        count++;
    }
    if (isfinite(sz) && sz > 0.0f)
    {
        scale += sz;
        count++;
    }
    if (count > 0)
        scale = (scale - 1.0f) / (float)count;

    dst[0][0] = scale;
    dst[1][1] = scale;
    dst[2][2] = scale;
    dst[3][0] = src[3][0];
    dst[3][1] = src[3][1];
    dst[3][2] = src[3][2];
}



/**
 * Resolve the camera view-plane direction for a viewer-style 3D scale bar.
 *
 * @param desc scale-bar descriptor
 * @param mvp panel MVP
 * @param out output normalized data-space direction
 * @return whether a finite direction was resolved
 */
static bool _scalebar_view_plane_direction(const DvzScaleBarDesc* desc, DvzMVP* mvp, vec3 out)
{
    ANN(desc);
    ANN(mvp);
    ANN(out);

    vec4 view_axis = {1.0f, 0.0f, 0.0f, 0.0f};
    if (desc->dimension == DVZ_DIM_Y)
    {
        view_axis[0] = 0.0f;
        view_axis[1] = 1.0f;
    }

    mat4 view = GLM_MAT4_IDENTITY_INIT;
    mat4 inv_view = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_copy(mvp->view, view);
    glm_mat4_inv(view, inv_view);

    vec4 world_axis = {0};
    glm_mat4_mulv(inv_view, view_axis, world_axis);
    out[0] = world_axis[0];
    out[1] = world_axis[1];
    out[2] = world_axis[2];
    if (!isfinite(out[0]) || !isfinite(out[1]) || !isfinite(out[2]))
        return false;
    if (glm_vec3_norm(out) <= 0.0f)
        return false;
    glm_vec3_normalize(out);
    return true;
}



/**
 * Resolve local world-units-per-pixel for a 3D scale bar.
 *
 * @param annotation scale-bar annotation
 * @param view_plane whether to resolve a camera-facing view-plane scale
 * @param out_units_per_px output physical units per panel pixel
 * @return whether a finite local scale was resolved
 */
static bool _scalebar_world_units_per_px(
    DvzAnnotation* annotation, bool view_plane, double* out_units_per_px)
{
    ANN(annotation);
    ANN(out_units_per_px);
    DvzScaleBarDesc* desc = &annotation->scalebar;
    vec3 reference = {
        (float)desc->reference_position[0],
        (float)desc->reference_position[1],
        (float)desc->reference_position[2],
    };
    if (!isfinite(reference[0]) || !isfinite(reference[1]) || !isfinite(reference[2]))
        return false;

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(annotation->panel, &mvp);
    if (view_plane)
    {
        mat4 model = GLM_MAT4_IDENTITY_INIT;
        _scalebar_model_without_rotation(mvp.model, model);
        glm_mat4_copy(model, mvp.model);
    }

    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    if (!_scalebar_project_world_to_panel_px(annotation->panel, &mvp, reference, &x0, &y0))
        return false;

    vec3 direction = {0};
    bool direction_ok = view_plane ? _scalebar_view_plane_direction(desc, &mvp, direction)
                                   : _scalebar_reference_direction(desc, direction);
    if (!direction_ok)
        return false;

    vec3 unit_point = {
        reference[0] + direction[0],
        reference[1] + direction[1],
        reference[2] + direction[2],
    };
    if (!_scalebar_project_world_to_panel_px(annotation->panel, &mvp, unit_point, &x1, &y1))
        return false;

    float dx = x1 - x0;
    float dy = y1 - y0;
    float px_per_data_unit = sqrtf(dx * dx + dy * dy);
    if (!isfinite(px_per_data_unit) || px_per_data_unit <= 0.0f)
        return false;

    double data_to_canonical = _scalebar_data_to_canonical(annotation);
    *out_units_per_px = data_to_canonical / (double)px_per_data_unit;
    return isfinite(*out_units_per_px) && *out_units_per_px > 0.0;
}



/**
 * Return whether two resolved scale bars have identical segment payloads.
 *
 * @param a first resolved scale-bar state
 * @param b second resolved scale-bar state
 * @return whether segment geometry and style payloads match exactly
 */
static bool _scalebar_segment_payload_equal(
    const DvzScaleBarRealization* a, const DvzScaleBarRealization* b)
{
    ANN(a);
    ANN(b);
    return a->horizontal == b->horizontal &&
           memcmp(a->starts, b->starts, sizeof(a->starts)) == 0 &&
           memcmp(a->ends, b->ends, sizeof(a->ends)) == 0 &&
           memcmp(a->line_colors, b->line_colors, sizeof(a->line_colors)) == 0 &&
           memcmp(a->line_width, b->line_width, sizeof(a->line_width)) == 0;
}



/**
 * Return whether two resolved scale bars have identical label anchor positions.
 *
 * @param a first resolved scale-bar state
 * @param b second resolved scale-bar state
 * @return whether label anchor positions match exactly
 */
static bool _scalebar_label_position_equal(
    const DvzScaleBarRealization* a, const DvzScaleBarRealization* b)
{
    ANN(a);
    ANN(b);
    return memcmp(a->label_position, b->label_position, sizeof(a->label_position)) == 0;
}



/**
 * Return whether two resolved scale bars can reuse the same glyph layout payload.
 *
 * @param a first resolved scale-bar state
 * @param b second resolved scale-bar state
 * @return whether text, style, and renderer payloads match exactly
 */
static bool _scalebar_label_layout_equal(
    const DvzScaleBarRealization* a, const DvzScaleBarRealization* b)
{
    ANN(a);
    ANN(b);
    return strcmp(a->label, b->label) == 0 &&
           fabsf(a->screen_scale - b->screen_scale) <= 1e-6f &&
           memcmp(a->label_anchor, b->label_anchor, sizeof(a->label_anchor)) == 0 &&
           a->label_size == b->label_size &&
           memcmp(&a->label_color, &b->label_color, sizeof(a->label_color)) == 0 &&
           a->label_angle == b->label_angle && a->renderer == b->renderer;
}



/**
 * Update or create the internal visuals for one screen-space scale-bar overlay.
 *
 * @param figure the figure being emitted
 * @param annotation scale-bar annotation
 * @param units_per_px physical units per panel pixel
 * @param horizontal whether the bar is drawn horizontally
 * @return whether preparation succeeded
 */
static bool _scalebar_prepare_overlay_visual(
    DvzFigure* figure, DvzAnnotation* annotation, double units_per_px, bool horizontal)
{
    ANN(figure);
    ANN(annotation);

    DvzScaleBarDesc* desc = &annotation->scalebar;
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(annotation->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    if (panel_width <= 0.0f || panel_height <= 0.0f)
    {
        _scalebar_hide(annotation);
        return true;
    }

    double length_units = 0.0;
    float length_px = 0.0f;
    if (!_scene_scalebar_choose_length(
            units_per_px,
            _scalebar_positive_or_default(
                desc->target_length_px, DVZ_SCALEBAR_TARGET_LENGTH_PX),
            _scalebar_positive_or_default(desc->min_length_px, DVZ_SCALEBAR_MIN_LENGTH_PX),
            _scalebar_positive_or_default(desc->max_length_px, DVZ_SCALEBAR_MAX_LENGTH_PX),
            &length_units, &length_px))
    {
        _scalebar_hide(annotation);
        return true;
    }
    if (!_scalebar_ensure_visuals(annotation))
        return false;

    DvzScaleBarRealization resolved = {
        .valid = true,
        .horizontal = horizontal,
        .units_per_px = units_per_px,
        .length_units = length_units,
        .length_px = length_px,
        .screen_scale = _scene_screen_scale(figure),
    };

    float anchor_x = 0.0f;
    float anchor_y = 0.0f;
    _scalebar_anchor_px(desc->anchor, panel_width, panel_height, &anchor_x, &anchor_y);
    float offset_x = desc->offset_px[0] != 0.0f ? desc->offset_px[0] : DVZ_SCALEBAR_OFFSET_PX;
    float offset_y = desc->offset_px[1] != 0.0f ? desc->offset_px[1] : DVZ_SCALEBAR_OFFSET_PX;
    anchor_x += _scalebar_anchor_right(desc->anchor) ? -offset_x : offset_x;
    anchor_y += _scalebar_anchor_bottom(desc->anchor) ? -offset_y : offset_y;

    float x0 = anchor_x;
    float y0 = anchor_y;
    float x1 = anchor_x;
    float y1 = anchor_y;
    if (horizontal)
    {
        if (_scalebar_anchor_center_x(desc->anchor))
            x0 -= 0.5f * length_px;
        else if (_scalebar_anchor_right(desc->anchor))
            x0 -= length_px;
        x1 = x0 + length_px;
    }
    else
    {
        y0 -= _scalebar_anchor_bottom(desc->anchor) ? length_px : 0.0f;
        y1 = y0 + length_px;
    }

    float tick = _scalebar_positive_or_default(desc->tick_length_px, DVZ_SCALEBAR_TICK_LENGTH_PX);
    float half_tick = 0.5f * tick;
    if (horizontal)
    {
        _text_panel_pixel_to_clip(annotation->panel, x0, y0, 0.0f, resolved.starts[0]);
        _text_panel_pixel_to_clip(annotation->panel, x1, y1, 0.0f, resolved.ends[0]);
        _text_panel_pixel_to_clip(annotation->panel, x0, y0 - half_tick, 0.0f, resolved.starts[1]);
        _text_panel_pixel_to_clip(annotation->panel, x0, y0 + half_tick, 0.0f, resolved.ends[1]);
        _text_panel_pixel_to_clip(annotation->panel, x1, y1 - half_tick, 0.0f, resolved.starts[2]);
        _text_panel_pixel_to_clip(annotation->panel, x1, y1 + half_tick, 0.0f, resolved.ends[2]);
    }
    else
    {
        _text_panel_pixel_to_clip(annotation->panel, x0, y0, 0.0f, resolved.starts[0]);
        _text_panel_pixel_to_clip(annotation->panel, x1, y1, 0.0f, resolved.ends[0]);
        _text_panel_pixel_to_clip(annotation->panel, x0 - half_tick, y0, 0.0f, resolved.starts[1]);
        _text_panel_pixel_to_clip(annotation->panel, x0 + half_tick, y0, 0.0f, resolved.ends[1]);
        _text_panel_pixel_to_clip(annotation->panel, x1 - half_tick, y1, 0.0f, resolved.starts[2]);
        _text_panel_pixel_to_clip(annotation->panel, x1 + half_tick, y1, 0.0f, resolved.ends[2]);
    }
    _scalebar_line_color(desc, &resolved.line_colors[0]);
    resolved.line_colors[1] = resolved.line_colors[0];
    resolved.line_colors[2] = resolved.line_colors[0];
    resolved.line_width[0] =
        _scalebar_positive_or_default(desc->line_width_px, DVZ_SCALEBAR_LINE_WIDTH_PX);
    resolved.line_width[1] = resolved.line_width[0];
    resolved.line_width[2] = resolved.line_width[0];

    DvzUnits* units = annotation->scalebar_units_format;
    if (units != NULL && isfinite(units->data_to_canonical) && units->data_to_canonical > 0.0)
    {
        DvzUnitFormatContext context = {.mode = DVZ_UNIT_DISPLAY_AUTO};
        double length_data = length_units / units->data_to_canonical;
        (void)_scene_units_format(
            units, length_data, &context, resolved.label, sizeof(resolved.label));
    }
    else
    {
        _scene_format_si_value(length_units, desc->unit, resolved.label, sizeof(resolved.label));
    }
    const DvzTextStyle* label_style = &annotation->style;
    resolved.label_size =
        _scalebar_positive_or_default(label_style->size_px, DVZ_SCALEBAR_LABEL_SIZE_PX);
    resolved.renderer =
        label_style->renderer != 0 ? label_style->renderer : DVZ_TEXT_RENDERER_MSDF_ATLAS;
    resolved.label_position[0] = horizontal ? 0.5f * (x0 + x1) : x1;
    resolved.label_position[1] = horizontal ? y0 : 0.5f * (y0 + y1);
    float label_gap = DVZ_SCALEBAR_LABEL_GAP_PX;
    if (desc->label_position == DVZ_SCALEBAR_LABEL_ABOVE)
        resolved.label_position[1] -= tick + label_gap;
    else
        resolved.label_position[1] += tick + label_gap;
    resolved.label_anchor[0] = 0.5f;
    resolved.label_anchor[1] = desc->label_position == DVZ_SCALEBAR_LABEL_ABOVE ? 1.0f : 0.0f;
    uint8_t label_color[4] = {0};
    _text_style_color(label_style, label_color);
    resolved.label_color =
        dvz_color_rgba(label_color[0], label_color[1], label_color[2], label_color[3]);
    resolved.label_angle = 0.0f;

    DvzScaleBarRealization* previous = &annotation->scalebar_realization;
    bool previous_valid = previous->valid;
    bool segment_dirty =
        !previous_valid || !_scalebar_segment_payload_equal(previous, &resolved);
    bool label_layout_dirty =
        !previous_valid || !_scalebar_label_layout_equal(previous, &resolved);
    bool label_position_dirty =
        label_layout_dirty || !previous_valid ||
        !_scalebar_label_position_equal(previous, &resolved);

    if (segment_dirty)
    {
        DvzVisualDataUpdate updates[4] = {
            {.attr_name = "position_start", .data = resolved.starts, .item_count = 3},
            {.attr_name = "position_end", .data = resolved.ends, .item_count = 3},
            {.attr_name = "color", .data = resolved.line_colors, .item_count = 3},
            {.attr_name = "line_width", .data = resolved.line_width, .item_count = 3},
        };
        if (dvz_visual_set_data_many(annotation->scalebar_visual, updates, 4) != 0)
            return false;
    }
    dvz_visual_set_visible(annotation->scalebar_visual, true);

    if (label_layout_dirty)
    {
        const char* labels[1] = {resolved.label};
        float label_pos[1][3] = {
            {resolved.label_position[0], resolved.label_position[1], resolved.label_position[2]}};
        float text_anchor[1][2] = {{resolved.label_anchor[0], resolved.label_anchor[1]}};
        float sizes[1] = {resolved.label_size};
        DvzColor text_colors[1] = {{0}};
        text_colors[0] = resolved.label_color;
        float angles[1] = {resolved.label_angle};
        DvzVisualDataUpdate text_updates[5] = {
            {.attr_name = "position", .data = label_pos, .item_count = 1},
            {.attr_name = "anchor", .data = text_anchor, .item_count = 1},
            {.attr_name = "size", .data = sizes, .item_count = 1},
            {.attr_name = "color", .data = text_colors, .item_count = 1},
            {.attr_name = "angle", .data = angles, .item_count = 1},
        };
        if (_visual_family_state(annotation->visual)->text.renderer != resolved.renderer &&
            _scene_text_visual_set_renderer(annotation->visual, resolved.renderer) != 0)
            return false;
        if (dvz_visual_set_strings(annotation->visual, "text", labels, 1) != 0 ||
            dvz_visual_set_data_many(annotation->visual, text_updates, 5) != 0)
            return false;
    }
    else if (label_position_dirty)
    {
        float label_pos[1][3] = {
            {resolved.label_position[0], resolved.label_position[1], resolved.label_position[2]}};
        if (dvz_visual_set_data(annotation->visual, "position", label_pos, 1) != 0)
            return false;
    }
    dvz_visual_set_visible(annotation->visual, true);

    if (label_position_dirty)
    {
        const DvzPanelAttach* attach = _scalebar_panel_attach(annotation->panel, annotation->visual);
        if (attach == NULL || !_text_visual_prepare(figure, annotation->panel, attach, annotation->visual))
            return false;
    }

    if (segment_dirty || label_position_dirty)
    {
        annotation->scalebar_realization = resolved;
        dvz_strlcpy(annotation->text, resolved.label, sizeof(annotation->text));
        annotation->scalebar_units = length_units;
        annotation->scalebar_px = length_px;
        annotation->version++;
    }
    annotation->dirty_flags = DVZ_TEXT_DIRTY_NONE;
    return true;
}



/**
 * Update or create the internal visuals for one scale-bar annotation.
 *
 * @param figure the figure being emitted
 * @param annotation scale-bar annotation
 * @return whether preparation succeeded
 */
bool _scalebar_prepare_visual(DvzFigure* figure, DvzAnnotation* annotation)
{
    ANN(figure);
    ANN(annotation);
    if (annotation->scene == NULL || annotation->panel == NULL ||
        annotation->panel->figure != figure)
        return true;
    if (annotation->kind != DVZ_ANNOTATION_SCALEBAR)
        return true;

    DvzScaleBarDesc* desc = &annotation->scalebar;
    if (desc->reference_mode == DVZ_SCALEBAR_REFERENCE_WORLD_POINT ||
        desc->reference_mode == DVZ_SCALEBAR_REFERENCE_VIEW_PLANE)
    {
        double units_per_px = 0.0;
        bool view_plane = desc->reference_mode == DVZ_SCALEBAR_REFERENCE_VIEW_PLANE;
        if (!_scalebar_world_units_per_px(annotation, view_plane, &units_per_px))
        {
            _scalebar_hide(annotation);
            return true;
        }
        return _scalebar_prepare_overlay_visual(
            figure, annotation, units_per_px, desc->dimension != DVZ_DIM_Y);
    }

    if (desc->dimension != DVZ_DIM_X && desc->dimension != DVZ_DIM_Y)
    {
        _scalebar_hide(annotation);
        return true;
    }
    double visible_min = 0.0;
    double visible_max = 0.0;
    if (!dvz_panel_visible_domain(annotation->panel, desc->dimension, &visible_min, &visible_max))
    {
        _scalebar_hide(annotation);
        return true;
    }

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(annotation->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    float span_px = desc->dimension == DVZ_DIM_X ? panel_width : panel_height;
    if (span_px <= 0.0f)
    {
        _scalebar_hide(annotation);
        return true;
    }

    double data_to_canonical = _scalebar_data_to_canonical(annotation);
    double units_per_px = fabs(visible_max - visible_min) * data_to_canonical / (double)span_px;
    if (!isfinite(units_per_px) || units_per_px <= 0.0)
    {
        _scalebar_hide(annotation);
        return true;
    }
    return _scalebar_prepare_overlay_visual(
        figure, annotation, units_per_px, desc->dimension == DVZ_DIM_X);
}






/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a retained scale bar.
 *
 * @param panel the panel
 * @param desc the scale-bar descriptor, or NULL for defaults
 * @return the scale bar, or NULL on allocation failure
 */
DvzScaleBar* dvz_scale_bar(DvzPanel* panel, const DvzScaleBarDesc* desc)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    if (!_scalebar_desc_validate(desc))
        return NULL;
    DvzScaleBarDesc fallback = dvz_scale_bar_desc();
    if (desc == NULL)
        desc = &fallback;

    DvzAnnotation* annotation = dvz_annotation(
        panel, &(DvzAnnotationDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzAnnotationDesc),
                   .kind = DVZ_ANNOTATION_SCALEBAR,
                   .annotation_flags = desc->scalebar_flags});
    if (annotation == NULL)
        return NULL;
    annotation->scalebar = *desc;
    if (
        annotation->scalebar.dimension != DVZ_DIM_Y &&
        annotation->scalebar.dimension != DVZ_DIM_Z)
        annotation->scalebar.dimension = DVZ_DIM_X;
    if (annotation->scalebar.anchor == DVZ_SCENE_ANCHOR_NONE)
        annotation->scalebar.anchor = DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT;
    if (annotation->style.size_px <= 0.0f)
        annotation->style.size_px = DVZ_SCALEBAR_LABEL_SIZE_PX;
    if (annotation->style.renderer == DVZ_TEXT_RENDERER_AUTO)
        annotation->style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    if (annotation->scalebar.data_to_unit == 0.0 ||
        !isfinite(annotation->scalebar.data_to_unit))
        annotation->scalebar.data_to_unit = 1.0;
    if (annotation->scalebar.target_length_px <= 0.0f)
        annotation->scalebar.target_length_px = DVZ_SCALEBAR_TARGET_LENGTH_PX;
    if (annotation->scalebar.min_length_px <= 0.0f)
        annotation->scalebar.min_length_px = DVZ_SCALEBAR_MIN_LENGTH_PX;
    if (annotation->scalebar.max_length_px <= 0.0f)
        annotation->scalebar.max_length_px = DVZ_SCALEBAR_MAX_LENGTH_PX;
    if (annotation->scalebar.tick_length_px <= 0.0f)
        annotation->scalebar.tick_length_px = DVZ_SCALEBAR_TICK_LENGTH_PX;
    if (annotation->scalebar.line_width_px <= 0.0f)
        annotation->scalebar.line_width_px = DVZ_SCALEBAR_LINE_WIDTH_PX;
    annotation->scalebar_units_format = _scalebar_descriptor_units(annotation);
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version = 1;
    _scene_notify_request_frame(panel->figure);
    return (DvzScaleBar*)annotation;
}


DvzResult dvz_scale_bar_set_dimension(DvzScaleBar* scalebar, DvzDim dim)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR ||
        (dim != DVZ_DIM_X && dim != DVZ_DIM_Y && dim != DVZ_DIM_Z))
        return -1;
    annotation->scalebar.dimension = dim;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_scale_bar_set_anchor(DvzScaleBar* scalebar, DvzSceneAnchor anchor)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR)
        return -1;
    annotation->scalebar.anchor = anchor == DVZ_SCENE_ANCHOR_NONE ?
                                      DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT :
                                      anchor;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_scale_bar_set_units(DvzScaleBar* scalebar, DvzUnits* units)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR || units == NULL ||
        units->scene != annotation->scene || units->ladder == NULL)
        return -1;
    annotation->scalebar_units_format = units;
    annotation->dirty_flags = DVZ_TEXT_DIRTY_ALL;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_scale_bar_set_duration_units(DvzScaleBar* scalebar, DvzUnits* duration_units)
{
    return dvz_scale_bar_set_units(scalebar, duration_units);
}


DvzResult dvz_scale_bar_set_label_style(DvzScaleBar* scalebar, const DvzTextStyle* style)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR)
        return -1;
    if (style != NULL && !_text_style_validate(style))
        return -1;
    DvzTextStyle resolved = style != NULL ?
                                *style :
                                (DvzTextStyle){DVZ_STRUCT_INIT_FIELDS(DvzTextStyle)};
    if (resolved.size_px <= 0.0f)
        resolved.size_px = DVZ_SCALEBAR_LABEL_SIZE_PX;
    if (resolved.renderer == DVZ_TEXT_RENDERER_AUTO)
        resolved.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    if (resolved.font != NULL && resolved.font->scene != annotation->scene)
        return -1;
    annotation->style = resolved;
    annotation->dirty_flags |= DVZ_TEXT_DIRTY_STYLE | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_scale_bar_set_placement(
    DvzScaleBar* scalebar, const DvzTextPlacement* placement)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR)
        return -1;
    if (placement != NULL && !_text_placement_validate(placement))
        return -1;
    annotation->placement = placement != NULL ?
                                *placement :
                                (DvzTextPlacement){DVZ_STRUCT_INIT_FIELDS(DvzTextPlacement)};
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_PLACEMENT | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}


DvzResult dvz_scale_bar_set_format(DvzScaleBar* scalebar, const DvzFormatDesc* format)
{
    DvzAnnotation* annotation = (DvzAnnotation*)scalebar;
    if (annotation == NULL || annotation->kind != DVZ_ANNOTATION_SCALEBAR)
        return -1;
    if (!_scene_format_desc_validate(format))
        return -1;
    annotation->has_format = format != NULL;
    _scene_format_state_copy(&annotation->format, format);
    annotation->dirty_flags |=
        DVZ_TEXT_DIRTY_STRING | DVZ_TEXT_DIRTY_LAYOUT | DVZ_TEXT_DIRTY_RENDER;
    annotation->version++;
    _scene_notify_request_frame(annotation->panel != NULL ? annotation->panel->figure : NULL);
    return 0;
}
