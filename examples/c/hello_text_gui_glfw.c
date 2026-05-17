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
#define TEXT_LAB_RAW_TEX_SIZE 32u
#define TEXT_LAB_FIGURE_WIDTH 1100.0f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef enum TextLabMode
{
    TEXT_LAB_MODE_SAMPLE = 0,
    TEXT_LAB_MODE_TICKS,
    TEXT_LAB_MODE_MULTILINE,
    TEXT_LAB_MODE_UTF8,
    TEXT_LAB_MODE_GLYPH,
} TextLabMode;



typedef struct TextLabState
{
    DvzAppWindow* win;
    DvzText* title;
    DvzText* sample;
    DvzText* multiline;
    DvzText* anchor_probe;
    DvzText* ticks[TEXT_LAB_MAX_TICKS];
    DvzVisual* raw_glyph;
    float size_pts;
    float tick_size_pts;
    float angle;
    float offset_x;
    float offset_y;
    float pivot_x;
    float pivot_y;
    float color[4];
    float tick_count_value;
    int mode;
    int anchor_index;
    int renderer_index;
    bool show_raw_glyph;
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
    if (mode == TEXT_LAB_MODE_GLYPH)
        return "Raw glyph shader probe";
    return "The quick brown fox jumps over 13 lazy glyphs.";
}



/**
 * Return the retained text renderer selected by the GUI.
 *
 * @param index GUI renderer index
 * @return selected text renderer
 */
static DvzTextRenderer selected_renderer(int index)
{
    if (index == 0)
        return DVZ_TEXT_RENDERER_AUTO;
    if (index == 2)
        return DVZ_TEXT_RENDERER_BITMAP_ATLAS;
    return DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
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
 * Apply one retained text style.
 *
 * @param text retained text object
 * @param size_pts text size
 * @param renderer retained renderer
 * @param color text color
 */
static void apply_text_style(
    DvzText* text, float size_pts, DvzTextRenderer renderer, const DvzColor color)
{
    ANN(text);
    DvzTextStyle style = {
        .size_pts = size_pts,
        .renderer = renderer,
        .color = {color[0], color[1], color[2], color[3]},
    };
    dvz_text_set_style(text, &style);
}



/**
 * Apply one retained screen placement.
 *
 * @param text retained text object
 * @param anchor panel anchor
 * @param x horizontal offset in pixels
 * @param y vertical offset in pixels
 * @param angle rotation angle in radians
 * @param pivot optional normalized text-box pivot
 */
static void apply_text_placement(
    DvzText* text, DvzSceneAnchor anchor, float x, float y, float angle, const float* pivot)
{
    ANN(text);
    DvzTextPlacement placement = {
        .mode = DVZ_TEXT_PLACEMENT_SCREEN,
        .anchor = anchor,
        .offset = {x, y},
        .angle = angle,
    };
    if (pivot != NULL)
    {
        placement.pivot[0] = pivot[0];
        placement.pivot[1] = pivot[1];
        placement.has_pivot = true;
    }
    dvz_text_set_placement(text, &placement);
}



/**
 * Update the raw glyph shader-probe quad.
 *
 * @param state example state
 */
static void update_raw_glyph(TextLabState* state)
{
    ANN(state);
    ANN(state->raw_glyph);

    float y = -0.18f;
    float h = 0.36f;
    float w = 0.36f;
    float x = -0.18f;
    bool visible = state->mode == TEXT_LAB_MODE_GLYPH || state->show_raw_glyph;
    if (!visible)
    {
        w = 0.0f;
        h = 0.0f;
    }

    float positions[6][3] = {
        {x,     y,     0.0f},
        {x,     y + h, 0.0f},
        {x + w, y,     0.0f},
        {x + w, y,     0.0f},
        {x,     y + h, 0.0f},
        {x + w, y + h, 0.0f},
    };
    float texcoords[6][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
    };
    DvzColor color = {255, 220, 96, 230};
    DvzColor colors[6] = {
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
        {color[0], color[1], color[2], color[3]},
    };

    dvz_visual_set_data(state->raw_glyph, "position", positions, 6);
    dvz_visual_set_data(state->raw_glyph, "texcoords", texcoords, 6);
    dvz_visual_set_data(state->raw_glyph, "color", colors, 6);
}



/**
 * Upload a synthetic SDF-like texture for the raw glyph visual.
 *
 * @param visual glyph visual
 */
static void upload_raw_glyph_texture(DvzVisual* visual)
{
    ANN(visual);
    uint8_t pixels[TEXT_LAB_RAW_TEX_SIZE * TEXT_LAB_RAW_TEX_SIZE * 4] = {0};
    const float center = ((float)TEXT_LAB_RAW_TEX_SIZE - 1.0f) * 0.5f;
    for (uint32_t y = 0; y < TEXT_LAB_RAW_TEX_SIZE; y++)
    {
        for (uint32_t x = 0; x < TEXT_LAB_RAW_TEX_SIZE; x++)
        {
            float dx = ((float)x - center) / center;
            float dy = ((float)y - center) / center;
            float d2 = dx * dx + dy * dy;
            float sd = 0.70f - d2;
            if (sd < 0.0f)
                sd = 0.0f;
            if (sd > 1.0f)
                sd = 1.0f;
            uint8_t v = (uint8_t)(sd * 255.0f + 0.5f);
            uint64_t i = ((uint64_t)y * TEXT_LAB_RAW_TEX_SIZE + x) * 4u;
            pixels[i + 0] = v;
            pixels[i + 1] = v;
            pixels[i + 2] = v;
            pixels[i + 3] = v;
        }
    }
    dvz_visual_set_texture(visual, pixels, TEXT_LAB_RAW_TEX_SIZE, TEXT_LAB_RAW_TEX_SIZE);
}



/**
 * Update every retained text object from the current GUI state.
 *
 * @param state example state
 */
static void update_text_scene(TextLabState* state)
{
    ANN(state);
    DvzColor color = {0};
    gui_color_to_dvz(state->color, color);
    DvzTextRenderer renderer = selected_renderer(state->renderer_index);
    DvzSceneAnchor anchor = selected_anchor(state->anchor_index);
    float pivot[2] = {state->pivot_x, state->pivot_y};

    dvz_text_set_string(state->title, "Text lab");
    apply_text_style(state->title, 22.0f, renderer, color);
    apply_text_placement(
        state->title, DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT, 24.0f, 24.0f, 0.0f, NULL);

    dvz_text_set_string(state->sample, "");
    dvz_text_set_string(state->multiline, "");
    dvz_text_set_string(state->anchor_probe, "");
    apply_text_style(state->multiline, state->size_pts, renderer, color);

    uint32_t tick_count = (uint32_t)(state->tick_count_value + 0.5f);
    if (tick_count > TEXT_LAB_MAX_TICKS)
        tick_count = TEXT_LAB_MAX_TICKS;
    for (uint32_t i = 0; i < TEXT_LAB_MAX_TICKS; i++)
    {
        dvz_text_set_string(state->ticks[i], "");
        apply_text_style(state->ticks[i], state->tick_size_pts, renderer, color);
        apply_text_placement(
            state->ticks[i], DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT, 0.0f, 0.0f, 0.0f, NULL);
    }

    if (state->mode == TEXT_LAB_MODE_TICKS)
    {
        dvz_text_set_string(state->sample, "Tick labels");
        apply_text_style(state->sample, 18.0f, renderer, color);
        apply_text_placement(
            state->sample, DVZ_SCENE_ANCHOR_PANEL_TOP, 0.0f, 96.0f, 0.0f, NULL);

        float left = 110.0f;
        float usable = TEXT_LAB_FIGURE_WIDTH - 2.0f * left;
        float step = tick_count > 1 ? usable / (float)(tick_count - 1u) : 0.0f;
        for (uint32_t i = 0; i < tick_count; i++)
        {
            char label[32] = {0};
            dvz_snprintf(label, sizeof(label), "%u", i);
            dvz_text_set_string(state->ticks[i], label);
            float x = left + (float)i * step;
            apply_text_placement(
                state->ticks[i], DVZ_SCENE_ANCHOR_PANEL_BOTTOM_LEFT, x, -96.0f, -0.35f,
                pivot);
        }
    }
    else if (state->mode == TEXT_LAB_MODE_MULTILINE)
    {
        dvz_text_set_string(state->multiline, "line one\nline two\nline three");
        apply_text_style(state->multiline, state->size_pts, renderer, color);
        apply_text_placement(
            state->multiline, DVZ_SCENE_ANCHOR_PANEL_CENTER, -180.0f, -20.0f, 0.0f,
            pivot);
    }
    else
    {
        dvz_text_set_string(state->sample, mode_sample_text(state->mode));
        apply_text_style(state->sample, state->size_pts, renderer, color);
        if (state->mode == TEXT_LAB_MODE_GLYPH)
        {
            apply_text_placement(
                state->sample, DVZ_SCENE_ANCHOR_PANEL_TOP, 0.0f, 96.0f, 0.0f, NULL);
        }
        else
        {
            apply_text_placement(
                state->sample, anchor, state->offset_x, state->offset_y, state->angle, pivot);
        }
    }

    update_raw_glyph(state);
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
    state->pivot_x = 0.5f;
    state->pivot_y = 0.5f;
    state->color[0] = 0.85f;
    state->color[1] = 0.92f;
    state->color[2] = 1.00f;
    state->color[3] = 1.00f;
    state->tick_count_value = 12.0f;
    state->mode = TEXT_LAB_MODE_SAMPLE;
    state->anchor_index = 4;
    state->renderer_index = 1;
    state->show_raw_glyph = false;
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
        static const char* const renderer_items[] = {
            "auto",
            "small bitmap atlas",
            "bitmap atlas",
        };
        static const char* const mode_items[] = {
            "sample",
            "ticks",
            "multiline",
            "UTF-8",
            "glyph probe",
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

        changed |= dvz_gui_combo(gui, "Mode", &state->mode, mode_items, 5);
        changed |= dvz_gui_combo(gui, "Renderer", &state->renderer_index, renderer_items, 3);
        changed |= dvz_gui_slider_float(gui, "Size", &state->size_pts, 6.0f, 64.0f);
        if (state->mode == TEXT_LAB_MODE_SAMPLE || state->mode == TEXT_LAB_MODE_UTF8)
        {
            changed |= dvz_gui_combo(gui, "Target", &state->anchor_index, anchor_items, 9);
            changed |= dvz_gui_slider_float(gui, "Angle", &state->angle, -1.57f, 1.57f);
            changed |= dvz_gui_slider_float(gui, "Offset X", &state->offset_x, -360.0f, 360.0f);
            changed |= dvz_gui_slider_float(gui, "Offset Y", &state->offset_y, -260.0f, 260.0f);
        }
        if (state->mode != TEXT_LAB_MODE_GLYPH)
        {
            changed |= dvz_gui_slider_float(gui, "Pivot X", &state->pivot_x, 0.0f, 1.0f);
            changed |= dvz_gui_slider_float(gui, "Pivot Y", &state->pivot_y, 0.0f, 1.0f);
        }
        if (state->mode == TEXT_LAB_MODE_TICKS)
        {
            changed |= dvz_gui_slider_float(gui, "Tick size", &state->tick_size_pts, 5.0f, 18.0f);
            changed |= dvz_gui_slider_float(
                gui, "Tick count", &state->tick_count_value, 2.0f, (float)TEXT_LAB_MAX_TICKS);
        }
        changed |= dvz_gui_slider_float(gui, "Red", &state->color[0], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Green", &state->color[1], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Blue", &state->color[2], 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Alpha", &state->color[3], 0.05f, 1.0f);
        changed |= dvz_gui_checkbox(gui, "Glyph probe overlay", &state->show_raw_glyph);
        changed |= dvz_gui_checkbox(gui, "Animate", &state->animate);
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
        if (dvz_gui_button(gui, "Reset"))
        {
            reset_text_state(state);
            changed = true;
        }
        igSeparator();
        igTextUnformatted("MSDF/MTSDF shader path: raw glyph quad", NULL);
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

    DvzFigure* figure = dvz_figure(scene, 1100, 760, 0);
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

    DvzTextStyle style = {
        .size_pts = state.size_pts,
        .renderer = DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS,
        .color = {220, 235, 255, 255},
    };
    DvzTextPlacement placement = {
        .mode = DVZ_TEXT_PLACEMENT_SCREEN,
        .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT,
        .offset = {24.0f, 24.0f},
    };
    state.title =
        dvz_text(panel, &(DvzTextDesc){.string = "", .style = style, .placement = placement});
    state.sample =
        dvz_text(panel, &(DvzTextDesc){.string = "", .style = style, .placement = placement});
    state.multiline =
        dvz_text(panel, &(DvzTextDesc){.string = "", .style = style, .placement = placement});
    state.anchor_probe =
        dvz_text(panel, &(DvzTextDesc){.string = "", .style = style, .placement = placement});
    if (state.title == NULL || state.sample == NULL || state.multiline == NULL ||
        state.anchor_probe == NULL)
    {
        dvz_fprintf(stderr, "dvz_text() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    for (uint32_t i = 0; i < TEXT_LAB_MAX_TICKS; i++)
    {
        state.ticks[i] =
            dvz_text(panel, &(DvzTextDesc){.string = "", .style = style, .placement = placement});
        if (state.ticks[i] == NULL)
        {
            dvz_fprintf(stderr, "dvz_text() failed for tick %u\n", i);
            dvz_scene_destroy(scene);
            return 1;
        }
    }

    state.raw_glyph = dvz_glyph(scene, 0);
    if (state.raw_glyph == NULL)
    {
        dvz_fprintf(stderr, "dvz_glyph() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    upload_raw_glyph_texture(state.raw_glyph);
    update_raw_glyph(&state);
    dvz_visual_set_alpha_mode(state.raw_glyph, DVZ_ALPHA_BLENDED);
    dvz_visual_set_depth_test(state.raw_glyph, false);
    DvzVisualAttachDesc raw_attach = {.z_layer = 2, .controller_mode = DVZ_CONTROLLER_FIXED};
    if (dvz_panel_add_visual(panel, state.raw_glyph, &raw_attach) != 0)
    {
        dvz_fprintf(stderr, "dvz_panel_add_visual() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    update_text_scene(&state);

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, 1100, 760, "hello_text_gui_glfw");
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
