/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene visual helpers                                                                         */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static DvzVisual* _scene_alloc_visual(DvzScene* scene, DvzVisualType type, uint32_t flags);

static DvzVisualBinding* _visual_binding(DvzVisual* visual, DvzVisualBindingKind kind);

static uint32_t _attr_item_size(DvzVisualType type, const char* name);

static bool _attr_supported(DvzVisualType type, const char* name, uint32_t* item_size);

static bool _attr_source_supported(
    DvzVisualType type, const char* name, DvzVisualAttrSource source);

static DvzVisualAttr* _attr_get_or_create(DvzVisual* visual, const char* name, uint32_t item_size);

static bool _visual_attr_count_consistent(
    const DvzVisual* visual, const char* attr_name, uint32_t item_count);

static void _scene_release_visual_scale(DvzVisual* visual);

static void _primitive_shading_default(DvzPrimitiveShadingState* shading);

static bool _mesh_ensure_default_color(DvzVisual* visual, uint32_t item_count);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the byte size of one supported visual attribute item.
 *
 * @param type the visual type
 * @param name the attribute name
 * @return the item size in bytes, or zero when unsupported
 */
static uint32_t _attr_item_size(DvzVisualType type, const char* name)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
    case DVZ_VISUAL_TYPE_PIXEL:
    case DVZ_VISUAL_TYPE_MARKER:
        if (strcmp(name, "position") == 0) return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)    return 4 * sizeof(uint8_t);
        if (strcmp(name, "size") == 0)     return sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_PRIMITIVE:
    case DVZ_VISUAL_TYPE_MESH:
        if (strcmp(name, "position") == 0) return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)    return 4 * sizeof(uint8_t);
        if (strcmp(name, "normal") == 0)   return 3 * sizeof(float);
        break;
    case DVZ_VISUAL_TYPE_PATH:
        if (strcmp(name, "position") == 0) return 3 * sizeof(float);
        if (strcmp(name, "color") == 0)    return 4 * sizeof(uint8_t);
        break;
    case DVZ_VISUAL_TYPE_IMAGE:
        if (strcmp(name, "position") == 0)  return 3 * sizeof(float);
        if (strcmp(name, "texcoords") == 0) return 2 * sizeof(float);
        break;
    default:
        break;
    }
    return 0;
}



/**
 * Validate that one attribute is supported by a visual family.
 *
 * @param type the visual type
 * @param name the attribute name
 * @param item_size output item byte size
 * @return true when supported
 */
static bool _attr_supported(DvzVisualType type, const char* name, uint32_t* item_size)
{
    ANN(name);
    ANN(item_size);
    *item_size = _attr_item_size(type, name);
    if (*item_size != 0)
        return true;

    const char* expected = "position, color, size";
    if (type == DVZ_VISUAL_TYPE_PRIMITIVE)
        expected = "position, color, normal";
    else if (type == DVZ_VISUAL_TYPE_MESH)
        expected = "position, color, normal";
    else if (type == DVZ_VISUAL_TYPE_PATH)
        expected = "position, color";
    else if (type == DVZ_VISUAL_TYPE_IMAGE)
        expected = "position, texcoords";

    log_error(
        "unsupported %s visual attribute '%s' (expected one of: %s)",
        _visual_type_name(type), name, expected);
    return false;
}



/**
 * Validate that one semantic source is accepted by a visual attribute.
 *
 * @param type the visual type
 * @param name the attribute name
 * @param source the semantic source
 * @return true when the source is accepted
 */
static bool _attr_source_supported(
    DvzVisualType type, const char* name, DvzVisualAttrSource source)
{
    ANN(name);
    uint32_t item_size = 0;
    if (!_attr_supported(type, name, &item_size))
        return false;

    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
        return true;

    bool is_color = strcmp(name, "color") == 0;
    bool is_size = strcmp(name, "size") == 0;

    if (source == DVZ_VISUAL_ATTR_SOURCE_CONSTANT && (is_color || is_size))
        return true;
    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_GROUP && (is_color || is_size))
        return true;
    if (source == DVZ_VISUAL_ATTR_SOURCE_PER_SPAN && type == DVZ_VISUAL_TYPE_PATH && is_color)
        return true;

    log_error(
        "%s visual attribute '%s' does not accept source %d", _visual_type_name(type), name,
        (int)source);
    return false;
}



/**
 * Find the index of one visual attribute by name.
 *
 * @param visual the visual
 * @param name the attribute name
 * @return the attribute index, or -1 when absent
 */
int _attr_index(const DvzVisual* visual, const char* name)
{
    ANN(visual);
    ANN(name);
    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        if (strcmp(visual->attrs[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}



/**
 * Return an existing visual attribute slot or create one.
 *
 * @param visual the visual
 * @param name the attribute name
 * @param item_size the attribute item size in bytes
 * @return the attribute slot, or NULL when the visual has no free slot
 */
static DvzVisualAttr* _attr_get_or_create(DvzVisual* visual, const char* name, uint32_t item_size)
{
    ANN(visual);
    ANN(name);
    int idx = _attr_index(visual, name);
    if (idx >= 0)
        return &visual->attrs[idx];
    if (visual->attr_count >= DVZ_SCENE_MAX_ITEM_ATTRS)
        return NULL;
    DvzVisualAttr* attr = &visual->attrs[visual->attr_count++];
    dvz_strlcpy(attr->name, name, sizeof(attr->name));
    attr->item_size = item_size;
    attr->source = DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    attr->mutability = DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC;
    return attr;
}



/**
 * Check that a new attribute item count matches existing dense attributes.
 *
 * @param visual the visual
 * @param attr_name the updated attribute name
 * @param item_count the updated item count
 * @return true when the count is accepted
 */
static bool _visual_attr_count_consistent(
    const DvzVisual* visual, const char* attr_name, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    if (item_count == 0)
        return false;

    for (uint32_t i = 0; i < visual->attr_count; i++)
    {
        const DvzVisualAttr* attr = &visual->attrs[i];
        bool attr_has_payload = attr->data != NULL || attr->buffer != NULL;
        if (visual->type == DVZ_VISUAL_TYPE_MESH && visual->mesh_default_color &&
            strcmp(attr_name, "position") == 0 && strcmp(attr->name, "color") == 0)
        {
            continue;
        }
        if (strcmp(attr->name, attr_name) == 0 || attr->item_count == 0 || !attr_has_payload)
            continue;
        if (attr->item_count == item_count)
            continue;

        log_error(
            "%s visual attribute '%s' item_count %u does not match existing attribute '%s' "
            "item_count %u",
            _visual_type_name(visual->type), attr_name, item_count, attr->name, attr->item_count);
        return false;
    }
    return true;
}



/**
 * Return the scene-global index of a figure visual.
 *
 * @param figure the figure
 * @param visual the visual
 * @param out_index output visual index
 * @return true when the visual belongs to the figure scene
 */
bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index)
{
    ANN(out_index);
    *out_index = 0;
    if (figure == NULL || figure->scene == NULL || visual == NULL)
        return false;
    if (visual->scene != figure->scene)
        return false;
    for (uint32_t i = 0; i < figure->scene->visual_count; i++)
    {
        if (&figure->scene->visuals[i] == visual)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}



/**
 * Allocate and initialize one scene-owned visual.
 *
 * @param scene the scene
 * @param type the visual type
 * @param flags the visual flags
 * @return the visual, or NULL when the scene is full
 */
static DvzVisual* _scene_alloc_visual(DvzScene* scene, DvzVisualType type, uint32_t flags)
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
    _primitive_shading_default(&visual->primitive_shading);
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
 * Return one mutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
static DvzVisualBinding* _visual_binding(DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    visual->bindings[idx].kind = kind;
    return &visual->bindings[idx];
}



/**
 * Return one immutable visual binding slot.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @return the binding slot, or NULL for unsupported kinds
 */
const DvzVisualBinding* _visual_binding_const(
    const DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(visual);
    uint32_t idx = UINT32_MAX;
    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        idx = 0;
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        idx = 1;
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        idx = 2;
        break;
    default:
        return NULL;
    }
    ASSERT(idx < DVZ_SCENE_MAX_VISUAL_BINDINGS);
    return &visual->bindings[idx];
}



/**
 * Assign one visual binding and keep legacy convenience fields in sync.
 *
 * @param visual the visual
 * @param kind the binding kind
 * @param slot_name the binding slot name, or NULL to clear
 * @param resource the bound resource, or NULL to clear
 * @param owned whether the visual owns the resource
 */
void _visual_binding_assign(
    DvzVisual* visual, DvzVisualBindingKind kind, const char* slot_name, void* resource, bool owned)
{
    ANN(visual);
    DvzVisualBinding* binding = _visual_binding(visual, kind);
    ANN(binding);
    binding->resource = resource;
    binding->owned = owned;
    dvz_memset(binding->slot, sizeof(binding->slot), 0, sizeof(binding->slot));
    if (slot_name != NULL && resource != NULL)
        dvz_strlcpy(binding->slot, slot_name, sizeof(binding->slot));

    switch (kind)
    {
    case DVZ_VISUAL_BINDING_FIELD:
        visual->field = (DvzSampledField*)resource;
        visual->field_owned = owned;
        dvz_memset(visual->field_slot, sizeof(visual->field_slot), 0, sizeof(visual->field_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->field_slot, slot_name, sizeof(visual->field_slot));
        break;
    case DVZ_VISUAL_BINDING_BUFFER:
        visual->buffer = (DvzSceneBuffer*)resource;
        dvz_memset(
            visual->buffer_slot, sizeof(visual->buffer_slot), 0, sizeof(visual->buffer_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->buffer_slot, slot_name, sizeof(visual->buffer_slot));
        break;
    case DVZ_VISUAL_BINDING_SCALE:
        visual->scale = (DvzScale*)resource;
        dvz_memset(visual->scale_slot, sizeof(visual->scale_slot), 0, sizeof(visual->scale_slot));
        if (slot_name != NULL && resource != NULL)
            dvz_strlcpy(visual->scale_slot, slot_name, sizeof(visual->scale_slot));
        break;
    default:
        break;
    }
}



/**
 * Clear one visual binding.
 *
 * @param visual the visual
 * @param kind the binding kind
 */
void _visual_binding_clear(DvzVisual* visual, DvzVisualBindingKind kind)
{
    _visual_binding_assign(visual, kind, NULL, NULL, false);
}



/**
 * Clear one visual scale binding.
 *
 * @param visual the visual
 */
static void _scene_release_visual_scale(DvzVisual* visual)
{
    if (visual == NULL)
        return;
    _visual_binding_clear(visual, DVZ_VISUAL_BINDING_SCALE);
}



/**
 * Initialize primitive shading defaults.
 *
 * @param shading the shading state
 */
static void _primitive_shading_default(DvzPrimitiveShadingState* shading)
{
    ANN(shading);
    dvz_memset(shading, sizeof(DvzPrimitiveShadingState), 0, sizeof(DvzPrimitiveShadingState));
    shading->light_direction[2] = 1.0f;
    shading->params[0] = 0.2f;
    shading->params[1] = 0.8f;
}



/**
 * Ensure a mesh has a default opaque-white color attribute.
 *
 * @param visual the mesh visual
 * @param item_count the color item count
 * @return true on success
 */
static bool _mesh_ensure_default_color(DvzVisual* visual, uint32_t item_count)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_MESH || item_count == 0)
        return true;

    DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
    if (color == NULL)
        return false;
    if (color->data != NULL && color->item_count == item_count)
        return true;
    if (color->data != NULL)
    {
        dvz_free(color->data);
        color->data = NULL;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, color->item_size, &byte_size))
        return false;
    color->data = dvz_malloc(byte_size);
    if (color->data == NULL)
        return false;
    dvz_memset(color->data, byte_size, 255, byte_size);
    color->item_count = item_count;
    color->dirty_first_item = 0;
    color->dirty_item_count = item_count;
    visual->mesh_default_color = true;
    return true;
}



/*************************************************************************************************/
/*  Panel visual attachment                                                                      */
/*************************************************************************************************/

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
    return 0;
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
    ANN(panel);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return;
    DvzScene* scene = panel->figure->scene;

    /* Fullscreen quad in clip space, TRIANGLE_STRIP order (TL, BL, TR, BR). The visual is
     * attached with controller_mode=FIXED so the panzoom/arcball MVP doesn't move it,
     * and z_layer=-1 so it draws behind every default-layer visual. */
    static const float positions[4 * 3] = {
        -1.0f, +1.0f, 0.0f, /* TL */
        -1.0f, -1.0f, 0.0f, /* BL */
        +1.0f, +1.0f, 0.0f, /* TR */
        +1.0f, -1.0f, 0.0f, /* BR */
    };
    DvzColor color = {
        (uint8_t)(r * 255.0f + 0.5f),
        (uint8_t)(g * 255.0f + 0.5f),
        (uint8_t)(b * 255.0f + 0.5f),
        (uint8_t)(a * 255.0f + 0.5f),
    };
    DvzColor colors[4] = {
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
    };

    if (panel->background_visual == NULL)
    {
        DvzVisual* bg = dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, 0);
        if (bg == NULL)
        {
            log_error("dvz_panel_set_background_color: failed to allocate background visual");
            return;
        }
        if (dvz_visual_set_data(bg, "position", positions, 4) != 0 ||
            dvz_visual_set_data(bg, "color", colors, 4) != 0)
        {
            log_error("dvz_panel_set_background_color: failed to set background data");
            return;
        }
        if (dvz_panel_add_visual(
                panel, bg,
                &(DvzVisualAttachDesc){
                    .z_layer = -1, .controller_mode = DVZ_CONTROLLER_FIXED}) != 0)
        {
            log_error("dvz_panel_set_background_color: failed to attach background visual");
            return;
        }
        panel->background_visual = bg;
    }
    else
    {
        /* Existing background - just update its color. Position is already correct. */
        dvz_visual_set_data(panel->background_visual, "color", colors, 4);
    }
}



/*************************************************************************************************/
/*  Visual lifecycle and data                                                                    */
/*************************************************************************************************/

/**
 * Set the picking capabilities exposed by a visual.
 *
 * @param visual the visual
 * @param capabilities the capability mask
 */
void dvz_visual_set_pick_capabilities(DvzVisual* visual, uint32_t capabilities)
{
    ANN(visual);
    visual->pick_capabilities = capabilities;
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
 * Override primitive shading parameters.
 *
 * @param visual the visual
 * @param desc the shading descriptor, or NULL to restore defaults
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_primitive_shading(
    DvzVisual* visual, const DvzPrimitiveShadingDesc* desc)
{
    ANN(visual);
    if (visual->type != DVZ_VISUAL_TYPE_PRIMITIVE && visual->type != DVZ_VISUAL_TYPE_MESH)
    {
        log_error("primitive shading is only supported for primitive and mesh visuals");
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "update primitive shading"))
        return -1;
    _primitive_shading_default(&visual->primitive_shading);
    if (desc != NULL)
    {
        visual->primitive_shading.light_direction[0] = desc->light_direction[0];
        visual->primitive_shading.light_direction[1] = desc->light_direction[1];
        visual->primitive_shading.light_direction[2] = desc->light_direction[2];
        visual->primitive_shading.params[0] = desc->ambient;
        visual->primitive_shading.params[1] = desc->diffuse;
    }
    visual->primitive_shading_dirty = true;
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
}



/**
 * Declare the semantic source for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param source the semantic source
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_attr_source(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrSource source)
{
    ANN(visual);
    ANN(attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual metadata"))
        return -1;
    if (source < DVZ_VISUAL_ATTR_SOURCE_PER_ITEM || source > DVZ_VISUAL_ATTR_SOURCE_PER_GROUP)
    {
        log_error("invalid visual attribute source %d", (int)source);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;
    if (!_attr_source_supported(visual->type, attr_name, source))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }
    if (attr->data != NULL && attr->item_count > 0 && source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error(
            "visual attribute '%s' already has dense per-item data and cannot switch source",
            attr_name);
        return -1;
    }

    attr->source = source;
    return 0;
}



/**
 * Return the semantic source for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return the semantic source
 */
DvzVisualAttrSource dvz_visual_attr_source(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return DVZ_VISUAL_ATTR_SOURCE_PER_ITEM;
    return visual->attrs[idx].source;
}



/**
 * Declare the expected update frequency for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param mutability the update-frequency hint
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_attr_mutability(
    DvzVisual* visual, const char* attr_name, DvzVisualAttrMutability mutability)
{
    ANN(visual);
    ANN(attr_name);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual metadata"))
        return -1;
    if (mutability < DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC ||
        mutability > DVZ_VISUAL_ATTR_MUTABILITY_STREAMING)
    {
        log_error("invalid visual attribute mutability %d", (int)mutability);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }

    attr->mutability = mutability;
    return 0;
}



/**
 * Return the expected update frequency for one visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return the mutability hint
 */
DvzVisualAttrMutability
dvz_visual_attr_mutability(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int idx = _attr_index(visual, attr_name);
    if (idx < 0)
        return DVZ_VISUAL_ATTR_MUTABILITY_DYNAMIC;
    return visual->attrs[idx].mutability;
}



/**
 * Bind a scene buffer as one per-item visual attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param buffer the scene buffer, or NULL to clear
 * @param byte_offset byte offset into the buffer
 * @param item_count number of attribute items
 * @return whether the binding was updated
 */
bool dvz_visual_set_attr_buffer(
    DvzVisual* visual, const char* attr_name, DvzSceneBuffer* buffer,
    uint64_t byte_offset, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    if (buffer != NULL && buffer->scene != visual->scene)
    {
        log_error("cannot bind an attribute buffer from a different scene");
        return false;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind visual attribute buffer"))
        return false;

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return false;
    if (!_attr_source_supported(visual->type, attr_name, DVZ_VISUAL_ATTR_SOURCE_PER_ITEM))
        return false;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return false;
    }

    if (buffer == NULL)
    {
        attr->buffer = NULL;
        attr->buffer_byte_offset = 0;
        if (attr->data == NULL)
            attr->item_count = 0;
        return true;
    }

    if (item_count == 0)
    {
        log_error("visual attribute buffer '%s' requires item_count > 0", attr_name);
        return false;
    }
    if (byte_offset != 0)
    {
        log_error("visual attribute buffer '%s' byte offsets are not supported yet", attr_name);
        return false;
    }
    if ((buffer->desc.usage & DVZ_SCENE_BUFFER_USAGE_VERTEX) == 0)
    {
        log_error("visual attribute buffer '%s' requires VERTEX usage", attr_name);
        return false;
    }
    if (buffer->desc.stride != item_size)
    {
        log_error(
            "visual attribute buffer '%s' stride %u does not match item size %u", attr_name,
            buffer->desc.stride, item_size);
        return false;
    }
    if (attr->data != NULL)
    {
        log_error("visual attribute '%s' already has dense data and cannot bind a buffer", attr_name);
        return false;
    }
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error("visual attribute buffer '%s' requires PER_ITEM source", attr_name);
        return false;
    }
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return false;

    uint64_t byte_size = 0;
    uint64_t byte_end = 0;
    if (_dvz_mul_u64_overflows(item_count, buffer->desc.stride, &byte_size) ||
        _dvz_add_u64_overflows(byte_offset, byte_size, &byte_end) ||
        byte_end > buffer->desc.byte_size)
    {
        log_error(
            "visual attribute buffer '%s' range exceeds buffer size (%" PRIu64 " > %" PRIu64 ")",
            attr_name, byte_end, buffer->desc.byte_size);
        return false;
    }

    attr->buffer = buffer;
    attr->buffer_byte_offset = byte_offset;
    attr->item_count = item_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = 0;
    return true;
}



/**
 * Replace one dense visual attribute payload.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param data the attribute data
 * @param item_count the number of attribute items
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_data(
    DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' requires item_count > 0", attr_name);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;
    if (!_visual_attr_count_consistent(visual, attr_name, item_count))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error(
            "visual attribute '%s' dense data requires PER_ITEM source; use source-specific data",
            attr_name);
        return -1;
    }
    if (attr->buffer != NULL)
    {
        log_error("visual attribute '%s' already has a bound buffer", attr_name);
        return -1;
    }

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }

    /* Reallocate if total size changed */
    if (attr->data != NULL && attr->item_count != item_count)
    {
        dvz_free(attr->data);
        attr->data = NULL;
    }
    if (attr->data == NULL)
    {
        attr->data = dvz_malloc(byte_size);
        if (attr->data == NULL)
        {
            log_error(
                "visual attribute '%s' allocation failed for %" PRIu64 " bytes", attr_name,
                byte_size);
            attr->item_count       = 0;
            attr->dirty_first_item = 0;
            attr->dirty_item_count = 0;
            return -1;
        }
    }

    dvz_memcpy(attr->data, byte_size, data, byte_size);
    attr->item_count       = item_count;
    attr->dirty_first_item = 0;
    attr->dirty_item_count = item_count; /* whole buffer dirty */
    if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "position") == 0)
    {
        DvzVisualAttr* color = _attr_get_or_create(visual, "color", 4 * sizeof(uint8_t));
        if (color == NULL)
            return -1;
        if (color->data == NULL || visual->mesh_default_color)
        {
            if (!_mesh_ensure_default_color(visual, item_count))
            {
                log_error("mesh default color allocation failed");
                return -1;
            }
        }
    }
    else if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "color") == 0)
    {
        visual->mesh_default_color = false;
    }
    return 0;
}



/**
 * Replace a subrange of one dense visual attribute payload.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @param data the source data
 * @param first_item the first item to replace
 * @param item_count the number of items to replace
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_data_range(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (!_scene_visual_mutation_allowed(visual->scene, "mutate scene visual data"))
        return -1;
    if (item_count == 0)
    {
        log_error("visual attribute '%s' range update requires item_count > 0", attr_name);
        return -1;
    }

    uint32_t item_size = 0;
    if (!_attr_supported(visual->type, attr_name, &item_size))
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
    {
        log_error("visual attribute '%s' could not be registered", attr_name);
        return -1;
    }
    if (attr->source != DVZ_VISUAL_ATTR_SOURCE_PER_ITEM)
    {
        log_error(
            "visual attribute '%s' dense range update requires PER_ITEM source", attr_name);
        return -1;
    }
    if (attr->buffer != NULL)
    {
        log_error("visual attribute '%s' range update cannot target a bound buffer", attr_name);
        return -1;
    }

    /* The attribute must already be fully allocated */
    if (attr->data == NULL || attr->item_count == 0)
    {
        log_error(
            "visual attribute '%s' range update requires prior full allocation with "
            "dvz_visual_set_data()",
            attr_name);
        return -1;
    }
    uint64_t item_end = 0;
    if (_dvz_add_u64_overflows(first_item, item_count, &item_end))
    {
        log_error(
            "visual attribute '%s' range update overflow for first_item=%u item_count=%u",
            attr_name, first_item, item_count);
        return -1;
    }
    if (item_end > attr->item_count)
    {
        log_error(
            "visual attribute '%s' range update [%u, %" PRIu64 ") exceeds item_count %u",
            attr_name, first_item, item_end, attr->item_count);
        return -1;
    }

    uint64_t byte_offset = 0;
    uint64_t byte_size   = 0;
    if (_dvz_mul_u64_overflows(first_item, item_size, &byte_offset))
    {
        log_error(
            "visual attribute '%s' byte offset overflow for first_item=%u item_size=%u", attr_name,
            first_item, item_size);
        return -1;
    }
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
    {
        log_error(
            "visual attribute '%s' byte size overflow for item_count=%u item_size=%u", attr_name,
            item_count, item_size);
        return -1;
    }
    dvz_memcpy((uint8_t*)attr->data + byte_offset, byte_size, data, byte_size);

    /* Extend dirty range to cover the new update */
    if (attr->dirty_item_count == 0)
    {
        attr->dirty_first_item = first_item;
        attr->dirty_item_count = item_count;
    }
    else
    {
        uint64_t old_end = 0;
        uint64_t new_end = 0;
        if (_dvz_add_u64_overflows(attr->dirty_first_item, attr->dirty_item_count, &old_end))
            return -1;
        if (_dvz_add_u64_overflows(first_item, item_count, &new_end))
            return -1;
        uint64_t merged_first = attr->dirty_first_item < first_item
                                    ? attr->dirty_first_item
                                    : first_item;
        uint64_t merged_end = old_end > new_end ? old_end : new_end;
        attr->dirty_first_item = merged_first;
        attr->dirty_item_count = merged_end - merged_first;
    }
    if (visual->type == DVZ_VISUAL_TYPE_MESH && strcmp(attr_name, "color") == 0)
        visual->mesh_default_color = false;
    return 0;
}



/*************************************************************************************************/
/*  Visual family constructors                                                                   */
/*************************************************************************************************/

/**
 * Create a point visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_point(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_POINT, flags);
    if (visual == NULL)
        return NULL;
    return visual;
}



/**
 * Create a pixel visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_pixel(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_PIXEL, flags);
    if (visual == NULL)
        return NULL;
    return visual;
}



/**
 * Create a primitive visual.
 *
 * @param scene the scene
 * @param topology the primitive topology
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_primitive(DvzScene* scene, DvzPrimitiveTopology topology, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_PRIMITIVE, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = topology;
    visual->primitive_shading_dirty = true;
    return visual;
}



/**
 * Create a mesh visual.
 *
 * First-slice mesh visuals reuse the indexed primitive triangle-list execution path.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_mesh(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_MESH, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    visual->primitive_shading_dirty = true;
    return visual;
}



/**
 * Create a path visual.
 *
 * First-slice path visuals reuse the primitive line-strip execution path.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_path(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    DvzVisual* visual = _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_PATH, flags);
    if (visual == NULL)
        return NULL;
    visual->topology = DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    return visual;
}



/**
 * Create an image visual.
 *
 * @param scene the scene
 * @param flags variant flags
 * @return the visual, or NULL on allocation failure
 */
DvzVisual* dvz_image(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    return _scene_alloc_visual(scene, DVZ_VISUAL_TYPE_IMAGE, flags);
}



/**
 * Bind a scene-owned scale to a named visual slot.
 *
 * First retained slice: image visuals accept the `"colormap"` slot. Other
 * visual families and slot names are rejected until their retained scale
 * wiring is implemented.
 *
 * @param visual the visual
 * @param slot_name the semantic slot name
 * @param scale the scale, or NULL to clear the binding
 * @return 0 on success, -1 on error
 */
int dvz_visual_set_scale(DvzVisual* visual, const char* slot_name, DvzScale* scale)
{
    ANN(visual);
    ANN(slot_name);
    if (scale != NULL && scale->scene != visual->scene)
    {
        log_error("cannot bind a scale from a different scene");
        return -1;
    }
    if (visual->type != DVZ_VISUAL_TYPE_IMAGE)
    {
        log_error("dvz_visual_set_scale is only supported for image visuals in the first slice");
        return -1;
    }
    if (strcmp(slot_name, "colormap") != 0)
    {
        log_error("unsupported image scale slot '%s' (expected 'colormap')", slot_name);
        return -1;
    }
    if (!_scene_visual_mutation_allowed(visual->scene, "bind scale"))
        return -1;
    _scene_release_visual_scale(visual);
    if (scale != NULL)
        _visual_binding_assign(visual, DVZ_VISUAL_BINDING_SCALE, slot_name, scale, false);
    if (visual->field != NULL && _field_format_is_scalar(visual->field->desc.format))
    {
        _scene_visual_texture_mark_clean(visual);
        visual->texture.dirty = true;
    }
    return 0;
}



/*************************************************************************************************/
/*  Visual names                                                                                 */
/*************************************************************************************************/

/**
 * Return the debug name of one visual type.
 *
 * @param type the visual type
 * @return the visual type name
 */
const char* _visual_type_name(DvzVisualType type)
{
    switch (type)
    {
    case DVZ_VISUAL_TYPE_POINT:
        return "point";
    case DVZ_VISUAL_TYPE_PIXEL:
        return "pixel";
    case DVZ_VISUAL_TYPE_MARKER:
        return "marker";
    case DVZ_VISUAL_TYPE_SEGMENT:
        return "segment";
    case DVZ_VISUAL_TYPE_PATH:
        return "path";
    case DVZ_VISUAL_TYPE_IMAGE:
        return "image";
    case DVZ_VISUAL_TYPE_MESH:
        return "mesh";
    case DVZ_VISUAL_TYPE_VOLUME:
        return "volume";
    case DVZ_VISUAL_TYPE_PRIMITIVE:
        return "primitive";
    default:
        return "unknown";
    }
}
