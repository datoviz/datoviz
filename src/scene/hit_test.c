/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene hit testing                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "datoviz/math/_cglm.h"
#include "_assertions.h"
#include "_compat.h"
#include "_scene.h"



/*************************************************************************************************/
/*  Function prototypes                                                                          */
/*************************************************************************************************/

static const char* _scene_pick_trace_path(void);

static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc);



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the active pick trace path.
 *
 * @return trace path, or NULL when tracing is disabled
 */
static const char* _scene_pick_trace_path(void)
{
    const char* path = getenv("DVZ_PICK_TRACE");
    if (path == NULL || path[0] == '\0' || strcmp(path, "0") == 0)
        return NULL;
    return path;
}



/**
 * Append one formatted line to the pick trace.
 *
 * @param format printf-compatible format string
 */
void _scene_pick_trace(const char* format, ...)
{
    const char* path = _scene_pick_trace_path();
    if (path == NULL)
        return;

    FILE* fp = fopen(path, "a");
    if (fp == NULL)
        return;

    va_list args;
    va_start(args, format);
    dvz_vfprintf(fp, format, args);
    va_end(args);
    fclose(fp);
}



/**
 * Shift an apply MVP so one NDC coordinate becomes the centered request target.
 *
 * @param mvp the MVP to update
 * @param ndc the NDC delta
 */
static void _scene_center_apply_mvp(DvzMVP* mvp, const vec2 ndc)
{
    ANN(mvp);
    mvp->proj[3][0] -= ndc[0];
    mvp->proj[3][1] -= ndc[1];
}



/*************************************************************************************************/
/*  Panel coordinates                                                                            */
/*************************************************************************************************/

/**
 * Convert a panel-local request coordinate to NDC.
 *
 * @param figure the figure
 * @param panel the panel
 * @param x the panel-local x coordinate
 * @param y the panel-local y coordinate
 * @param out_ndc the output NDC coordinate
 * @return true when the request is inside the panel
 */
bool _scene_pick_request_ndc(
    const DvzFigure* figure, const DvzPanel* panel, double x, double y, vec2 out_ndc)
{
    ANN(figure);
    ANN(panel);
    ANN(out_ndc);
    if (figure->width == 0 || figure->height == 0)
        return false;

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;

    double px = x / panel_width;
    double py = y / panel_height;
    if (px < 0.0 || px > 1.0 || py < 0.0 || py > 1.0)
        return false;

    out_ndc[0] = (float)(2.0 * px - 1.0);
    out_ndc[1] = (float)(1.0 - 2.0 * py);
    return true;
}



/**
 * Resolve the stable public panel id within one figure.
 *
 * @param figure the figure
 * @param panel the panel
 * @return the 1-based public panel id, or 1 when not found
 */
uint64_t _scene_panel_public_id(const DvzFigure* figure, const DvzPanel* panel)
{
    ANN(figure);
    ANN(panel);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        if (&figure->panels[pi] == panel)
            return (uint64_t)pi + 1;
    }
    return 1;
}



/**
 * Build an apply MVP that recenters one panel-local request onto the readback pixel.
 *
 * Image probing currently reads back one fixed pixel from a synthetic full-target render. The
 * request coordinate is therefore shifted onto the shared synthetic target NDC.
 *
 * @param panel the panel
 * @param request_ndc the requested panel-local NDC coordinate
 * @param out the destination MVP
 */
void _scene_request_apply_mvp(const DvzPanel* panel, const vec2 request_ndc, DvzMVP* out)
{
    ANN(panel);
    ANN(request_ndc);
    ANN(out);
    _scene_panel_apply_mvp(panel, out);
    vec2 target_ndc = {-0.75f, -0.75f};
    vec2 delta = {request_ndc[0] - target_ndc[0], request_ndc[1] - target_ndc[1]};
    _scene_center_apply_mvp(out, delta);
}



/*************************************************************************************************/
/*  CPU point picking                                                                            */
/*************************************************************************************************/

/**
 * Resolve one point-visual hit directly in panel pixel space.
 *
 * @param figure the figure
 * @param panel the panel
 * @param visual the point visual
 * @param x the panel-local request x coordinate
 * @param y the panel-local request y coordinate
 * @param out_item_id the resolved item id
 * @return true when the request falls inside a point sprite
 */
bool _scene_point_pick_cpu(
    const DvzFigure* figure, const DvzPanel* panel, const DvzVisual* visual, double x, double y,
    uint64_t* out_item_id)
{
    enum
    {
        TRACE_NEAREST_COUNT = 8,
    };

    ANN(figure);
    ANN(panel);
    ANN(visual);
    ANN(out_item_id);

    double panel_width = panel->desc.width * (double)figure->width;
    double panel_height = panel->desc.height * (double)figure->height;
    if (panel_width <= 0.0 || panel_height <= 0.0)
        return false;

    int pos_idx = _attr_index(visual, "position");
    int size_idx = _attr_index(visual, "size");
    if (pos_idx < 0 || size_idx < 0)
        return false;
    const DvzVisualAttr* pos_attr = &visual->attrs[pos_idx];
    const DvzVisualAttr* size_attr = &visual->attrs[size_idx];
    if (pos_attr->data == NULL || size_attr->data == NULL || pos_attr->item_count == 0 ||
        size_attr->item_count != pos_attr->item_count || pos_attr->item_size != sizeof(vec3) ||
        size_attr->item_size != sizeof(float))
    {
        _scene_pick_trace(
            "picker_point invalid_attrs visual=%p pos_idx=%d size_idx=%d pos_data=%p "
            "size_data=%p pos_count=%llu size_count=%llu pos_size=%u size_size=%u\n",
            (const void*)visual, pos_idx, size_idx, pos_attr->data, size_attr->data,
            (unsigned long long)pos_attr->item_count,
            (unsigned long long)size_attr->item_count, pos_attr->item_size, size_attr->item_size);
        return false;
    }

    DvzMVP mvp = {0};
    _scene_panel_apply_mvp(panel, &mvp);
    const vec3* positions = (const vec3*)pos_attr->data;
    const float* sizes = (const float*)size_attr->data;
    double nearest_metric[TRACE_NEAREST_COUNT] = {0};
    uint64_t nearest_id[TRACE_NEAREST_COUNT] = {0};
    double nearest_px[TRACE_NEAREST_COUNT] = {0};
    double nearest_py[TRACE_NEAREST_COUNT] = {0};
    double nearest_dx[TRACE_NEAREST_COUNT] = {0};
    double nearest_dy[TRACE_NEAREST_COUNT] = {0};
    double nearest_half[TRACE_NEAREST_COUNT] = {0};
    int nearest_hit[TRACE_NEAREST_COUNT] = {0};
    uint32_t nearest_count = 0;
    bool found = false;
    uint64_t selected = 0;

    for (uint64_t k = pos_attr->item_count; k > 0; k--)
    {
        uint64_t i = k - 1;
        vec4 p = {positions[i][0], positions[i][1], positions[i][2], 1.0f};
        vec4 tmp0 = {0};
        vec4 tmp1 = {0};
        vec4 clip = {0};
        glm_mat4_mulv(mvp.model, p, tmp0);
        glm_mat4_mulv(mvp.view, tmp0, tmp1);
        glm_mat4_mulv(mvp.proj, tmp1, clip);
        if (clip[3] == 0.0f)
            continue;

        double ndc_x = (double)(clip[0] / clip[3]);
        double ndc_y = (double)(clip[1] / clip[3]);
        double px = 0.5 * (ndc_x + 1.0) * panel_width;
        double py = 0.5 * (1.0 - ndc_y) * panel_height;
        double half_size = 0.5 * (double)sizes[i];
        double dx = px - x;
        double dy = py - y;
        double metric = fabs(dx) + fabs(dy);
        bool hit = fabs(dx) <= half_size && fabs(dy) <= half_size;

        uint32_t slot = nearest_count;
        if (nearest_count < TRACE_NEAREST_COUNT)
        {
            nearest_count++;
        }
        else
        {
            slot = 0;
            for (uint32_t j = 1; j < TRACE_NEAREST_COUNT; j++)
            {
                if (nearest_metric[j] > nearest_metric[slot])
                    slot = j;
            }
            if (metric >= nearest_metric[slot])
                slot = TRACE_NEAREST_COUNT;
        }
        if (slot < TRACE_NEAREST_COUNT)
        {
            nearest_metric[slot] = metric;
            nearest_id[slot] = i;
            nearest_px[slot] = px;
            nearest_py[slot] = py;
            nearest_dx[slot] = dx;
            nearest_dy[slot] = dy;
            nearest_half[slot] = half_size;
            nearest_hit[slot] = hit ? 1 : 0;
        }

        if (hit && !found)
        {
            found = true;
            selected = i;
        }
    }

    _scene_pick_trace(
        "picker_point request=%.3f,%.3f panel=%.3fx%.3f visual=%p count=%llu "
        "selected=%s%llu\n",
        x, y, panel_width, panel_height, (const void*)visual,
        (unsigned long long)pos_attr->item_count, found ? "" : "none/",
        (unsigned long long)selected);
    for (uint32_t j = 0; j < nearest_count; j++)
    {
        _scene_pick_trace(
            "picker_point_nearest rank_slot=%u id=%llu px=%.3f py=%.3f dx=%.3f dy=%.3f "
            "half=%.3f metric=%.3f hit=%d\n",
            j, (unsigned long long)nearest_id[j], nearest_px[j], nearest_py[j], nearest_dx[j],
            nearest_dy[j], nearest_half[j], nearest_metric[j], nearest_hit[j]);
    }

    if (found)
        *out_item_id = selected;
    return found;
}
