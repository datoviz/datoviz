/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene JSON serialization                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_json.h"
#include "_scene.h"
#include "domain/buffer_internal.h"
#include "domain/field_internal.h"
#include "visuals/bindings_internal.h"
#include "_visual_internal.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static uint32_t _visual_index(const DvzScene* scene, const DvzVisual* visual);

static void _json_append_visual_binding(
    JsonBuilder* b, const DvzVisual* visual, DvzVisualBindingKind kind);

static void _json_append_field(
    JsonBuilder* b, const DvzScene* scene, uint32_t field_idx, bool* first);

static void _json_append_buffer(
    JsonBuilder* b, const DvzScene* scene, uint32_t buffer_idx, bool* first);

static void _json_append_visual_attr(
    JsonBuilder* b, const DvzVisualAttr* attr, bool* first);

static void _json_append_visual(
    JsonBuilder* b, const DvzScene* scene, const DvzVisual* visual, bool* first);

static void _json_append_panel(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, uint32_t panel_idx,
    const DvzPanel* panel, bool* first);

static void _json_append_figure(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, bool* first);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/* Return the scene-global index of a visual, or UINT32_MAX if not found. */
static uint32_t _visual_index(const DvzScene* scene, const DvzVisual* visual)
{
    for (uint32_t i = 0; i < scene->visual_count; i++)
        if (&scene->visuals[i] == visual)
            return i;
    return UINT32_MAX;
}



static void _json_append_visual_binding(
    JsonBuilder* b, const DvzVisual* visual, DvzVisualBindingKind kind)
{
    ANN(b);
    if (visual == NULL || visual->scene == NULL)
    {
        _json_append(b, "null");
        return;
    }

    const DvzVisualBinding* binding = _visual_binding_const(visual, kind);
    if (binding == NULL || binding->resource == NULL)
    {
        _json_append(b, "null");
        return;
    }

    switch (kind)
    {
    case DVZ_VISUAL_BINDING_SCALE:
        for (uint32_t si = 0; si < visual->scene->scale_count; si++)
        {
            if (&visual->scene->scales[si] != (DvzScale*)binding->resource)
                continue;
            _json_append(b, "{\"id\":\"s%u\",\"slot\":", si);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    case DVZ_VISUAL_BINDING_FIELD:
    {
        uint32_t field_idx = _scene_field_index(visual->scene, (DvzSampledField*)binding->resource);
        if (field_idx != UINT32_MAX)
        {
            _json_append(b, "{\"id\":\"f%u\",\"slot\":", field_idx);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    }
    case DVZ_VISUAL_BINDING_BUFFER:
    {
        uint32_t buffer_idx = _scene_buffer_index(visual->scene, (DvzSceneBuffer*)binding->resource);
        if (buffer_idx != UINT32_MAX)
        {
            _json_append(b, "{\"id\":\"b%u\",\"slot\":", buffer_idx);
            _json_append_escaped_string(b, binding->slot);
            _json_append(b, "}");
            return;
        }
        break;
    }
    default:
        break;
    }

    _json_append(b, "null");
}



/**
 * Append one sampled-field JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param field_idx the field index
 * @param first whether this is the first array item
 */
static void _json_append_field(
    JsonBuilder* b, const DvzScene* scene, uint32_t field_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzSampledField* field = &scene->fields[field_idx];
    if (field->scene != scene)
        return;
    _json_append(
        b,
        "%s{\"id\":\"f%u\",\"dim\":%u,\"format\":%u,\"semantic\":%u,"
        "\"width\":%u,\"height\":%u,\"depth\":%u,\"data\":",
        *first ? "" : ",", field_idx, (uint32_t)field->desc.dim, (uint32_t)field->desc.format,
        (uint32_t)field->desc.semantic, field->desc.width, field->desc.height, field->desc.depth);
    if (field->data != NULL && field->data_size > 0)
        _json_append_base64(b, (const uint8_t*)field->data, field->data_size);
    else
        _json_append(b, "null");
    _json_append(
        b,
        ",\"geometry\":{\"axis_order\":[%u,%u,%u],\"axis_flip\":[%s,%s,%s],"
        "\"origin\":[%.6g,%.6g,%.6g],\"spacing\":[%.6g,%.6g,%.6g],\"unit\":",
        field->geometry.axis_order[0], field->geometry.axis_order[1], field->geometry.axis_order[2],
        field->geometry.axis_flip[0] ? "true" : "false",
        field->geometry.axis_flip[1] ? "true" : "false",
        field->geometry.axis_flip[2] ? "true" : "false", field->geometry.origin[0],
        field->geometry.origin[1], field->geometry.origin[2], field->geometry.spacing[0],
        field->geometry.spacing[1], field->geometry.spacing[2]);
    _json_append_escaped_string(b, field->geometry.unit);
    _json_append(
        b,
        "},\"dirty\":{\"pending\":%s,\"full\":%s,\"region\":{\"x\":%u,\"y\":%u,\"z\":%u,"
        "\"width\":%u,\"height\":%u,\"depth\":%u}}}",
        field->dirty ? "true" : "false", field->dirty_full ? "true" : "false",
        field->dirty_region.x, field->dirty_region.y, field->dirty_region.z,
        field->dirty_region.width, field->dirty_region.height, field->dirty_region.depth);
    *first = false;
}



/**
 * Append one scene-buffer JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param buffer_idx the buffer index
 * @param first whether this is the first array item
 */
static void _json_append_buffer(
    JsonBuilder* b, const DvzScene* scene, uint32_t buffer_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzSceneBuffer* buffer = &scene->buffers[buffer_idx];
    if (buffer->scene != scene)
        return;
    _json_append(
        b, "%s{\"id\":\"b%u\",\"usage\":%u,\"stride\":%u,\"byte_size\":%" PRIu64 ",\"data\":",
        *first ? "" : ",", buffer_idx, buffer->desc.usage, buffer->desc.stride,
        buffer->desc.byte_size);
    if (buffer->data != NULL && buffer->desc.byte_size > 0)
        _json_append_base64(b, (const uint8_t*)buffer->data, buffer->desc.byte_size);
    else
        _json_append(b, "null");
    _json_append(b, ",\"dirty\":{\"pending\":%s}}", buffer->dirty ? "true" : "false");
    *first = false;
}



/**
 * Append one visual attribute JSON object.
 *
 * @param b the JSON builder
 * @param attr the attribute
 * @param first whether this is the first array item
 */
static void _json_append_visual_attr(
    JsonBuilder* b, const DvzVisualAttr* attr, bool* first)
{
    ANN(b);
    ANN(attr);
    ANN(first);
    uint64_t byte_size = (uint64_t)attr->item_count * attr->item_size;
    _json_append(
        b, "%s{\"name\":\"%s\",\"item_count\":%u,\"item_size\":%u,\"data\":",
        *first ? "" : ",", attr->name, attr->item_count, attr->item_size);
    if (attr->data != NULL && byte_size > 0)
        _json_append_base64(b, (const uint8_t*)attr->data, byte_size);
    else
        _json_append(b, "null");
    _json_append(b, "}");
    *first = false;
}



/**
 * Append one panel visual JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param visual the visual
 * @param first whether this is the first array item
 */
static void _json_append_visual(
    JsonBuilder* b, const DvzScene* scene, const DvzVisual* visual, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(visual);
    ANN(first);
    uint32_t visual_idx = _visual_index(scene, visual);
    _json_append(
        b, "%s{\"id\":\"v%u\",\"type\":\"%s\",\"visible\":%s,\"attrs\":[", *first ? "" : ",",
        visual_idx, _visual_type_name(visual->type), visual->visible ? "true" : "false");
    bool first_attr = true;
    for (uint32_t ai = 0; ai < visual->attr_count; ai++)
        _json_append_visual_attr(b, &visual->attrs[ai], &first_attr);
    _json_append(b, "],\"scale\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_SCALE);
    _json_append(b, ",\"field\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_FIELD);
    _json_append(b, ",\"buffer\":");
    _json_append_visual_binding(b, visual, DVZ_VISUAL_BINDING_BUFFER);
    _json_append(b, ",\"field_state\":");
    if (visual->field != NULL)
    {
        _json_append(
            b,
            "{\"pending\":%s,\"full\":%s,\"region\":{\"x\":%u,\"y\":%u,\"z\":%u,"
            "\"width\":%u,\"height\":%u,\"depth\":%u}}",
            visual->texture.field_dirty ? "true" : "false",
            visual->texture.field_dirty_full ? "true" : "false",
            visual->texture.field_dirty_region.x, visual->texture.field_dirty_region.y,
            visual->texture.field_dirty_region.z, visual->texture.field_dirty_region.width,
            visual->texture.field_dirty_region.height, visual->texture.field_dirty_region.depth);
    }
    else
    {
        _json_append(b, "null");
    }
    _json_append(b, "}");
    *first = false;
}



/**
 * Append one panel JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param figure_idx the parent figure index
 * @param panel_idx the panel index
 * @param panel the panel
 * @param first whether this is the first array item
 */
static void _json_append_panel(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, uint32_t panel_idx,
    const DvzPanel* panel, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(panel);
    ANN(first);
    _json_append(
        b,
        "%s{\"id\":\"fig%u_p%u\","
        "\"desc\":{\"x\":%.6g,\"y\":%.6g,\"width\":%.6g,\"height\":%.6g},"
        "\"visuals\":[",
        *first ? "" : ",", figure_idx, panel_idx, (double)panel->desc.x, (double)panel->desc.y,
        (double)panel->desc.width, (double)panel->desc.height);
    bool first_visual = true;
    for (uint32_t vi = 0; vi < panel->visual_count; vi++)
    {
        const DvzVisual* visual = panel->visuals[vi].visual;
        if (visual == NULL)
            continue;
        _json_append_visual(b, scene, visual, &first_visual);
    }
    _json_append(b, "]}");
    *first = false;
}



/**
 * Append one figure JSON object.
 *
 * @param b the JSON builder
 * @param scene the owning scene
 * @param figure_idx the figure index
 * @param first whether this is the first array item
 */
static void _json_append_figure(
    JsonBuilder* b, const DvzScene* scene, uint32_t figure_idx, bool* first)
{
    ANN(b);
    ANN(scene);
    ANN(first);
    const DvzFigure* figure = &scene->figures[figure_idx];
    if (figure->scene == NULL)
        return;
    _json_append(
        b, "%s{\"id\":\"fig%u\",\"width\":%u,\"height\":%u,\"panels\":[", *first ? "" : ",",
        figure_idx, figure->width, figure->height);
    bool first_panel = true;
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        _json_append_panel(b, scene, figure_idx, pi, &figure->panels[pi], &first_panel);
    _json_append(b, "]}");
    *first = false;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Serialize the retained scene graph to JSON.
 *
 * @param scene the scene
 * @return the owned JSON string, or NULL on allocation failure
 */
char* dvz_scene_json(const DvzScene* scene)
{
    ANN(scene);

    JsonBuilder b = {0};
    if (!_json_init(&b))
        return NULL;

    _json_append(&b, "{\"fields\":[");
    bool first_field = true;
    for (uint32_t i = 0; i < scene->field_count; i++)
        _json_append_field(&b, scene, i, &first_field);

    _json_append(&b, "],\"buffers\":[");
    bool first_buffer = true;
    for (uint32_t i = 0; i < scene->buffer_count; i++)
        _json_append_buffer(&b, scene, i, &first_buffer);

    _json_append(&b, "],\"figures\":[");
    bool first_figure = true;
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
        _json_append_figure(&b, scene, fi, &first_figure);
    _json_append(&b, "]}");

    return _json_finish(&b);
}



/**
 * Destroy a JSON string returned by dvz_scene_json().
 *
 * @param json the JSON string
 */
void dvz_scene_json_destroy(char* json)
{
    dvz_free(json);
}
