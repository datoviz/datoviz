/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* colorbar - static scalar image with a scene-generated colorbar.
 *
 * Build:  just example-c visuals/colorbar
 * Run:    ./build/examples/c/visuals/colorbar
 * Smoke:  ./build/examples/c/visuals/colorbar 1
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH      800
#define HEIGHT     600
#define FIELD_SIZE 96
#define SCALE_MIN  0.0
#define SCALE_MAX  30.0
#define FIELD_MIN  10.0f
#define FIELD_MAX  20.0f
#define COLORMAP_COUNT 6u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ColorbarState
{
    DvzScale* scale;
    DvzPanel* panel;
    DvzColorbar* colorbar;
    DvzAxis* x_axis;
    DvzAxis* y_axis;
    DvzAxisStyle x_axis_style;
    DvzAxisStyle y_axis_style;
    DvzColormap* colormaps[COLORMAP_COUNT];
    DvzView* win;
    int colormap_index;
    int placement_mode;
    int orientation;
    int anchor_index;
    int placement_space;
    int horizontal_anchor;
    int vertical_anchor;
    int precision;
    double range_min;
    double range_max;
    float reserve_px;
    float ramp_width_px;
    float edge_offset_px;
    float plot_gap_px;
    float tick_length_px;
    float label_gap_px;
    float detached_offset_x;
    float detached_offset_y;
    float detached_width;
    float detached_height;
    int padding_left_px;
    int padding_right_px;
    int padding_top_px;
    int padding_bottom_px;
    bool show_x_axis;
    bool show_y_axis;
    bool show_grid;
    float x_axis_reserve_px;
    float y_axis_reserve_px;
} ColorbarState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Fill a scalar image with a smooth bounded synthetic field.
 *
 * @param values output scalar field values
 */
static void _fill_field(float values[FIELD_SIZE * FIELD_SIZE])
{
    for (uint32_t y = 0; y < FIELD_SIZE; y++)
    {
        for (uint32_t x = 0; x < FIELD_SIZE; x++)
        {
            float fx = (float)x / (float)(FIELD_SIZE - 1);
            float fy = (float)y / (float)(FIELD_SIZE - 1);
            float ridge = 1.0f - 4.0f * (fx - 0.60f) * (fx - 0.60f);
            float basin = 1.0f - 5.0f * (fy - 0.35f) * (fy - 0.35f);
            float diagonal = 0.45f * fx + 0.25f * fy;
            float value = 0.20f + 0.30f * ridge + 0.28f * basin + diagonal;
            if (value < 0.0f)
                value = 0.0f;
            if (value > 1.0f)
                value = 1.0f;
            values[y * FIELD_SIZE + x] = FIELD_MIN + (FIELD_MAX - FIELD_MIN) * value;
        }
    }
}


/**
 * Return the colorbar anchor enum selected by the GUI.
 *
 * @param index GUI anchor index
 * @return scene anchor
 */
static DvzSceneAnchor _anchor_from_index(int index)
{
    switch (index)
    {
    case 1:
        return DVZ_SCENE_ANCHOR_PANEL_LEFT;
    case 2:
        return DVZ_SCENE_ANCHOR_PANEL_TOP;
    case 3:
        return DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
    default:
        return DVZ_SCENE_ANCHOR_PANEL_RIGHT;
    }
}



/**
 * Return an orientation-compatible colorbar anchor selected by the GUI.
 *
 * @param state colorbar example state
 * @param orientation resolved colorbar orientation
 * @return scene anchor
 */
static DvzSceneAnchor
_anchor_from_state(ColorbarState* state, DvzColorbarOrientation orientation)
{
    if (state == NULL)
        return _anchor_from_index(0);
    DvzSceneAnchor anchor = _anchor_from_index(state->anchor_index);
    if (orientation == DVZ_COLORBAR_ORIENTATION_HORIZONTAL)
    {
        if (anchor == DVZ_SCENE_ANCHOR_PANEL_TOP || anchor == DVZ_SCENE_ANCHOR_PANEL_BOTTOM)
            return anchor;
        state->anchor_index = 3;
        return DVZ_SCENE_ANCHOR_PANEL_BOTTOM;
    }
    if (anchor == DVZ_SCENE_ANCHOR_PANEL_LEFT || anchor == DVZ_SCENE_ANCHOR_PANEL_RIGHT)
        return anchor;
    state->anchor_index = 0;
    return DVZ_SCENE_ANCHOR_PANEL_RIGHT;
}



/**
 * Return the placement space selected by the GUI.
 *
 * @param index GUI placement-space index
 * @return placement space
 */
static DvzPlacementSpace _placement_space_from_index(int index)
{
    return index == 1 ? DVZ_PLACEMENT_SPACE_FIGURE : DVZ_PLACEMENT_SPACE_PANEL;
}


/**
 * Return the horizontal placement anchor selected by the GUI.
 *
 * @param index GUI horizontal-anchor index
 * @return horizontal anchor
 */
static DvzHorizontalAnchor _horizontal_anchor_from_index(int index)
{
    if (index == 1)
        return DVZ_HORIZONTAL_ANCHOR_CENTER;
    if (index == 2)
        return DVZ_HORIZONTAL_ANCHOR_RIGHT;
    return DVZ_HORIZONTAL_ANCHOR_LEFT;
}


/**
 * Return the vertical placement anchor selected by the GUI.
 *
 * @param index GUI vertical-anchor index
 * @return vertical anchor
 */
static DvzVerticalAnchor _vertical_anchor_from_index(int index)
{
    if (index == 1)
        return DVZ_VERTICAL_ANCHOR_CENTER;
    if (index == 2)
        return DVZ_VERTICAL_ANCHOR_BOTTOM;
    return DVZ_VERTICAL_ANCHOR_TOP;
}


/**
 * Return the example's default axis style.
 *
 * @return axis style
 */
static DvzAxisStyle _axis_style(void)
{
    return (DvzAxisStyle){
        .spine_width = 1.0f,
        .major_tick_width = 1.0f,
        .minor_tick_width = 1.0f,
        .grid_width = 1.0f,
        .major_tick_length = 9.0f,
        .minor_tick_length = 5.0f,
        .reserve_px = 0.0f,
        .tick_gap_px = 6.0f,
        .label_gap_px = 28.0f,
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
 * Apply the current colorbar layout controls.
 *
 * @param state colorbar example state
 */
static void _apply_colorbar_layout(ColorbarState* state)
{
    if (state == NULL || state->colorbar == NULL)
        return;
    DvzColorbarOrientation orientation =
        state->orientation == 1 ? DVZ_COLORBAR_ORIENTATION_HORIZONTAL :
                                  DVZ_COLORBAR_ORIENTATION_VERTICAL;
    DvzColorbarPlacementMode placement_mode =
        state->placement_mode == 1 ? DVZ_COLORBAR_PLACEMENT_DETACHED :
                                     DVZ_COLORBAR_PLACEMENT_ATTACHED;
    DvzPlacement placement = {
        .space = _placement_space_from_index(state->placement_space),
        .horizontal_anchor = _horizontal_anchor_from_index(state->horizontal_anchor),
        .vertical_anchor = _vertical_anchor_from_index(state->vertical_anchor),
        .offset_x_px = state->detached_offset_x,
        .offset_y_px = state->detached_offset_y,
        .width_px = state->detached_width,
        .height_px = state->detached_height,
    };
    (void)dvz_colorbar_set_layout(
        state->colorbar, &(DvzColorbarDesc){
                             .placement_mode = placement_mode,
                             .orientation = orientation,
                             .anchor = _anchor_from_state(state, orientation),
                             .title = "Intensity",
                             .reserve_px = state->reserve_px,
                             .ramp_width_px = state->ramp_width_px,
                             .edge_offset_px = state->edge_offset_px,
                             .plot_gap_px = state->plot_gap_px,
                             .tick_length_px = state->tick_length_px,
                             .label_gap_px = state->label_gap_px,
                             .placement = placement,
                         });
}


/**
 * Apply the current axis controls.
 *
 * @param state colorbar example state
 */
static void _apply_axis_controls(ColorbarState* state)
{
    if (state == NULL)
        return;
    if (state->x_axis != NULL)
    {
        (void)dvz_axis_set_visible(state->x_axis, state->show_x_axis);
        (void)dvz_axis_set_grid(state->x_axis, state->show_grid);
        state->x_axis_style.reserve_px = state->x_axis_reserve_px;
        state->x_axis_style.show_grid = state->show_grid;
        (void)dvz_axis_set_style(state->x_axis, &state->x_axis_style);
    }
    if (state->y_axis != NULL)
    {
        (void)dvz_axis_set_visible(state->y_axis, state->show_y_axis);
        (void)dvz_axis_set_grid(state->y_axis, state->show_grid);
        state->y_axis_style.reserve_px = state->y_axis_reserve_px;
        state->y_axis_style.show_grid = state->show_grid;
        (void)dvz_axis_set_style(state->y_axis, &state->y_axis_style);
    }
}



/**
 * Apply the current panel padding controls.
 *
 * @param state colorbar example state
 */
static void _apply_panel_padding(ColorbarState* state)
{
    if (state == NULL || state->panel == NULL)
        return;

    (void)dvz_panel_set_padding(
        state->panel, &(DvzPanelReserve){
                          .left_px = (float)state->padding_left_px,
                          .right_px = (float)state->padding_right_px,
                          .top_px = (float)state->padding_top_px,
                          .bottom_px = (float)state->padding_bottom_px,
                      });
}



/**
 * Apply the current GUI controls to the scene scale.
 *
 * @param state colorbar example state
 * @param update_colormap whether the selected colormap changed
 * @param update_range whether the visible scale range changed
 */
static void _apply_colorbar_controls(
    ColorbarState* state, bool update_colormap, bool update_range)
{
    if (state == NULL || state->scale == NULL)
        return;

    if (state->range_min > state->range_max - 0.01f)
        state->range_min = state->range_max - 0.01f;
    if (state->range_max < state->range_min + 0.01f)
        state->range_max = state->range_min + 0.01f;
    if (state->range_min < SCALE_MIN)
        state->range_min = SCALE_MIN;
    if (state->range_max > SCALE_MAX)
        state->range_max = SCALE_MAX;

    if (update_colormap)
    {
        uint32_t index = state->colormap_index >= 0 ? (uint32_t)state->colormap_index : 0;
        if (index >= COLORMAP_COUNT)
            index = 0;
        dvz_scale_set_colormap(state->scale, state->colormaps[index]);
    }
    if (update_range)
    {
        dvz_scale_set_view_range(state->scale, state->range_min, state->range_max);
    }
    if (state->win != NULL)
        dvz_view_request_frame(state->win);
}



/**
 * Render the colorbar example controls.
 *
 * @param gui GUI context
 * @param win view
 * @param user_data colorbar example state
 */
static void _colorbar_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    ColorbarState* state = (ColorbarState*)user_data;
    if (state == NULL)
        return;

    bool colormap_changed = false;
    bool range_changed = false;
    bool layout_changed = false;
    bool axis_changed = false;
    bool padding_changed = false;
    if (dvz_gui_begin(gui, "Colorbar", NULL, 0))
    {
        const char* const colormap_names[COLORMAP_COUNT] = {
            "Viridis", "Magma", "Plasma", "Inferno", "Cividis", "Turbo",
        };
        dvz_gui_separator_text(gui, "Scale");
        colormap_changed = dvz_gui_combo(
            gui, "Colormap", &state->colormap_index, colormap_names, (int)COLORMAP_COUNT);
        range_changed = dvz_gui_slider_range_double(
            gui, "Range", &state->range_min, &state->range_max, SCALE_MIN, SCALE_MAX, "%.2f");
        if (dvz_gui_button(gui, "Reset range"))
        {
            state->range_min = FIELD_MIN;
            state->range_max = FIELD_MAX;
            range_changed = true;
        }

        dvz_gui_separator_text(gui, "Panel");
        padding_changed |= dvz_gui_slider_int(gui, "Padding L", &state->padding_left_px, 0, 96);
        padding_changed |= dvz_gui_slider_int(gui, "Padding R", &state->padding_right_px, 0, 96);
        padding_changed |= dvz_gui_slider_int(gui, "Padding T", &state->padding_top_px, 0, 96);
        padding_changed |= dvz_gui_slider_int(gui, "Padding B", &state->padding_bottom_px, 0, 96);

        dvz_gui_separator_text(gui, "Colorbar");
        const char* const modes[] = {"Attached", "Detached"};
        const char* const orientations[] = {"Vertical", "Horizontal"};
        const char* const anchors[] = {"Right", "Left", "Top", "Bottom"};
        layout_changed |= dvz_gui_combo(gui, "Mode", &state->placement_mode, modes, 2);
        layout_changed |=
            dvz_gui_combo(gui, "Orientation", &state->orientation, orientations, 2);
        layout_changed |= dvz_gui_combo(gui, "Side", &state->anchor_index, anchors, 4);
        layout_changed |= dvz_gui_slider_float(
            gui, "Reserve px", &state->reserve_px, 48.0f, 240.0f);
        layout_changed |= dvz_gui_slider_float(
            gui, "Ramp width", &state->ramp_width_px, 8.0f, 96.0f);
        layout_changed |= dvz_gui_slider_float(
            gui, "Edge offset", &state->edge_offset_px, 0.0f, 64.0f);
        layout_changed |=
            dvz_gui_slider_float(gui, "Plot gap", &state->plot_gap_px, 0.0f, 64.0f);
        layout_changed |= dvz_gui_slider_float(
            gui, "Tick length", &state->tick_length_px, 0.0f, 24.0f);
        layout_changed |=
            dvz_gui_slider_float(gui, "Label gap", &state->label_gap_px, 0.0f, 32.0f);

        if (state->placement_mode == 1)
        {
            dvz_gui_separator_text(gui, "Detached placement");
            const char* const spaces[] = {"Panel", "Figure"};
            const char* const horizontal[] = {"Left", "Center", "Right"};
            const char* const vertical[] = {"Top", "Center", "Bottom"};
            layout_changed |=
                dvz_gui_combo(gui, "Space", &state->placement_space, spaces, 2);
            layout_changed |= dvz_gui_combo(
                gui, "Horizontal", &state->horizontal_anchor, horizontal, 3);
            layout_changed |=
                dvz_gui_combo(gui, "Vertical", &state->vertical_anchor, vertical, 3);
            layout_changed |= dvz_gui_slider_float(
                gui, "Offset X", &state->detached_offset_x, -320.0f, 320.0f);
            layout_changed |= dvz_gui_slider_float(
                gui, "Offset Y", &state->detached_offset_y, -240.0f, 240.0f);
            layout_changed |= dvz_gui_slider_float(
                gui, "Width", &state->detached_width, 24.0f, 480.0f);
            layout_changed |= dvz_gui_slider_float(
                gui, "Height", &state->detached_height, 24.0f, 480.0f);
        }

        dvz_gui_separator_text(gui, "Axes");
        axis_changed |= dvz_gui_checkbox(gui, "X axis", &state->show_x_axis);
        axis_changed |= dvz_gui_checkbox(gui, "Y axis", &state->show_y_axis);
        axis_changed |= dvz_gui_checkbox(gui, "Grid", &state->show_grid);
        axis_changed |= dvz_gui_slider_float(
            gui, "X reserve", &state->x_axis_reserve_px, 0.0f, 120.0f);
        axis_changed |= dvz_gui_slider_float(
            gui, "Y reserve", &state->y_axis_reserve_px, 0.0f, 120.0f);

        dvz_gui_separator_text(gui, "Debug");
        if (state->panel != NULL)
        {
            DvzPanelReserve padding = {0};
            DvzPanelReserve reserve = {0};
            DvzRect inner = {0};
            DvzRect rect = {0};
            char line[128] = {0};
            if (dvz_panel_get_padding(state->panel, &padding))
            {
                dvz_snprintf(
                    line, sizeof(line), "Padding L %.0f R %.0f T %.0f B %.0f",
                    padding.left_px, padding.right_px, padding.top_px, padding.bottom_px);
                dvz_gui_text(gui, line);
            }
            if (dvz_panel_get_reserve(state->panel, &reserve))
            {
                dvz_snprintf(
                    line, sizeof(line), "Reserve L %.0f R %.0f T %.0f B %.0f",
                    reserve.left_px, reserve.right_px, reserve.top_px, reserve.bottom_px);
                dvz_gui_text(gui, line);
            }
            if (dvz_panel_inner_rect_px(state->panel, &inner))
            {
                dvz_snprintf(
                    line, sizeof(line), "Inner %.0fx%.0f at %.0f, %.0f", inner.width,
                    inner.height, inner.x, inner.y);
                dvz_gui_text(gui, line);
            }
            if (dvz_panel_plot_rect_px(state->panel, &rect))
            {
                dvz_snprintf(
                    line, sizeof(line), "Plot %.0fx%.0f at %.0f, %.0f", rect.width, rect.height,
                    rect.x, rect.y);
                dvz_gui_text(gui, line);
            }
        }
    }
    dvz_gui_end(gui);

    if (colormap_changed || range_changed)
        _apply_colorbar_controls(state, colormap_changed, range_changed);
    if (layout_changed)
        _apply_colorbar_layout(state);
    if (axis_changed)
        _apply_axis_controls(state);
    if (padding_changed)
        _apply_panel_padding(state);
    if ((layout_changed || axis_changed || padding_changed) && state->win != NULL)
        dvz_view_request_frame(state->win);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CONTINUOUS,
                   .label = "Intensity",
                   .format = {.precision = 2},
               });
    EXAMPLE_CHECK(scale != NULL, "dvz_scale() failed");
    dvz_scale_set_domain(scale, SCALE_MIN, SCALE_MAX);
    dvz_scale_set_view_range(scale, FIELD_MIN, FIELD_MAX);

    ColorbarState state = {
        .scale = scale,
        .panel = panel,
        .x_axis_style = _axis_style(),
        .y_axis_style = _axis_style(),
        .colormap_index = 0,
        .placement_mode = 0,
        .orientation = 0,
        .anchor_index = 0,
        .placement_space = 0,
        .horizontal_anchor = 2,
        .vertical_anchor = 1,
        .precision = 2,
        .range_min = FIELD_MIN,
        .range_max = FIELD_MAX,
        .reserve_px = 140.0f,
        .ramp_width_px = 36.0f,
        .edge_offset_px = 0.0f,
        .plot_gap_px = 12.0f,
        .tick_length_px = 6.0f,
        .label_gap_px = 6.0f,
        .detached_offset_x = -40.0f,
        .detached_offset_y = 0.0f,
        .detached_width = 64.0f,
        .detached_height = 320.0f,
        .padding_left_px = 32.0f,
        .padding_right_px = 32.0f,
        .padding_top_px = 32.0f,
        .padding_bottom_px = 32.0f,
        .show_x_axis = false,
        .show_y_axis = false,
        .show_grid = false,
        .x_axis_reserve_px = 44.0f,
        .y_axis_reserve_px = 58.0f,
    };
    const DvzBuiltinColormap builtins[COLORMAP_COUNT] = {
        DVZ_BUILTIN_COLORMAP_VIRIDIS, //
        DVZ_BUILTIN_COLORMAP_MAGMA,   //
        DVZ_BUILTIN_COLORMAP_PLASMA,  //
        DVZ_BUILTIN_COLORMAP_INFERNO, //
        DVZ_BUILTIN_COLORMAP_CIVIDIS, //
        DVZ_BUILTIN_COLORMAP_TURBO,   //
    };
    for (uint32_t i = 0; i < COLORMAP_COUNT; i++)
    {
        state.colormaps[i] = dvz_colormap_builtin(scene, builtins[i]);
        EXAMPLE_CHECK(state.colormaps[i] != NULL, "dvz_colormap_builtin() failed");
    }
    EXAMPLE_CHECK(
        state.colormaps[state.colormap_index] != NULL, "dvz_colormap_builtin() failed");
    dvz_scale_set_colormap(scale, state.colormaps[state.colormap_index]);
    _apply_panel_padding(&state);

    DvzVisual* image = dvz_image(scene, 0);
    EXAMPLE_CHECK(image != NULL, "dvz_image() failed");

    vec3 positions[4] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    int rc = dvz_visual_set_data(image, "position", positions, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(position) failed");

    rc = dvz_visual_set_data(image, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(texcoords) failed");

    rc = dvz_visual_set_scale(image, "colormap", scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale(colormap) failed");

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                   .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                   .width = FIELD_SIZE,
                   .height = FIELD_SIZE,
                   .depth = 1,
               });
    EXAMPLE_CHECK(field != NULL, "dvz_sampled_field() failed");

    float values[FIELD_SIZE * FIELD_SIZE] = {0};
    _fill_field(values);
    bool ok = dvz_sampled_field_set_data(
        field, &(DvzFieldDataView){
                   .data = values,
                   .bytes_per_row = FIELD_SIZE * sizeof(float),
                   .rows_per_image = FIELD_SIZE,
               });
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data() failed");

    ok = dvz_visual_set_field(image, "field", field);
    EXAMPLE_CHECK(ok, "dvz_visual_set_field() failed");

    rc = dvz_panel_add_visual(panel, image, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual() failed");

    DvzColorbar* colorbar = dvz_colorbar(
        panel, scale,
        &(DvzColorbarDesc){
            .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .title = "Intensity",
        });
    EXAMPLE_CHECK(colorbar != NULL, "dvz_colorbar() failed");
    dvz_colorbar_set_format(colorbar, &(DvzFormatDesc){.precision = 2});
    state.colorbar = colorbar;

    rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(x) failed");

    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_set_domain(y) failed");
    state.x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
    state.y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);
    if (state.x_axis != NULL)
        (void)dvz_axis_set_label(state.x_axis, "x");
    if (state.y_axis != NULL)
        (void)dvz_axis_set_label(state.y_axis, "y");
    _apply_axis_controls(&state);

    dvz_panel_set_background_color(panel, 0.04f, 0.05f, 0.06f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "colorbar");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");
    state.win = win;

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _colorbar_gui, &state);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
