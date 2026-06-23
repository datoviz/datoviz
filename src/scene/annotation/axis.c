/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene 2D axes                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "_log.h"
#include "_scene.h"
#include "axis_internal.h"
#include "core/scene_notify_internal.h"
#include "datoviz/scene.h"
#include "annotation/text_visual_bridge.h"


/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#define DVZ_AXIS_TICK_POLICY_KNOWN_FLAGS 0u
#define DVZ_AXIS_STYLE_KNOWN_FLAGS 0u
#define DVZ_PANEL_AXES_2D_DESC_KNOWN_FLAGS 0u


static bool _axis_tick_policy_validate(const DvzAxisTickPolicy* policy)
{
    if (policy == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(policy, DvzAxisTickPolicy, DVZ_AXIS_TICK_POLICY_KNOWN_FLAGS))
    {
        log_error("invalid axis tick policy ABI");
        return false;
    }
    return true;
}


static bool _axis_style_validate(const DvzAxisStyle* style)
{
    if (style == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(style, DvzAxisStyle, DVZ_AXIS_STYLE_KNOWN_FLAGS))
    {
        log_error("invalid axis style ABI");
        return false;
    }
    return true;
}


static bool _panel_axes_2d_desc_validate(const DvzPanelAxes2DDesc* desc)
{
    if (desc == NULL)
        return true;
    if (!DVZ_STRUCT_VALID(desc, DvzPanelAxes2DDesc, DVZ_PANEL_AXES_2D_DESC_KNOWN_FLAGS))
    {
        log_error("invalid panel 2D axes descriptor ABI");
        return false;
    }
    if (!_axis_tick_policy_validate(&desc->tick_policy) || !_axis_style_validate(&desc->x_style) ||
        !_axis_style_validate(&desc->y_style))
        return false;
    return true;
}



/**
 * Return the default WIP axis tick policy.
 *
 * @return default axis tick policy
 */
DvzAxisTickPolicy _axis_default_tick_policy(void)
{
    return (DvzAxisTickPolicy){
        DVZ_STRUCT_INIT_FIELDS(DvzAxisTickPolicy),
        .target_count = 6,
        .min_pixel_spacing = 100.0f,
        .minor_per_interval = 4,
    };
}



/**
 * Return the default WIP axis line and text style.
 *
 * @return default axis style
 */
DvzAxisStyle _axis_default_style(void)
{
    return (DvzAxisStyle){
        DVZ_STRUCT_INIT_FIELDS(DvzAxisStyle),
        .spine_width = 1.0f,
        .major_tick_width = 1.0f,
        .minor_tick_width = 1.0f,
        .grid_width = 1.0f,
        .major_tick_length = 9.0f,
        .minor_tick_length = 5.0f,
        .reserve_px = 0.0f,
        .tick_gap_px = AXIS_TEXT_TICK_GAP,
        .label_gap_px = AXIS_TEXT_LABEL_GAP,
        .tick_size_px = AXIS_TEXT_TICK_SIZE,
        .label_size_px = AXIS_TEXT_LABEL_SIZE,
        .text_renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS,
        .plot_margin_left = 0.0f,
        .plot_margin_right = 0.0f,
        .plot_margin_bottom = 0.0f,
        .plot_margin_top = 0.0f,
        .spine_color = {220, 220, 220, 255},
        .major_tick_color = {220, 220, 220, 255},
        .minor_tick_color = {170, 170, 170, 220},
        .grid_color = {90, 95, 105, 180},
        .show_spine = true,
        .show_major_ticks = true,
        .show_minor_ticks = true,
        .show_grid = false,
    };
}



/**
 * Return whether a dimension is supported by the first WIP 2D axis slice.
 *
 * @param dim the dimension
 * @return true for X/Y
 */
bool _axis_dim_supported(DvzDim dim)
{
    return dim == DVZ_DIM_X || dim == DVZ_DIM_Y;
}



/**
 * Mark one axis layout and derived visuals dirty.
 *
 * @param axis the axis
 */
void _axis_mark_dirty(DvzAxis* axis)
{
    if (axis == NULL)
        return;
    axis->tick_cache_valid = false;
    axis->dirty = true;
    axis->version++;
    _scene_panel_refresh_axis_reserve(axis->panel);
    _scene_notify_request_frame(axis->panel != NULL ? axis->panel->figure : NULL);
}



/**
 * Return the panel-owned axis slot for a supported dimension.
 *
 * @param panel the panel
 * @param dim the dimension
 * @return the axis slot, or NULL
 */
DvzAxis* _panel_axis_slot(DvzPanel* panel, DvzDim dim)
{
    if (panel == NULL || !_axis_dim_supported(dim))
        return NULL;
    return &panel->axes[(uint32_t)dim];
}


/**
 * Initialize one panel-owned axis slot if it has not been initialized yet.
 *
 * @param axis the axis
 * @param panel the owning panel
 * @param dim the dimension
 */
void _axis_init(DvzAxis* axis, DvzPanel* panel, DvzDim dim)
{
    ANN(axis);
    ANN(panel);
    if (axis->panel != NULL)
        return;
    axis->panel = panel;
    axis->dim = dim;
    axis->enabled = false;
    axis->dirty = true;
    axis->domain_set = false;
    axis->domain = (DvzDataDomain){.min = -1.0, .max = +1.0};
    axis->tick_policy = _axis_default_tick_policy();
    axis->style = _axis_default_style();
    axis->tick_cache_valid = false;
}


/**
 * Return the visual-space plot interval for one axis dimension.
 *
 * @param axis the axis
 * @param out_min output plot minimum
 * @param out_max output plot maximum
 */
void _axis_plot_interval(const DvzAxis* axis, float* out_min, float* out_max)
{
    ANN(axis);
    ANN(out_min);
    ANN(out_max);
    const DvzAxisStyle* style = &axis->style;
    float plot[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (axis->panel != NULL)
        _scene_panel_plot_visual_rect(axis->panel, plot);
    if (axis->dim == DVZ_DIM_X)
    {
        *out_min = plot[0] + style->plot_margin_left;
        *out_max = plot[1] - style->plot_margin_right;
    }
    else
    {
        *out_min = plot[2] + style->plot_margin_bottom;
        *out_max = plot[3] - style->plot_margin_top;
    }
    if (*out_max <= *out_min)
    {
        *out_min = -1.0f;
        *out_max = +1.0f;
    }
}



/**
 * Convert one fixed visual-space coordinate to panel-local pixel coordinates.
 *
 * @param axis the axis owning the panel
 * @param visual_x visual-space x coordinate
 * @param visual_y visual-space y coordinate
 * @param out_x output x coordinate in panel-local pixels
 * @param out_y output y coordinate in panel-local pixels from the panel top
 */
void _axis_visual_to_pixels(
    const DvzAxis* axis, float visual_x, float visual_y, float* out_x, float* out_y)
{
    ANN(axis);
    ANN(out_x);
    ANN(out_y);
    float panel_x = 0.0f;
    float panel_y = 0.0f;
    float panel_width = 0.0f;
    float panel_height = 0.0f;
    _scene_panel_pixel_rect(axis->panel, &panel_x, &panel_y, &panel_width, &panel_height);
    (void)panel_x;
    (void)panel_y;
    *out_x = 0.5f * (visual_x + 1.0f) * panel_width;
    *out_y = 0.5f * (1.0f - visual_y) * panel_height;
}



/**
 * Map one data coordinate to fixed visual coordinates for an interval.
 *
 * @param value data value
 * @param min interval minimum
 * @param max interval maximum
 * @return visual coordinate
 */
float _axis_data_to_visual(
    double value, double min, double max, float visual_min, float visual_max)
{
    double denom = max - min;
    if (fabs(denom) < AXIS_EPS)
        return 0.0f;
    double t = (value - min) / denom;
    return (float)((double)visual_min + ((double)visual_max - (double)visual_min) * t);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static int _panel_set_domain(DvzPanel* panel, DvzDim dim, double min, double max, bool clear_fit)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || !isfinite(min) || !isfinite(max) || !(max > min))
        return -1;
    if (clear_fit)
        panel->view2d_enabled = false;
    _axis_init(axis, panel, dim);
    axis->domain = (DvzDataDomain){.min = min, .max = max};
    axis->domain_set = true;
    axis->tick_lstep = 0.0;
    axis->tick_cache_valid = false;
    axis->dirty = true;
    axis->version++;
    _scene_notify_request_frame(panel->figure);
    return 0;
}


/**
 * Set a panel data domain for one axis dimension.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param min data minimum
 * @param max data maximum
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_set_domain(DvzPanel* panel, DvzDim dim, double min, double max)
{
    return _panel_set_domain(panel, dim, min, max, true);
}


/**
 * Return the current visible data domain for one panel dimension.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @param out_min output visible data minimum
 * @param out_max output visible data maximum
 * @return whether the visible domain was written
 */
bool dvz_panel_visible_domain(DvzPanel* panel, DvzDim dim, double* out_min, double* out_max)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || out_min == NULL || out_max == NULL)
        return false;
    _axis_init(axis, panel, dim);
    return _axis_visible_domain(axis, out_min, out_max);
}


/**
 * Normalize tightly packed 3D data positions through the panel X/Y domains.
 *
 * @param panel the panel
 * @param data_positions tightly packed input positions, 3 floats per item
 * @param visual_positions tightly packed output positions, 3 floats per item
 * @param count number of positions
 * @return 0 on success, -1 on validation error
 */
int dvz_panel_data_to_visual_positions(
    DvzPanel* panel, const float* data_positions, float* visual_positions, uint32_t count)
{
    if (panel == NULL || data_positions == NULL || visual_positions == NULL)
        return -1;
    mat4 data_to_view = GLM_MAT4_IDENTITY_INIT;
    DvzAxis* x_axis = _panel_axis_slot(panel, DVZ_DIM_X);
    DvzAxis* y_axis = _panel_axis_slot(panel, DVZ_DIM_Y);
    bool has_x = x_axis != NULL && x_axis->panel != NULL && x_axis->domain_set;
    bool has_y = y_axis != NULL && y_axis->panel != NULL && y_axis->domain_set;
    if (panel->view2d_enabled)
    {
        if (!_scene_panel_data_model(panel, data_to_view))
            return -1;
    }
    else
    {
        if (has_x && fabs(x_axis->domain.max - x_axis->domain.min) < AXIS_EPS)
            return -1;
        if (has_y && fabs(y_axis->domain.max - y_axis->domain.min) < AXIS_EPS)
            return -1;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        float x = data_positions[3 * i + 0];
        float y = data_positions[3 * i + 1];
        float z = data_positions[3 * i + 2];
        if (!isfinite(x) || !isfinite(y) || !isfinite(z))
            return -1;
        if (panel->view2d_enabled)
        {
            visual_positions[3 * i + 0] = data_to_view[0][0] * x + data_to_view[3][0];
            visual_positions[3 * i + 1] = data_to_view[1][1] * y + data_to_view[3][1];
        }
        else
        {
            if (has_x)
            {
                float x0 = -1.0f;
                float x1 = +1.0f;
                _axis_plot_interval(x_axis, &x0, &x1);
                visual_positions[3 * i + 0] =
                    _axis_data_to_visual((double)x, x_axis->domain.min, x_axis->domain.max, x0, x1);
            }
            else
            {
                visual_positions[3 * i + 0] = x;
            }
            if (has_y)
            {
                float y0 = -1.0f;
                float y1 = +1.0f;
                _axis_plot_interval(y_axis, &y0, &y1);
                visual_positions[3 * i + 1] =
                    _axis_data_to_visual((double)y, y_axis->domain.min, y_axis->domain.max, y0, y1);
            }
            else
            {
                visual_positions[3 * i + 1] = y;
            }
        }
        visual_positions[3 * i + 2] = z;
    }
    return 0;
}



/**
 * Return a panel-owned axis, creating its derived primitive visuals on first use.
 *
 * @param panel the panel
 * @param dim axis dimension
 * @return the panel-owned axis, or NULL
 */
DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim)
{
    DvzAxis* axis = _panel_axis_slot(panel, dim);
    if (axis == NULL || panel->figure == NULL || panel->figure->scene == NULL)
        return NULL;
    _axis_init(axis, panel, dim);
    if (axis->visual == NULL)
    {
        axis->visual =
            dvz_primitive(panel->figure->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        if (axis->visual == NULL)
            return NULL;
        if (dvz_visual_set_depth_test(axis->visual, false) != 0)
            return NULL;
        axis->visual->visible = false;
        DvzVisualAttachDesc attach = dvz_visual_attach_desc();
        attach.z_layer = 1000;
        attach.controller_mode = DVZ_CONTROLLER_FIXED;
        attach.coord_space = DVZ_COORD_VIEW;
        if (dvz_panel_add_visual(panel, axis->visual, &attach) != 0)
        {
            axis->visual = NULL;
            return NULL;
        }
    }
    if (axis->grid_visual == NULL)
    {
        axis->grid_visual =
            dvz_primitive(panel->figure->scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0);
        if (axis->grid_visual == NULL)
            return NULL;
        if (dvz_visual_set_depth_test(axis->grid_visual, false) != 0)
            return NULL;
        axis->grid_visual->visible = false;
        DvzVisualAttachDesc attach = dvz_visual_attach_desc();
        attach.z_layer = -1;
        attach.controller_mode = DVZ_CONTROLLER_APPLY;
        attach.coord_space = DVZ_COORD_VIEW;
        if (dvz_panel_add_visual(panel, axis->grid_visual, &attach) != 0)
        {
            axis->grid_visual = NULL;
            return NULL;
        }
    }
    axis->enabled = true;
    _axis_mark_dirty(axis);
    return axis;
}


/**
 * Return the default 2D panel axes descriptor.
 *
 * @return default 2D axes descriptor
 */
DvzPanelAxes2DDesc dvz_panel_axes_2d_desc(void)
{
    DvzAxisStyle x_style = _axis_default_style();
    DvzAxisStyle y_style = _axis_default_style();
    x_style.show_grid = true;
    y_style.show_grid = true;

    return (DvzPanelAxes2DDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzPanelAxes2DDesc),
        .x_label = NULL,
        .y_label = NULL,
        .tick_policy = _axis_default_tick_policy(),
        .x_style = x_style,
        .y_style = y_style,
    };
}


/**
 * Apply common 2D axes defaults to a panel-owned X/Y axis pair.
 *
 * @param panel the panel
 * @param desc axes descriptor, or NULL for defaults
 * @return whether the axes were updated
 */
bool dvz_panel_set_axes_2d(DvzPanel* panel, const DvzPanelAxes2DDesc* desc)
{
    if (panel == NULL)
        return false;
    DvzPanelAxes2DDesc resolved = desc != NULL ? *desc : dvz_panel_axes_2d_desc();
    if (!_panel_axes_2d_desc_validate(&resolved))
        return false;

    DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (x_axis == NULL || y_axis == NULL)
        return false;

    if (!dvz_axis_set_tick_policy(x_axis, &resolved.tick_policy))
        return false;
    if (!dvz_axis_set_tick_policy(y_axis, &resolved.tick_policy))
        return false;
    if (!dvz_axis_set_style(x_axis, &resolved.x_style))
        return false;
    if (!dvz_axis_set_style(y_axis, &resolved.y_style))
        return false;
    if (!dvz_axis_set_label(x_axis, resolved.x_label))
        return false;
    if (!dvz_axis_set_label(y_axis, resolved.y_label))
        return false;
    if (!dvz_axis_set_visible(x_axis, true))
        return false;
    return dvz_axis_set_visible(y_axis, true);
}


/**
 * Return the default axis tick policy.
 *
 * @return default axis tick policy
 */
DvzAxisTickPolicy dvz_axis_tick_policy(void)
{
    return _axis_default_tick_policy();
}


/**
 * Return the default axis line and text style.
 *
 * @return default axis style
 */
DvzAxisStyle dvz_axis_style(void)
{
    return _axis_default_style();
}



/**
 * Show or hide one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether the axis is visible
 * @return whether the axis was updated
 */
bool dvz_axis_set_visible(DvzAxis* axis, bool visible)
{
    if (axis == NULL)
        return false;
    axis->enabled = visible;
    if (axis->visual != NULL && !visible)
        axis->visual->visible = false;
    if (axis->grid_visual != NULL && !visible)
        axis->grid_visual->visible = false;
    if (!visible)
        _axis_hide_text(axis);
    _axis_mark_dirty(axis);
    return true;
}
/**
 * Enable or disable grid lines for one panel-owned axis.
 *
 * @param axis the axis
 * @param visible whether grid lines are visible
 * @return whether the axis was updated
 */
bool dvz_axis_set_grid(DvzAxis* axis, bool visible)
{
    if (axis == NULL)
        return false;
    axis->style.show_grid = visible;
    _axis_mark_dirty(axis);
    return true;
}



/**
 * Set the label stored on one panel-owned axis.
 *
 * @param axis the axis
 * @param label label string, or NULL to clear
 * @return whether the axis was updated
 */
bool dvz_axis_set_label(DvzAxis* axis, const char* label)
{
    if (axis == NULL)
        return false;
    dvz_strlcpy(axis->label, label != NULL ? label : "", sizeof(axis->label));
    _axis_mark_dirty(axis);
    return true;
}



/**
 * Set the tick policy for one panel-owned axis.
 *
 * @param axis the axis
 * @param policy tick policy, or NULL for defaults
 * @return whether the axis was updated
 */
bool dvz_axis_set_tick_policy(DvzAxis* axis, const DvzAxisTickPolicy* policy)
{
    if (axis == NULL)
        return false;
    if (!_axis_tick_policy_validate(policy))
        return false;
    axis->tick_policy = policy != NULL ? *policy : _axis_default_tick_policy();
    axis->tick_lstep = 0.0;
    _axis_mark_dirty(axis);
    return true;
}



/**
 * Set the line and text style for one panel-owned axis.
 *
 * @param axis the axis
 * @param style axis style, or NULL for defaults
 * @return whether the axis was updated
 */
bool dvz_axis_set_style(DvzAxis* axis, const DvzAxisStyle* style)
{
    if (axis == NULL)
        return false;
    if (!_axis_style_validate(style))
        return false;
    axis->style = style != NULL ? *style : _axis_default_style();
    _axis_mark_dirty(axis);
    return true;
}


/**
 * Set plot-area margins for one panel-owned axis.
 *
 * @param axis the axis
 * @param left left margin
 * @param right right margin
 * @param bottom bottom margin
 * @param top top margin
 * @return whether the margins were updated
 */
bool dvz_axis_set_plot_margins(
    DvzAxis* axis, float left, float right, float bottom, float top)
{
    if (axis == NULL)
        return false;
    if (!isfinite(left) || !isfinite(right) || !isfinite(bottom) || !isfinite(top) ||
        left < 0.0f || right < 0.0f || bottom < 0.0f || top < 0.0f ||
        left + right >= 2.0f || bottom + top >= 2.0f)
        return false;
    axis->style.plot_margin_left = left;
    axis->style.plot_margin_right = right;
    axis->style.plot_margin_bottom = bottom;
    axis->style.plot_margin_top = top;
    _axis_mark_dirty(axis);
    return true;
}


/**
 * Attach numeric units to one panel-owned axis.
 *
 * @param axis the axis
 * @param units units object, or NULL to restore plain numeric formatting
 * @return whether the axis was updated
 */
bool dvz_axis_set_units(DvzAxis* axis, DvzUnits* units)
{
    if (axis == NULL)
        return false;
    if (units != NULL)
    {
        DvzScene* scene =
            axis->panel != NULL && axis->panel->figure != NULL ? axis->panel->figure->scene : NULL;
        if (units->scene != scene || units->ladder == NULL)
            return false;
    }
    axis->units = units;
    _axis_mark_dirty(axis);
    return true;
}


/**
 * Attach an absolute datetime formatter to one panel-owned axis.
 *
 * @param axis the axis
 * @param format datetime format, or NULL to restore numeric/unit formatting
 * @return whether the axis was updated
 */
bool dvz_axis_set_datetime(DvzAxis* axis, DvzDateTimeFormat* format)
{
    if (axis == NULL)
        return false;
    if (format != NULL)
    {
        DvzScene* scene =
            axis->panel != NULL && axis->panel->figure != NULL ? axis->panel->figure->scene : NULL;
        if (format->scene != scene)
            return false;
    }
    axis->datetime_format = format;
    axis->tick_lstep = 0.0;
    axis->tick_cache_valid = false;
    _axis_mark_dirty(axis);
    return true;
}


/**
 * Map compact axis data coordinates to absolute UTC timestamps.
 *
 * @param axis the axis
 * @param data0 first data coordinate
 * @param data1 second data coordinate
 * @param t0 timestamp corresponding to data0
 * @param t1 timestamp corresponding to data1
 * @return whether the mapping was updated
 */
bool dvz_axis_set_datetime_range(
    DvzAxis* axis, double data0, double data1, DvzTimestamp t0, DvzTimestamp t1)
{
    if (axis == NULL || !isfinite(data0) || !isfinite(data1) ||
        fabs(data1 - data0) <= AXIS_EPS || t0 == t1)
        return false;
    axis->datetime_data0 = data0;
    axis->datetime_data1 = data1;
    axis->datetime_t0 = t0;
    axis->datetime_t1 = t1;
    axis->datetime_range_set = true;
    axis->tick_lstep = 0.0;
    axis->tick_cache_valid = false;
    _axis_mark_dirty(axis);
    return true;
}
