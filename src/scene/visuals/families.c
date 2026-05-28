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
#include <inttypes.h>
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
#include "_scene_resource_key.h"
#include "_visual_internal.h"
#include "stroke/internal.h"
#include "sample_profile.h"
#include "datoviz/scene.h"


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
    visual->scene = scene;
    visual->type = type;
    visual->flags = flags;
    visual->visible = true;
    visual->z_layer = 0;
    visual->alpha_mode = DVZ_ALPHA_OPAQUE;
    visual->depth_test_enabled = true;
    visual->depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    _material_state_default(&visual->material, type);
    _material_params_default(&visual->material_params);
    _material_params_sync_state(&visual->material_params, &visual->material);
    if (type == DVZ_VISUAL_TYPE_POINT || type == DVZ_VISUAL_TYPE_MARKER)
        _point_style_sync_params(&visual->material_params, &visual->material.point_style);
    if (type == DVZ_VISUAL_TYPE_SEGMENT)
    {
        visual->segment.start_cap = DVZ_SEGMENT_CAP_BUTT;
        visual->segment.end_cap = DVZ_SEGMENT_CAP_BUTT;
        _segment_sync_params(visual);
    }
    if (type == DVZ_VISUAL_TYPE_VECTOR)
    {
        visual->vector.scale = 1.0f;
        visual->vector.anchor = DVZ_VECTOR_ANCHOR_TAIL;
        visual->vector.start_cap = DVZ_SEGMENT_CAP_NONE;
        visual->vector.end_cap = DVZ_SEGMENT_CAP_TRIANGLE_OUT;
        visual->vector.join = DVZ_PATH_JOIN_ROUND;
        visual->vector.miter_limit = 4.0f;
        _vector_sync_params(visual);
    }
    if (type == DVZ_VISUAL_TYPE_PATH)
    {
        visual->path.cap_start = DVZ_SEGMENT_CAP_ROUND;
        visual->path.cap_end = DVZ_SEGMENT_CAP_ROUND;
        visual->path.join = DVZ_PATH_JOIN_ROUND;
        visual->path.miter_limit = 4.0f;
        _path_sync_params(visual);
    }
    _labels_state_default(&visual->labels);
    _volume_state_default(&visual->volume);
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
    if (visual->type == DVZ_VISUAL_TYPE_TEXT && visual->text.glyph_visual != NULL)
        dvz_visual_set_visible(visual->text.glyph_visual, false);
    _stroke_quad_gpu_cache_free(&visual->segment.gpu);
    _path_stroke_gpu_cache_free(&visual->path.gpu);
    _stroke_quad_gpu_cache_free(&visual->vector.stroke_gpu);
    _path_stroke_gpu_cache_free(&visual->vector.path_gpu);
    _image_gpu_cache_free(&visual->image_gpu);
    dvz_free(visual->path.subpath_lengths);
    visual->path.subpath_lengths = NULL;
    visual->path.subpath_count = 0;
    dvz_free(visual->vector.subpath_lengths);
    visual->vector.subpath_lengths = NULL;
    visual->vector.subpath_count = 0;
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
        if (visual->texture.upload != NULL)
        {
            dvz_free(visual->texture.upload);
            visual->texture.upload = NULL;
            visual->texture.upload_size = 0;
        }
        _scene_visual_texture_mark_clean(visual);
    }
    if (visual->link_keys != NULL)
    {
        dvz_free(visual->link_keys);
        visual->link_keys = NULL;
    }
    if (visual->texture.rgba != NULL)
    {
        dvz_free(visual->texture.rgba);
        visual->texture.rgba = NULL;
        visual->texture.rgba_size = 0;
    }
    if (visual->texture.label_lookup != NULL)
    {
        dvz_free(visual->texture.label_lookup);
        visual->texture.label_lookup = NULL;
        visual->texture.label_lookup_size = 0;
    }
    if (visual->text.strings != NULL)
    {
        for (uint32_t i = 0; i < visual->text.string_count; i++)
            dvz_free(visual->text.strings[i]);
        dvz_free(visual->text.strings);
        visual->text.strings = NULL;
    }
    dvz_free(visual->text.spans);
    visual->text.spans = NULL;
    dvz_memset(visual, sizeof(DvzVisual), 0, sizeof(DvzVisual));
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



/**
 * Create a labels visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_labels(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_LABELS, flags);
    if (visual != NULL)
    {
        visual->alpha_mode = DVZ_ALPHA_BLENDED;
        visual->depth_test_enabled = false;
    }
    return visual;
}


/**
 * Validate a retained labels visual for state mutation.
 *
 * @param visual the visual
 * @param action mutation action used in diagnostics
 * @return whether the visual can be mutated
 */
static bool _labels_state_mutation_allowed(DvzVisual* visual, const char* action)
{
    ANN(visual);
    ANN(action);
    if (visual->type != DVZ_VISUAL_TYPE_LABELS)
    {
        log_error("%s requires a labels visual", action);
        return false;
    }
    return _scene_visual_mutation_allowed(visual->scene, action);
}



/**
 * Mark labels presentation state dirty.
 *
 * @param visual the labels visual
 */
static void _labels_state_mark_dirty(DvzVisual* visual)
{
    ANN(visual);
    _visual_bump_version(&visual->labels.version);
    visual->image_gpu.dirty = true;
    _scene_notify_visual_changed(visual);
}



/**
 * Set the global opacity multiplier on a labels visual.
 *
 * @param visual the labels visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_opacity(DvzVisual* visual, float opacity)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels opacity"))
        return -1;
    if (!isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
    {
        log_error("labels opacity must be finite and in [0, 1]");
        return -1;
    }
    visual->labels.opacity = opacity;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the transparent background label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id background label ID
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_background(DvzVisual* visual, DvzCategoryId label_id)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels background"))
        return -1;
    visual->labels.background_id = label_id;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @param label_id selected label ID
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_selected(DvzVisual* visual, DvzCategoryId label_id)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels selection"))
        return -1;
    visual->labels.selected_enabled = true;
    visual->labels.selected_id = label_id;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Clear the selected label ID on a labels visual.
 *
 * @param visual the labels visual
 * @return 0 on success, -1 on error
 */
int dvz_labels_clear_selected(DvzVisual* visual)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "clear labels selection"))
        return -1;
    visual->labels.selected_enabled = false;
    visual->labels.selected_id = 0;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the hidden label IDs on a labels visual.
 *
 * @param visual the labels visual
 * @param ids hidden label IDs, or NULL when count is 0
 * @param count hidden label ID count
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_hidden(DvzVisual* visual, const DvzCategoryId* ids, uint32_t count)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set hidden labels"))
        return -1;
    if (count > DVZ_LABELS_MAX_HIDDEN)
    {
        log_error("too many hidden labels (%" PRIu32 " > %u)", count, DVZ_LABELS_MAX_HIDDEN);
        return -1;
    }
    if (count > 0 && ids == NULL)
    {
        log_error("hidden labels ids are NULL");
        return -1;
    }
    dvz_memset(
        visual->labels.hidden_ids, sizeof(visual->labels.hidden_ids), 0,
        sizeof(visual->labels.hidden_ids));
    if (count > 0)
        dvz_memcpy(
            visual->labels.hidden_ids, count * sizeof(DvzCategoryId), ids,
            count * sizeof(DvzCategoryId));
    visual->labels.hidden_count = count;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Configure boundary rendering on a labels visual.
 *
 * @param visual the labels visual
 * @param enabled whether boundary rendering is enabled
 * @param width_px boundary width in pixels
 * @param color boundary color
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_boundary(DvzVisual* visual, bool enabled, float width_px, DvzColor color)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels boundary"))
        return -1;
    if (!isfinite(width_px) || width_px < 0.0f)
    {
        log_error("labels boundary width must be finite and non-negative");
        return -1;
    }
    visual->labels.boundary_enabled = enabled;
    visual->labels.boundary_width_px = width_px;
    visual->labels.boundary_color = color;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the deterministic fallback-color seed on a labels visual.
 *
 * @param visual the labels visual
 * @param seed fallback-color seed
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_fallback_seed(DvzVisual* visual, uint32_t seed)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels fallback seed"))
        return -1;
    visual->labels.fallback_seed = seed;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the first-slice axis for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param axis slice axis
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels slice axis"))
        return -1;
    if (axis != DVZ_VOLUME_AXIS_X && axis != DVZ_VOLUME_AXIS_Y && axis != DVZ_VOLUME_AXIS_Z)
    {
        log_error("unsupported labels slice axis %d", (int)axis);
        return -1;
    }
    visual->labels.slice_axis = axis;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Set the first-slice position for a 3D labels visual.
 *
 * @param visual the labels visual
 * @param position normalized slice position in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_labels_set_slice_position(DvzVisual* visual, double position)
{
    ANN(visual);
    if (!_labels_state_mutation_allowed(visual, "set labels slice position"))
        return -1;
    if (!isfinite(position) || position < 0.0 || position > 1.0)
    {
        log_error("labels slice position must be finite and in [0, 1]");
        return -1;
    }
    visual->labels.slice_position = position;
    _labels_state_mark_dirty(visual);
    return 0;
}



/**
 * Return the retained labels state for inspection.
 *
 * @param visual the labels visual
 * @return the labels state, or NULL on error
 */
const DvzLabelsState* dvz_labels_state(const DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_LABELS)
        return NULL;
    return &visual->labels;
}



/**
 * Create an internal batched text visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* _scene_text_visual(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_TEXT, flags);
    if (visual != NULL)
    {
        visual->alpha_mode = DVZ_ALPHA_BLENDED;
        visual->depth_test_enabled = false;
        visual->text.renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    }
    return visual;
}



/**
 * Select the renderer used by an internal batched text visual.
 *
 * @param visual text visual
 * @param renderer renderer selection
 * @return 0 on success, -1 on error
 */
int _scene_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_TEXT)
    {
        log_error("dvz_text_set_renderer requires a text visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate text renderer"))
        return -1;
    if (renderer != DVZ_TEXT_RENDERER_AUTO && renderer != DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS &&
        renderer != DVZ_TEXT_RENDERER_BITMAP_ATLAS && renderer != DVZ_TEXT_RENDERER_MSDF_ATLAS)
    {
        log_error("text renderer %d is not implemented for batched text visuals yet", renderer);
        return -1;
    }
    if (visual->text.renderer != renderer)
    {
        visual->text.renderer = renderer;
        visual->text.renderer_version++;
        _scene_notify_visual_changed(visual);
    }
    return 0;
}



/**
 * Resolve a generated adornment text renderer to a supported internal renderer.
 *
 * @param renderer requested renderer
 * @return supported renderer for generated labels
 */
DvzTextRenderer _scene_adornment_text_renderer(DvzTextRenderer renderer)
{
    switch (renderer)
    {
    case DVZ_TEXT_RENDERER_AUTO:
    case DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS:
    case DVZ_TEXT_RENDERER_BITMAP_ATLAS:
    case DVZ_TEXT_RENDERER_MSDF_ATLAS:
        return renderer;
    default:
        return DVZ_TEXT_RENDERER_MSDF_ATLAS;
    }
}



/**
 * Create an internal text visual for scene-generated labels.
 *
 * @param scene the scene
 * @param renderer requested renderer
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* _scene_adornment_text_visual(DvzScene* scene, DvzTextRenderer renderer)
{
    ANN(scene);
    DvzVisual* visual = _scene_text_visual(scene, 0);
    if (visual == NULL)
        return NULL;
    if (_scene_adornment_text_visual_set_renderer(visual, renderer) != 0)
        return NULL;
    return visual;
}



/**
 * Select the renderer for an internal scene-generated label text visual.
 *
 * @param visual text visual
 * @param renderer requested renderer
 * @return 0 on success, -1 on error
 */
int _scene_adornment_text_visual_set_renderer(DvzVisual* visual, DvzTextRenderer renderer)
{
    ANN(visual);
    return _scene_text_visual_set_renderer(visual, _scene_adornment_text_renderer(renderer));
}



/**
 * Apply the current retained volume bounds to the proxy cube attributes.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
int _volume_apply_bounds_geometry(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return -1;

    static const float texcoords[36][3] = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
    };
    float positions[36][3] = {0};
    for (uint32_t i = 0; i < 36; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            double min_value = visual->volume.bounds_min[j];
            double max_value = visual->volume.bounds_max[j];
            positions[i][j] = (float)(min_value + (max_value - min_value) * texcoords[i][j]);
        }
    }

    if (dvz_visual_set_data(visual, "position", positions, 36) != 0 ||
        dvz_visual_set_data(visual, "texcoords", texcoords, 36) != 0)
    {
        log_error("dvz_volume: failed to initialize default box geometry");
        return -1;
    }
    return 0;
}


/**
 * Create a volume visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_volume(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_VOLUME, flags);
    if (visual == NULL)
        return NULL;

    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (_volume_apply_bounds_geometry(visual) != 0)
        log_error("dvz_volume: failed to apply default bounds");
    return visual;
}


/**
 * Set the global opacity multiplier on a volume visual.
 *
 * @param visual the volume visual
 * @param opacity opacity multiplier in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_opacity(DvzVisual* visual, float opacity)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_opacity requires a volume visual");
        return -1;
    }
    if (!isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
    {
        log_error("volume opacity must be finite and in [0, 1]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume opacity"))
        return -1;
    visual->volume.opacity = opacity;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the texture sampling mode on a volume visual.
 *
 * @param visual the volume visual
 * @param sampling the sampling mode
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_sampling(DvzVisual* visual, DvzVolumeSamplingMode sampling)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_sampling requires a volume visual");
        return -1;
    }
    if (sampling != DVZ_VOLUME_SAMPLING_LINEAR && sampling != DVZ_VOLUME_SAMPLING_NEAREST)
    {
        log_error("unsupported volume sampling mode %d", (int)sampling);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume sampling"))
        return -1;
    visual->volume.sampling = sampling;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the volume render mode.
 *
 * @param visual the volume visual
 * @param mode the render mode
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_render_mode(DvzVisual* visual, DvzVolumeRenderMode mode)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_render_mode requires a volume visual");
        return -1;
    }
    if (mode != DVZ_VOLUME_RENDER_SLICE && mode != DVZ_VOLUME_RENDER_MIP &&
        mode != DVZ_VOLUME_RENDER_COMPOSITE)
    {
        log_error("unsupported volume render mode %d", (int)mode);
        return -1;
    }
    DvzSceneSampleProfile profile = {0};
    if (
        mode == DVZ_VOLUME_RENDER_MIP && visual->field != NULL &&
        _scene_sample_profile_resolve(
            visual->field->desc.format, visual->field->desc.semantic, visual->field->desc.dim,
            &profile) &&
        _scene_sample_profile_is_integer_label(&profile))
    {
        log_error("label volumes only support slice and composite render modes");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume render mode"))
        return -1;
    visual->volume.render_mode = mode;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the slice axis used by slice rendering.
 *
 * @param visual the volume visual
 * @param axis axis normal for slice plane selection
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_slice_axis(DvzVisual* visual, DvzVolumeAxis axis)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_slice_axis requires a volume visual");
        return -1;
    }
    if (axis < DVZ_VOLUME_AXIS_X || axis > DVZ_VOLUME_AXIS_Z)
    {
        log_error("unsupported volume slice axis %d", (int)axis);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume slice axis"))
        return -1;
    visual->volume.slice_axis = axis;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the normalized slice position for slice rendering.
 *
 * @param visual the volume visual
 * @param position normalized slice coordinate in [0, 1]
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_slice_position(DvzVisual* visual, double position)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_slice_position requires a volume visual");
        return -1;
    }
    if (!isfinite(position) || position < 0.0 || position > 1.0)
    {
        log_error("volume slice position must be finite and in [0, 1]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume slice position"))
        return -1;
    visual->volume.slice_position = position;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the volume raymarch step count used by MIP rendering.
 *
 * @param visual the volume visual
 * @param step_count number of raymarch samples
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_step_count(DvzVisual* visual, uint32_t step_count)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_step_count requires a volume visual");
        return -1;
    }
    if (step_count == 0 || step_count > 1024)
    {
        log_error("volume step count must be in [1, 1024]");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume step count"))
        return -1;
    visual->volume.step_count = step_count;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Set the object-space volume proxy bounds.
 *
 * @param visual the volume visual
 * @param bounds_min minimum object-space coordinate
 * @param bounds_max maximum object-space coordinate
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_bounds(
    DvzVisual* visual, const double bounds_min[3], const double bounds_max[3])
{
    ANN(visual);
    ANN(bounds_min);
    ANN(bounds_max);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_bounds requires a volume visual");
        return -1;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(bounds_min[i]) || !isfinite(bounds_max[i]) || bounds_min[i] >= bounds_max[i])
        {
            log_error("volume bounds must be finite and satisfy min < max");
            return -1;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume bounds"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.bounds_min[i] = bounds_min[i];
        visual->volume.bounds_max[i] = bounds_max[i];
    }
    if (_volume_apply_bounds_geometry(visual) != 0)
        return -1;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the mapping from normalized volume coordinates to texture UVW coordinates.
 *
 * @param visual the volume visual
 * @param axis_order texture-axis source order
 * @param axis_flip optional per-texture-axis flips
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_axis_mapping(
    DvzVisual* visual, const uint32_t axis_order[3], const bool axis_flip[3])
{
    ANN(visual);
    ANN(axis_order);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_axis_mapping requires a volume visual");
        return -1;
    }
    bool seen[3] = {false, false, false};
    for (uint32_t i = 0; i < 3; i++)
    {
        if (axis_order[i] > 2 || seen[axis_order[i]])
        {
            log_error("volume axis order must be a permutation of 0, 1, 2");
            return -1;
        }
        seen[axis_order[i]] = true;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume axis mapping"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.axis_order[i] = axis_order[i];
        visual->volume.axis_flip[i] = axis_flip != NULL ? axis_flip[i] : false;
    }
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set the scalar value range used before transfer texture lookup.
 *
 * @param visual the volume visual
 * @param min minimum scalar value mapped to 0
 * @param max maximum scalar value mapped to 1
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_value_range(DvzVisual* visual, double min, double max)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_value_range requires a volume visual");
        return -1;
    }
    if (!isfinite(min) || !isfinite(max) || min >= max)
    {
        log_error("volume value range must be finite and satisfy min < max");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume value range"))
        return -1;
    visual->volume.value_min = min;
    visual->volume.value_max = max;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Set piecewise-linear opacity stops for scalar volume transfer.
 *
 * @param visual the volume visual
 * @param stops alpha stops
 * @param count number of stops
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_alpha_stops(DvzVisual* visual, const DvzVolumeAlphaStop* stops, uint32_t count)
{
    ANN(visual);
    ANN(stops);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_alpha_stops requires a volume visual");
        return -1;
    }
    if (count == 0 || count > 8)
    {
        log_error("volume alpha stop count must be in [1, 8]");
        return -1;
    }
    DvzVolumeAlphaStop sorted[8] = {0};
    for (uint32_t i = 0; i < count; i++)
    {
        if (!isfinite(stops[i].position) || !isfinite(stops[i].alpha) || stops[i].position < 0.0 ||
            stops[i].position > 1.0 || stops[i].alpha < 0.0f || stops[i].alpha > 1.0f)
        {
            log_error("volume alpha stops require finite position and alpha in [0, 1]");
            return -1;
        }
        sorted[i] = stops[i];
    }
    for (uint32_t i = 1; i < count; i++)
    {
        DvzVolumeAlphaStop stop = sorted[i];
        uint32_t j = i;
        while (j > 0 && sorted[j - 1].position > stop.position)
        {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = stop;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume alpha stops"))
        return -1;
    for (uint32_t i = 0; i < count; i++)
        visual->volume.alpha_stops[i] = sorted[i];
    visual->volume.alpha_stop_count = count;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Enable axis-aligned clipping on a volume visual.
 *
 * @param visual the volume visual
 * @param clip_min minimum normalized clip coordinate
 * @param clip_max maximum normalized clip coordinate
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_clipping_box(
    DvzVisual* visual, const double clip_min[3], const double clip_max[3])
{
    ANN(visual);
    ANN(clip_min);
    ANN(clip_max);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_clipping_box requires a volume visual");
        return -1;
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(clip_min[i]) || !isfinite(clip_max[i]) || clip_min[i] < 0.0 ||
            clip_max[i] > 1.0 || clip_min[i] > clip_max[i])
        {
            log_error("volume clipping box coordinates must satisfy 0 <= min <= max <= 1");
            return -1;
        }
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume clipping box"))
        return -1;
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.clip_min[i] = clip_min[i];
        visual->volume.clip_max[i] = clip_max[i];
    }
    visual->volume.clipping_enabled = true;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Enable arbitrary plane clipping on a volume visual.
 *
 * @param visual the volume visual
 * @param point point on the clipping plane, in normalized volume coordinates
 * @param normal non-zero clipping plane normal
 * @param keep_positive whether to keep the positive side of the plane
 * @return 0 on success, -1 on error
 */
int dvz_volume_set_clipping_plane(
    DvzVisual* visual, const double point[3], const double normal[3], bool keep_positive)
{
    ANN(visual);
    ANN(point);
    ANN(normal);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_set_clipping_plane requires a volume visual");
        return -1;
    }
    double norm2 = 0.0;
    for (uint32_t i = 0; i < 3; i++)
    {
        if (!isfinite(point[i]) || !isfinite(normal[i]) || point[i] < 0.0 || point[i] > 1.0)
        {
            log_error("volume clipping plane point must be finite and in [0, 1]");
            return -1;
        }
        norm2 += normal[i] * normal[i];
    }
    if (norm2 <= 0.0 || !isfinite(norm2))
    {
        log_error("volume clipping plane normal must be finite and non-zero");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "set volume clipping plane"))
        return -1;
    double inv_norm = 1.0 / sqrt(norm2);
    for (uint32_t i = 0; i < 3; i++)
    {
        visual->volume.clip_plane_point[i] = point[i];
        visual->volume.clip_plane_normal[i] = normal[i] * inv_norm;
    }
    visual->volume.clip_plane_keep_positive = keep_positive;
    visual->volume.clip_plane_enabled = true;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}


/**
 * Disable arbitrary plane clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
int dvz_volume_clear_clipping_plane(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_clear_clipping_plane requires a volume visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "clear volume clipping plane"))
        return -1;
    visual->volume.clip_plane_enabled = false;
    visual->volume.clip_plane_keep_positive = false;
    visual->volume.clip_plane_point[0] = 0.5;
    visual->volume.clip_plane_point[1] = 0.5;
    visual->volume.clip_plane_point[2] = 0.5;
    visual->volume.clip_plane_normal[0] = 1.0;
    visual->volume.clip_plane_normal[1] = 0.0;
    visual->volume.clip_plane_normal[2] = 0.0;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Disable all clipping on a volume visual.
 *
 * @param visual the volume visual
 * @return 0 on success, -1 on error
 */
int dvz_volume_clear_clipping(DvzVisual* visual)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_VOLUME)
    {
        log_error("dvz_volume_clear_clipping requires a volume visual");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "clear volume clipping"))
        return -1;
    visual->volume.clipping_enabled = false;
    visual->volume.clip_min[0] = 0.0;
    visual->volume.clip_min[1] = 0.0;
    visual->volume.clip_min[2] = 0.0;
    visual->volume.clip_max[0] = 1.0;
    visual->volume.clip_max[1] = 1.0;
    visual->volume.clip_max[2] = 1.0;
    visual->volume.clip_plane_enabled = false;
    visual->volume.clip_plane_keep_positive = false;
    visual->volume.clip_plane_point[0] = 0.5;
    visual->volume.clip_plane_point[1] = 0.5;
    visual->volume.clip_plane_point[2] = 0.5;
    visual->volume.clip_plane_normal[0] = 1.0;
    visual->volume.clip_plane_normal[1] = 0.0;
    visual->volume.clip_plane_normal[2] = 0.0;
    _visual_bump_version(&visual->volume.version);
    _scene_notify_visual_changed(visual);
    return 0;
}



/**
 * Return the retained volume state for inspection.
 *
 * @param visual the volume visual
 * @return the volume state, or NULL on error
 */
const DvzVolumeState* dvz_volume_state(const DvzVisual* visual)
{
    if (visual == NULL || visual->type != DVZ_VISUAL_TYPE_VOLUME)
        return NULL;
    return &visual->volume;
}



/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * Image and volume visuals accept the `"colormap"` slot. Other visual families and slot names are
 * rejected until their retained scale wiring is implemented.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
