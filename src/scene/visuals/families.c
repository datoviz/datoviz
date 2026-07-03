/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene visual families */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"
#include "core/generated_visual_policy.h"
#include "core/scene_notify_internal.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "annotation/scale_internal.h"
#include "bindings_internal.h"
#include "datoviz/ffi.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"
#include "domain/buffer_internal.h"
#include "domain/field_internal.h"
#include "image/cache.h"
#include "registry/registry.h"
#include "stroke/cache.h"
#include "stroke/state.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

#define DVZ_VISUAL_ATTACH_DESC_KNOWN_FLAGS 0u
#define DVZ_PANEL_BACKGROUND_DESC_KNOWN_FLAGS 0u
#define DVZ_PANEL_BORDER_DESC_KNOWN_FLAGS 0u
#define DVZ_VISUAL_TRANSFORM_DESC_KNOWN_FLAGS 0u
#define DVZ_VISUAL_SHADER_DESC_KNOWN_FLAGS 0u



static bool _visual_attach_desc_validate(const DvzVisualAttachDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzVisualAttachDesc, DVZ_VISUAL_ATTACH_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzVisualAttachDesc ABI prologue");
        return false;
    }
    if (
        desc->coord_space != DVZ_VISUAL_COORD_VIEW && desc->coord_space != DVZ_VISUAL_COORD_DATA &&
        desc->coord_space != DVZ_VISUAL_COORD_PANEL)
    {
        log_error("invalid visual coordinate space");
        return false;
    }
    if (
        desc->controller_mode != DVZ_CONTROLLER_APPLY &&
        desc->controller_mode != DVZ_CONTROLLER_FIXED &&
        desc->controller_mode != DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL &&
        desc->controller_mode != DVZ_CONTROLLER_APPLY_VIEW_PROJ)
    {
        log_error("invalid visual controller mode");
        return false;
    }
    if (
        desc->clip_rect != DVZ_VISUAL_CLIP_AUTO &&
        desc->clip_rect != DVZ_VISUAL_CLIP_PANEL &&
        desc->clip_rect != DVZ_VISUAL_CLIP_PLOT)
    {
        log_error("invalid visual clip rectangle");
        return false;
    }
    if (
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_AUTO &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_PANEL &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_PLOT &&
        desc->viewport_rect != DVZ_VISUAL_VIEWPORT_TARGET)
    {
        log_error("invalid visual viewport rectangle");
        return false;
    }
    return true;
}



static bool _panel_background_desc_validate(const DvzPanelBackgroundDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzPanelBackgroundDesc, DVZ_PANEL_BACKGROUND_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzPanelBackgroundDesc ABI prologue");
        return false;
    }
    return true;
}


static bool _panel_border_desc_validate(const DvzPanelBorderDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzPanelBorderDesc, DVZ_PANEL_BORDER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzPanelBorderDesc ABI prologue");
        return false;
    }
    if (!isfinite(desc->width_px) || desc->width_px < 0.0f)
    {
        log_error("panel border width must be finite and non-negative");
        return false;
    }
    if (!isfinite(desc->inset_px) || desc->inset_px < 0.0f)
    {
        log_error("panel border inset must be finite and non-negative");
        return false;
    }
    return true;
}



static bool _visual_transform_kind_valid(DvzVisualTransformKind kind)
{
    switch (kind)
    {
    case DVZ_VISUAL_TRANSFORM_NONE:
    case DVZ_VISUAL_TRANSFORM_LINEAR:
    case DVZ_VISUAL_TRANSFORM_NONLINEAR:
    case DVZ_VISUAL_TRANSFORM_CUSTOM:
        return true;
    default:
        return false;
    }
}



static bool _visual_transform_space_valid(DvzVisualTransformSpace space)
{
    switch (space)
    {
    case DVZ_VISUAL_TRANSFORM_SPACE_DATA:
    case DVZ_VISUAL_TRANSFORM_SPACE_VISUAL:
    case DVZ_VISUAL_TRANSFORM_SPACE_PANEL:
        return true;
    default:
        return false;
    }
}



static bool _visual_transform_desc_validate(const DvzVisualTransformDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzVisualTransformDesc, DVZ_VISUAL_TRANSFORM_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzVisualTransformDesc ABI prologue");
        return false;
    }
    if (!_visual_transform_kind_valid(desc->kind))
    {
        log_error("invalid DvzVisualTransformDesc kind");
        return false;
    }
    if (
        !_visual_transform_space_valid(desc->input_space) ||
        !_visual_transform_space_valid(desc->output_space))
    {
        log_error("invalid DvzVisualTransformDesc space");
        return false;
    }
    return true;
}



static bool _visual_shader_kind_valid(DvzVisualShaderKind kind)
{
    switch (kind)
    {
    case DVZ_VISUAL_SHADER_NONE:
    case DVZ_VISUAL_SHADER_CUSTOM_FAMILY:
    case DVZ_VISUAL_SHADER_BUILTIN_REPLACEMENT:
        return true;
    default:
        return false;
    }
}



static bool _visual_shader_source_valid(DvzVisualShaderSource source)
{
    switch (source)
    {
    case DVZ_VISUAL_SHADER_SOURCE_NONE:
    case DVZ_VISUAL_SHADER_SOURCE_GLSL:
    case DVZ_VISUAL_SHADER_SOURCE_WGSL:
    case DVZ_VISUAL_SHADER_SOURCE_SPIRV:
        return true;
    default:
        return false;
    }
}



static bool _visual_shader_desc_validate(const DvzVisualShaderDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzVisualShaderDesc, DVZ_VISUAL_SHADER_DESC_KNOWN_FLAGS))
    {
        log_error("invalid DvzVisualShaderDesc ABI prologue");
        return false;
    }
    if (!_visual_shader_kind_valid(desc->kind))
    {
        log_error("invalid DvzVisualShaderDesc kind");
        return false;
    }
    if (
        !_visual_shader_source_valid(desc->vertex_source) ||
        !_visual_shader_source_valid(desc->fragment_source))
    {
        log_error("invalid DvzVisualShaderDesc source");
        return false;
    }
    if (
        (desc->vertex_code == NULL && desc->vertex_code_size != 0) ||
        (desc->vertex_code != NULL && desc->vertex_code_size == 0) ||
        (desc->fragment_code == NULL && desc->fragment_code_size != 0) ||
        (desc->fragment_code != NULL && desc->fragment_code_size == 0))
    {
        log_error("invalid DvzVisualShaderDesc code payload");
        return false;
    }
    if (
        desc->kind == DVZ_VISUAL_SHADER_NONE &&
        (desc->vertex_source != DVZ_VISUAL_SHADER_SOURCE_NONE ||
         desc->fragment_source != DVZ_VISUAL_SHADER_SOURCE_NONE || desc->shader_id != 0 ||
         desc->family != NULL || desc->variant != NULL || desc->vertex_code != NULL ||
         desc->fragment_code != NULL))
    {
        log_error("DvzVisualShaderDesc payload requires a non-NONE shader kind");
        return false;
    }
    return true;
}



DvzVisualAttachDesc dvz_visual_attach_desc(void)
{
    DvzVisualAttachDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualAttachDesc),
        .z_layer = 0,
        .controller_mode = DVZ_CONTROLLER_APPLY,
        .coord_space = DVZ_VISUAL_COORD_DATA,
        .clip_rect = DVZ_VISUAL_CLIP_AUTO,
        .viewport_rect = DVZ_VISUAL_VIEWPORT_AUTO,
    };
    return desc;
}


DvzVisualTransformDesc dvz_visual_transform_desc(void)
{
    DvzVisualTransformDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualTransformDesc),
        .kind = DVZ_VISUAL_TRANSFORM_NONE,
        .input_space = DVZ_VISUAL_TRANSFORM_SPACE_DATA,
        .output_space = DVZ_VISUAL_TRANSFORM_SPACE_VISUAL,
    };
    glm_mat4_identity(desc.matrix);
    return desc;
}


bool dvz_ffi_visual_transform_desc(DvzVisualTransformDesc* out)
{
    if (out == NULL)
        return false;
    *out = dvz_visual_transform_desc();
    return true;
}



DvzVisualShaderDesc dvz_visual_shader_desc(void)
{
    DvzVisualShaderDesc desc = {
        DVZ_STRUCT_INIT_FIELDS(DvzVisualShaderDesc),
        .kind = DVZ_VISUAL_SHADER_NONE,
        .vertex_source = DVZ_VISUAL_SHADER_SOURCE_NONE,
        .fragment_source = DVZ_VISUAL_SHADER_SOURCE_NONE,
    };
    return desc;
}



DvzPanelBackgroundDesc dvz_panel_background_desc(void)
{
    DvzPanelBackgroundDesc desc = {DVZ_STRUCT_INIT_FIELDS(DvzPanelBackgroundDesc)};
    desc.type = DVZ_PANEL_BACKGROUND_NONE;
    desc.color[3] = 1.0f;
    desc.gradient.color0[3] = 1.0f;
    desc.gradient.color1[3] = 1.0f;
    return desc;
}


bool dvz_ffi_panel_background_desc(DvzPanelBackgroundDesc* out)
{
    if (out == NULL)
        return false;
    *out = dvz_panel_background_desc();
    return true;
}


DvzPanelBorderDesc dvz_panel_border_desc(void)
{
    return (DvzPanelBorderDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelBorderDesc),
        .visible = true,
        .color = {91, 118, 139, 255},
        .width_px = 1.0f,
        .inset_px = 0.5f,
    };
}



DvzVisual* _scene_alloc_visual(DvzScene* scene, DvzVisualType type, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
    DvzVisualFamilyState* state = dvz_calloc(1, sizeof(DvzVisualFamilyState));
    if (state == NULL)
    {
        scene->visual_count--;
        return NULL;
    }
    visual->scene = scene;
    visual->id = _scene_next_id(scene);
    visual->type = type;
    visual->ops = _scene_visual_family_ops(type);
    visual->family_state = state;
    visual->flags = flags;
    visual->visible = true;
    visual->z_layer = 0;
    visual->alpha_mode = DVZ_ALPHA_OPAQUE;
    visual->depth_test_enabled = true;
    visual->depth_compare_op = DVZ_COMPARE_OP_LESS_OR_EQUAL;
    visual->transform_desc = dvz_visual_transform_desc();
    visual->shader_desc = dvz_visual_shader_desc();
    glm_mat4_identity(visual->local_transform);
    visual->has_local_transform = false;
    visual->local_transform_version = 1;
    _material_state_default(&visual->material, type);
    _material_params_default(&state->material_params);
    _material_params_sync_state(&state->material_params, &visual->material);
    if (visual->ops != NULL && visual->ops->init_state != NULL)
        visual->ops->init_state(visual);
    return visual;
}



/**
 * Reset one visual slot and optionally release owned subordinate resources.
 *
 * @param visual the visual slot
 * @param release_owned_resources whether owned bindings should be destroyed
 */
void _scene_visual_reset(DvzVisual* visual, bool release_owned_resources)
{
    if (visual == NULL)
        return;
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (visual->ops != NULL && visual->ops->reset_state != NULL)
        visual->ops->reset_state(visual);
    if (state != NULL)
    {
        _stroke_quad_gpu_cache_free(&state->segment.gpu);
        _path_stroke_gpu_cache_free(&state->path.gpu);
        _stroke_quad_gpu_cache_free(&state->vector.stroke_gpu);
        _path_stroke_gpu_cache_free(&state->vector.path_gpu);
        _image_gpu_cache_free(&state->image_gpu);
        dvz_free(state->path.subpath_lengths);
        state->path.subpath_lengths = NULL;
        state->path.subpath_count = 0;
        dvz_free(state->vector.subpath_lengths);
        state->vector.subpath_lengths = NULL;
        state->vector.subpath_count = 0;
    }
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (visual->attrs[i].data != NULL)
        {
            dvz_free(visual->attrs[i].data);
            visual->attrs[i].data = NULL;
        }
    }
    if (release_owned_resources)
    {
        _scene_release_visual_field(visual);
        _scene_release_visual_buffer(visual);
        _scene_release_visual_scale(visual);
    }
    else
    {
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_FIELD);
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_BUFFER);
        _visual_binding_clear(visual, DVZ_VISUAL_BINDING_SCALE);
        if (state != NULL && state->texture.upload != NULL)
        {
            dvz_free(state->texture.upload);
            state->texture.upload = NULL;
            state->texture.upload_size = 0;
        }
        _scene_visual_texture_mark_clean(visual);
    }
    if (visual->link_keys != NULL)
    {
        dvz_free(visual->link_keys);
        visual->link_keys = NULL;
    }
    if (state != NULL && state->texture.rgba != NULL)
    {
        dvz_free(state->texture.rgba);
        state->texture.rgba = NULL;
        state->texture.rgba_size = 0;
    }
    if (state != NULL && state->texture.label_lookup != NULL)
    {
        dvz_free(state->texture.label_lookup);
        state->texture.label_lookup = NULL;
        state->texture.label_lookup_size = 0;
    }
    if (state != NULL && state->text.strings != NULL)
    {
        for (uint32_t i = 0; i < state->text.string_count; i++)
            dvz_free(state->text.strings[i]);
        dvz_free(state->text.strings);
        state->text.strings = NULL;
    }
    if (state != NULL)
    {
        dvz_free(state->text.spans);
        state->text.spans = NULL;
        dvz_free(state);
        visual->family_state = NULL;
    }
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
}



/*************************************************************************************************/
/*  Family lifecycle helpers                                                                     */
/*************************************************************************************************/

/**
 * Initialize point-style material parameters for point-like visuals.
 *
 * @param visual the visual
 */
void _scene_visual_init_point_style(DvzVisual* visual)
{
    ANN(visual);
    _point_style_sync_params(
        &_visual_family_state(visual)->material_params, &visual->material.point_style);
}



/**
 * Initialize segment retained state.
 *
 * @param visual the visual
 */
void _scene_segment_visual_init_state(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->segment.start_cap = DVZ_SEGMENT_CAP_BUTT;
    _visual_family_state(visual)->segment.end_cap = DVZ_SEGMENT_CAP_BUTT;
    _segment_sync_params(visual);
}



/**
 * Initialize path retained state.
 *
 * @param visual the visual
 */
void _scene_path_visual_init_state(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->path.cap_start = DVZ_SEGMENT_CAP_ROUND;
    _visual_family_state(visual)->path.cap_end = DVZ_SEGMENT_CAP_ROUND;
    _visual_family_state(visual)->path.join = DVZ_PATH_JOIN_ROUND;
    _visual_family_state(visual)->path.miter_limit = 4.0f;
    _path_sync_params(visual);
}



/**
 * Initialize vector retained state.
 *
 * @param visual the visual
 */
void _scene_vector_visual_init_state(DvzVisual* visual)
{
    ANN(visual);
    _visual_family_state(visual)->vector.scale = 1.0f;
    _visual_family_state(visual)->vector.anchor = DVZ_VECTOR_ANCHOR_TAIL;
    _visual_family_state(visual)->vector.start_cap = DVZ_SEGMENT_CAP_NONE;
    _visual_family_state(visual)->vector.end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
    _visual_family_state(visual)->vector.join = DVZ_PATH_JOIN_ROUND;
    _visual_family_state(visual)->vector.miter_limit = 4.0f;
    _vector_sync_params(visual);
}



/**
 * Initialize labels retained state.
 *
 * @param visual the visual
 */
void _scene_labels_visual_init_state(DvzVisual* visual)
{
    ANN(visual);
    _labels_state_default(&_visual_family_state(visual)->labels);
}



/**
 * Initialize volume retained state.
 *
 * @param visual the visual
 */
void _scene_volume_visual_init_state(DvzVisual* visual)
{
    ANN(visual);
    _volume_state_default(&_visual_family_state(visual)->volume);
}



/**
 * Reset text family state before generic visual cleanup runs.
 *
 * @param visual the visual
 */
void _scene_text_visual_reset_state(DvzVisual* visual)
{
    ANN(visual);
    DvzVisualFamilyState* state = _visual_family_state(visual);
    if (state != NULL && state->text.glyph_visual != NULL)
        dvz_visual_set_visible(state->text.glyph_visual, false);
}



/**
 * Return the scene-local public id of one scene visual.
 *
 * @param scene the scene
 * @param visual the visual
 * @return the public visual id, or zero when absent
 */
uint64_t _scene_visual_public_id(const DvzScene* scene, const DvzVisual* visual)
{
    ANN(scene);
    ANN(visual);
    return visual->scene == scene ? visual->id : DVZ_ID_NONE;
}



/**
 * Return panel visual attachment indices sorted by draw order.
 *
 * @param panel the panel
 * @param order output attachment-order index array
 */
void _scene_panel_visual_order(const DvzPanel* panel, uint32_t* order)
{
    ANN(panel);
    ANN(order);
    for (uint32_t i = 0; i < panel->visual_count; i++)
        order[i] = i;
    for (uint32_t i = 1; i < panel->visual_count; i++)
    {
        uint32_t cur = order[i];
        int32_t cur_z = panel->visuals[cur].z_layer;
        uint32_t cur_ins = panel->visuals[cur].insertion_index;
        uint32_t j = i;
        while (j > 0)
        {
            uint32_t prev = order[j - 1];
            int32_t prev_z = panel->visuals[prev].z_layer;
            uint32_t prev_ins = panel->visuals[prev].insertion_index;
            if (prev_z < cur_z || (prev_z == cur_z && prev_ins <= cur_ins))
                break;
            order[j] = order[j - 1];
            j--;
        }
        order[j] = cur;
    }
}



/**
 * Convert a normalized color component to an unsigned byte.
 *
 * @param value normalized component value
 * @return clamped byte value
 */
static uint8_t _panel_background_u8(float value)
{
    if (!isfinite(value))
        value = 0.0f;
    if (value < 0.0f)
        value = 0.0f;
    if (value > 1.0f)
        value = 1.0f;
    return (uint8_t)(value * 255.0f + 0.5f);
}



/**
 * Convert normalized float RGBA values to a DvzColor.
 *
 * @param rgba normalized RGBA values
 * @param out output color
 */
static void _panel_background_color(const float rgba[4], DvzColor* out)
{
    ANN(rgba);
    ANN(out);
    *out = dvz_color_rgba(
        _panel_background_u8(rgba[0]), //
        _panel_background_u8(rgba[1]), //
        _panel_background_u8(rgba[2]), //
        _panel_background_u8(rgba[3]));
}



/**
 * Detach the current background visual from one panel.
 *
 * @param panel the panel
 */
static void _panel_background_detach(DvzPanel* panel)
{
    ANN(panel);
    if (panel->background_visual == NULL)
    {
        panel->background_type = DVZ_PANEL_BACKGROUND_NONE;
        return;
    }

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual != panel->background_visual)
            continue;
        for (uint32_t j = i + 1; j < panel->visual_count; j++)
            panel->visuals[j - 1] = panel->visuals[j];
        panel->visual_count--;
        for (uint32_t j = 0; j < panel->visual_count; j++)
            panel->visuals[j].insertion_index = j;
        break;
    }

    dvz_visual_destroy(panel->background_visual);
    panel->background_visual = NULL;
    panel->background_type = DVZ_PANEL_BACKGROUND_NONE;
    if (panel->figure != NULL)
        _scene_notify_request_frame(panel->figure);
}


/**
 * Detach the current border visual from one panel.
 *
 * @param panel the panel
 */
static void _panel_border_detach(DvzPanel* panel)
{
    ANN(panel);
    if (panel->border_visual == NULL)
    {
        panel->border.visible = false;
        return;
    }

    for (uint32_t i = 0; i < panel->visual_count; i++)
    {
        if (panel->visuals[i].visual != panel->border_visual)
            continue;
        for (uint32_t j = i + 1; j < panel->visual_count; j++)
            panel->visuals[j - 1] = panel->visuals[j];
        panel->visual_count--;
        for (uint32_t j = 0; j < panel->visual_count; j++)
            panel->visuals[j].insertion_index = j;
        break;
    }

    dvz_visual_destroy(panel->border_visual);
    panel->border_visual = NULL;
    panel->border.visible = false;
    if (panel->figure != NULL)
        _scene_notify_request_frame(panel->figure);
}


/**
 * Fill one panel border segment payload from a descriptor.
 *
 * @param panel the panel
 * @param border border descriptor
 * @param starts output start positions
 * @param ends output end positions
 * @param colors output colors
 * @param widths output widths
 * @return whether the payload was finite
 */
static bool _panel_border_payload(
    const DvzPanel* panel, const DvzPanelBorderDesc* border, vec3 starts[4], vec3 ends[4],
    DvzColor colors[4], float widths[4])
{
    ANN(panel);
    ANN(border);
    ANN(starts);
    ANN(ends);
    ANN(colors);
    ANN(widths);

    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    if (width <= 0.0f || height <= 0.0f)
        return false;

    const float inset = border->inset_px;
    float x0 = -1.0f + 2.0f * inset / width;
    float x1 = +1.0f - 2.0f * inset / width;
    float y0 = -1.0f + 2.0f * inset / height;
    float y1 = +1.0f - 2.0f * inset / height;
    if (!isfinite(x0) || !isfinite(x1) || !isfinite(y0) || !isfinite(y1) || x0 > x1 || y0 > y1)
        return false;

    starts[0][0] = x0;
    starts[0][1] = y0;
    starts[0][2] = 0.0f;
    ends[0][0] = x1;
    ends[0][1] = y0;
    ends[0][2] = 0.0f;

    starts[1][0] = x1;
    starts[1][1] = y0;
    starts[1][2] = 0.0f;
    ends[1][0] = x1;
    ends[1][1] = y1;
    ends[1][2] = 0.0f;

    starts[2][0] = x1;
    starts[2][1] = y1;
    starts[2][2] = 0.0f;
    ends[2][0] = x0;
    ends[2][1] = y1;
    ends[2][2] = 0.0f;

    starts[3][0] = x0;
    starts[3][1] = y1;
    starts[3][2] = 0.0f;
    ends[3][0] = x0;
    ends[3][1] = y0;
    ends[3][2] = 0.0f;

    for (uint32_t i = 0; i < 4; i++)
    {
        colors[i] = border->color;
        widths[i] = border->width_px;
    }
    return true;
}



/**
 * Fill the four fullscreen background quad colors from a linear gradient.
 *
 * @param background background descriptor
 * @param colors output vertex colors
 */
static void
_panel_background_gradient_colors(const DvzPanelBackgroundDesc* background, DvzColor colors[4])
{
    ANN(background);
    ANN(colors);

    const float* start = background->gradient.start;
    const float* end = background->gradient.end;
    float dx = end[0] - start[0];
    float dy = end[1] - start[1];
    float len2 = dx * dx + dy * dy;
    const float points[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    for (uint32_t i = 0; i < 4; i++)
    {
        float t = 0.0f;
        if (len2 > FLT_EPSILON && isfinite(len2))
            t = ((points[i][0] - start[0]) * dx + (points[i][1] - start[1]) * dy) / len2;
        if (!isfinite(t))
            t = 0.0f;
        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        float rgba[4] = {0};
        for (uint32_t c = 0; c < 4; c++)
        {
            rgba[c] = background->gradient.color0[c] * (1.0f - t) +
                      background->gradient.color1[c] * t;
        }
        _panel_background_color(rgba, &colors[i]);
    }
}



/**
 * Attach a visual as the panel background.
 *
 * @param panel the panel
 * @param visual the visual
 * @param type background type represented by the visual
 * @return whether the visual was attached
 */
static bool _panel_background_attach(
    DvzPanel* panel, DvzVisual* visual, DvzPanelBackgroundType type)
{
    ANN(panel);
    ANN(visual);
    if (_scene_panel_add_generated_visual(
            panel, visual, DVZ_GENERATED_VISUAL_PANEL_BACKGROUND, 0) != 0)
    {
        return false;
    }
    panel->background_visual = visual;
    panel->background_type = type;
    return true;
}



/**
 * Mark panel colorbar adornments dirty after a background change.
 *
 * @param panel the panel
 */
static void _panel_background_mark_colorbars_dirty(DvzPanel* panel)
{
    ANN(panel);
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
        _scene_mark_colorbar_dirty(panel->colorbars[i]);
}



/**
 * Clear one visual scale binding.
 *
 * @param visual the visual
 */
void _scene_release_visual_scale(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_SCALE);
}



/**
 * Attach one scene visual to a panel.
 *
 * @param panel the panel
 * @param visual the visual
 * @param desc optional attachment descriptor
 * @return 0 on success, -1 on error
 */
int dvz_panel_add_visual(DvzPanel* panel, DvzVisual* visual, const DvzVisualAttachDesc* desc)
{
    ANN(panel);
    ANN(visual);
    if (!_visual_attach_desc_validate(desc))
        return -1;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    if (visual->scene != panel->figure->scene)
        return -1;
    if (panel->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return -1;
    DvzVisualAttachDesc resolved = desc != NULL ? *desc : dvz_visual_attach_desc();
    DvzPanelAttach* slot = &panel->visuals[panel->visual_count];
    slot->visual = visual;
    slot->z_layer = resolved.z_layer;
    slot->controller_mode = resolved.controller_mode;
    slot->coord_space = resolved.coord_space;
    slot->clip_rect = resolved.clip_rect;
    slot->viewport_rect = resolved.viewport_rect;
    slot->insertion_index = panel->visual_count;
    panel->visual_count++;
    _scene_notify_request_frame(panel->figure);
    return 0;
}



/**
 * Clear a panel background.
 *
 * @param panel the panel
 */
void dvz_panel_clear_background(DvzPanel* panel)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return;
    DvzScene* scene = panel->figure->scene;
    if (!_scene_visual_mutation_allowed(scene, "clear panel background"))
        return;
    _panel_background_detach(panel);
}



/**
 * Set or update a panel background.
 *
 * @param panel the panel
 * @param background the background descriptor, or NULL to clear
 * @return whether the background was updated
 */
bool dvz_panel_set_background(DvzPanel* panel, const DvzPanelBackgroundDesc* background)
{
    ANN(panel);
    if (!_panel_background_desc_validate(background))
        return false;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    DvzScene* scene = panel->figure->scene;
    if (!_scene_visual_mutation_allowed(scene, "set panel background"))
        return false;

    if (background == NULL || background->type == DVZ_PANEL_BACKGROUND_NONE)
    {
        _panel_background_detach(panel);
        _panel_background_mark_colorbars_dirty(panel);
        return true;
    }

    /* Fullscreen quad in clip space, TRIANGLE_STRIP order (TL, BL, TR, BR). The visual is
     * attached with controller_mode=FIXED so the panzoom/arcball MVP doesn't move it,
     * and z_layer=-1 so it draws behind every default-layer visual. */
    static const float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f, /* TL */
        -1.0f, -1.0f, 0.0f, /* BL */
        +1.0f, +1.0f, 0.0f, /* TR */
        +1.0f, -1.0f, 0.0f, /* BR */
    };

    if (background->type == DVZ_PANEL_BACKGROUND_COLOR ||
        background->type == DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT)
    {
        DvzColor colors[4] = {0};
        if (background->type == DVZ_PANEL_BACKGROUND_COLOR)
        {
            DvzColor color = {0};
            _panel_background_color(background->color, &color);
            for (uint32_t i = 0; i < 4; i++)
                colors[i] = color;
        }
        else
        {
            _panel_background_gradient_colors(background, colors);
        }

        if (panel->background_visual != NULL &&
            panel->background_type != DVZ_PANEL_BACKGROUND_COLOR &&
            panel->background_type != DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT)
        {
            _panel_background_detach(panel);
        }

        if (panel->background_visual == NULL)
        {
            DvzVisual* bg = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
            if (bg == NULL)
            {
                log_error("dvz_panel_set_background: failed to allocate background visual");
                return false;
            }
            if (dvz_visual_set_data(bg, "position", positions, 4) != 0 ||
                dvz_visual_set_data(bg, "color", colors, 4) != 0)
            {
                log_error("dvz_panel_set_background: failed to set background data");
                dvz_visual_destroy(bg);
                return false;
            }
            if (!_panel_background_attach(panel, bg, background->type))
            {
                log_error("dvz_panel_set_background: failed to attach background visual");
                dvz_visual_destroy(bg);
                return false;
            }
        }
        else if (dvz_visual_set_data(panel->background_visual, "color", colors, 4) != 0)
        {
            log_error("dvz_panel_set_background: failed to update background color data");
            return false;
        }
        panel->background_type = background->type;
        _panel_background_mark_colorbars_dirty(panel);
        return true;
    }

    if (background->type == DVZ_PANEL_BACKGROUND_IMAGE)
    {
        if (background->image.rgba == NULL || background->image.width == 0 ||
            background->image.height == 0)
        {
            log_error("dvz_panel_set_background: image background requires non-empty RGBA8 data");
            return false;
        }

        static const float texcoords[4 * 2] = {
            0.0f, 0.0f, /* TL */
            0.0f, 1.0f, /* BL */
            1.0f, 0.0f, /* TR */
            1.0f, 1.0f, /* BR */
        };

        if (panel->background_visual != NULL &&
            panel->background_type != DVZ_PANEL_BACKGROUND_IMAGE)
        {
            _panel_background_detach(panel);
        }

        if (panel->background_visual == NULL)
        {
            DvzVisual* bg = dvz_image(scene, 0);
            if (bg == NULL)
            {
                log_error("dvz_panel_set_background: failed to allocate image background visual");
                return false;
            }
            if (dvz_visual_set_data(bg, "position", positions, 4) != 0 ||
                dvz_visual_set_data(bg, "texcoords", texcoords, 4) != 0 ||
                dvz_visual_set_texture(
                    bg, background->image.rgba, background->image.width,
                    background->image.height) != 0)
            {
                log_error("dvz_panel_set_background: failed to set image background data");
                dvz_visual_destroy(bg);
                return false;
            }
            if (!_panel_background_attach(panel, bg, DVZ_PANEL_BACKGROUND_IMAGE))
            {
                log_error("dvz_panel_set_background: failed to attach image background visual");
                dvz_visual_destroy(bg);
                return false;
            }
        }
        else if (dvz_visual_set_texture(
                     panel->background_visual, background->image.rgba, background->image.width,
                     background->image.height) != 0)
        {
            log_error("dvz_panel_set_background: failed to update image background texture");
            return false;
        }
        panel->background_type = DVZ_PANEL_BACKGROUND_IMAGE;
        _panel_background_mark_colorbars_dirty(panel);
        return true;
    }

    log_error("dvz_panel_set_background: unknown background type");
    return false;
}



/**
 * Set or update the panel background color visual.
 *
 * @param panel the panel
 * @param color RGBA8 background color
 */
void dvz_panel_set_background_color(DvzPanel* panel, DvzColor color)
{
    DvzPanelBackgroundDesc background = dvz_panel_background_desc();
    background.type = DVZ_PANEL_BACKGROUND_COLOR;
    background.color[0] = (float)color.r / 255.0f;
    background.color[1] = (float)color.g / 255.0f;
    background.color[2] = (float)color.b / 255.0f;
    background.color[3] = (float)color.a / 255.0f;
    (void)dvz_panel_set_background(panel, &background);
}


/**
 * Clear a panel border.
 *
 * @param panel the panel
 */
void dvz_panel_clear_border(DvzPanel* panel)
{
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return;
    DvzScene* scene = panel->figure->scene;
    if (!_scene_visual_mutation_allowed(scene, "clear panel border"))
        return;
    _panel_border_detach(panel);
}


/**
 * Set or update a panel border visual.
 *
 * @param panel the panel
 * @param border the border descriptor, or NULL to clear
 * @return whether the border was updated
 */
bool dvz_panel_set_border(DvzPanel* panel, const DvzPanelBorderDesc* border)
{
    ANN(panel);
    if (!_panel_border_desc_validate(border))
        return false;
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    DvzScene* scene = panel->figure->scene;
    if (!_scene_visual_mutation_allowed(scene, "set panel border"))
        return false;

    if (border == NULL || !border->visible || border->width_px == 0.0f)
    {
        _panel_border_detach(panel);
        return true;
    }

    vec3 starts[4] = {{0}};
    vec3 ends[4] = {{0}};
    DvzColor colors[4] = {{0}};
    float widths[4] = {0};
    if (!_panel_border_payload(panel, border, starts, ends, colors, widths))
    {
        log_error("dvz_panel_set_border: failed to resolve border geometry");
        return false;
    }

    if (panel->border_visual == NULL)
    {
        DvzVisual* visual = dvz_segment(scene, 0);
        if (visual == NULL)
        {
            log_error("dvz_panel_set_border: failed to allocate border visual");
            return false;
        }
        if (dvz_segment_set_caps(visual, DVZ_SEGMENT_CAP_SQUARE, DVZ_SEGMENT_CAP_SQUARE) != 0 ||
            dvz_visual_set_depth_test(visual, false) != 0)
        {
            log_error("dvz_panel_set_border: failed to configure border visual");
            dvz_visual_destroy(visual);
            return false;
        }

        DvzVisualDataUpdate updates[4] = {
            {.attr_name = "position_start", .data = starts, .item_count = 4},
            {.attr_name = "position_end", .data = ends, .item_count = 4},
            {.attr_name = "color", .data = colors, .item_count = 4},
            {.attr_name = "line_width", .data = widths, .item_count = 4},
        };
        if (dvz_visual_set_data_many(visual, updates, 4) != 0)
        {
            log_error("dvz_panel_set_border: failed to set border data");
            dvz_visual_destroy(visual);
            return false;
        }

        if (_scene_panel_add_generated_visual(
                panel, visual, DVZ_GENERATED_VISUAL_PANEL_BORDER, 0) != 0)
        {
            log_error("dvz_panel_set_border: failed to attach border visual");
            dvz_visual_destroy(visual);
            return false;
        }
        panel->border_visual = visual;
    }
    else
    {
        DvzVisualDataUpdate updates[4] = {
            {.attr_name = "position_start", .data = starts, .item_count = 4},
            {.attr_name = "position_end", .data = ends, .item_count = 4},
            {.attr_name = "color", .data = colors, .item_count = 4},
            {.attr_name = "line_width", .data = widths, .item_count = 4},
        };
        if (dvz_visual_set_data_many(panel->border_visual, updates, 4) != 0)
        {
            log_error("dvz_panel_set_border: failed to update border data");
            return false;
        }
    }

    panel->border = *border;
    dvz_visual_set_visible(panel->border_visual, true);
    _scene_notify_request_frame(panel->figure);
    return true;
}


/**
 * Refresh one panel border visual after pixel-size changes.
 *
 * @param panel the panel
 * @return whether the refresh succeeded or no border exists
 */
bool _scene_panel_refresh_border(DvzPanel* panel)
{
    if (panel == NULL || panel->border_visual == NULL || !panel->border.visible)
        return true;

    vec3 starts[4] = {{0}};
    vec3 ends[4] = {{0}};
    DvzColor colors[4] = {{0}};
    float widths[4] = {0};
    if (!_panel_border_payload(panel, &panel->border, starts, ends, colors, widths))
        return false;

    DvzVisualDataUpdate updates[4] = {
        {.attr_name = "position_start", .data = starts, .item_count = 4},
        {.attr_name = "position_end", .data = ends, .item_count = 4},
        {.attr_name = "color", .data = colors, .item_count = 4},
        {.attr_name = "line_width", .data = widths, .item_count = 4},
    };
    if (dvz_visual_set_data_many(panel->border_visual, updates, 4) != 0)
        return false;
    return true;
}



/*************************************************************************************************/
/*  Visual lifecycle and data                                                                    */
/*************************************************************************************************/

/**
 * Set the query capabilities exposed by a visual.
 *
 * @param visual the visual
 * @param capabilities the capability mask
 */
void dvz_visual_set_query_capabilities(DvzVisual* visual, uint32_t capabilities)
{
    ANN(visual);
    visual->query_capabilities = capabilities;
}



/**
 * Bind link keys for one visual on one scene link channel.
 *
 * @param visual the visual
 * @param channel the link channel
 * @param link_keys per-item link keys
 * @param item_count the number of link keys
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_link_keys(
    DvzVisual* visual, DvzLinkChannel* channel, const uint64_t* link_keys, uint32_t item_count)
{
    ANN(visual);
    if (channel == NULL || channel->scene != visual->scene)
    {
        log_error("cannot bind link keys with a channel from a different scene");
        return -1;
    }
    if (item_count > 0 && link_keys == NULL)
    {
        log_error("link key array is NULL");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind link keys"))
        return -1;
    if (visual->link_keys != NULL)
    {
        dvz_free(visual->link_keys);
        visual->link_keys = NULL;
    }
    visual->link_channel = channel;
    visual->link_key_count = item_count;
    if (item_count == 0)
        return 0;
    visual->link_keys = (uint64_t*)dvz_calloc(item_count, sizeof(uint64_t));
    if (visual->link_keys == NULL)
    {
        visual->link_key_count = 0;
        return -1;
    }
    dvz_memcpy(
        visual->link_keys, item_count * sizeof(uint64_t), link_keys,
        item_count * sizeof(uint64_t));
    return 0;
}



/**
 * Destroy one scene-owned visual slot.
 *
 * @param visual the visual
 */
void dvz_visual_destroy(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    if (!_scene_visual_mutation_allowed(visual->scene, "destroy scene-owned visual data"))
        return;
    _scene_visual_reset(visual, true);
}


DvzId dvz_visual_id(const DvzVisual* visual)
{
    return visual != NULL && visual->scene != NULL ? visual->id : DVZ_ID_NONE;
}



/**
 * Set visual visibility.
 *
 * @param visual the visual
 * @param visible whether the visual should be visible
 */
void dvz_visual_set_visible(DvzVisual* visual, bool visible)
{
    ANN(visual);
    visual->visible = visible;
    _scene_notify_visual_changed(visual);
}



/**
 * Set the retained visual-local transform.
 *
 * @param visual the visual
 * @param transform retained local model transform
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_set_transform(DvzVisual* visual, mat4 transform)
{
    ANN(visual);
    ANN(transform);
    if (!_scene_visual_mutation_allowed(visual->scene, "set visual transform"))
        return -1;
    for (uint32_t col = 0; col < 4; col++)
        for (uint32_t row = 0; row < 4; row++)
            visual->local_transform[col][row] = transform[col][row];
    visual->has_local_transform = true;
    _visual_bump_version(&visual->local_transform_version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Return whether a visual has a retained local transform.
 *
 * @param visual the visual
 * @return whether a local transform is retained
 */
bool dvz_visual_has_transform(const DvzVisual* visual)
{
    ANN(visual);
    return visual->has_local_transform;
}



/**
 * Copy the retained visual-local transform.
 *
 * @param visual the visual
 * @param out output local model transform
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_get_transform(const DvzVisual* visual, mat4 out)
{
    ANN(visual);
    ANN(out);
    if (visual->has_local_transform)
    {
        for (uint32_t col = 0; col < 4; col++)
            for (uint32_t row = 0; row < 4; row++)
                out[col][row] = visual->local_transform[col][row];
    }
    else
        glm_mat4_identity(out);
    return 0;
}



/**
 * Clear the retained visual-local transform.
 *
 * @param visual the visual
 * @return 0 on success, -1 on validation error
 */
int dvz_visual_clear_transform(DvzVisual* visual)
{
    ANN(visual);
    if (!_scene_visual_mutation_allowed(visual->scene, "clear visual transform"))
        return -1;
    glm_mat4_identity(visual->local_transform);
    visual->has_local_transform = false;
    _visual_bump_version(&visual->local_transform_version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the future scene-managed visual transform descriptor.
 *
 * @param visual the visual
 * @param desc transform descriptor, or NULL to clear the future transform slot
 * @return 0 on success, -1 on validation error or unsupported transform kind
 */
int dvz_visual_set_transform_desc(DvzVisual* visual, const DvzVisualTransformDesc* desc)
{
    ANN(visual);
    DvzVisualTransformDesc default_desc = dvz_visual_transform_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_visual_transform_desc_validate(desc))
        return -1;
    if (!_scene_visual_mutation_allowed(visual->scene, "set future visual transform descriptor"))
        return -1;
    if (desc->kind != DVZ_VISUAL_TRANSFORM_NONE)
    {
        log_error("scene-managed nonlinear/custom visual transforms are deferred in v0.4");
        return -1;
    }
    visual->transform_desc = *desc;
    return 0;
}



/**
 * Set the future visual shader descriptor.
 *
 * @param visual the visual
 * @param desc shader descriptor, or NULL to clear the future shader slot
 * @return 0 on success, -1 on validation error or unsupported shader kind
 */
int dvz_visual_set_shader_desc(DvzVisual* visual, const DvzVisualShaderDesc* desc)
{
    ANN(visual);
    DvzVisualShaderDesc default_desc = dvz_visual_shader_desc();
    if (desc == NULL)
        desc = &default_desc;
    if (!_visual_shader_desc_validate(desc))
        return -1;
    if (!_scene_visual_mutation_allowed(visual->scene, "set future visual shader descriptor"))
        return -1;
    if (desc->kind != DVZ_VISUAL_SHADER_NONE)
    {
        log_error("custom visual shaders and built-in shader replacement are deferred in v0.4");
        return -1;
    }
    visual->shader_desc = *desc;
    return 0;
}
