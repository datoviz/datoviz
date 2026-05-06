/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene graph — DvzScene / DvzFigure / DvzPanel / DvzVisual                                  */
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
#include "_json.h"
#include "_overflow.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static uint32_t _attr_item_size(DvzVisualType type, const char* name)
{
    (void)type; /* all families share the same conventions for now */
    if (strcmp(name, "position") == 0)
        return 3 * sizeof(float); /* vec3f */
    if (strcmp(name, "color") == 0)
        return 4 * sizeof(uint8_t); /* cvec4 */
    if (strcmp(name, "size") == 0)
        return sizeof(float);
    return 0;
}


static int _attr_index(const DvzVisual* visual, const char* name)
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
    return attr;
}



static bool _figure_visual_index(const DvzFigure* figure, const DvzVisual* visual, uint32_t* out_index)
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



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    if (scene == NULL)
        return NULL;
    dvz_capability_snapshot_default(&scene->caps);
    scene->emitter = dvz_frame_plan_emitter();
    if (scene->emitter == NULL)
    {
        dvz_free(scene);
        return NULL;
    }
    return scene;
}


void dvz_scene_set_capabilities(DvzScene* scene, const DvzCapabilitySnapshot* caps)
{
    ANN(scene);
    ANN(caps);
    dvz_capability_snapshot_copy(&scene->caps, caps);
}


void dvz_scene_destroy(DvzScene* scene)
{
    if (scene == NULL)
        return;
    /* Destroy all visuals first (free attribute data) */
    for (uint32_t i = 0; i < scene->visual_count; i++)
    {
        DvzVisual* v = &scene->visuals[i];
        for (uint32_t j = 0; j < v->attr_count; j++)
        {
            if (v->attrs[j].data != NULL)
            {
                dvz_free(v->attrs[j].data);
                v->attrs[j].data = NULL;
            }
        }
    }
    if (scene->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(scene->emitter);
        scene->emitter = NULL;
    }
    dvz_free(scene);
}



/*************************************************************************************************/
/*  Figure                                                                                       */
/*************************************************************************************************/

DvzFigure* dvz_figure(DvzScene* scene, uint32_t width, uint32_t height, uint32_t flags)
{
    ANN(scene);
    if (scene->figure_count >= DVZ_SCENE_MAX_FIGURES)
        return NULL;
    DvzFigure* fig = &scene->figures[scene->figure_count++];
    fig->scene  = scene;
    fig->width  = width;
    fig->height = height;
    fig->flags  = flags;
    return fig;
}


void dvz_figure_destroy(DvzFigure* figure)
{
    if (figure == NULL)
        return;
    /* Mark slot as empty */
    figure->scene = NULL;
}


DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(figure);
    ANN(figure->scene);
    ANN(figure->scene->emitter);
    DvzFramePlanEmitter* emitter = figure->scene->emitter;

    /* Use a stable figure_id from its position in the scene array */
    char figure_id[64];
    dvz_strlcpy(figure_id, "fig0", sizeof(figure_id));
    if (figure->scene != NULL)
    {
        for (uint32_t i = 0; i < figure->scene->figure_count; i++)
        {
            if (&figure->scene->figures[i] == figure)
            {
                dvz_snprintf(figure_id, sizeof(figure_id), "fig%u", i);
                break;
            }
        }
    }

    /* Build a fresh FramePlan */
    DvzFramePlan* plan = dvz_frame_plan(figure_id, 0);
    if (plan == NULL)
        return NULL;

    /* --- Upload nodes: one per dirty visual attribute --- */
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi];
            if (visual == NULL || !visual->visible)
                continue;
            /* Resolve visual membership by identity, avoiding cross-array pointer arithmetic. */
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue; /* not from this scene */
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                DvzVisualAttr* attr = &visual->attrs[ai];
                if (attr->dirty_item_count == 0 || attr->data == NULL || attr->item_count == 0)
                    continue;
                char resource_id[128];
                dvz_snprintf(resource_id, sizeof(resource_id), "v%u_%s", vidx, attr->name);
                uint64_t byte_offset =
                    (uint64_t)attr->dirty_first_item * attr->item_size;
                uint64_t byte_size =
                    (uint64_t)attr->dirty_item_count * attr->item_size;
                const void* data_ptr = (const uint8_t*)attr->data + byte_offset;
                dvz_frame_plan_upload_bytes(
                    plan, resource_id, byte_offset, byte_size, attr->name, data_ptr);
            }
        }
    }

    /* --- Render nodes: one per panel --- */
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];

        char panel_id[64];
        dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
        dvz_frame_plan_render(plan, panel_id, "rt", false);

        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi];
            if (visual == NULL || !visual->visible)
                continue;
            uint32_t vidx = 0;
            if (!_figure_visual_index(figure, visual, &vidx))
                continue;
            char visual_id[64];
            dvz_snprintf(visual_id, sizeof(visual_id), "v%u", vidx);
            dvz_frame_plan_render_visual(plan, visual_id);
        }
    }

    /* Resolve nullable args */
    DvzCapabilitySnapshot default_caps;
    if (caps == NULL)
    {
        dvz_capability_snapshot_default(&default_caps);
        caps = &default_caps;
    }
    DvzFramePlanEmitConfig default_cfg = dvz_frame_plan_emit_config();
    if (cfg == NULL)
        cfg = &default_cfg;
    DvzDiagnosticReport local_report;
    if (report == NULL)
    {
        dvz_diagnostic_report_init(&local_report);
        report = &local_report;
    }

    DvzDrp2CommandStream* stream =
        dvz_frame_plan_emitter_emit_drp2(emitter, plan, caps, report, cfg);

    /* Clear dirty flags after successful emit */
    if (stream != NULL)
    {
        for (uint32_t pi = 0; pi < figure->panel_count; pi++)
        {
            DvzPanel* panel = &figure->panels[pi];
            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                DvzVisual* visual = panel->visuals[vi];
                if (visual == NULL)
                    continue;
                for (uint32_t ai = 0; ai < visual->attr_count; ai++)
                    visual->attrs[ai].dirty_item_count = 0;
            }
        }
    }

    dvz_frame_plan_destroy(plan);
    return stream;
}



DvzDrp2CommandStream* dvz_figure_emit(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report)
{
    return dvz_figure_emit_ex(figure, caps, report, NULL);
}



/*************************************************************************************************/
/*  Panel                                                                                        */
/*************************************************************************************************/

DvzPanel* dvz_panel(DvzFigure* figure, DvzPanelDesc desc)
{
    ANN(figure);
    if (figure->panel_count >= DVZ_SCENE_MAX_PANELS)
        return NULL;
    DvzPanel* panel     = &figure->panels[figure->panel_count++];
    panel->figure       = figure;
    panel->desc         = desc;
    panel->visual_count = 0;
    return panel;
}


void dvz_panel_destroy(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    panel->figure       = NULL;
    panel->visual_count = 0;
}


int dvz_panel_add_visual(DvzPanel* panel, DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
    if (panel->figure == NULL || panel->figure->scene == NULL)
        return -1;
    if (visual->scene != panel->figure->scene)
        return -1;
    if (panel->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return -1;
    panel->visuals[panel->visual_count++] = visual;
    return 0;
}



/*************************************************************************************************/
/*  Visual — lifecycle and data                                                                  */
/*************************************************************************************************/

void dvz_visual_destroy(DvzVisual* visual)
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
    visual->attr_count = 0;
    visual->scene      = NULL;
}


void dvz_visual_set_visible(DvzVisual* visual, bool visible)
{
    ANN(visual);
    visual->visible = visible;
}


int dvz_visual_set_data(
    DvzVisual* visual, const char* attr_name, const void* data, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (item_count == 0)
        return -1;

    uint32_t item_size = _attr_item_size(visual->type, attr_name);
    if (item_size == 0)
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
        return -1;

    uint64_t byte_size = 0;
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
        return -1;

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
    return 0;
}



int dvz_visual_set_data_range(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count)
{
    ANN(visual);
    ANN(attr_name);
    ANN(data);
    if (item_count == 0)
        return -1;

    uint32_t item_size = _attr_item_size(visual->type, attr_name);
    if (item_size == 0)
        return -1;

    DvzVisualAttr* attr = _attr_get_or_create(visual, attr_name, item_size);
    if (attr == NULL)
        return -1;

    /* The attribute must already be fully allocated */
    if (attr->data == NULL || attr->item_count == 0)
        return -1;
    uint64_t item_end = 0;
    if (_dvz_add_u64_overflows(first_item, item_count, &item_end))
        return -1;
    if (item_end > attr->item_count)
        return -1;

    uint64_t byte_offset = 0;
    uint64_t byte_size   = 0;
    if (_dvz_mul_u64_overflows(first_item, item_size, &byte_offset))
        return -1;
    if (_dvz_mul_u64_overflows(item_count, item_size, &byte_size))
        return -1;
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
    return 0;
}



/*************************************************************************************************/
/*  Visual family constructors                                                                   */
/*************************************************************************************************/

DvzVisual* dvz_point(DvzScene* scene, uint32_t flags)
{
    ANN(scene);
    if (scene->visual_count >= DVZ_SCENE_MAX_VISUALS)
        return NULL;
    DvzVisual* visual = &scene->visuals[scene->visual_count++];
    visual->scene   = scene;
    visual->type    = DVZ_VISUAL_TYPE_POINT;
    visual->flags   = flags;
    visual->visible = true;
    visual->z_layer = 0;
    return visual;
}



/*************************************************************************************************/
/*  Scene JSON serialization                                                                     */
/*************************************************************************************************/

static const char* _visual_type_name(DvzVisualType type)
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
    default:
        return "unknown";
    }
}

/* Return the scene-global index of a visual, or UINT32_MAX if not found. */
static uint32_t _visual_index(const DvzScene* scene, const DvzVisual* visual)
{
    for (uint32_t i = 0; i < scene->visual_count; i++)
        if (&scene->visuals[i] == visual)
            return i;
    return UINT32_MAX;
}

char* dvz_scene_json(const DvzScene* scene)
{
    ANN(scene);

    JsonBuilder b = {0};
    if (!_json_init(&b))
        return NULL;

    _json_append(&b, "{\"figures\":[");
    for (uint32_t fi = 0; fi < scene->figure_count; fi++)
    {
        const DvzFigure* fig = &scene->figures[fi];
        if (fig->scene == NULL)
            continue;
        _json_append(&b, "%s{\"id\":\"fig%u\",\"width\":%u,\"height\":%u,\"panels\":[",
                     fi == 0 ? "" : ",", fi, fig->width, fig->height);

        for (uint32_t pi = 0; pi < fig->panel_count; pi++)
        {
            const DvzPanel* panel = &fig->panels[pi];
            _json_append(
                &b,
                "%s{\"id\":\"fig%u_p%u\","
                "\"desc\":{\"x\":%.6g,\"y\":%.6g,\"width\":%.6g,\"height\":%.6g},"
                "\"visuals\":[",
                pi == 0 ? "" : ",", fi, pi,
                (double)panel->desc.x, (double)panel->desc.y,
                (double)panel->desc.width, (double)panel->desc.height);

            for (uint32_t vi = 0; vi < panel->visual_count; vi++)
            {
                const DvzVisual* vis = panel->visuals[vi];
                if (vis == NULL)
                    continue;
                uint32_t vidx = _visual_index(scene, vis);
                _json_append(
                    &b,
                    "%s{\"id\":\"v%u\",\"type\":\"%s\",\"visible\":%s,\"attrs\":[",
                    vi == 0 ? "" : ",", vidx,
                    _visual_type_name(vis->type),
                    vis->visible ? "true" : "false");

                for (uint32_t ai = 0; ai < vis->attr_count; ai++)
                {
                    const DvzVisualAttr* attr = &vis->attrs[ai];
                    uint64_t byte_size = (uint64_t)attr->item_count * attr->item_size;
                    _json_append(
                        &b,
                        "%s{\"name\":\"%s\",\"item_count\":%u,\"item_size\":%u,\"data\":",
                        ai == 0 ? "" : ",",
                        attr->name, attr->item_count, attr->item_size);
                    if (attr->data != NULL && byte_size > 0)
                        _json_append_base64(&b, (const uint8_t*)attr->data, byte_size);
                    else
                        _json_append(&b, "null");
                    _json_append(&b, "}");
                }
                _json_append(&b, "]}"); /* close attrs + visual */
            }
            _json_append(&b, "]}"); /* close visuals + panel */
        }
        _json_append(&b, "]}"); /* close panels + figure */
    }
    _json_append(&b, "]}"); /* close figures + root */

    return _json_finish(&b);
}



void dvz_scene_json_destroy(char* json)
{
    dvz_free(json);
}
