/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* text_msdf_lab - interactive text renderer comparison and pixel inspector.
 *
 * Build:  cmake --build build --target example_c_tools_text_msdf_lab
 * Run:    ./build/examples/c/tools/text_msdf_lab
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "_alloc.h"
#include "_compat.h"
#include "_scene.h"
#include "datoviz/app.h"
#include "datoviz/canvas.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TEXT_MSDF_LAB_SOURCE_WIDTH  720u
#define TEXT_MSDF_LAB_SOURCE_HEIGHT 240u
#define TEXT_MSDF_LAB_HOST_WIDTH    1500u
#define TEXT_MSDF_LAB_HOST_HEIGHT   980u
#define TEXT_MSDF_LAB_TEXT_MAX      256u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct TextLabSource
{
    DvzFigure* figure;
    DvzPanel* panel;
    DvzAppWindow* win;
    DvzGuiViewport* viewport;
    DvzVisual* text;
    DvzPanzoom* panzoom;
    DvzTextRenderer renderer;
    uint8_t* rgba;
    uint32_t width;
    uint32_t height;
    bool capture_valid;
} TextLabSource;



typedef struct TextMsdfLabState
{
    TextLabSource sources[2];
    DvzAppWindow* host_win;
    char text[TEXT_MSDF_LAB_TEXT_MAX];
    float size_pts;
    float angle;
    float color[4];
    int renderer_index[2];
    int sample_index;
    int crop_center[2];
    int crop_size;
    int pixel_zoom;
    int hover_source;
    bool link_panzoom;
    bool lock_layout;
    bool show_grid;
    bool show_demo;
    vec2 last_pan[2];
    vec2 last_zoom[2];
} TextMsdfLabState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

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
        out[i] = (uint8_t)(255.0f * v + 0.5f);
    }
}



/**
 * Return the renderer selected by a combo index.
 *
 * @param index combo index
 * @return text renderer
 */
static DvzTextRenderer selected_renderer(int index)
{
    switch (index)
    {
    case 1:
        return DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS;
    case 2:
        return DVZ_TEXT_RENDERER_BITMAP_ATLAS;
    case 3:
        return DVZ_TEXT_RENDERER_MSDF_ATLAS;
    case 0:
    default:
        return DVZ_TEXT_RENDERER_AUTO;
    }
}



/**
 * Resolve the expected atlas backend for a renderer and text size.
 *
 * @param renderer requested renderer
 * @param size_pts text size in points
 * @return atlas backend
 */
static DvzTextAtlasBackend renderer_backend(DvzTextRenderer renderer, float size_pts)
{
    if (renderer == DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS)
        return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
    if (renderer == DVZ_TEXT_RENDERER_BITMAP_ATLAS)
    {
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
        return DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
#else
        return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
#endif
    }
    if (renderer == DVZ_TEXT_RENDERER_MSDF_ATLAS)
        return DVZ_TEXT_ATLAS_BACKEND_MSDF;
    if (renderer == DVZ_TEXT_RENDERER_AUTO)
    {
        if (size_pts < 14.0f)
        {
#if defined(DVZ_HAS_FREETYPE) && DVZ_HAS_FREETYPE
            return DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP;
#else
            return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
#endif
        }
        return DVZ_TEXT_ATLAS_BACKEND_MSDF;
    }
    return DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP;
}


/**
 * Return a human-readable atlas backend name.
 *
 * @param backend atlas backend
 * @return backend name
 */
static const char* atlas_backend_name(DvzTextAtlasBackend backend)
{
    switch (backend)
    {
    case DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP:
        return "builtin_bitmap";
    case DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP:
        return "freetype_bitmap";
    case DVZ_TEXT_ATLAS_BACKEND_STB_SDF:
        return "stb_sdf";
    case DVZ_TEXT_ATLAS_BACKEND_MSDF:
        return "msdf";
    default:
        return "unknown";
    }
}


/**
 * Return a human-readable atlas encoding name.
 *
 * @param encoding atlas texture encoding
 * @return encoding name
 */
static const char* atlas_encoding_name(DvzTextAtlasEncoding encoding)
{
    switch (encoding)
    {
    case DVZ_TEXT_ATLAS_ENCODING_BITMAP_ALPHA:
        return "bitmap_alpha";
    case DVZ_TEXT_ATLAS_ENCODING_SDF_ALPHA:
        return "sdf_alpha";
    case DVZ_TEXT_ATLAS_ENCODING_MSDF_RGB:
        return "msdf_rgb";
    default:
        return "unknown";
    }
}


/**
 * Return the atlas generated for one source.
 *
 * @param source source state
 * @param size_pts text size in points
 * @return atlas pointer, or NULL when unavailable
 */
static DvzTextAtlas* source_atlas(const TextLabSource* source, float size_pts)
{
    ANN(source);
    if (source->figure == NULL || source->figure->scene == NULL ||
        source->figure->scene->font_count == 0)
        return NULL;
    DvzFont* font = &source->figure->scene->fonts[0];
    DvzTextAtlasBackend backend = renderer_backend(source->renderer, size_pts);
    switch (backend)
    {
    case DVZ_TEXT_ATLAS_BACKEND_FREETYPE_BITMAP:
        return font->bitmap_atlas;
    case DVZ_TEXT_ATLAS_BACKEND_MSDF:
        return font->msdf_atlas != NULL ? font->msdf_atlas : font->sdf_atlas;
    case DVZ_TEXT_ATLAS_BACKEND_STB_SDF:
        return font->sdf_atlas;
    case DVZ_TEXT_ATLAS_BACKEND_BUILTIN_BITMAP:
    default:
        return NULL;
    }
}


/**
 * Format one compact atlas summary.
 *
 * @param prefix summary prefix
 * @param source source state
 * @param size_pts text size in points
 * @param out output string
 * @param out_size output string capacity
 */
static void atlas_summary_line(
    const char* prefix, const TextLabSource* source, float size_pts, char* out, size_t out_size)
{
    ANN(prefix);
    ANN(source);
    ANN(out);
    if (out_size == 0)
        return;
    DvzTextAtlas* atlas = source_atlas(source, size_pts);
    if (atlas == NULL)
    {
        dvz_snprintf(out, out_size, "%s atlas: none", prefix);
        return;
    }
    dvz_snprintf(
        out, out_size,
        "%s atlas: %s %s %ux%u c%u glyphs %u/%u miss %u gen %" PRIu64
        " px %.1f range %.1f line %.1f",
        prefix, atlas_backend_name(atlas->backend), atlas_encoding_name(atlas->encoding),
        atlas->width, atlas->height, atlas->channels, atlas->glyph_count,
        DVZ_SCENE_TEXT_ATLAS_MAX_GLYPHS, atlas->missing_glyph_count, atlas->generation,
        atlas->pixel_height, atlas->pixel_range, atlas->line_height);
}


/**
 * Copy a sample string into the editable text buffer.
 *
 * @param state lab state
 */
static void set_sample_text(TextMsdfLabState* state)
{
    ANN(state);
    static const char* const samples[] = {
        "e b S  space  0123456789",
        "The quick brown fox jumps over 13 lazy glyphs.",
        "UTF-8: cafe" "\xCC" "\x81" " cafe" "\xC3" "\xA9" " A" "\xCE" "\xA9" "B -> ?",
        "llll iii MMM WWW eaeae bgbgb 88888",
    };
    uint32_t count = (uint32_t)(sizeof(samples) / sizeof(samples[0]));
    uint32_t index = state->sample_index < 0 ? 0u : (uint32_t)state->sample_index;
    if (index >= count)
        index = 0;
    dvz_snprintf(state->text, sizeof(state->text), "%s", samples[index]);
}



/**
 * Update one source text visual.
 *
 * @param state lab state
 * @param source source index
 */
static void update_source_text(TextMsdfLabState* state, uint32_t source)
{
    ANN(state);
    ASSERT(source < 2);
    TextLabSource* src = &state->sources[source];
    if (src->text == NULL)
        return;

    DvzColor color = {0};
    gui_color_to_dvz(state->color, color);
    src->renderer = selected_renderer(state->renderer_index[source]);
    (void)dvz_text_set_renderer(src->text, src->renderer);

    const char* strings[1] = {state->text};
    float positions[1][3] = {{36.0f, 118.0f, 0.0f}};
    float anchors[1][2] = {{0.0f, 0.5f}};
    float sizes[1] = {state->size_pts};
    float angles[1] = {state->angle};
    DvzColor colors[1] = {{color[0], color[1], color[2], color[3]}};
    DvzVisualDataUpdate updates[5] = {
        {.attr_name = "position", .data = positions, .item_count = 1},
        {.attr_name = "anchor", .data = anchors, .item_count = 1},
        {.attr_name = "size", .data = sizes, .item_count = 1},
        {.attr_name = "color", .data = colors, .item_count = 1},
        {.attr_name = "angle", .data = angles, .item_count = 1},
    };
    (void)dvz_visual_set_strings(src->text, "text", strings, 1);
    (void)dvz_visual_set_data_many(src->text, updates, 5);
    if (src->win != NULL)
        dvz_app_window_request_frame(src->win);
}



/**
 * Update both text sources.
 *
 * @param state lab state
 */
static void update_sources(TextMsdfLabState* state)
{
    ANN(state);
    update_source_text(state, 0);
    update_source_text(state, 1);
}



/**
 * Return whether a panzoom changed since the last GUI frame.
 *
 * @param state lab state
 * @param source source index
 * @return whether pan or zoom changed
 */
static bool panzoom_changed(TextMsdfLabState* state, uint32_t source)
{
    ANN(state);
    ASSERT(source < 2);
    DvzPanzoom* pz = state->sources[source].panzoom;
    if (pz == NULL)
        return false;
    return pz->pan[0] != state->last_pan[source][0] ||
           pz->pan[1] != state->last_pan[source][1] ||
           pz->zoom[0] != state->last_zoom[source][0] ||
           pz->zoom[1] != state->last_zoom[source][1];
}



/**
 * Copy one panzoom view state into another.
 *
 * @param dst destination panzoom
 * @param src source panzoom
 */
static void copy_panzoom(DvzPanzoom* dst, const DvzPanzoom* src)
{
    ANN(dst);
    ANN(src);
    dvz_panzoom_pan(dst, (vec2){src->pan[0], src->pan[1]});
    dvz_panzoom_zoom(dst, (vec2){src->zoom[0], src->zoom[1]});
    dvz_panzoom_end(dst);
}



/**
 * Remember the current panzoom states for change detection.
 *
 * @param state lab state
 */
static void store_panzooms(TextMsdfLabState* state)
{
    ANN(state);
    for (uint32_t i = 0; i < 2; i++)
    {
        DvzPanzoom* pz = state->sources[i].panzoom;
        if (pz == NULL)
            continue;
        state->last_pan[i][0] = pz->pan[0];
        state->last_pan[i][1] = pz->pan[1];
        state->last_zoom[i][0] = pz->zoom[0];
        state->last_zoom[i][1] = pz->zoom[1];
    }
}



/**
 * Synchronize linked source panzooms.
 *
 * @param state lab state
 */
static void sync_panzooms(TextMsdfLabState* state)
{
    ANN(state);
    if (!state->link_panzoom || state->sources[0].panzoom == NULL ||
        state->sources[1].panzoom == NULL)
    {
        store_panzooms(state);
        return;
    }

    bool changed_a = panzoom_changed(state, 0);
    bool changed_b = panzoom_changed(state, 1);
    if (changed_a && !changed_b)
    {
        copy_panzoom(state->sources[1].panzoom, state->sources[0].panzoom);
        if (state->sources[1].win != NULL)
            dvz_app_window_request_frame(state->sources[1].win);
    }
    else if (changed_b && !changed_a)
    {
        copy_panzoom(state->sources[0].panzoom, state->sources[1].panzoom);
        if (state->sources[0].win != NULL)
            dvz_app_window_request_frame(state->sources[0].win);
    }
    store_panzooms(state);
}



/**
 * Capture one source framebuffer into CPU memory.
 *
 * @param source source state
 */
static void capture_source(TextLabSource* source)
{
    ANN(source);
    source->capture_valid = false;
    if (source->win == NULL)
        return;
    DvzCanvas* canvas = dvz_app_window_canvas(source->win);
    if (canvas == NULL)
        return;

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t* rgba = NULL;
    if (dvz_canvas_capture_rgba(canvas, &width, &height, &rgba) != 0 || rgba == NULL)
        return;

    dvz_free(source->rgba);
    source->rgba = rgba;
    source->width = width;
    source->height = height;
    source->capture_valid = width > 0 && height > 0;
}



/**
 * Capture a source window after a successful source frame.
 *
 * @param win source app window
 * @param user_data source state
 */
static void source_frame_callback(DvzAppWindow* win, void* user_data)
{
    (void)win;
    TextLabSource* source = (TextLabSource*)user_data;
    if (source != NULL)
        capture_source(source);
}



/**
 * Return an ImGui color from RGBA bytes.
 *
 * @param rgba four RGBA bytes
 * @return ImGui packed color
 */
static ImU32 imgui_color(const uint8_t* rgba)
{
    ANN(rgba);
    return (ImU32)rgba[0] | ((ImU32)rgba[1] << 8) | ((ImU32)rgba[2] << 16) |
           ((ImU32)rgba[3] << 24);
}



/**
 * Draw one nearest-neighbor pixel crop using ImGui rectangles.
 *
 * @param source source state
 * @param center_x crop center x
 * @param center_y crop center y
 * @param crop_size crop size in source pixels
 * @param pixel_zoom displayed source pixel size
 * @param show_grid whether to draw a visible pixel grid
 */
static void draw_pixel_crop(
    const TextLabSource* source, int center_x, int center_y, int crop_size, int pixel_zoom,
    bool show_grid)
{
    ANN(source);
    if (!source->capture_valid || source->rgba == NULL || crop_size <= 0 || pixel_zoom <= 0)
    {
        igDummy((ImVec2){320.0f, 220.0f});
        return;
    }

    if (crop_size > 128)
        crop_size = 128;
    int width = (int)source->width;
    int height = (int)source->height;
    int x0 = center_x - crop_size / 2;
    int y0 = center_y - crop_size / 2;
    ImVec2 origin = {0};
    igGetCursorScreenPos(&origin);
    ImDrawList* draw_list = igGetWindowDrawList();
    float z = (float)pixel_zoom;
    for (int y = 0; y < crop_size; y++)
    {
        for (int x = 0; x < crop_size; x++)
        {
            int sx = x0 + x;
            int sy = y0 + y;
            ImU32 col = 0xFF1B1F26u;
            if (sx >= 0 && sy >= 0 && sx < width && sy < height)
            {
                uint64_t idx = ((uint64_t)sy * (uint64_t)width + (uint64_t)sx) * 4u;
                col = imgui_color(&source->rgba[idx]);
            }
            ImVec2 p0 = {origin.x + (float)x * z, origin.y + (float)y * z};
            ImVec2 p1 = {p0.x + z, p0.y + z};
            ImDrawList_AddRectFilled(draw_list, p0, p1, col, 0.0f, 0);
        }
    }
    if (show_grid && pixel_zoom >= 4)
    {
        ImU32 grid = 0x55303030u;
        for (int x = 0; x <= crop_size; x++)
        {
            ImVec2 p0 = {origin.x + (float)x * z, origin.y};
            ImVec2 p1 = {p0.x, origin.y + (float)crop_size * z};
            ImDrawList_AddLine(draw_list, p0, p1, grid, 1.0f);
        }
        for (int y = 0; y <= crop_size; y++)
        {
            ImVec2 p0 = {origin.x, origin.y + (float)y * z};
            ImVec2 p1 = {origin.x + (float)crop_size * z, p0.y};
            ImDrawList_AddLine(draw_list, p0, p1, grid, 1.0f);
        }
    }
    igDummy((ImVec2){(float)crop_size * z, (float)crop_size * z});
}


/**
 * Clamp an integer into an inclusive range.
 *
 * @param value input value
 * @param min_value lower bound
 * @param max_value upper bound
 * @return clamped value
 */
static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}


/**
 * Update the inspector crop center from a hovered source viewport.
 *
 * @param state lab state
 * @param source source index
 */
static void update_hover_crop(TextMsdfLabState* state, uint32_t source)
{
    ANN(state);
    ASSERT(source < 2);
    TextLabSource* src = &state->sources[source];
    float pos[2] = {0};
    float size[2] = {0};
    bool hovered = false;
    if (!dvz_gui_viewport_mouse(src->viewport, pos, size, &hovered) || !hovered)
        return;
    if (size[0] <= 0.0f || size[1] <= 0.0f)
        return;

    uint32_t width = src->width > 0 ? src->width : TEXT_MSDF_LAB_SOURCE_WIDTH;
    uint32_t height = src->height > 0 ? src->height : TEXT_MSDF_LAB_SOURCE_HEIGHT;
    int x = (int)(pos[0] * (float)width / size[0] + 0.5f);
    int y = (int)(pos[1] * (float)height / size[1] + 0.5f);
    state->crop_center[0] = clamp_int(x, 0, (int)width - 1);
    state->crop_center[1] = clamp_int(y, 0, (int)height - 1);
    state->hover_source = (int)source;
}


/**
 * Apply the default fixed lab layout to the next ImGui window.
 *
 * @param state lab state
 * @param kind window kind
 */
static void set_next_lab_window_rect(const TextMsdfLabState* state, int kind)
{
    ANN(state);
    if (!state->lock_layout)
        return;

    ImGuiViewport* viewport = igGetMainViewport();
    if (viewport == NULL)
        return;
    ImVec2 pos = viewport->WorkPos;
    ImVec2 size = viewport->WorkSize;
    float controls_w = 260.0f;
    float top_h = size.y * 0.52f;
    float content_x = pos.x + controls_w;
    float content_w = size.x - controls_w;
    if (content_w < 200.0f)
        content_w = 200.0f;
    float half_w = content_w * 0.5f;

    ImVec2 win_pos = pos;
    ImVec2 win_size = size;
    if (kind == 0)
    {
        win_size.x = controls_w;
    }
    else if (kind == 1)
    {
        win_pos.x = content_x;
        win_size.x = half_w;
        win_size.y = top_h;
    }
    else if (kind == 2)
    {
        win_pos.x = content_x + half_w;
        win_size.x = half_w;
        win_size.y = top_h;
    }
    else
    {
        win_pos.x = content_x;
        win_pos.y = pos.y + top_h;
        win_size.x = content_w;
        win_size.y = size.y - top_h;
    }
    igSetNextWindowPos(win_pos, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize(win_size, ImGuiCond_Always);
}



/**
 * Build the pixel inspector window.
 *
 * @param gui GUI overlay
 * @param state lab state
 */
static void draw_inspector(DvzGui* gui, TextMsdfLabState* state)
{
    ANN(gui);
    ANN(state);
    set_next_lab_window_rect(state, 3);
    if (dvz_gui_begin(gui, "Pixel inspector", NULL, 0))
    {
        (void)dvz_gui_checkbox(gui, "Grid", &state->show_grid);
        (void)dvz_gui_slider_int(gui, "Crop", &state->crop_size, 8, 96);
        (void)dvz_gui_slider_int(gui, "Pixel zoom", &state->pixel_zoom, 2, 20);

        char line[256] = {0};
        dvz_snprintf(
            line, sizeof(line), "hover: %s  pixel: %d, %d",
            state->hover_source == 0 ? "left" : state->hover_source == 1 ? "right" : "none",
            state->crop_center[0], state->crop_center[1]);
        dvz_gui_text(gui, line);
        dvz_snprintf(
            line, sizeof(line), "left: %ux%u %s", state->sources[0].width,
            state->sources[0].height, state->sources[0].capture_valid ? "captured" : "empty");
        dvz_gui_text(gui, line);
        dvz_snprintf(
            line, sizeof(line), "right: %ux%u %s", state->sources[1].width,
            state->sources[1].height, state->sources[1].capture_valid ? "captured" : "empty");
        dvz_gui_text(gui, line);
        atlas_summary_line("left", &state->sources[0], state->size_pts, line, sizeof(line));
        dvz_gui_text(gui, line);
        atlas_summary_line("right", &state->sources[1], state->size_pts, line, sizeof(line));
        dvz_gui_text(gui, line);

        igBeginGroup();
        igSeparatorText("Left");
        draw_pixel_crop(
            &state->sources[0], state->crop_center[0], state->crop_center[1], state->crop_size,
            state->pixel_zoom, state->show_grid);
        igEndGroup();
        dvz_gui_same_line(gui, 0.0f, 12.0f);
        igBeginGroup();
        igSeparatorText("Right");
        draw_pixel_crop(
            &state->sources[1], state->crop_center[0], state->crop_center[1], state->crop_size,
            state->pixel_zoom, state->show_grid);
        igEndGroup();
    }
    dvz_gui_end(gui);
}



/**
 * Build the control GUI.
 *
 * @param gui GUI overlay
 * @param win app window
 * @param user_data lab state
 */
static void gui_callback(DvzGui* gui, DvzAppWindow* win, void* user_data)
{
    (void)win;
    TextMsdfLabState* state = (TextMsdfLabState*)user_data;
    ANN(state);

    set_next_lab_window_rect(state, 1);
    (void)dvz_gui_viewport_window(state->sources[0].viewport, "Baseline", NULL, 0);
    update_hover_crop(state, 0);
    set_next_lab_window_rect(state, 2);
    (void)dvz_gui_viewport_window(state->sources[1].viewport, "Candidate", NULL, 0);
    update_hover_crop(state, 1);
    sync_panzooms(state);

    bool changed = false;
    set_next_lab_window_rect(state, 0);
    if (dvz_gui_begin(gui, "Text controls", NULL, 0))
    {
        static const char* const sample_items[] = {
            "glyph stress",
            "sentence",
            "UTF-8",
            "dense strokes",
        };
        static const char* const renderer_items[] = {
            "auto",
            "6x8 bitmap",
            "FreeType bitmap",
            "MSDF",
        };
        int old_sample = state->sample_index;
        changed |= dvz_gui_combo(gui, "Sample", &state->sample_index, sample_items, 4);
        if (old_sample != state->sample_index)
            set_sample_text(state);
        changed |= igInputText(
            "Text", state->text, sizeof(state->text), ImGuiInputTextFlags_EnterReturnsTrue, NULL,
            NULL);
        changed |= dvz_gui_combo(gui, "Left renderer", &state->renderer_index[0], renderer_items, 4);
        changed |=
            dvz_gui_combo(gui, "Right renderer", &state->renderer_index[1], renderer_items, 4);
        changed |=
            dvz_gui_slider_float_format(gui, "Size", &state->size_pts, 6.0f, 160.0f, "%.1f pt");
        changed |= dvz_gui_slider_float(gui, "Angle", &state->angle, -1.57f, 1.57f);
        changed |= dvz_gui_color_edit4(gui, "Color", state->color, 0);
        (void)dvz_gui_checkbox(gui, "Linked panzoom", &state->link_panzoom);
        (void)dvz_gui_checkbox(gui, "Lock layout", &state->lock_layout);
        if (dvz_gui_button(gui, "Reset panzoom"))
        {
            for (uint32_t i = 0; i < 2; i++)
            {
                if (state->sources[i].panzoom != NULL)
                    dvz_panzoom_reset(state->sources[i].panzoom);
                if (state->sources[i].win != NULL)
                    dvz_app_window_request_frame(state->sources[i].win);
            }
            store_panzooms(state);
        }
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
    }
    dvz_gui_end(gui);

    if (changed)
        update_sources(state);
    draw_inspector(gui, state);
    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);
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



/**
 * Initialize one source scene.
 *
 * @param scene owning scene
 * @param source source state
 * @return zero on success
 */
static int setup_source_scene(DvzScene* scene, TextLabSource* source)
{
    ANN(scene);
    ANN(source);
    source->figure = dvz_figure(scene, TEXT_MSDF_LAB_SOURCE_WIDTH, TEXT_MSDF_LAB_SOURCE_HEIGHT, 0);
    if (source->figure == NULL)
        return -1;
    source->panel = dvz_panel(
        source->figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 1.0f, .height = 1.0f});
    if (source->panel == NULL)
        return -1;
    dvz_panel_set_background_color(source->panel, 0.055f, 0.065f, 0.085f, 1.0f);
    source->text = dvz_text(scene, 0);
    if (source->text == NULL)
        return -1;
    DvzVisualAttachDesc attach = {
        .z_layer = 4,
        .controller_mode = DVZ_CONTROLLER_APPLY_ISOTROPIC_LOCAL,
    };
    if (dvz_panel_add_visual(source->panel, source->text, &attach) != 0)
        return -1;
    return 0;
}



/**
 * Release CPU capture buffers.
 *
 * @param state lab state
 */
static void free_captures(TextMsdfLabState* state)
{
    ANN(state);
    for (uint32_t i = 0; i < 2; i++)
    {
        dvz_free(state->sources[i].rgba);
        state->sources[i].rgba = NULL;
    }
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

/**
 * Run the interactive text renderer comparison lab.
 *
 * @param argc argument count
 * @param argv arguments
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

    TextMsdfLabState state = {0};
    state.size_pts = 72.0f;
    state.color[0] = 0.86f;
    state.color[1] = 0.93f;
    state.color[2] = 1.00f;
    state.color[3] = 1.00f;
    state.renderer_index[0] = 2;
    state.renderer_index[1] = 3;
    state.sample_index = 0;
    state.crop_center[0] = 150;
    state.crop_center[1] = 118;
    state.crop_size = 48;
    state.pixel_zoom = 8;
    state.hover_source = -1;
    state.link_panzoom = true;
    state.lock_layout = true;
    state.show_grid = true;
    set_sample_text(&state);

    if (setup_source_scene(scene, &state.sources[0]) != 0 ||
        setup_source_scene(scene, &state.sources[1]) != 0)
    {
        dvz_fprintf(stderr, "source setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    DvzFigure* host_figure = dvz_figure(scene, TEXT_MSDF_LAB_HOST_WIDTH, TEXT_MSDF_LAB_HOST_HEIGHT, 0);
    DvzPanel* host_panel = host_figure != NULL
                               ? dvz_panel(host_figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f})
                               : NULL;
    if (host_figure == NULL || host_panel == NULL)
    {
        dvz_fprintf(stderr, "host figure setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(host_panel, 0.045f, 0.052f, 0.062f, 1.0f);
    update_sources(&state);

    DvzAppConfig app_config = dvz_app_config();
    app_config.schedule_mode = DVZ_APP_SCHEDULE_CONTINUOUS;
    DvzApp* app = dvz_app_with_config(scene, &app_config);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    for (uint32_t i = 0; i < 2; i++)
    {
        state.sources[i].win = dvz_app_window(
            app, state.sources[i].figure, TEXT_MSDF_LAB_SOURCE_WIDTH, TEXT_MSDF_LAB_SOURCE_HEIGHT);
        if (state.sources[i].win == NULL)
        {
            dvz_fprintf(stderr, "source app-window setup failed\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
        dvz_app_window_set_frame_callback(state.sources[i].win, source_frame_callback, &state.sources[i]);
    }
    state.host_win =
        dvz_app_window_glfw(app, host_figure, TEXT_MSDF_LAB_HOST_WIDTH, TEXT_MSDF_LAB_HOST_HEIGHT,
                            "text_msdf_lab");
    if (state.host_win == NULL)
    {
        dvz_fprintf(stderr, "host GLFW window setup failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_app_window_gui(state.host_win, &gui_config);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "GUI setup failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzGuiViewportConfig viewport_config = dvz_gui_viewport_config();
    viewport_config.initial_width = TEXT_MSDF_LAB_SOURCE_WIDTH;
    viewport_config.initial_height = TEXT_MSDF_LAB_SOURCE_HEIGHT;
    viewport_config.resize_step = 16;
    for (uint32_t i = 0; i < 2; i++)
    {
        state.sources[i].viewport =
            dvz_gui_viewport_from_window(gui, state.sources[i].win, &viewport_config);
        if (state.sources[i].viewport == NULL)
        {
            dvz_fprintf(stderr, "GUI viewport setup failed\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
        DvzController* panzoom_controller = dvz_panzoom(scene, NULL);
        state.sources[i].panzoom = dvz_controller_panzoom(panzoom_controller);
        if (state.sources[i].panzoom == NULL ||
            dvz_panel_bind_controller(
                state.sources[i].panel, panzoom_controller, DVZ_DIM_MASK_XY) != 0)
        {
            dvz_fprintf(stderr, "failed to create or bind panzoom controller\n");
            dvz_app_destroy(app);
            dvz_scene_destroy(scene);
            return 1;
        }
        dvz_panel_connect_input(
            state.sources[i].panel, dvz_gui_viewport_input(state.sources[i].viewport));
    }
    store_panzooms(&state);
    dvz_app_window_set_gui_callback(state.host_win, gui_callback, &state);

    dvz_app_run(app, frame_count(argc, argv));

    free_captures(&state);
    dvz_gui_viewport_destroy(state.sources[0].viewport);
    dvz_gui_viewport_destroy(state.sources[1].viewport);
    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
