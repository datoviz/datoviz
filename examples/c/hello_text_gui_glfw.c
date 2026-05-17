/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* hello_text_gui_glfw - interactive text and glyph inspection scene.
 *
 * Build:  just example-c hello_text_gui_glfw
 * Run:    ./build/examples/c/hello_text_gui_glfw
 * Smoke:  ./build/examples/c/hello_text_gui_glfw 120
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TEXT_LAB_MAX_TICKS 64u
#define TEXT_LAB_FIGURE_WIDTH 1100.0f
#define TEXT_LAB_FIGURE_HEIGHT 760.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum TextLabMode
{
    TEXT_LAB_MODE_SAMPLE = 0,
    TEXT_LAB_MODE_TICKS,
    TEXT_LAB_MODE_MULTILINE,
    TEXT_LAB_MODE_UTF8,
} TextLabMode;



typedef struct TextLabState
{
    DvzAppWindow* win;
    DvzVisual* title;
    DvzVisual* sample;
    DvzVisual* multiline;
    DvzVisual* ticks;
    float size_pts;
    float tick_size_pts;
    float angle;
    float offset_x;
    float offset_y;
    float text_anchor_x;
    float text_anchor_y;
    float color[4];
    float tick_count_value;
    int mode;
    int anchor_index;
    bool animate;
    bool show_demo;
    uint32_t frame_index;
} TextLabState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return the scene anchor selected by the GUI.
 *
 * @param index GUI anchor index
 * @return selected scene anchor
 */
static DvzSceneAnchor selected_anchor(int index)
{
    static const DvzSceneAnchor anchors[] = {
        DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
        DVZ_SCENE_ANCHOR_PANEL_TOP,
        DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT,
        DVZ_SCENE_ANCHOR_PANEL_LEFT,
        DVZ_SCENE_ANCHOR_PANEL_CENTER,
        DVZ_SCENE_ANCHOR_PANEL_RIGHT,
        DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT,
        DVZ_SCENE_ANCHOR_PANEL_BOTTOM,
        DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT,
    };
    if (index < 0)
        index = 0;
    if ((uint32_t)index >= (uint32_t)(sizeof(anchors) / sizeof(anchors[0])))
        index = 0;
    return anchors[index];
}



/**
 * Return one display string for the selected mode.
 *
 * @param mode GUI mode index
 * @return mode text string
 */
static const char* mode_sample_text(int mode)
{
    if (mode == TEXT_LAB_MODE_UTF8)
        return "UTF-8 fallback: A" "\xCE" "\xA9" "B cafe" "\xCC" "\x81" " -> ?";
    return "The quick brown fox jumps over 13 lazy glyphs.";
}



/**
 * Convert GUI float RGBA channels to one DvzColor.
 *
 * @param rgba float RGBA channels in [0, 1]
 * @param out output color
 */
static void gui_color_to_dvz(const float rgba[4], DvzColor out)
{
    ANN(rgba);
    ANN(out);
    for (uint32_t i = 0; i < 4; i++)
    {
        float v = rgba[i];
        if (v < 0.0f)
            v = 0.0f;
        if (v > 1.0f)
            v = 1.0f;
        out[i] = (uint8_t)(v * 255.0f + 0.5f);
    }
}



/**
 * Resolve a panel target anchor in figure pixels.
 *
 * @param anchor panel target anchor
 * @param out output x/y figure pixels
 */
static void target_anchor_pixels(DvzSceneAnchor anchor, float out[2])
{
    ANN(out);
    out[0] = 0.0f;
    out[1] = 0.0f;
    switch (anchor)
    {
    case DVZ_SCENE_ANCHOR_PANEL_TOP:
        out[0] = 0.5f * TEXT_LAB_FIGURE_WIDTH;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_RIGHT:
        out[0] = TEXT_LAB_FIGURE_WIDTH;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_LEFT:
        out[1] = 0.5f * TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_CENTER:
        out[0] = 0.5f * TEXT_LAB_FIGURE_WIDTH;
        out[1] = 0.5f * TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_RIGHT:
        out[0] = TEXT_LAB_FIGURE_WIDTH;
        out[1] = 0.5f * TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT:
        out[1] = TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM:
        out[0] = 0.5f * TEXT_LAB_FIGURE_WIDTH;
        out[1] = TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_BOTTOM_RIGHT:
        out[0] = TEXT_LAB_FIGURE_WIDTH;
        out[1] = TEXT_LAB_FIGURE_HEIGHT;
        return;
    case DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT:
    case DVZ_SCENE_ANCHOR_NONE:
    default:
        return;
    }
}



/**
 * Update a batched text visual with constant style attributes.
 *
 * @param visual text visual
 * @param strings text strings
 * @param count number of text strings
 * @param positions figure-pixel text positions
 * @param size_pts text size in points
 * @param color text color
 * @param text_anchor normalized text-box anchor
 * @param angles per-string angles, or NULL for zero
 */
static void set_text_items(
    DvzVisual* visual, const char* const* strings, uint32_t count, float positions[][3],
    float size_pts, const DvzColor color, const float text_anchor[2], const float* angles)
{
    ANN(visual);
    ANN(strings);
    ANN(positions);
    ANN(text_anchor);
    if (count > TEXT_LAB_MAX_TICKS)
        count = TEXT_LAB_MAX_TICKS;
    float sizes[TEXT_LAB_MAX_TICKS] = {0};
    float text_anchors[TEXT_LAB_MAX_TICKS][2] = {0};
    float angle_values[TEXT_LAB_MAX_TICKS] = {0};
    DvzColor colors[TEXT_LAB_MAX_TICKS] = {0};
    for (uint32_t i = 0; i < count && i < TEXT_LAB_MAX_TICKS; i++)
    {
        sizes[i] = size_pts;
        text_anchors[i][0] = text_anchor[0];
        text_anchors[i][1] = text_anchor[1];
        angle_values[i] = angles != NULL ? angles[i] : 0.0f;
        colors[i][0] = color[0];
        colors[i][1] = color[1];
        colors[i][2] = color[2];
        colors[i][3] = color[3];
    }
    dvz_visual_set_visible(visual, true);
    dvz_visual_set_strings(visual, "text", strings, count);
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = count},
        {.attr_name = "anchor", .data = text_anchors, .item_count = count},
        {.attr_name = "size", .data = sizes, .item_count = count},
        {.attr_name = "color", .data = colors, .item_count = count},
        {.attr_name = "angle", .data = angle_values, .item_count = count},
    };
    dvz_visual_set_data_many(visual, updates, 5);
}



/**
 * Update every batched text visual from the current GUI state.
 *
 * @param state example state
 */
static void update_text_scene(TextLabState* state)
{
    ANN(state);
    DvzColor color = {0};
    gui_color_to_dvz(state->color, color);
    DvzSceneAnchor anchor = selected_anchor(state->anchor_index);
    float text_anchor[2] = {state->text_anchor_x, state->text_anchor_y};
    float target[2] = {0};
    target_anchor_pixels(anchor, target);

    dvz_visual_set_visible(state->sample, false);
    dvz_visual_set_visible(state->multiline, false);
    dvz_visual_set_visible(state->ticks, false);

    const char* title_string = "Text Lab";
    float title_pos[1][3] = {{24.0f, 24.0f, 0.0f}};
    const float title_anchor[2] = {0.0f, 0.0f};
    set_text_items(state->title, &title_string, 1, title_pos, 22.0f, color, title_anchor, NULL);

    const char* sample_string = NULL;
    float sample_pos[1][3] = {{target[0] + state->offset_x, target[1] + state->offset_y, 0.0f}};
    float sample_angle[1] = {state->angle};

    uint32_t tick_count = (uint32_t)(state->tick_count_value + 0.5f);
    if (tick_count > TEXT_LAB_MAX_TICKS)
        tick_count = TEXT_LAB_MAX_TICKS;

    if (state->mode == TEXT_LAB_MODE_TICKS)
    {
        sample_string = "Tick Labels";
        float heading_pos[1][3] = {
            {0.5f * TEXT_LAB_FIGURE_WIDTH, 76.0f, 0.0f},
        };
        const float heading_anchor[2] = {0.5f, 0.5f};
        set_text_items(
            state->sample, &sample_string, 1, heading_pos, 18.0f, color, heading_anchor, NULL);

        float usable = 0.72f * TEXT_LAB_FIGURE_WIDTH;
        float left = 0.5f * (TEXT_LAB_FIGURE_WIDTH - usable);
        float step = tick_count > 1 ? usable / (float)(tick_count - 1u) : 0.0f;
        char labels[TEXT_LAB_MAX_TICKS][32] = {{0}};
        const char* strings[TEXT_LAB_MAX_TICKS] = {0};
        float positions[TEXT_LAB_MAX_TICKS][3] = {{0}};
        for (uint32_t i = 0; i < tick_count; i++)
        {
            dvz_snprintf(labels[i], sizeof(labels[i]), "%u", i);
            strings[i] = labels[i];
            positions[i][0] = left + (float)i * step;
            positions[i][1] = 0.5f * TEXT_LAB_FIGURE_HEIGHT;
            positions[i][2] = 0.0f;
        }
        set_text_items(
            state->ticks, strings, tick_count, positions, state->tick_size_pts, color, text_anchor, NULL);
    }
    else if (state->mode == TEXT_LAB_MODE_MULTILINE)
    {
        const char* multiline_string = "line one\nline two\nline three";
        float multiline_pos[1][3] = {
            {0.5f * TEXT_LAB_FIGURE_WIDTH - 180.0f, 0.5f * TEXT_LAB_FIGURE_HEIGHT - 20.0f, 0.0f},
        };
        set_text_items(
            state->multiline, &multiline_string, 1, multiline_pos, state->size_pts, color, text_anchor,
            NULL);
    }
    else
    {
        sample_string = mode_sample_text(state->mode);
        set_text_items(
            state->sample, &sample_string, 1, sample_pos, state->size_pts, color, text_anchor,
            sample_angle);
    }

    if (state->win != NULL)
        dvz_app_window_request_frame(state->win);
}



/**
 * Reset GUI-editable text parameters.
 *
 * @param state example state
 */
static void reset_text_state(TextLabState* state)
{
    ANN(state);
    state->size_pts = 18.0f;
    state->tick_size_pts = 8.0f;
    state->angle = 0.0f;
    state->offset_x = 0.0f;
    state->offset_y = 0.0f;
    state->text_anchor_x = 0.5f;
    state->text_anchor_y = 0.5f;
    state->color[0] = 0.85f;
    state->color[1] = 0.92f;
    state->color[2] = 1.00f;
    state->color[3] = 1.00f;
    state->tick_count_value = 12.0f;
    state->mode = TEXT_LAB_MODE_SAMPLE;
    state->anchor_index = 4;
    state->animate = false;
}



/**
 * Build the dockable GUI controls for the text lab.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data example state
 */
static void gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    TextLabState* state = (TextLabState*)user_data;
    ANN(state);
    state->win = win;
    bool changed = false;

    if (dvz_gui_begin(gui, "Text", NULL, 0))
    {
        static const char* const mode_items[] = {
            "sample",
            "ticks",
            "multiline",
            "UTF-8",
        };
        static const char* const anchor_items[] = {
            "top left",
            "top",
            "top right",
            "left",
            "center",
            "right",
            "bottom left",
            "bottom",
            "bottom right",
        };

        changed |= dvz_gui_combo(gui, "Mode", &state->mode, mode_items, 4);
        if (state->mode == TEXT_LAB_MODE_SAMPLE || state->mode == TEXT_LAB_MODE_UTF8)
        {
            changed |= dvz_gui_slider_float_format(
                gui, "Text size", &state->size_pts, 6.0f, 64.0f, "%.1f pt");
        }
        else if (state->mode == TEXT_LAB_MODE_MULTILINE)
        {
            changed |= dvz_gui_slider_float_format(
                gui, "Multiline text size", &state->size_pts, 6.0f, 64.0f, "%.1f pt");
        }
        if (state->mode == TEXT_LAB_MODE_SAMPLE || state->mode == TEXT_LAB_MODE_UTF8)
        {
            changed |= dvz_gui_combo(gui, "Target", &state->anchor_index, anchor_items, 9);
            changed |= dvz_gui_slider_float(gui, "Angle", &state->angle, -1.57f, 1.57f);
            changed |= dvz_gui_slider_float(gui, "Offset X", &state->offset_x, -360.0f, 360.0f);
            changed |= dvz_gui_slider_float(gui, "Offset Y", &state->offset_y, -260.0f, 260.0f);
        }
        changed |= dvz_gui_slider_float(gui, "Text anchor X", &state->text_anchor_x, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Text anchor Y", &state->text_anchor_y, 0.0f, 1.0f);
        if (state->mode == TEXT_LAB_MODE_TICKS)
        {
            changed |= dvz_gui_slider_float_format(
                gui, "Tick count", &state->tick_count_value, 2.0f, (float)TEXT_LAB_MAX_TICKS,
                "%.0f");
            changed |= dvz_gui_slider_float_format(
                gui, "Tick size", &state->tick_size_pts, 5.0f, 18.0f, "%.1f pt");
        }
        changed |= dvz_gui_slider_float(gui, "Red", &state->color[0], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Green", &state->color[1], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Blue", &state->color[2], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->color[3], 0.05f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Animate", &state->animate);
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
        if (dvz_gui_button(gui, "Reset"))
        {
            reset_text_state(state);
            changed = true;
        }
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);

    if (state->animate)
    {
        state->frame_index++;
        state->angle = 0.65f * (float)((int32_t)(state->frame_index % 240u) - 120) / 120.0f;
        changed = true;
    }
    if (changed)
        update_text_scene(state);
}



/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;
    char* end = NULL;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (end == argv[1] || (end != NULL && *end != '\0'))
        return 0;
    if (value > UINT32_MAX)
        return UINT32_MAX;
    return (uint32_t)value;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the interactive text GUI example.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure =
        dvz_figure(scene, (uint32_t)TEXT_LAB_FIGURE_WIDTH, (uint32_t)TEXT_LAB_FIGURE_HEIGHT, 0);
    if (figure == NULL)
    {
        dvz_fprintf(stderr, "dvz_figure() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzPanel* panel =
        dvz_panel(figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (panel == NULL)
    {
        dvz_fprintf(stderr, "dvz_panel() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.06f, 0.07f, 0.09f, 1.0f);

    TextLabState state = {0};
    reset_text_state(&state);

    state.title = dvz_text(scene, 0);
    state.sample = dvz_text(scene, 0);
    state.multiline = dvz_text(scene, 0);
    state.ticks = dvz_text(scene, 0);
    if (state.title == NULL || state.sample == NULL || state.multiline == NULL ||
        state.ticks == NULL)
    {
        dvz_fprintf(stderr, "dvz_text() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzVisual* text_visuals[] = {state.title, state.sample, state.multiline, state.ticks};
    DvzVisualAttachDesc text_attach = {
        .z_layer = 4,
        .controller_mode = DVZ_CONTROLLER_FIXED,
    };
    for (uint32_t i = 0; i < (uint32_t)(sizeof(text_visuals) / sizeof(text_visuals[0])); i++)
    {
        if (dvz_panel_add_visual(panel, text_visuals[i], &text_attach) != 0)
        {
            dvz_fprintf(stderr, "dvz_panel_add_visual() failed for text visual %u\n", i);
            dvz_scene_destroy(scene);
            return 1;
        }
    }

    update_text_scene(&state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(
        app, figure, (uint32_t)TEXT_LAB_FIGURE_WIDTH, (uint32_t)TEXT_LAB_FIGURE_HEIGHT,
        "hello_text_gui_glfw");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    state.win = win;

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_app_window_set_gui_callback(win, gui_callback, &state);

    dvz_app_run(app, frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
