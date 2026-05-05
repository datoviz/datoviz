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



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

DvzScene* dvz_scene(void)
{
    DvzScene* scene = (DvzScene*)dvz_calloc(1, sizeof(DvzScene));
    dvz_capability_snapshot_default(&scene->caps);
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
    ANN(scene);
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
    /* Destroy figures (and their emitters) */
    for (uint32_t i = 0; i < scene->figure_count; i++)
    {
        DvzFigure* fig = &scene->figures[i];
        if (fig->emitter != NULL)
        {
            dvz_frame_plan_emitter_destroy(fig->emitter);
            fig->emitter = NULL;
        }
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
    fig->scene   = scene;
    fig->width   = width;
    fig->height  = height;
    fig->flags   = flags;
    fig->emitter = dvz_frame_plan_emitter();
    return fig;
}


void dvz_figure_destroy(DvzFigure* figure)
{
    ANN(figure);
    if (figure->emitter != NULL)
    {
        dvz_frame_plan_emitter_destroy(figure->emitter);
        figure->emitter = NULL;
    }
    /* Mark slot as empty so the scene destroy loop doesn't double-free */
    figure->scene = NULL;
}


DvzDrp2CommandStream* dvz_figure_emit_ex(
    DvzFigure* figure, const DvzCapabilitySnapshot* caps, DvzDiagnosticReport* report,
    const DvzFramePlanEmitConfig* cfg)
{
    ANN(figure);
    ANN(figure->emitter);

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
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                DvzVisualAttr* attr = &visual->attrs[ai];
                if (!attr->dirty || attr->data == NULL || attr->item_count == 0)
                    continue;
                char resource_id[128];
                uint32_t vidx = 0;
                for (uint32_t k = 0; k < figure->scene->visual_count; k++)
                {
                    if (&figure->scene->visuals[k] == visual)
                    {
                        vidx = k;
                        break;
                    }
                }
                dvz_snprintf(resource_id, sizeof(resource_id), "v%u_%s", vidx, attr->name);
                uint64_t byte_size = (uint64_t)attr->item_count * attr->item_size;
                dvz_frame_plan_upload(plan, resource_id, 0, byte_size, attr->name);
            }
        }
    }

    /* --- Render nodes: one per panel --- */
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        if (panel->visual_count == 0)
            continue;

        char panel_id[64];
        dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
        dvz_frame_plan_render(plan, panel_id, "rt", false);

        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi];
            if (visual == NULL || !visual->visible)
                continue;
            uint32_t vidx = 0;
            for (uint32_t k = 0; k < figure->scene->visual_count; k++)
            {
                if (&figure->scene->visuals[k] == visual)
                {
                    vidx = k;
                    break;
                }
            }
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
        dvz_frame_plan_emitter_emit_drp2(figure->emitter, plan, caps, report, cfg);

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
                    visual->attrs[ai].dirty = false;
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
    ANN(panel);
    panel->figure       = NULL;
    panel->visual_count = 0;
}


int dvz_panel_add_visual(DvzPanel* panel, DvzVisual* visual)
{
    ANN(panel);
    ANN(visual);
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
    ANN(visual);
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

    uint64_t byte_size = (uint64_t)item_count * item_size;

    /* Reallocate if size changed */
    if (attr->data != NULL && attr->item_count != item_count)
    {
        dvz_free(attr->data);
        attr->data = NULL;
    }
    if (attr->data == NULL)
        attr->data = dvz_malloc(byte_size);

    dvz_memcpy(attr->data, byte_size, data, byte_size);
    attr->item_count = item_count;
    attr->dirty      = true;
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
