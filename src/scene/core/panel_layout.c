/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Scene panel layout                                                                           */
/*************************************************************************************************/

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "core/scene_notify_internal.h"
#include "core/panel_layout_internal.h"
#include "_visual_internal.h"


/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return whether a panel layout reservation is finite and leaves non-empty plot space.
 *
 * @param reserve the reservation descriptor
 * @return whether the reservation is valid
 */
static bool _panel_layout_reserve_valid(const DvzPanelLayoutReserve* reserve)
{
    ANN(reserve);
    if (!isfinite(reserve->left) || !isfinite(reserve->right) || !isfinite(reserve->bottom) ||
        !isfinite(reserve->top))
        return false;
    if (reserve->left < 0.0f || reserve->right < 0.0f || reserve->bottom < 0.0f ||
        reserve->top < 0.0f)
        return false;
    return reserve->left + reserve->right < 2.0f && reserve->bottom + reserve->top < 2.0f;
}

static bool _panel_reserve_valid_for_padding(
    const DvzPanel* panel, const DvzPanelReserve* reserve, const DvzPanelReserve* padding);

/**
 * Return whether a panel pixel reservation is finite and leaves non-empty plot space.
 *
 * @param panel the panel
 * @param reserve the pixel reservation descriptor
 * @return whether the reservation is valid
 */
bool _panel_reserve_valid(const DvzPanel* panel, const DvzPanelReserve* reserve)
{
    ANN(panel);
    ANN(reserve);
    return _panel_reserve_valid_for_padding(panel, reserve, &panel->padding);
}



/**
 * Return a positive finite scale, falling back to one.
 *
 * @param scale input scale
 * @return valid scale
 */
float _scene_scale_or_one(float scale)
{
    return isfinite(scale) && scale > 0.0f ? scale : 1.0f;
}



/**
 * Resolve the scalar screen-space style scale for a figure.
 *
 * @param figure parent figure
 * @return device scale multiplied by user scale
 */
float _scene_screen_scale(const DvzFigure* figure)
{
    if (figure == NULL)
        return 1.0f;
    float sx = _scene_scale_or_one(figure->device_scale_x);
    float sy = _scene_scale_or_one(figure->device_scale_y);
    float user = _scene_scale_or_one(figure->user_scale);
    return 0.5f * (sx + sy) * user;
}



/**
 * Return whether one attribute stores screen-space style pixels.
 *
 * @param attr_name attribute name
 * @return whether the attribute should be regenerated after screen scale changes
 */
static bool _scene_attr_is_screen_space(const char* attr_name)
{
    return attr_name != NULL &&
           (strcmp(attr_name, "size") == 0 || strcmp(attr_name, "line_width") == 0 ||
            strcmp(attr_name, "sigma") == 0);
}



/**
 * Return whether one visual has CPU-side data for an attribute.
 *
 * @param visual the visual
 * @param attr_name the attribute name
 * @return whether the attribute exists and has data
 */
static bool _panel_visual_has_attr_data(const DvzVisual* visual, const char* attr_name)
{
    ANN(visual);
    ANN(attr_name);
    int attr_idx = _attr_index(visual, attr_name);
    return attr_idx >= 0 && visual->attrs[attr_idx].data != NULL &&
           visual->attrs[attr_idx].item_count > 0;
}



/**
 * Mark screen-space visual resources dirty after a DPI or user-scale change.
 *
 * @param figure the figure whose visible visuals should be marked
 */
void _scene_figure_mark_screen_space_dirty(DvzFigure* figure)
{
    ANN(figure);
    for (uint32_t pi = 0; pi < figure->panel_count; pi++)
    {
        DvzPanel* panel = &figure->panels[pi];
        _panel_mark_layout_changed(panel);
        for (uint32_t vi = 0; vi < panel->visual_count; vi++)
        {
            DvzVisual* visual = panel->visuals[vi].visual;
            if (visual == NULL)
                continue;
            for (uint32_t ai = 0; ai < visual->attr_count; ai++)
            {
                DvzVisualAttr* attr = &visual->attrs[ai];
                if (!_scene_attr_is_screen_space(attr->name) || attr->item_count == 0)
                    continue;
                attr->dirty_first_item = 0;
                attr->dirty_item_count = attr->item_count;
            }
            if (visual->type == DVZ_VISUAL_TYPE_POINT ||
                visual->type == DVZ_VISUAL_TYPE_MARKER ||
                visual->type == DVZ_VISUAL_TYPE_PATH)
            {
                _visual_family_state(visual)->material_params_dirty = true;
            }
            if (visual->type == DVZ_VISUAL_TYPE_SEGMENT)
                _visual_family_state(visual)->segment.gpu.dirty = true;
            if (visual->type == DVZ_VISUAL_TYPE_IMAGE &&
                _panel_visual_has_attr_data(visual, "position_px") &&
                _panel_visual_has_attr_data(visual, "extent_px"))
            {
                _visual_family_state(visual)->image_gpu.dirty = true;
            }
            if (visual->type == DVZ_VISUAL_TYPE_PATH)
                _visual_family_state(visual)->path.gpu.dirty = true;
        }
    }
}



/**
 * Return whether a panel pixel reservation leaves non-empty plot space with explicit padding.
 *
 * @param panel the panel
 * @param reserve the pixel reservation descriptor
 * @param padding the pixel padding descriptor
 * @return whether the reservation is valid
 */
static bool _panel_reserve_valid_for_padding(
    const DvzPanel* panel, const DvzPanelReserve* reserve, const DvzPanelReserve* padding)
{
    ANN(panel);
    ANN(reserve);
    ANN(padding);
    if (!isfinite(reserve->left_px) || !isfinite(reserve->right_px) ||
        !isfinite(reserve->top_px) || !isfinite(reserve->bottom_px))
        return false;
    if (reserve->left_px < 0.0f || reserve->right_px < 0.0f ||
        reserve->top_px < 0.0f || reserve->bottom_px < 0.0f)
        return false;

    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    if (!_panel_padding_valid(panel, padding))
        return false;
    width -= padding->left_px + padding->right_px;
    height -= padding->top_px + padding->bottom_px;
    return reserve->left_px + reserve->right_px < width &&
           reserve->top_px + reserve->bottom_px < height;
}


/**
 * Return whether two panel reservations are equivalent.
 *
 * @param a first reservation
 * @param b second reservation
 * @return whether all sides match within layout tolerance
 */
static bool _panel_reserve_equal(const DvzPanelReserve* a, const DvzPanelReserve* b)
{
    ANN(a);
    ANN(b);
    return fabsf(a->left_px - b->left_px) <= 1e-4f &&
           fabsf(a->right_px - b->right_px) <= 1e-4f &&
           fabsf(a->top_px - b->top_px) <= 1e-4f &&
           fabsf(a->bottom_px - b->bottom_px) <= 1e-4f;
}


/**
 * Add one reserve contribution into another one.
 *
 * @param reserve destination reservation
 * @param contribution contribution to add
 */
static void _panel_reserve_add(DvzPanelReserve* reserve, const DvzPanelReserve* contribution)
{
    ANN(reserve);
    ANN(contribution);
    reserve->left_px += contribution->left_px;
    reserve->right_px += contribution->right_px;
    reserve->top_px += contribution->top_px;
    reserve->bottom_px += contribution->bottom_px;
}


/**
 * Return whether a panel padding is finite and leaves non-empty inner panel space.
 *
 * @param panel the panel
 * @param padding the pixel padding descriptor
 * @return whether the padding is valid
 */
bool _panel_padding_valid(const DvzPanel* panel, const DvzPanelReserve* padding)
{
    ANN(panel);
    ANN(padding);
    if (!isfinite(padding->left_px) || !isfinite(padding->right_px) ||
        !isfinite(padding->top_px) || !isfinite(padding->bottom_px))
        return false;
    if (padding->left_px < 0.0f || padding->right_px < 0.0f ||
        padding->top_px < 0.0f || padding->bottom_px < 0.0f)
        return false;

    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    return padding->left_px + padding->right_px < width &&
           padding->top_px + padding->bottom_px < height;
}



/**
 * Resolve one panel's base and decoration reserve contributions.
 *
 * @param panel the panel
 * @return resolved reservation
 */
static DvzPanelReserve _panel_resolve_reserve(const DvzPanel* panel)
{
    ANN(panel);
    DvzPanelReserve reserve = panel->base_reserve;
    _panel_reserve_add(&reserve, &panel->axis_reserve);
    _panel_reserve_add(&reserve, &panel->colorbar_reserve);
    _panel_reserve_add(&reserve, &panel->legend_reserve);
    if (!_panel_reserve_valid(panel, &reserve))
    {
        if (_panel_reserve_valid(panel, &panel->base_reserve))
            reserve = panel->base_reserve;
        else
            reserve = (DvzPanelReserve){0};
    }
    return reserve;
}


/**
 * Recompute one panel's resolved reservation.
 *
 * @param panel the panel
 */
static bool _panel_update_reserve(DvzPanel* panel)
{
    ANN(panel);
    DvzPanelReserve next = _panel_resolve_reserve(panel);
    if (_panel_reserve_equal(&panel->reserve, &next))
        return false;
    panel->reserve = next;
    _panel_mark_layout_changed(panel);
    (void)_scene_panel_apply_domain_fit(panel);
    return true;
}



/**
 * Mark panel layout-dependent state dirty after a plot rectangle change.
 *
 * @param panel the panel
 */
void _panel_mark_layout_changed(DvzPanel* panel)
{
    ANN(panel);
    for (uint32_t dim = 0; dim < 2; dim++)
    {
        DvzAxis* axis = &panel->axes[dim];
        if (axis->panel == NULL)
            continue;
        axis->tick_cache_valid = false;
        axis->dirty = true;
        axis->version++;
    }
    for (uint32_t i = 0; i < panel->colorbar_count; i++)
    {
        DvzColorbar* colorbar = panel->colorbars[i];
        if (colorbar == NULL)
            continue;
        colorbar->dirty = true;
        colorbar->version = colorbar->version == UINT64_MAX ? 1 : colorbar->version + 1;
    }
    for (uint32_t i = 0; i < panel->legend_count; i++)
    {
        DvzLegend* legend = panel->legends[i];
        if (legend == NULL)
            continue;
        legend->dirty = true;
        legend->version = legend->version == UINT64_MAX ? 1 : legend->version + 1;
    }
    _scene_notify_request_frame(panel->figure);
}


/**
 * Set one panel's aggregate axis reserve contribution.
 *
 * @param panel the panel
 * @param reserve axis reserve contribution, or NULL for zero
 */
void _scene_panel_set_axis_reserve(DvzPanel* panel, const DvzPanelReserve* reserve)
{
    if (panel == NULL)
        return;
    DvzPanelReserve next = reserve != NULL ? *reserve : (DvzPanelReserve){0};
    if (!_panel_reserve_valid(panel, &next) && !_panel_reserve_equal(&next, &(DvzPanelReserve){0}))
        next = (DvzPanelReserve){0};
    if (_panel_reserve_equal(&panel->axis_reserve, &next))
        return;
    panel->axis_reserve = next;
    _panel_update_reserve(panel);
}


/**
 * Set one panel's aggregate colorbar reserve contribution.
 *
 * @param panel the panel
 * @param reserve colorbar reserve contribution, or NULL for zero
 */
void _scene_panel_set_colorbar_reserve(DvzPanel* panel, const DvzPanelReserve* reserve)
{
    if (panel == NULL)
        return;
    DvzPanelReserve next = reserve != NULL ? *reserve : (DvzPanelReserve){0};
    if (!_panel_reserve_valid(panel, &next) && !_panel_reserve_equal(&next, &(DvzPanelReserve){0}))
        next = (DvzPanelReserve){0};
    if (_panel_reserve_equal(&panel->colorbar_reserve, &next))
        return;
    panel->colorbar_reserve = next;
    _panel_update_reserve(panel);
}


/**
 * Set one panel's aggregate legend reserve contribution.
 *
 * @param panel the panel
 * @param reserve legend reserve contribution, or NULL for zero
 */
void _scene_panel_set_legend_reserve(DvzPanel* panel, const DvzPanelReserve* reserve)
{
    if (panel == NULL)
        return;
    DvzPanelReserve next = reserve != NULL ? *reserve : (DvzPanelReserve){0};
    if (!_panel_reserve_valid(panel, &next) && !_panel_reserve_equal(&next, &(DvzPanelReserve){0}))
        next = (DvzPanelReserve){0};
    if (_panel_reserve_equal(&panel->legend_reserve, &next))
        return;
    panel->legend_reserve = next;
    _panel_update_reserve(panel);
}




/**
 * Return the default panel layout reservation.
 *
 * @return default panel layout reservation
 */
DvzPanelLayoutReserve dvz_panel_layout_reserve(void)
{
    return (DvzPanelLayoutReserve){0};
}


/**
 * Set a fixed pixel reservation around one panel's plot area.
 *
 * @param panel the panel
 * @param reserve pixel reservation descriptor, or NULL for zero reserve
 * @return whether the reservation was accepted
 */
bool dvz_panel_set_reserve(DvzPanel* panel, const DvzPanelReserve* reserve)
{
    if (panel == NULL)
        return false;
    DvzPanelReserve next = reserve != NULL ? *reserve : (DvzPanelReserve){0};
    if (!_panel_reserve_valid(panel, &next))
        return false;
    panel->layout_reserve_enabled = false;
    panel->layout_reserve = dvz_panel_layout_reserve();
    if (_panel_reserve_equal(&panel->base_reserve, &next))
        return true;
    panel->base_reserve = next;
    _panel_update_reserve(panel);
    return true;
}


/**
 * Return one panel's fixed pixel reservation.
 *
 * @param panel the panel
 * @param out output pixel reservation
 * @return whether the reservation was written
 */
bool dvz_panel_get_reserve(const DvzPanel* panel, DvzPanelReserve* out)
{
    if (panel == NULL || out == NULL)
        return false;
    *out = panel->reserve;
    return true;
}


/**
 * Set a fixed pixel padding inside one panel's outer rectangle.
 *
 * @param panel the panel
 * @param padding pixel padding descriptor, or NULL for zero padding
 * @return whether the padding was accepted
 */
bool dvz_panel_set_padding(DvzPanel* panel, const DvzPanelReserve* padding)
{
    if (panel == NULL)
        return false;
    DvzPanelReserve next = padding != NULL ? *padding : (DvzPanelReserve){0};
    if (!_panel_padding_valid(panel, &next))
        return false;
    DvzPanelReserve reserve = panel->base_reserve;
    _panel_reserve_add(&reserve, &panel->axis_reserve);
    _panel_reserve_add(&reserve, &panel->colorbar_reserve);
    _panel_reserve_add(&reserve, &panel->legend_reserve);
    if (!_panel_reserve_valid_for_padding(panel, &reserve, &next))
        return false;
    if (_panel_reserve_equal(&panel->padding, &next))
        return true;

    panel->padding = next;
    bool reserve_changed = _panel_update_reserve(panel);
    if (!reserve_changed)
    {
        _panel_mark_layout_changed(panel);
        (void)_scene_panel_apply_domain_fit(panel);
    }
    return true;
}



/**
 * Return one panel's fixed pixel padding.
 *
 * @param panel the panel
 * @param out output pixel padding
 * @return whether the padding was written
 */
bool dvz_panel_get_padding(const DvzPanel* panel, DvzPanelReserve* out)
{
    if (panel == NULL || out == NULL)
        return false;
    *out = panel->padding;
    return true;
}



/**
 * Return one panel's current inner rectangle in figure pixel coordinates.
 *
 * @param panel the panel
 * @param out output inner rectangle in logical pixels
 * @return whether the rectangle was written
 */
bool dvz_panel_inner_rect_px(const DvzPanel* panel, DvzRect* out)
{
    if (panel == NULL || out == NULL)
        return false;
    _scene_panel_inner_pixel_rect(panel, &out->x, &out->y, &out->width, &out->height);
    return true;
}



/**
 * Return one panel's current plot rectangle in figure pixel coordinates.
 *
 * @param panel the panel
 * @param out output plot rectangle in logical pixels
 * @return whether the rectangle was written
 */
bool dvz_panel_plot_rect_px(const DvzPanel* panel, DvzRect* out)
{
    if (panel == NULL || out == NULL)
        return false;
    _scene_panel_plot_pixel_rect(panel, &out->x, &out->y, &out->width, &out->height);
    return true;
}


/**
 * Reserve visual-space room around one panel's plot area for future adornments.
 *
 * @param panel the panel
 * @param reserve reservation descriptor, or NULL for defaults
 * @return whether the reservation was accepted
 */
bool dvz_panel_set_layout_reserve(DvzPanel* panel, const DvzPanelLayoutReserve* reserve)
{
    if (panel == NULL)
        return false;
    DvzPanelLayoutReserve next = reserve != NULL ? *reserve : dvz_panel_layout_reserve();
    if (!_panel_layout_reserve_valid(&next))
        return false;

    panel->layout_reserve_enabled = true;
    panel->layout_reserve = next;
    return _scene_panel_refresh_layout_reserve(panel);
}


/**
 * Refresh one panel's pixel reserve from its normalized layout reserve.
 *
 * @param panel the panel
 * @return whether the reserve is valid after refresh
 */
bool _scene_panel_refresh_layout_reserve(DvzPanel* panel)
{
    if (panel == NULL)
        return false;
    if (!panel->layout_reserve_enabled)
    {
        (void)_scene_panel_apply_domain_fit(panel);
        return true;
    }
    DvzPanelLayoutReserve next = panel->layout_reserve;
    if (!_panel_layout_reserve_valid(&next))
        return false;

    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    DvzPanelReserve pixel_reserve = {
        .left_px = 0.5f * next.left * width,
        .right_px = 0.5f * next.right * width,
        .top_px = 0.5f * next.top * height,
        .bottom_px = 0.5f * next.bottom * height,
    };
    if (!_panel_reserve_valid(panel, &pixel_reserve))
        return false;
    if (_panel_reserve_equal(&panel->base_reserve, &pixel_reserve))
    {
        (void)_scene_panel_apply_domain_fit(panel);
        return true;
    }
    panel->base_reserve = pixel_reserve;
    _panel_update_reserve(panel);
    return true;
}


/**
 * Return one panel's layout reservation.
 *
 * @param panel the panel
 * @param out output reservation
 * @return whether the reservation was written
 */
bool dvz_panel_get_layout_reserve(DvzPanel* panel, DvzPanelLayoutReserve* out)
{
    if (panel == NULL || out == NULL)
        return false;
    if (panel->layout_reserve_enabled)
    {
        *out = panel->layout_reserve;
        return true;
    }
    float width = 0.0f;
    float height = 0.0f;
    _scene_panel_pixel_size(panel, &width, &height);
    DvzPanelReserve reserve = panel->base_reserve;
    if (!_panel_reserve_valid(panel, &reserve))
        reserve = (DvzPanelReserve){0};
    *out = (DvzPanelLayoutReserve){
        .left = width > 0.0f ? 2.0f * reserve.left_px / width : 0.0f,
        .right = width > 0.0f ? 2.0f * reserve.right_px / width : 0.0f,
        .bottom = height > 0.0f ? 2.0f * reserve.bottom_px / height : 0.0f,
        .top = height > 0.0f ? 2.0f * reserve.top_px / height : 0.0f,
    };
    return true;
}
