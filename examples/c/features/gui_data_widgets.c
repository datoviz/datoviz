/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* features_gui_data_widgets - This example combines retained tree and table widgets.
 *
 * Scenario: features_gui_data_widgets
 * Style: features, native GUI/app
 *
 * Build:  just example-c features/gui_data_widgets
 * Run:    ./build/examples/c/features/gui_data_widgets
 *
 * What to look for: the hierarchy keeps stable keyed rows, swatches, styles, and expansion state,
 * while the table uses typed columns and stable row selection. Both widgets are populated once and
 * drawn through the public retained API; the callback drains compact events without rebuilding any
 * labels or cell buffers.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "example_style.h"

#define WIDTH      EXAMPLE_WINDOW_WIDTH
#define HEIGHT     EXAMPLE_WINDOW_HEIGHT
#define TREE_ROWS  10u
#define TABLE_ROWS 6u

typedef struct GuiDataWidgetsState
{
    DvzGuiTree* tree;
    DvzGuiTable* table;
    char status[96];
} GuiDataWidgetsState;

static void _record_events(
    GuiDataWidgetsState* state, const DvzGuiDataEvent* events, uint32_t written, uint32_t dropped)
{
    if (dropped != 0)
        (void)snprintf(state->status, sizeof(state->status), "Events dropped: %u", dropped);
    else if (written != 0)
        (void)snprintf(
            state->status, sizeof(state->status), "Last event: %u (key %llu)",
            events[written - 1].type, (unsigned long long)events[written - 1].row_key);
}

static void _gui_data_widgets_callback(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    GuiDataWidgetsState* state = (GuiDataWidgetsState*)user_data;
    if (state == NULL || state->tree == NULL || state->table == NULL)
        return;

    DvzGuiDataEvent events[8] = {0};
    uint32_t written = 0;
    uint32_t dropped = 0;
    if (dvz_gui_begin(gui, "Retained data widgets", NULL, 0))
    {
        dvz_gui_text(gui, "Allen-inspired hierarchy and typed region table");
        dvz_gui_separator_text(gui, "Atlas hierarchy");
        (void)dvz_gui_tree_draw(gui, state->tree, events, 8, &written, &dropped);
        _record_events(state, events, written, dropped);
        dvz_gui_separator_text(gui, "Mapped regions");
        (void)dvz_gui_table_draw(gui, state->table, events, 8, &written, &dropped);
        _record_events(state, events, written, dropped);
        dvz_gui_text(gui, state->status);
    }
    dvz_gui_end(gui);
}

static bool _configure_widgets(GuiDataWidgetsState* state)
{
    static const uint64_t tree_keys[TREE_ROWS] = {997,  8,   567, 688,  315,
                                                  1089, 623, 343, 1129, 549};
    static const uint32_t tree_parents[TREE_ROWS] = {
        UINT32_MAX, 0, 1, 2, 3, 2, 2, 1, 7, 8,
    };
    static const char* const tree_labels[TREE_ROWS] = {
        "root", "grey", "CH", "CTX", "Isocortex", "HPF", "CNU", "BS", "IB", "TH",
    };
    static const char* const tree_secondary[TREE_ROWS] = {
        "root",
        "Basic cell groups and regions",
        "Cerebrum",
        "Cerebral cortex",
        "Isocortex",
        "Hippocampal formation",
        "Cerebral nuclei",
        "Brain stem",
        "Interbrain",
        "Thalamus",
    };
    const DvzColor tree_swatches[TREE_ROWS] = {
        dvz_color_from_unit(0.95f, 0.95f, 0.95f, 1.0f),
        dvz_color_from_unit(0.65f, 0.86f, 0.91f, 1.0f),
        dvz_color_from_unit(0.65f, 0.86f, 0.91f, 1.0f),
        dvz_color_from_unit(0.55f, 0.88f, 0.60f, 1.0f),
        dvz_color_from_unit(0.35f, 0.95f, 0.43f, 1.0f),
        dvz_color_from_unit(0.48f, 0.82f, 0.42f, 1.0f),
        dvz_color_from_unit(0.48f, 0.74f, 0.87f, 1.0f),
        dvz_color_from_unit(0.98f, 0.37f, 0.48f, 1.0f),
        dvz_color_from_unit(0.98f, 0.37f, 0.48f, 1.0f),
        dvz_color_from_unit(0.98f, 0.43f, 0.50f, 1.0f),
    };
    DvzGuiDataStyle style = dvz_gui_data_style();
    style.row_key = 315;
    style.flags = DVZ_GUI_DATA_STYLE_FLAGS_ACCENT;
    style.accent = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY);

    state->tree = dvz_gui_tree("features_gui_data_widgets_tree", DVZ_GUI_DATA_WIDGET_FLAGS_FILTER);
    if (state->tree == NULL)
        return false;
    if (dvz_gui_tree_set_rows(
            state->tree, TREE_ROWS, tree_keys, tree_parents, tree_labels, tree_secondary,
            DVZ_GUI_DATA_SET_FLAGS_NONE) != DVZ_OK ||
        dvz_gui_tree_set_swatches(state->tree, TREE_ROWS, tree_swatches) != DVZ_OK ||
        dvz_gui_tree_set_styles(state->tree, 1, &style) != DVZ_OK ||
        dvz_gui_tree_expand_all(state->tree) != DVZ_OK)
        return false;

    DvzGuiTableColumnDesc columns[4] = {0};
    columns[0] = dvz_gui_table_column_desc();
    columns[0].column_id = 1;
    columns[0].type = DVZ_GUI_TABLE_COLUMN_TEXT;
    columns[0].flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE | DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH;
    columns[0].title = "Region";
    columns[1] = dvz_gui_table_column_desc();
    columns[1].column_id = 2;
    columns[1].type = DVZ_GUI_TABLE_COLUMN_DOUBLE;
    columns[1].flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    columns[1].title = "Signal";
    columns[1].format = "%.2f";
    columns[2] = dvz_gui_table_column_desc();
    columns[2].column_id = 3;
    columns[2].type = DVZ_GUI_TABLE_COLUMN_INT64;
    columns[2].flags = DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    columns[2].title = "Count";
    columns[3] = dvz_gui_table_column_desc();
    columns[3].column_id = 4;
    columns[3].type = DVZ_GUI_TABLE_COLUMN_COLOR;
    columns[3].title = "Color";
    state->table = dvz_gui_table(
        "features_gui_data_widgets_table", 4, columns,
        DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT | DVZ_GUI_DATA_WIDGET_FLAGS_FILTER);
    if (state->table == NULL)
        return false;

    static const uint64_t table_keys[TABLE_ROWS] = {1001, 1002, 1003, 1004, 1005, 1006};
    static const char* const names[TABLE_ROWS] = {"VISp", "MOp", "CA1", "TH", "HY", "SCs"};
    static const double signal[TABLE_ROWS] = {0.18, 0.92, 0.47, 0.31, 0.76, 0.59};
    static const int64_t count[TABLE_ROWS] = {120, 480, 256, 192, 384, 320};
    const DvzColor colors[TABLE_ROWS] = {
        dvz_color_from_unit(0.25f, 0.70f, 0.95f, 1.0f),
        dvz_color_from_unit(0.95f, 0.45f, 0.35f, 1.0f),
        dvz_color_from_unit(0.45f, 0.85f, 0.55f, 1.0f),
        dvz_color_from_unit(0.70f, 0.55f, 0.95f, 1.0f),
        dvz_color_from_unit(0.95f, 0.75f, 0.25f, 1.0f),
        dvz_color_from_unit(0.35f, 0.85f, 0.85f, 1.0f),
    };
    if (dvz_gui_table_set_rows(
            state->table, TABLE_ROWS, table_keys, DVZ_GUI_DATA_SET_FLAGS_NONE) != DVZ_OK ||
        dvz_gui_table_set_column_text(state->table, 1, TABLE_ROWS, names) != DVZ_OK ||
        dvz_gui_table_set_column_double(state->table, 2, TABLE_ROWS, signal) != DVZ_OK ||
        dvz_gui_table_set_column_int64(state->table, 3, TABLE_ROWS, count) != DVZ_OK ||
        dvz_gui_table_set_column_color(state->table, 4, TABLE_ROWS, colors) != DVZ_OK)
        return false;
    (void)snprintf(state->status, sizeof(state->status), "Select regions or sort a column");
    return true;
}

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = dvz_scene();
    DvzApp* app = NULL;
    DvzView* view = NULL;
    GuiDataWidgetsState state = {0};
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");
    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "failed to create scene objects");
    example_graphite_cyan_set_panel_background(panel);
    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");
    view = dvz_view_window(app, figure, WIDTH, HEIGHT, "gui_data_widgets");
    EXAMPLE_CHECK(view != NULL, "dvz_view_window() failed (GLFW unavailable?)");
    DvzGuiConfig gui_config = dvz_gui_config();
    gui_config.default_window_width = 640;
    EXAMPLE_CHECK(dvz_view_gui(view, &gui_config) != NULL, "dvz_view_gui() failed");
    EXAMPLE_CHECK(_configure_widgets(&state), "failed to configure retained widgets");
    EXAMPLE_CHECK(
        dvz_view_set_gui_callback(view, _gui_data_widgets_callback, &state) == DVZ_OK,
        "dvz_view_set_gui_callback() failed");
    if (example_png_capture_requested(argc, argv))
    {
        DvzAppCaptureConfig capture = {0};
        EXAMPLE_CHECK(
            example_png_capture_config("feature_gui_data_widgets", &capture),
            "failed to configure PNG capture");
        EXAMPLE_CHECK(
            example_run_with_capture(
                app, view, example_frame_count_any_or_default(argc, argv, 4), &capture),
            "PNG capture failed");
    }
    else
        dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (view != NULL)
        (void)dvz_view_set_gui_callback(view, NULL, NULL);
    dvz_gui_tree_destroy(state.tree);
    dvz_gui_table_destroy(state.table);
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
