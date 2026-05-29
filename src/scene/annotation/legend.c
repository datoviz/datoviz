/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene legends                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "datoviz/scene.h"
#include "prepare_internal.h"
#include "scale_internal.h"
#include "text/internal.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define LEGEND_RESERVE_PX 140.0f
#define LEGEND_EDGE_OFFSET_PX 8.0f
#define LEGEND_PLOT_GAP_PX 12.0f
#define LEGEND_ENTRY_GAP_PX 6.0f
#define LEGEND_MARK_SIZE_PX 12.0f
#define LEGEND_MARK_LABEL_GAP_PX 6.0f
#define LEGEND_TEXT_SIZE_PX 12.0f
#define LEGEND_TITLE_TEXT_SIZE_PX 13.0f
#define LEGEND_LAYOUT_EPS 1e-3f



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Report a legend realization error through logs and optional diagnostics.
 *
 * @param report optional diagnostic report
 * @param message the diagnostic message
 */
static void _legend_report(DvzDiagnosticReport* report, const char* message)
{
    ANN(message);
    log_error("%s", message);
    if (report != NULL)
        (void)dvz_diagnostic_report_add(report, message);
}



/**
 * Convert panel-local pixels to fixed panel visual coordinates.
 *
 * @param width panel width in pixels
 * @param height panel height in pixels
 * @param x x coordinate in pixels from the panel left
 * @param y y coordinate in pixels from the panel top
 * @param z z coordinate
 * @param out output visual position
 */
static void _legend_pixel_to_visual(
    float width, float height, float x, float y, float z, float out[3])
{
    ANN(out);
    out[0] = width > 0.0f ? 2.0f * x / width - 1.0f : -1.0f;
    out[1] = height > 0.0f ? 1.0f - 2.0f * y / height : 1.0f;
    out[2] = z;
}



/**
 * Mark one retained legend layout as dirty.
 *
 * @param legend the legend
 */
void _scene_mark_legend_dirty(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    legend->dirty = true;
    legend->version = legend->version == UINT64_MAX ? 1 : legend->version + 1;
    _scene_notify_request_frame(legend->panel != NULL ? legend->panel->figure : NULL);
}





/**
 * Return whether a legend anchor is supported by the first slice.
 *
 * @param anchor the anchor
 * @return whether the anchor is a panel edge
 */
static bool _legend_anchor_supported(DvzSceneAnchor anchor)
{
    return anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT || anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT ||
           anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
}


/**
 * Return a positive legend descriptor value or its fallback.
 *
 * @param value descriptor value
 * @param fallback default value
 * @return value when positive, otherwise fallback
 */
static float _legend_positive_or_default(float value, float fallback)
{
    return value > 0.0f && isfinite(value) ? value : fallback;
}


/**
 * Refresh aggregate attached legend reserve for one panel.
 *
 * @param panel the panel
 */
void _scene_panel_refresh_legend_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return;
    DvzPanelReserve reserve = {0};
    for (uint32_t i = 0; i < panel->legend_count; i++)
    {
        DvzLegend* legend = panel->legends[i];
        if (legend == NULL || legend->panel != panel)
            continue;
        DvzPanelReserve applied = {0};
        if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED &&
            _legend_anchor_supported(legend->anchor))
        {
            float reserve_px = _legend_positive_or_default(legend->reserve_px, LEGEND_RESERVE_PX);
            switch (legend->anchor)
            {
            case DVZ_SCENE_ANCHOR_PANEL_LEFT:
                applied.left_px = reserve_px;
                reserve.left_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
                applied.right_px = reserve_px;
                reserve.right_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_TOP:
                applied.top_px = reserve_px;
                reserve.top_px += reserve_px;
                break;
            case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
                applied.bottom_px = reserve_px;
                reserve.bottom_px += reserve_px;
                break;
            default:
                break;
            }
        }
        legend->auto_reserve = applied;
    }
    _scene_panel_set_legend_reserve(panel, &reserve);
}


/**
 * Apply the deterministic first-slice panel reserve for a legend edge.
 *
 * @param legend the legend
 */
static void _legend_apply_auto_reserve(DvzLegend* legend)
{
    ANN(legend);
    if (legend->panel == NULL)
        return;
    _scene_panel_refresh_legend_reserve(legend->panel);
}





/**
 * Hide all derived visuals owned by a legend.
 *
 * @param legend the legend
 */
static void _legend_hide(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    if (legend->mark_visual != NULL)
        dvz_visual_set_visible(legend->mark_visual, false);
    if (legend->text_visual != NULL)
    {
        dvz_visual_set_visible(legend->text_visual, false);
        if (legend->text_visual->text.glyph_visual != NULL)
            dvz_visual_set_visible(legend->text_visual->text.glyph_visual, false);
    }
}


/**
 * Hide one invalid legend and report the validation failure once per dirty cycle.
 *
 * @param legend the legend
 * @param report optional diagnostic report
 * @param message the diagnostic message
 */
static void _legend_fail(DvzLegend* legend, DvzDiagnosticReport* report, const char* message)
{
    ANN(legend);
    ANN(message);
    if (legend->dirty || report != NULL)
        _legend_report(report, message);
    legend->dirty = false;
    _legend_hide(legend);
}


/**
 * Ensure one legend-derived visual is attached to the panel.
 *
 * @param legend the legend
 * @param visual the visual
 * @param z_layer z layer for panel sorting
 * @return whether the visual is attached
 */
static bool _legend_attach_visual(DvzLegend* legend, DvzVisual* visual, int32_t z_layer)
{
    ANN(legend);
    ANN(legend->panel);
    ANN(visual);
    DvzVisualAttachDesc attach = {.z_layer = z_layer, .controller_mode = DVZ_CONTROLLER_FIXED};
    for (uint32_t i = 0; i < legend->panel->visual_count; i++)
    {
        DvzPanelAttach* existing = &legend->panel->visuals[i];
        if (existing->visual != visual)
            continue;
        existing->z_layer = attach.z_layer;
        existing->controller_mode = attach.controller_mode;
        return true;
    }
    return dvz_panel_add_visual(legend->panel, visual, &attach) == 0;
}


/**
 * Ensure derived mark and text visuals exist for a legend.
 *
 * @param legend the legend
 * @return whether all derived visuals exist
 */
static bool _legend_ensure_visuals(DvzLegend* legend)
{
    ANN(legend);
    if (legend->scene == NULL || legend->panel == NULL)
        return false;
    if (legend->mark_visual == NULL)
    {
        legend->mark_visual = dvz_marker(legend->scene, 0);
        if (legend->mark_visual == NULL)
            return false;
        legend->mark_visual->visible = false;
    }
    if (!_legend_attach_visual(legend, legend->mark_visual, 1002))
        return false;

    if (legend->text_visual == NULL)
    {
        legend->text_visual = _scene_adornment_text_visual(legend->scene, legend->text_renderer);
        if (legend->text_visual == NULL)
            return false;
        legend->text_visual->visible = false;
    }
    return _legend_attach_visual(legend, legend->text_visual, 1003);
}


/**
 * Append one text item to legend text arrays.
 *
 * @param legend the legend
 * @param label text label
 * @param x text position x in panel-local pixels
 * @param y text position y in panel-local pixels
 * @param anchor_x text anchor x
 * @param anchor_y text anchor y
 * @param size text size in pixels
 */
static void _legend_append_text(
    DvzLegend* legend, const char* label, float x, float y, float anchor_x, float anchor_y,
    float size)
{
    ANN(legend);
    ANN(label);
    if (legend->text_count >= DVZ_SCENE_MAX_LEGEND_TEXTS)
        return;
    uint32_t i = legend->text_count++;
    dvz_strlcpy(legend->text_labels[i], label, sizeof(legend->text_labels[i]));
    legend->text_positions[i][0] = x;
    legend->text_positions[i][1] = y;
    legend->text_positions[i][2] = 0.0f;
    legend->text_anchors[i][0] = anchor_x;
    legend->text_anchors[i][1] = anchor_y;
    legend->text_sizes[i] = size;
    legend->text_colors[i][0] = 255;
    legend->text_colors[i][1] = 255;
    legend->text_colors[i][2] = 255;
    legend->text_colors[i][3] = 255;
    legend->text_angles[i] = 0.0f;
}


/**
 * Update the legend batched text visual.
 *
 * @param legend the legend
 */
static void _legend_update_text(DvzLegend* legend)
{
    ANN(legend);
    if (legend->text_visual == NULL || legend->text_count == 0)
    {
        if (legend->text_visual != NULL)
            dvz_visual_set_visible(legend->text_visual, false);
        return;
    }
    if (_scene_adornment_text_visual_set_renderer(legend->text_visual, legend->text_renderer) != 0)
    {
        dvz_visual_set_visible(legend->text_visual, false);
        return;
    }
    const char* strings[DVZ_SCENE_MAX_LEGEND_TEXTS] = {0};
    for (uint32_t i = 0; i < legend->text_count; i++)
        strings[i] = legend->text_labels[i];
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = legend->text_positions, .item_count = legend->text_count},
        {.attr_name = "anchor", .data = legend->text_anchors, .item_count = legend->text_count},
        {.attr_name = "size", .data = legend->text_sizes, .item_count = legend->text_count},
        {.attr_name = "color", .data = legend->text_colors, .item_count = legend->text_count},
        {.attr_name = "angle", .data = legend->text_angles, .item_count = legend->text_count},
    };
    if (dvz_visual_set_strings(legend->text_visual, "text", strings, legend->text_count) == 0 &&
        dvz_visual_set_data_many(legend->text_visual, updates, 5) == 0)
    {
        dvz_visual_set_visible(legend->text_visual, true);
    }
    else
    {
        dvz_visual_set_visible(legend->text_visual, false);
    }
}


/**
 * Resolve an anchored detached legend rectangle to panel-local pixels.
 *
 * @param legend the legend
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _legend_detached_rect(
    const DvzLegend* legend, float panel_x, float panel_y, float panel_width, float panel_height,
    float out[4])
{
    ANN(legend);
    ANN(out);
    const DvzPlacement* placement = &legend->placement;
    float space_x = 0.0f;
    float space_y = 0.0f;
    float space_width = panel_width;
    float space_height = panel_height;
    if (placement->space == DVZ_PLACEMENT_SPACE_FIGURE && legend->panel != NULL &&
        legend->panel->figure != NULL)
    {
        space_x = -panel_x;
        space_y = -panel_y;
        space_width = legend->panel->figure->width > 0 ? (float)legend->panel->figure->width :
                                                         panel_width;
        space_height = legend->panel->figure->height > 0 ? (float)legend->panel->figure->height :
                                                           panel_height;
    }

    float default_height =
        (legend->scale != NULL ? (float)legend->scale->category_count : 1.0f) *
            (legend->mark_size_px + legend->entry_gap_px) +
        (legend->title[0] != '\0' ? 20.0f : 0.0f);
    float width = _legend_positive_or_default(placement->width_px, legend->reserve_px);
    float height = _legend_positive_or_default(placement->height_px, default_height);
    float x = space_x + placement->offset_x_px;
    if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_CENTER)
        x = space_x + 0.5f * (space_width - width) + placement->offset_x_px;
    else if (placement->horizontal_anchor == DVZ_HORIZONTAL_ANCHOR_RIGHT)
        x = space_x + space_width - width + placement->offset_x_px;

    float y = space_y + placement->offset_y_px;
    if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_CENTER)
        y = space_y + 0.5f * (space_height - height) + placement->offset_y_px;
    else if (placement->vertical_anchor == DVZ_VERTICAL_ANCHOR_BOTTOM)
        y = space_y + space_height - height + placement->offset_y_px;

    out[0] = x;
    out[1] = y;
    out[2] = x + width;
    out[3] = y + height;
}


/**
 * Resolve the legend content rectangle to panel-local pixels.
 *
 * @param legend the legend
 * @param panel_x panel x origin in figure pixels
 * @param panel_y panel y origin in figure pixels
 * @param panel_width panel width in pixels
 * @param panel_height panel height in pixels
 * @param out output content rectangle as x0, y0, x1, y1 in panel-local pixels
 */
static void _legend_content_rect(
    const DvzLegend* legend, float panel_x, float panel_y, float panel_width, float panel_height,
    float out[4])
{
    ANN(legend);
    ANN(out);
    if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_DETACHED)
    {
        _legend_detached_rect(legend, panel_x, panel_y, panel_width, panel_height, out);
        return;
    }

    float plot_x = 0.0f;
    float plot_y = 0.0f;
    float plot_width = 0.0f;
    float plot_height = 0.0f;
    _scene_panel_plot_pixel_rect(legend->panel, &plot_x, &plot_y, &plot_width, &plot_height);
    plot_x -= panel_x;
    plot_y -= panel_y;

    if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT)
    {
        out[0] = legend->edge_offset_px;
        out[2] = plot_x - legend->plot_gap_px;
        out[1] = plot_y + legend->edge_offset_px;
        out[3] = plot_y + plot_height - legend->edge_offset_px;
    }
    else if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT)
    {
        out[0] = plot_x + plot_width + legend->plot_gap_px;
        out[2] = panel_width - legend->edge_offset_px;
        out[1] = plot_y + legend->edge_offset_px;
        out[3] = plot_y + plot_height - legend->edge_offset_px;
    }
    else if (legend->anchor == DVZ_SCENE_ANCHOR_PANEL_TOP)
    {
        out[0] = plot_x + legend->edge_offset_px;
        out[2] = plot_x + plot_width - legend->edge_offset_px;
        out[1] = legend->edge_offset_px;
        out[3] = plot_y - legend->plot_gap_px;
    }
    else
    {
        out[0] = plot_x + legend->edge_offset_px;
        out[2] = plot_x + plot_width - legend->edge_offset_px;
        out[1] = plot_y + plot_height + legend->plot_gap_px;
        out[3] = panel_height - legend->edge_offset_px;
    }
}


/**
 * Return sorted category indices for a legend scale.
 *
 * @param scale the categorical scale
 * @param out output index table
 * @return number of indices written
 */
static uint32_t _legend_sorted_category_indices(const DvzScale* scale, uint32_t* out)
{
    ANN(scale);
    ANN(out);
    uint32_t count = scale->category_count;
    for (uint32_t i = 0; i < count; i++)
        out[i] = i;
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            const DvzScaleCategoryState* a = &scale->categories[out[i]];
            const DvzScaleCategoryState* b = &scale->categories[out[j]];
            if (b->order < a->order)
            {
                uint32_t tmp = out[i];
                out[i] = out[j];
                out[j] = tmp;
            }
        }
    }
    return count;
}


/**
 * Return whether a category id is highlighted in one retained legend.
 *
 * @param legend the legend
 * @param id the category id
 * @return whether the category is highlighted
 */
static bool _legend_category_highlighted(const DvzLegend* legend, DvzCategoryId id)
{
    ANN(legend);
    for (uint32_t i = 0; i < legend->highlight_count; i++)
    {
        if (legend->highlighted_ids[i] == id)
            return true;
    }
    return false;
}


/**
 * Rebuild the derived visuals for one retained legend.
 *
 * @param legend the legend
 * @param report optional diagnostic report
 */
static void _legend_update_visuals(DvzLegend* legend, DvzDiagnosticReport* report)
{
    ANN(legend);
    if (legend->scene == NULL || legend->panel == NULL || legend->scale == NULL)
        return;
    if (legend->placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED &&
        !_legend_anchor_supported(legend->anchor))
    {
        _legend_fail(legend, report, "attached legend anchor must be a panel edge");
        return;
    }
    if (legend->scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        _legend_fail(legend, report, "legends require a categorical scale");
        return;
    }
    if (legend->scale->category_count == 0)
    {
        _legend_fail(legend, report, "legend scale has no retained categories");
        return;
    }

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(legend->panel, &panel_x, &panel_y, &width, &height);
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
    {
        legend->realized_panel_width = 0.0f;
        legend->realized_panel_height = 0.0f;
        _legend_hide(legend);
        return;
    }
    legend->realized_panel_width = width;
    legend->realized_panel_height = height;
    if (!_legend_ensure_visuals(legend))
    {
        _legend_hide(legend);
        return;
    }

    float rect[4] = {0};
    _legend_content_rect(legend, panel_x, panel_y, width, height, rect);
    if (rect[0] < 0.0f || rect[1] < 0.0f || rect[2] > width || rect[3] > height ||
        rect[2] <= rect[0] || rect[3] <= rect[1])
    {
        _legend_fail(legend, report, "panel is too small for deterministic legend layout");
        return;
    }

    uint32_t sorted[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    uint32_t count = _legend_sorted_category_indices(legend->scale, sorted);
    legend->entry_count = count;
    legend->text_count = 0;

    if (legend->title[0] != '\0')
    {
        _legend_append_text(
            legend, legend->title, rect[0], rect[1], 0.0f, 0.0f, LEGEND_TITLE_TEXT_SIZE_PX);
    }

    float mark_positions[DVZ_SCENE_MAX_SCALE_CATEGORIES][3] = {{0}};
    DvzColor mark_colors[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {{0}};
    float mark_sizes[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    float mark_angles[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    uint32_t mark_shapes[DVZ_SCENE_MAX_SCALE_CATEGORIES] = {0};
    float y = rect[1] + (legend->title[0] != '\0' ? 24.0f : 0.0f) + 0.5f * legend->mark_size_px;
    uint32_t mark_count = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvzScaleCategoryState* category = &legend->scale->categories[sorted[i]];
        float mark_x = rect[0] + 0.5f * legend->mark_size_px;
        if (y + 0.5f * legend->mark_size_px > rect[3])
            break;
        _legend_pixel_to_visual(width, height, mark_x, y, 0.0f, mark_positions[mark_count]);
        mark_colors[mark_count] = category->color;
        mark_sizes[mark_count] = _legend_category_highlighted(legend, category->category_id) ?
                                     1.45f * legend->mark_size_px :
                                     legend->mark_size_px;
        mark_angles[mark_count] = 0.0f;
        mark_shapes[mark_count] = DVZ_MARKER_SHAPE_SQUARE;

        char fallback[32] = {0};
        const char* label = category->has_label ? category->label : fallback;
        if (!category->has_label)
            dvz_snprintf(fallback, sizeof(fallback), "%" PRId64, category->category_id);
        _legend_append_text(
            legend, label, rect[0] + legend->mark_size_px + legend->mark_label_gap_px, y, 0.0f,
            0.5f, LEGEND_TEXT_SIZE_PX);
        y += legend->mark_size_px + legend->entry_gap_px;
        mark_count++;
    }

    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = mark_positions, .item_count = mark_count},
        {.attr_name = "color", .data = mark_colors, .item_count = mark_count},
        {.attr_name = "diameter", .data = mark_sizes, .item_count = mark_count},
        {.attr_name = "angle", .data = mark_angles, .item_count = mark_count},
        {.attr_name = "shape", .data = mark_shapes, .item_count = mark_count},
    };
    if (mark_count > 0 && dvz_visual_set_data_many(legend->mark_visual, updates, 5) == 0)
        dvz_visual_set_visible(legend->mark_visual, true);
    else
        dvz_visual_set_visible(legend->mark_visual, false);
    _legend_update_text(legend);
    legend->dirty = false;
}


/**
 * Return whether one retained legend needs its derived visuals rebuilt.
 *
 * @param legend the legend
 * @return whether the legend visual payloads need rebuilding
 */
static bool _legend_needs_visual_update(const DvzLegend* legend)
{
    ANN(legend);
    if (legend->dirty || legend->mark_visual == NULL || legend->text_visual == NULL)
        return true;
    if (legend->panel == NULL)
        return false;

    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_rect(legend->panel, &panel_x, &panel_y, &width, &height);
    (void)panel_x;
    (void)panel_y;
    if (!(width > 0.0f) || !(height > 0.0f) || !isfinite(width) || !isfinite(height))
        return true;
    return fabsf(width - legend->realized_panel_width) > LEGEND_LAYOUT_EPS ||
           fabsf(height - legend->realized_panel_height) > LEGEND_LAYOUT_EPS;
}






/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a panel-attached legend bound to a categorical scale.
 *
 * @param panel the panel
 * @param scale the categorical scale
 * @param desc the legend descriptor, or NULL for defaults
 * @return the legend, or NULL on allocation failure
 */
DvzLegend* dvz_legend(DvzPanel* panel, DvzScale* scale, const DvzLegendDesc* desc)
{
    ANN(panel);
    ANN(scale);
    if (panel->figure == NULL || panel->figure->scene == NULL)
    {
        log_error("cannot create a legend on a detached panel");
        return NULL;
    }
    DvzScene* scene = panel->figure->scene;
    if (scale->scene != scene)
    {
        log_error("cannot attach a scale from a different scene to a panel legend");
        return NULL;
    }
    if (scale->kind != DVZ_SCALE_CATEGORICAL)
    {
        log_error("legends require a categorical scale; use a colorbar for continuous scales");
        return NULL;
    }
    DvzLegendPlacementMode placement_mode =
        desc != NULL ? desc->placement_mode : DVZ_LEGEND_PLACEMENT_ATTACHED;
    DvzSceneAnchor anchor = desc != NULL && desc->anchor != DVZ_SCENE_ANCHOR_NONE ?
                                desc->anchor :
                                DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    if (placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED && !_legend_anchor_supported(anchor))
    {
        log_error("attached legend anchor must be a panel edge");
        return NULL;
    }
    if (scene->legend_count >= DVZ_SCENE_MAX_LEGENDS)
    {
        log_error("maximum legend count reached");
        return NULL;
    }
    if (panel->legend_count >= DVZ_SCENE_MAX_PANEL_LEGENDS)
    {
        log_error("maximum panel legend count reached");
        return NULL;
    }

    DvzLegend* legend = &scene->legends[scene->legend_count++];
    dvz_memset(legend, sizeof(DvzLegend), 0, sizeof(DvzLegend));
    legend->scene = scene;
    legend->panel = panel;
    legend->scale = scale;
    legend->placement_mode = placement_mode;
    legend->anchor = anchor;
    legend->flags = desc != NULL ? desc->flags : 0;
    legend->reserve_px =
        _legend_positive_or_default(desc != NULL ? desc->reserve_px : 0.0f, LEGEND_RESERVE_PX);
    legend->edge_offset_px = _legend_positive_or_default(
        desc != NULL ? desc->edge_offset_px : 0.0f, LEGEND_EDGE_OFFSET_PX);
    legend->plot_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->plot_gap_px : 0.0f, LEGEND_PLOT_GAP_PX);
    legend->entry_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->entry_gap_px : 0.0f, LEGEND_ENTRY_GAP_PX);
    legend->mark_size_px = _legend_positive_or_default(
        desc != NULL ? desc->mark_size_px : 0.0f, LEGEND_MARK_SIZE_PX);
    legend->mark_label_gap_px = _legend_positive_or_default(
        desc != NULL ? desc->mark_label_gap_px : 0.0f, LEGEND_MARK_LABEL_GAP_PX);
    legend->text_renderer = _scene_adornment_text_renderer(
        desc != NULL && desc->text_renderer != 0 ? desc->text_renderer :
                                                   DVZ_TEXT_RENDERER_MSDF_ATLAS);
    legend->placement =
        desc != NULL ? desc->placement :
                       (DvzPlacement){
                           .space = DVZ_PLACEMENT_SPACE_PANEL,
                           .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_LEFT,
                           .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
                       };
    if (desc != NULL && desc->title != NULL)
        dvz_strlcpy(legend->title, desc->title, sizeof(legend->title));
    legend->dirty = true;
    legend->version = 1;
    panel->legends[panel->legend_count++] = legend;
    _legend_apply_auto_reserve(legend);
    return legend;
}


/**
 * Destroy a legend.
 *
 * @param legend the legend
 */
void dvz_legend_destroy(DvzLegend* legend)
{
    if (legend == NULL)
        return;
    _legend_hide(legend);
    DvzPanel* panel = legend->panel;
    if (panel != NULL)
    {
        for (uint32_t i = 0; i < panel->legend_count; i++)
        {
            if (panel->legends[i] != legend)
                continue;
            for (uint32_t j = i + 1; j < panel->legend_count; j++)
                panel->legends[j - 1] = panel->legends[j];
            panel->legends[panel->legend_count - 1] = NULL;
            panel->legend_count--;
            break;
        }
        _scene_panel_refresh_legend_reserve(panel);
    }
    legend->scene = NULL;
    legend->panel = NULL;
    legend->scale = NULL;
    legend->dirty = false;
    legend->mark_visual = NULL;
    legend->text_visual = NULL;
    legend->entry_count = 0;
}


/**
 * Update legend layout and placement parameters.
 *
 * @param legend the legend
 * @param desc layout descriptor
 * @return true when the layout was accepted
 */
bool dvz_legend_set_layout(DvzLegend* legend, const DvzLegendDesc* desc)
{
    ANN(legend);
    if (desc == NULL)
        return false;
    DvzLegendPlacementMode placement_mode = desc->placement_mode;
    DvzSceneAnchor anchor =
        desc->anchor != DVZ_SCENE_ANCHOR_NONE ? desc->anchor : DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    if (placement_mode == DVZ_LEGEND_PLACEMENT_ATTACHED && !_legend_anchor_supported(anchor))
    {
        log_error("attached legend anchor must be a panel edge");
        return false;
    }
    legend->placement_mode = placement_mode;
    legend->anchor = anchor;
    legend->flags = desc->flags;
    legend->reserve_px = _legend_positive_or_default(desc->reserve_px, LEGEND_RESERVE_PX);
    legend->edge_offset_px =
        _legend_positive_or_default(desc->edge_offset_px, LEGEND_EDGE_OFFSET_PX);
    legend->plot_gap_px = _legend_positive_or_default(desc->plot_gap_px, LEGEND_PLOT_GAP_PX);
    legend->entry_gap_px = _legend_positive_or_default(desc->entry_gap_px, LEGEND_ENTRY_GAP_PX);
    legend->mark_size_px = _legend_positive_or_default(desc->mark_size_px, LEGEND_MARK_SIZE_PX);
    legend->mark_label_gap_px =
        _legend_positive_or_default(desc->mark_label_gap_px, LEGEND_MARK_LABEL_GAP_PX);
    legend->text_renderer = _scene_adornment_text_renderer(
        desc->text_renderer != 0 ? desc->text_renderer : DVZ_TEXT_RENDERER_MSDF_ATLAS);
    legend->placement = desc->placement;
    if (desc->title != NULL)
        dvz_strlcpy(legend->title, desc->title, sizeof(legend->title));
    _legend_apply_auto_reserve(legend);
    _scene_mark_legend_dirty(legend);
    return true;
}


/**
 * Set the legend title.
 *
 * @param legend the legend
 * @param title the title, or NULL to clear
 */
void dvz_legend_set_title(DvzLegend* legend, const char* title)
{
    ANN(legend);
    const char* src = title != NULL ? title : "";
    if (strcmp(legend->title, src) == 0)
        return;
    dvz_strlcpy(legend->title, src, sizeof(legend->title));
    _scene_mark_legend_dirty(legend);
}


/**
 * Highlight one categorical legend entry.
 *
 * @param legend the legend
 * @param id category id to highlight
 * @return true when the highlight state was accepted
 */
bool dvz_legend_set_highlight(DvzLegend* legend, DvzCategoryId id)
{
    return dvz_legend_set_highlights(legend, &id, 1);
}


/**
 * Clear all highlighted categorical legend entries.
 *
 * @param legend the legend
 * @return true when the highlight state was accepted
 */
bool dvz_legend_clear_highlight(DvzLegend* legend)
{
    ANN(legend);
    if (legend->highlight_count == 0)
        return true;
    dvz_memset(
        legend->highlighted_ids, sizeof(legend->highlighted_ids), 0,
        sizeof(legend->highlighted_ids));
    legend->highlight_count = 0;
    _scene_mark_legend_dirty(legend);
    return true;
}


/**
 * Highlight multiple categorical legend entries.
 *
 * @param legend the legend
 * @param ids category ids to highlight
 * @param count number of highlighted category ids
 * @return true when the highlight state was accepted
 */
bool dvz_legend_set_highlights(DvzLegend* legend, const DvzCategoryId* ids, uint32_t count)
{
    ANN(legend);
    if (count > DVZ_SCENE_MAX_SCALE_CATEGORIES)
    {
        log_error("too many legend highlights (%" PRIu32 " > %u)", count,
                  DVZ_SCENE_MAX_SCALE_CATEGORIES);
        return false;
    }
    if (count > 0 && ids == NULL)
    {
        log_error("legend highlight ids are required when count is nonzero");
        return false;
    }

    if (legend->highlight_count == count)
    {
        bool same = true;
        for (uint32_t i = 0; i < count; i++)
        {
            if (legend->highlighted_ids[i] != ids[i])
            {
                same = false;
                break;
            }
        }
        if (same)
            return true;
    }

    dvz_memset(
        legend->highlighted_ids, sizeof(legend->highlighted_ids), 0,
        sizeof(legend->highlighted_ids));
    if (count > 0)
        dvz_memcpy(
            legend->highlighted_ids, count * sizeof(DvzCategoryId), ids,
            count * sizeof(DvzCategoryId));
    legend->highlight_count = count;
    _scene_mark_legend_dirty(legend);
    return true;
}



/**
 * Rebuild all retained legend visuals before FramePlan emission.
 *
 * @param figure the figure
 * @param report optional diagnostic report
 */
void _scene_prepare_legend_visuals(DvzFigure* figure, DvzDiagnosticReport* report)
{
    if (figure == NULL || figure->scene == NULL)
        return;
    DvzScene* scene = figure->scene;
    for (uint32_t i = 0; i < scene->legend_count; i++)
    {
        DvzLegend* legend = &scene->legends[i];
        if (legend->scene != scene || legend->panel == NULL || legend->panel->figure != figure)
            continue;
        _legend_apply_auto_reserve(legend);
        if (_legend_needs_visual_update(legend))
            _legend_update_visuals(legend, report);
    }
}
