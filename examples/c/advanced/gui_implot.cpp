/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* gui_implot - This example combines an embedded Datoviz 3D view with ImPlot 2D charts.
 *
 * What to look for: the left dock contains an arcball-controlled Datoviz viewport showing a Lorenz
 * trajectory. The right dock plots x(t) and z(t) from the same samples through ImPlot in Datoviz's
 * Dear ImGui frame. Compare this immediate-mode integration with features/panel_mixed_2d_3d, where
 * all three views are retained Datoviz panels.
 *
 * ImPlot is an example-only dependency. CMake fetches the pinned ImPlot v1.0 source by default, or
 * uses DVZ_IMPLOT_SOURCE_DIR when it points at a local checkout.
 *
 * Build:  just example-c advanced/gui_implot
 * Run:    ./build/examples/c/advanced/gui_implot
 * Smoke:  ./build/examples/c/advanced/gui_implot --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"

extern "C"
{
#include "example_common.h"
#include "example_style.h"
}

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define HOST_WIDTH    EXAMPLE_WINDOW_WIDTH
#define HOST_HEIGHT   EXAMPLE_WINDOW_HEIGHT
#define SOURCE_WIDTH  720u
#define SOURCE_HEIGHT 672u
#define SAMPLE_COUNT  1600u
#define WARMUP_COUNT  1200u

static const double LORENZ_DT = 0.005;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GuiImPlotState
{
    DvzGuiViewport* viewport;
    ImPlotContext* implot;
    bool layout_initialized;
    double time[SAMPLE_COUNT];
    double x[SAMPLE_COUNT];
    double z[SAMPLE_COUNT];
} GuiImPlotState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Advance one Lorenz-system sample with a small explicit integration step.
 *
 * @param x mutable X state
 * @param y mutable Y state
 * @param z mutable Z state
 */
static void _lorenz_step(double* x, double* y, double* z)
{
    const double dx = 10.0 * (*y - *x);
    const double dy = *x * (28.0 - *z) - *y;
    const double dz = *x * *y - (8.0 / 3.0) * *z;
    *x += LORENZ_DT * dx;
    *y += LORENZ_DT * dy;
    *z += LORENZ_DT * dz;
}



/**
 * Generate the normalized 3D trajectory and persistent ImPlot arrays.
 *
 * @param state example state receiving time-series arrays
 * @param trajectory output normalized XYZ positions
 */
static void _fill_lorenz(GuiImPlotState* state, vec3* trajectory)
{
    double x = 0.1;
    double y = 0.0;
    double z = 0.0;
    for (uint32_t i = 0; i < WARMUP_COUNT; i++)
        _lorenz_step(&x, &y, &z);

    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        _lorenz_step(&x, &y, &z);
        state->time[i] = (double)i * LORENZ_DT;
        state->x[i] = x;
        state->z[i] = z;
        trajectory[i][0] = (float)(x / 22.0);
        trajectory[i][1] = (float)((z - 25.0) / 28.0);
        trajectory[i][2] = (float)(y / 30.0);
    }
}



/**
 * Populate the source figure rendered inside the GUI viewport.
 *
 * @param scene scene owning the figure and visual
 * @param state example state receiving ImPlot arrays
 * @param out_figure output source figure
 * @param out_panel output source panel
 * @return true on success
 */
static bool _build_source_figure(
    DvzScene* scene, GuiImPlotState* state, DvzFigure** out_figure, DvzPanel** out_panel)
{
    vec3 trajectory[SAMPLE_COUNT] = {{0}};
    DvzColor colors[SAMPLE_COUNT] = {};
    float widths[SAMPLE_COUNT] = {0};
    _fill_lorenz(state, trajectory);

    DvzColor color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);
    color.a = 244u;
    for (uint32_t i = 0; i < SAMPLE_COUNT; i++)
    {
        colors[i] = color;
        widths[i] = 2.4f;
    }

    DvzFigure* figure = dvz_figure(scene, SOURCE_WIDTH, SOURCE_HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    if (figure == NULL || panel == NULL)
        return false;
    example_graphite_cyan_set_panel_background(panel);

    DvzPanelBorderDesc border = dvz_panel_border_desc();
    border.color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    border.width_px = 1.5f;
    if (dvz_panel_set_border(panel, &border) != DVZ_OK)
        return false;

    DvzVisual* path = dvz_path(scene, 0);
    if (path == NULL)
        return false;
    DvzVisualDataUpdate updates[3] = {};
    updates[0].attr_name = "position";
    updates[0].data = trajectory;
    updates[0].item_count = SAMPLE_COUNT;
    updates[1].attr_name = "color";
    updates[1].data = colors;
    updates[1].item_count = SAMPLE_COUNT;
    updates[2].attr_name = "stroke_width_px";
    updates[2].data = widths;
    updates[2].item_count = SAMPLE_COUNT;
    if (dvz_visual_set_data_many(path, updates, 3) != DVZ_OK)
        return false;
    if (dvz_path_set_caps(path, DVZ_SEGMENT_CAP_ROUND, DVZ_SEGMENT_CAP_ROUND) != DVZ_OK)
        return false;
    if (dvz_path_set_join(path, DVZ_PATH_JOIN_ROUND, 4.0f) != DVZ_OK)
        return false;
    if (dvz_visual_set_depth_test(path, true) != DVZ_OK)
        return false;
    if (dvz_panel_add_visual(panel, path, NULL) != DVZ_OK)
        return false;

    if (example_set_default_3d_camera(panel, 1.15f) == NULL)
        return false;
    DvzReferenceGridDesc reference = dvz_reference_grid_desc();
    reference.plane = DVZ_REFERENCE_GRID_XZ;
    reference.origin[1] = -0.92f;
    reference.size[0] = 2.4f;
    reference.size[1] = 2.4f;
    reference.spacing = 0.2f;
    reference.major_every = 5u;
    if (dvz_reference_grid(panel, &reference) == NULL)
        return false;

    *out_figure = figure;
    *out_panel = panel;
    return true;
}



/**
 * Draw one ImPlot time series with stable initial limits.
 *
 * @param title plot title
 * @param y_label Y axis label
 * @param time time samples
 * @param values value samples
 * @param y_min initial Y minimum
 * @param y_max initial Y maximum
 * @param color line color
 * @param height plot height in logical pixels
 */
static void _draw_plot(
    const char* title, const char* y_label, const double* time, const double* values, double y_min,
    double y_max, const ImVec4& color, float height)
{
    ImPlot::PushStyleColor(
        ImPlotCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    if (!ImPlot::BeginPlot(title, ImVec2(-1.0f, height)))
    {
        ImPlot::PopStyleColor();
        return;
    }

    ImPlot::SetupAxes("time (s)", y_label);
    ImPlot::SetupAxesLimits(
        0.0, (double)(SAMPLE_COUNT - 1u) * LORENZ_DT, y_min, y_max, ImPlotCond_Once);
    ImPlotSpec spec;
    spec.LineColor = color;
    spec.LineWeight = 2.4f;
    ImPlot::PlotLine(title, time, values, (int)SAMPLE_COUNT, spec);
    ImPlot::EndPlot();
    ImPlot::PopStyleColor();
}



/**
 * Build the docked Datoviz viewport and ImPlot charts.
 *
 * @param gui Datoviz GUI overlay
 * @param view host GLFW view
 * @param user_data GUI/ImPlot example state
 */
static void _gui_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    GuiImPlotState* state = (GuiImPlotState*)user_data;
    if (gui == NULL || state == NULL || state->viewport == NULL || state->implot == NULL)
        return;

    ImPlot::SetCurrentContext(state->implot);
    if (!state->layout_initialized)
    {
        (void)dvz_gui_dock_window_once(
            gui, "ImPlot signals", DVZ_GUI_DOCK_SLOT_RIGHT, 510.0f);
        const ImGuiID dockspace_id = ImGui::GetID("DatovizDockSpace");
        ImGuiDockNode* central_node = ImGui::DockBuilderGetCentralNode(dockspace_id);
        if (central_node != NULL)
        {
            ImGui::DockBuilderDockWindow("Datoviz 3D", central_node->ID);
            ImGui::DockBuilderFinish(dockspace_id);
            state->layout_initialized = true;
        }
    }
    (void)dvz_gui_viewport_window(state->viewport, "Datoviz 3D", NULL, 0);

    if (dvz_gui_begin(gui, "ImPlot signals", NULL, 0))
    {
        ImGui::TextUnformatted("Immediate-mode charts sharing the Datoviz ImGui frame");
        ImGui::Separator();
        ImVec2 available = ImGui::GetContentRegionAvail();
        float plot_height = 0.5f * (available.y - ImGui::GetStyle().ItemSpacing.y);
        if (plot_height < 180.0f)
            plot_height = 180.0f;
        _draw_plot(
            "x(t)", "x", state->time, state->x, -22.0, 22.0,
            ImVec4(0.22f, 0.78f, 0.92f, 1.0f), plot_height);
        _draw_plot(
            "z(t)", "z", state->time, state->z, 0.0, 52.0,
            ImVec4(0.38f, 0.88f, 0.68f, 1.0f), plot_height);
    }
    dvz_gui_end(gui);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    DvzFigure* source_figure = NULL;
    DvzFigure* host_figure = NULL;
    DvzPanel* source_panel = NULL;
    DvzPanel* host_panel = NULL;
    DvzView* host_view = NULL;
    DvzGui* gui = NULL;
    DvzController* arcball = NULL;
    DvzGuiConfig gui_config = {};
    DvzGuiViewportConfig viewport_config = {};
    DvzAppCaptureConfig capture = {};
    GuiImPlotState state = {};
    bool host_alive = false;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    EXAMPLE_CHECK(
        _build_source_figure(scene, &state, &source_figure, &source_panel),
        "failed to build the Datoviz source figure");

    host_figure = dvz_figure(scene, HOST_WIDTH, HOST_HEIGHT, 0);
    host_panel = host_figure != NULL ? dvz_panel_full(host_figure) : NULL;
    EXAMPLE_CHECK(host_figure != NULL && host_panel != NULL, "failed to build the host figure");
    dvz_panel_set_background_color(host_panel, dvz_color_from_unit(0.045f, 0.050f, 0.064f, 1.0f));

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");
    host_view = dvz_view_window(app, host_figure, HOST_WIDTH, HOST_HEIGHT, "gui_implot");
    EXAMPLE_CHECK(host_view != NULL, "dvz_view_window() failed (GLFW unavailable?)");

    gui_config = dvz_gui_config();
    gui_config.gui_flags |= DVZ_GUI_FLAGS_DOCKING | DVZ_GUI_FLAGS_DOCKSPACE;
    gui_config.ini_path = NULL;
    gui = dvz_view_gui(host_view, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");

    state.implot = ImPlot::CreateContext();
    EXAMPLE_CHECK(state.implot != NULL, "ImPlot::CreateContext() failed");

    viewport_config = dvz_gui_viewport_config();
    viewport_config.viewport_flags = DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT;
    state.viewport = dvz_gui_viewport(gui, source_figure, &viewport_config);
    EXAMPLE_CHECK(state.viewport != NULL, "dvz_gui_viewport() failed");

    arcball = dvz_arcball(scene, NULL);
    EXAMPLE_CHECK(arcball != NULL, "dvz_arcball() failed");
    EXAMPLE_CHECK(
        dvz_panel_bind_controller(source_panel, arcball, DVZ_DIM_MASK_XYZ) == DVZ_OK,
        "dvz_panel_bind_controller() failed");
    EXAMPLE_CHECK(
        dvz_panel_connect_input(source_panel, dvz_gui_viewport_input(state.viewport)) == DVZ_OK,
        "dvz_panel_connect_input() failed");
    EXAMPLE_CHECK(
        dvz_view_set_gui_callback(host_view, _gui_callback, &state) == DVZ_OK,
        "dvz_view_set_gui_callback() failed");

    if (example_png_capture_requested(argc, argv))
    {
        EXAMPLE_CHECK(
            example_png_capture_config("advanced_gui_implot", &capture),
            "failed to configure PNG capture");
        EXAMPLE_CHECK(
            example_run_with_capture(
                app, host_view, example_frame_count_any_or_default(argc, argv, 4), &capture),
            "PNG capture failed");
    }
    else
    {
        dvz_app_run(app, example_frame_count(argc, argv));
    }
    ret = 0;

cleanup:
    host_alive = host_view != NULL && dvz_view_canvas(host_view) != NULL;
    if (host_alive)
        (void)dvz_view_set_gui_callback(host_view, NULL, NULL);
    if (source_panel != NULL && state.viewport != NULL && host_alive)
        (void)dvz_panel_connect_input(source_panel, NULL);
    if (state.viewport != NULL && host_alive)
        dvz_gui_viewport_destroy(state.viewport);
    if (state.implot != NULL)
        ImPlot::DestroyContext(state.implot);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
