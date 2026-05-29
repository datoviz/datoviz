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
#include "core/scene_notify_internal.h"
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "bindings_internal.h"
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
    visual->type = type;
    visual->ops = _scene_visual_family_ops(type);
    visual->family_state = state;
    visual->flags = flags;
    visual->visible = true;
    visual->z_layer = 0;
    visual->alpha_mode = DVZ_ALPHA_OPAQUE;
    visual->depth_test_enabled = true;
    visual->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
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
 * Return the public one-based id of one scene visual.
 *
 * @param scene the scene
 * @param visual the visual
 * @return the public visual id, or zero when absent
 */
uint64_t _scene_visual_public_id(const DvzScene* scene, const DvzVisual* visual)
{
    ANN(scene);
    ANN(visual);
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        if (&scene->visuals[i] == visual)
            return (uint64_t)i + 1;
    }
    return 0;
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
    if (dvz_panel_add_visual(
            panel, visual,
            &(DvzVisualAttachDesc){.z_layer = -1, .controller_mode = DVZ_CONTROLLER_FIXED}) != 0)
    {
        return false;
    }
    panel->background_visual = visual;
    panel->background_type = type;
    return true;
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
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    if (visual->scene != panel->figure->scene)
        return -1;
    if (panel->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return -1;
    DvzPanelAttach* slot = &panel->visuals[panel->visual_count];
    slot->visual = visual;
    slot->z_layer = desc ? desc->z_layer : 0;
    slot->controller_mode = desc ? desc->controller_mode : DVZ_CONTROLLER_APPLY;
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
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return false;
    DvzScene* scene = panel->figure->scene;
    if (!_scene_visual_mutation_allowed(scene, "set panel background"))
        return false;

    if (background == NULL || background->type == DVZ_PANEL_BACKGROUND_NONE)
    {
        _panel_background_detach(panel);
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
        return true;
    }

    log_error("dvz_panel_set_background: unknown background type");
    return false;
}



/**
 * Set or update the panel background color visual.
 *
 * @param panel the panel
 * @param r red channel in normalized units
 * @param g green channel in normalized units
 * @param b blue channel in normalized units
 * @param a alpha channel in normalized units
 */
void dvz_panel_set_background_color(DvzPanel* panel, float r, float g, float b, float a)
{
    DvzPanelBackgroundDesc background = {
        .type = DVZ_PANEL_BACKGROUND_COLOR,
        .color = {r, g, b, a},
    };
    (void)dvz_panel_set_background(panel, &background);
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
