/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Panel light upload emission                                                                  */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_scene.h"
#include "_scene_resource_key.h"
#include "datoviz/drp2/runtime.h"
#include "scene_emit/internal.h"
#include "scene_emit/scene_emit.h"



/**
 * Pack one normalized scene light into the stable shader payload.
 *
 * @param light the scene light
 * @param out the packed light
 */
static void _scene_pack_light(const DvzLight* light, DvzSceneLightGpu* out)
{
    ANN(light);
    ANN(out);
    const DvzLightDesc* desc = &light->desc;
    out->color_intensity[0] = desc->color[0];
    out->color_intensity[1] = desc->color[1];
    out->color_intensity[2] = desc->color[2];
    out->color_intensity[3] = desc->intensity;
    out->direction_type[0] = desc->direction[0];
    out->direction_type[1] = desc->direction[1];
    out->direction_type[2] = desc->direction[2];
    out->direction_type[3] = (float)desc->type;
}



void _scene_emit_panel_light_uploads(
    DvzFigure* figure, DvzFramePlan* plan, const char* figure_id, DvzDiagnosticReport* report)
{
    ANN(figure);
    ANN(plan);
    ANN(figure_id);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        panel->lights.gpu_upload_pending = false;
        if (!panel->lights.gpu_dirty && panel->lights.gpu_realized)
            continue;
        bool needed = false;
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            const DvzVisual* visual = panel->visuals[vi].visual;
            if (visual != NULL && visual->visible && _scene_visual_needs_material_params(visual))
            {
                needed = true;
                break;
            }
        }
        if (!needed)
            continue;

        DvzScenePanelLightsGpu* payload = (DvzScenePanelLightsGpu*)dvz_calloc(
            1, sizeof(DvzScenePanelLightsGpu));
        if (payload == NULL)
        {
            (void)dvz_diagnostic_report_add(report, "panel light payload allocation failed");
            continue;
        }
        payload->active_count = panel->lights.count;
        for (uint32_t i = 0; i < panel->lights.count; i++)
            _scene_pack_light(panel->lights.items[i], &payload->lights[i]);

        char panel_id[DVZ_SCENE_LABEL_SIZE] = {0};
        char resource_id[DVZ_SCENE_LABEL_SIZE] = {0};
        dvz_snprintf(panel_id, sizeof(panel_id), "%s_p%u", figure_id, pi);
        if (!_scene_resource_key_panel_lights(panel_id, resource_id, sizeof(resource_id)) ||
            !dvz_frame_plan_upload_bytes(
                plan, resource_id, 0, sizeof(DvzScenePanelLightsGpu), "panel_lights", payload))
        {
            dvz_free(payload);
            (void)dvz_diagnostic_report_add(report, "panel light upload planning failed");
            continue;
        }

        DvzFramePlanNode* node = &plan->nodes[plan->count - 1];
        node->u.upload.owned_data = payload;
        node->u.upload.buffer_usage = DVZ_DRP2_BUFFER_USAGE_UNIFORM |
                                      DVZ_DRP2_BUFFER_USAGE_MAP_WRITE |
                                      DVZ_DRP2_BUFFER_USAGE_COPY_DST;
        DvzFramePlanUploadMeta metadata = {
            .kind = DVZ_FRAME_PLAN_RESOURCE_KIND_BUFFER,
            .role = DVZ_FRAME_PLAN_RESOURCE_ROLE_PANEL_LIGHTS,
            .buffer_index = UINT32_MAX,
            .logical_item_count = panel->lights.count,
            .has_logical_extent = true,
        };
        (void)dvz_frame_plan_upload_metadata(plan, &metadata);
        panel->lights.gpu_upload_pending = true;
    }
}
