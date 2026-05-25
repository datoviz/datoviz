/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* labels - napari Labels-layer proof of concept.
 *
 * This demo intentionally emulates labels on the CPU: a synthetic label map is colorized into an
 * RGBA8 overlay texture, then composited over a synthetic microscopy image with ordinary source-over
 * blending. The final LabelsVisual architecture should keep the label IDs as an integer GPU texture.
 *
 * Build:  cmake --build build --target labels
 * Run:    ./build/examples/c/showcase/labels
 * Smoke:  ./build/examples/c/showcase/labels 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_alloc.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/scene.h"
#include "example_common.h"
#include "datoviz/scene/panzoom.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH          1100
#define HEIGHT         780
#define TEX_W          384
#define TEX_H          384
#define CELL_COUNT     135
#define IMAGE_MIN_NDC -0.92f
#define IMAGE_MAX_NDC +0.92f



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LabelCell LabelCell;
typedef struct LabelsDemoState LabelsDemoState;


struct LabelCell
{
    uint32_t id;
    int32_t cx;
    int32_t cy;
    int32_t rx;
    int32_t ry;
};


struct LabelsDemoState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzView* win;
    DvzVisual* overlay;
    uint32_t* labels;
    uint8_t* base_rgba;
    uint8_t* overlay_rgba;
    uint8_t* pick_rgba;
    uint32_t label_count;
    float opacity;
    float selected_label_value;
    bool labels_visible;
    bool boundary_mode;
    bool selected_only;
    bool dirty_overlay;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    uint32_t hover_label;
    uint64_t next_probe_request_id;
    uint64_t select_probe_request_id;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a deterministic pseudo-random integer.
 *
 * @param x input integer
 * @return hashed integer
 */
static uint32_t _hash_u32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}



/**
 * Convert a label id to a bright categorical color.
 *
 * @param id label id
 * @param out output RGBA color
 */
static void _label_color(uint32_t id, uint8_t out[4])
{
    uint32_t h = _hash_u32(id);
    out[0] = (uint8_t)(70u + (h & 0x7fu));
    out[1] = (uint8_t)(80u + ((h >> 8) & 0x7fu));
    out[2] = (uint8_t)(90u + ((h >> 16) & 0x7fu));
    out[3] = 255;
}



/**
 * Return whether a label-map pixel lies on a label boundary.
 *
 * @param labels label map
 * @param x pixel x coordinate
 * @param y pixel y coordinate
 * @return whether the pixel borders a different label id
 */
static bool _label_boundary(const uint32_t* labels, uint32_t x, uint32_t y)
{
    uint32_t id = labels[y * TEX_W + x];
    if (id == 0)
        return false;
    if (x > 0 && labels[y * TEX_W + x - 1] != id)
        return true;
    if (x + 1 < TEX_W && labels[y * TEX_W + x + 1] != id)
        return true;
    if (y > 0 && labels[(y - 1) * TEX_W + x] != id)
        return true;
    if (y + 1 < TEX_H && labels[(y + 1) * TEX_W + x] != id)
        return true;
    return false;
}



/**
 * Generate synthetic microscopy-like image data.
 *
 * @param rgba output RGBA8 image
 */
static void _generate_base_image(uint8_t* rgba)
{
    for (uint32_t y = 0; y < TEX_H; y++)
    {
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            uint32_t h = _hash_u32(x * 1973u + y * 9277u + 19u);
            uint32_t cx = x > TEX_W / 2 ? x - TEX_W / 2 : TEX_W / 2 - x;
            uint32_t cy = y > TEX_H / 2 ? y - TEX_H / 2 : TEX_H / 2 - y;
            uint32_t vignette = (cx + cy) / 9u;
            uint32_t value = 34u + ((x + 2u * y) / 24u) + (h & 31u);
            value = value > vignette ? value - vignette : 12u;
            if (value > 210u)
                value = 210u;

            uint64_t i = 4ull * ((uint64_t)y * TEX_W + x);
            rgba[i + 0] = (uint8_t)value;
            rgba[i + 1] = (uint8_t)(value + value / 8u);
            rgba[i + 2] = (uint8_t)(value + value / 5u);
            rgba[i + 3] = 255;
        }
    }
}



/**
 * Build deterministic ellipse descriptors for synthetic nuclei.
 *
 * @param cells output cell descriptors
 */
static void _generate_cells(LabelCell cells[CELL_COUNT])
{
    for (uint32_t i = 0; i < CELL_COUNT; i++)
    {
        uint32_t h0 = _hash_u32(101u + i * 13u);
        uint32_t h1 = _hash_u32(809u + i * 29u);
        cells[i] = (LabelCell){
            .id = i + 1u,
            .cx = 18 + (int32_t)(h0 % (TEX_W - 36u)),
            .cy = 18 + (int32_t)(h1 % (TEX_H - 36u)),
            .rx = 7 + (int32_t)((h0 >> 12) % 11u),
            .ry = 6 + (int32_t)((h1 >> 14) % 12u),
        };
    }
}



/**
 * Generate a synthetic instance-label map and brighten the base image inside cells.
 *
 * @param cells synthetic cell descriptors
 * @param labels output label map
 * @param base_rgba microscopy image to brighten in labeled regions
 */
static void _generate_labels(
    const LabelCell cells[CELL_COUNT], uint32_t* labels, uint8_t* base_rgba)
{
    dvz_memset(labels, TEX_W * TEX_H * sizeof(uint32_t), 0, TEX_W * TEX_H * sizeof(uint32_t));

    for (uint32_t i = 0; i < CELL_COUNT; i++)
    {
        const LabelCell* cell = &cells[i];
        int32_t x0 = cell->cx - cell->rx - 2;
        int32_t x1 = cell->cx + cell->rx + 2;
        int32_t y0 = cell->cy - cell->ry - 2;
        int32_t y1 = cell->cy + cell->ry + 2;
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= (int32_t)TEX_W)
            x1 = (int32_t)TEX_W - 1;
        if (y1 >= (int32_t)TEX_H)
            y1 = (int32_t)TEX_H - 1;

        int64_t rx2 = (int64_t)cell->rx * cell->rx;
        int64_t ry2 = (int64_t)cell->ry * cell->ry;
        int64_t rxy = rx2 * ry2;
        for (int32_t y = y0; y <= y1; y++)
        {
            for (int32_t x = x0; x <= x1; x++)
            {
                int64_t dx = x - cell->cx;
                int64_t dy = y - cell->cy;
                if (dx * dx * ry2 + dy * dy * rx2 > rxy)
                    continue;

                labels[(uint32_t)y * TEX_W + (uint32_t)x] = cell->id;
                uint64_t p = 4ull * ((uint64_t)(uint32_t)y * TEX_W + (uint32_t)x);
                for (uint32_t c = 0; c < 3; c++)
                {
                    uint32_t value = base_rgba[p + c] + 52u;
                    base_rgba[p + c] = (uint8_t)(value > 255u ? 255u : value);
                }
            }
        }
    }
}



/**
 * Rebuild the CPU-colored RGBA labels overlay.
 *
 * @param state demo state
 */
static void _rebuild_overlay(LabelsDemoState* state)
{
    if (state == NULL || state->labels == NULL || state->overlay_rgba == NULL)
        return;

    uint32_t selected = (uint32_t)(state->selected_label_value + 0.5f);
    float opacity = state->opacity;
    if (opacity < 0.0f)
        opacity = 0.0f;
    if (opacity > 1.0f)
        opacity = 1.0f;

    for (uint32_t y = 0; y < TEX_H; y++)
    {
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            uint32_t id = state->labels[y * TEX_W + x];
            uint64_t p = 4ull * ((uint64_t)y * TEX_W + x);
            state->overlay_rgba[p + 0] = 0;
            state->overlay_rgba[p + 1] = 0;
            state->overlay_rgba[p + 2] = 0;
            state->overlay_rgba[p + 3] = 0;
            if (!state->labels_visible || id == 0)
                continue;
            if (state->selected_only && selected != 0 && id != selected)
                continue;
            if (state->boundary_mode && !_label_boundary(state->labels, x, y))
                continue;

            uint8_t color[4] = {0};
            _label_color(id, color);
            if (selected != 0 && id == selected)
            {
                color[0] = 255;
                color[1] = 246;
                color[2] = 96;
            }
            float alpha = opacity;
            if (selected != 0 && id == selected && alpha < 0.86f)
                alpha = 0.86f;
            state->overlay_rgba[p + 0] = color[0];
            state->overlay_rgba[p + 1] = color[1];
            state->overlay_rgba[p + 2] = color[2];
            state->overlay_rgba[p + 3] = (uint8_t)(255.0f * alpha + 0.5f);
        }
    }
    state->dirty_overlay = false;
}



/**
 * Encode label IDs into an RGBA8 texture for scene segment probing.
 *
 * @param labels source uint32 label map
 * @param rgba output RGBA8 pick texture
 */
static void _encode_pick_texture(const uint32_t* labels, uint8_t* rgba)
{
    if (labels == NULL || rgba == NULL)
        return;

    for (uint32_t y = 0; y < TEX_H; y++)
    {
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            /* Match the displayed image orientation used by the scene image visual. */
            uint32_t src_y = TEX_H - 1u - y;
            uint32_t id = labels[src_y * TEX_W + x];
            uint64_t p = 4ull * ((uint64_t)y * TEX_W + x);
            rgba[p + 0] = (uint8_t)(id & 0xffu);
            rgba[p + 1] = (uint8_t)((id >> 8) & 0xffu);
            rgba[p + 2] = (uint8_t)((id >> 16) & 0xffu);
            rgba[p + 3] = id == 0 ? 0 : 255;
        }
    }
}



/**
 * Rebuild and upload the overlay texture if a control or click changed label display state.
 *
 * @param state demo state
 */
static void _upload_overlay_if_dirty(LabelsDemoState* state)
{
    if (state == NULL || !state->dirty_overlay || state->overlay == NULL)
        return;

    _rebuild_overlay(state);
    if (dvz_visual_set_texture(state->overlay, state->overlay_rgba, TEX_W, TEX_H) != 0)
        fprintf(stderr, "labels overlay texture update failed\n");
}



/**
 * Select a decoded label and mark the overlay for refresh.
 *
 * @param state demo state
 * @param label_id label id, or 0 to clear the selection
 */
static void _select_label(LabelsDemoState* state, uint32_t label_id)
{
    if (state == NULL)
        return;

    uint32_t selected = label_id;
    if (selected > state->label_count)
        selected = 0;
    if ((uint32_t)(state->selected_label_value + 0.5f) != selected)
    {
        state->selected_label_value = (float)selected;
        state->dirty_overlay = true;
    }
}



/**
 * Queue a segment probe at the current cursor position.
 *
 * @param state demo state
 * @param select whether the resolved probe should update the selected label
 */
static void _request_label_probe(LabelsDemoState* state, bool select)
{
    if (state == NULL || !state->cursor_valid || state->panel == NULL)
        return;

    uint64_t request_id = ++state->next_probe_request_id;
    if (request_id == 0)
        request_id = ++state->next_probe_request_id;
    if (dvz_panel_probe(
            state->panel, state->cursor_x, state->cursor_y,
            &(DvzProbeRequest){
                .request_id = request_id,
                .target = DVZ_SCENE_TARGET_SEGMENT,
            }) != 0)
    {
        dvz_fprintf(stderr, "label segment probe request failed\n");
        return;
    }
    if (select)
        state->select_probe_request_id = request_id;
    if (state->win != NULL)
        dvz_view_request_frame(state->win);
}



/**
 * Create and initialize one image visual.
 *
 * @param scene owning scene
 * @param pixels RGBA8 pixels
 * @param alpha_mode alpha mode for the visual
 * @return initialized image visual, or NULL
 */
static DvzVisual*
_image_visual(DvzScene* scene, const uint8_t* pixels, DvzAlphaMode alpha_mode)
{
    DvzVisual* visual = dvz_image(scene, 0);
    if (visual == NULL)
        return NULL;

    vec3 positions[4] = {
        {IMAGE_MIN_NDC, IMAGE_MIN_NDC, 0.0f},
        {IMAGE_MIN_NDC, IMAGE_MAX_NDC, 0.0f},
        {IMAGE_MAX_NDC, IMAGE_MIN_NDC, 0.0f},
        {IMAGE_MAX_NDC, IMAGE_MAX_NDC, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (dvz_visual_set_data(visual, "position", positions, 4) != 0 ||
        dvz_visual_set_data(visual, "texcoords", texcoords, 4) != 0 ||
        dvz_visual_set_texture(visual, pixels, TEX_W, TEX_H) != 0 ||
        dvz_visual_set_alpha_mode(visual, alpha_mode) != 0)
    {
        return NULL;
    }
    return visual;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Track cursor movement for CPU label readout.
 *
 * @param router input router emitting the pointer event
 * @param event pointer event payload
 * @param user_data demo state
 */
static void _pointer_callback(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    LabelsDemoState* state = (LabelsDemoState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_x = event->pos[0];
    state->cursor_y = event->pos[1];
    _request_label_probe(state, false);
}



/**
 * Handle derived click gestures emitted by the pointer gesture handler.
 *
 * @param router input router emitting the event
 * @param event input-event payload
 * @param user_data demo state
 */
static void _input_event_callback(DvzInputRouter* router, const DvzInputEvent* event, void* user_data)
{
    (void)router;
    LabelsDemoState* state = (LabelsDemoState*)user_data;
    if (state == NULL || event == NULL || event->type != DVZ_INPUT_EVENT_POINTER)
        return;

    const DvzPointerEvent* pointer = &event->content.pointer;
    if (pointer->type != DVZ_POINTER_EVENT_CLICK)
        return;

    state->cursor_valid = true;
    state->cursor_x = pointer->pos[0];
    state->cursor_y = pointer->pos[1];
    _request_label_probe(state, true);
}



/**
 * Apply deferred overlay updates after input events.
 *
 * @param win view whose frame just completed
 * @param user_data demo state
 */
static void _frame_callback(DvzView* win, void* user_data)
{
    (void)win;
    LabelsDemoState* state = (LabelsDemoState*)user_data;
    if (state == NULL)
        return;

    DvzProbeResult probe = {0};
    while (dvz_scene_poll_probe(state->scene, &probe))
    {
        if (probe.target != DVZ_SCENE_TARGET_SEGMENT)
            continue;

        uint32_t label_id =
            probe.hit && probe.category_id <= UINT32_MAX ? (uint32_t)probe.category_id : 0;
        state->hover_label = label_id;
        if (state->select_probe_request_id != 0 &&
            probe.request_id == state->select_probe_request_id)
        {
            _select_label(state, label_id);
            state->select_probe_request_id = 0;
        }
    }
    _upload_overlay_if_dirty(state);
}



/**
 * Build the demo control panel.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data demo state
 */
static void _gui_callback(DvzGui* gui, DvzView* win, void* user_data)
{
    (void)win;
    LabelsDemoState* state = (LabelsDemoState*)user_data;
    if (state == NULL)
        return;

    bool changed = false;
    if (dvz_gui_begin(gui, "Napari labels", NULL, 0))
    {
        changed |= dvz_gui_checkbox(gui, "Labels", &state->labels_visible);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.0f, 1.0f);
        changed |=
            dvz_gui_slider_float(gui, "Selected", &state->selected_label_value, 0.0f,
                                 (float)state->label_count);
        changed |= dvz_gui_checkbox(gui, "Selected only", &state->selected_only);
        changed |= dvz_gui_checkbox(gui, "Boundaries", &state->boundary_mode);
        if (dvz_gui_button(gui, "Clear selection"))
        {
            state->selected_label_value = 0.0f;
            changed = true;
        }
        igSeparator();
        igText("Hover: label %u", state->hover_label);
    }
    dvz_gui_end(gui);

    if (changed)
        state->dirty_overlay = true;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int status = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;
    uint8_t* base_rgba = (uint8_t*)dvz_calloc(TEX_W * TEX_H * 4ull, 1);
    uint8_t* overlay_rgba = (uint8_t*)dvz_calloc(TEX_W * TEX_H * 4ull, 1);
    uint8_t* pick_rgba = (uint8_t*)dvz_calloc(TEX_W * TEX_H * 4ull, 1);
    uint32_t* labels = (uint32_t*)dvz_calloc(TEX_W * TEX_H, sizeof(uint32_t));
    EXAMPLE_CHECK(
        base_rgba != NULL && overlay_rgba != NULL && pick_rgba != NULL && labels != NULL,
        "texture allocation failed");

    LabelCell cells[CELL_COUNT] = {0};
    _generate_base_image(base_rgba);
    _generate_cells(cells);
    _generate_labels(cells, labels, base_rgba);
    _encode_pick_texture(labels, pick_rgba);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "scene panel setup failed");

    LabelsDemoState state = {
        .scene = scene,
        .panel = panel,
        .labels = labels,
        .base_rgba = base_rgba,
        .overlay_rgba = overlay_rgba,
        .pick_rgba = pick_rgba,
        .label_count = CELL_COUNT,
        .opacity = 0.48f,
        .selected_label_value = 0.0f,
        .labels_visible = true,
    };
    _rebuild_overlay(&state);

    DvzVisual* base = _image_visual(scene, base_rgba, DVZ_ALPHA_OPAQUE);
    DvzVisual* overlay = _image_visual(scene, overlay_rgba, DVZ_ALPHA_BLENDED);
    DvzVisual* label_pick = _image_visual(scene, pick_rgba, DVZ_ALPHA_OPAQUE);
    state.overlay = overlay;
    EXAMPLE_CHECK(base != NULL && overlay != NULL && label_pick != NULL, "image visual setup failed");
    dvz_visual_set_pick_capabilities(label_pick, DVZ_PICK_CAPABILITY_GROUP);
    dvz_visual_set_visible(label_pick, false);

    int rc = dvz_panel_add_visual(panel, base, &(DvzVisualAttachDesc){.z_layer = 0});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(base) failed");

    rc = dvz_panel_add_visual(panel, overlay, &(DvzVisualAttachDesc){.z_layer = 1});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(overlay) failed");

    rc = dvz_panel_add_visual(panel, label_pick, &(DvzVisualAttachDesc){.z_layer = 2});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(label_pick) failed");
    dvz_panel_set_background_color(panel, 0.02f, 0.025f, 0.03f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win =
        dvz_view_glfw(app, figure, WIDTH, HEIGHT, "labels");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");
    state.win = win;

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, &panzoom_desc);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_input_subscribe_pointer(router, _pointer_callback, &state);
    dvz_input_subscribe_event(router, _input_event_callback, &state);
    dvz_view_set_frame_callback(win, _frame_callback, &state);

    DvzGuiConfig gui_config = dvz_gui_config();
    DvzGui* gui = dvz_view_gui(win, &gui_config);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");
    dvz_view_set_gui_callback(win, _gui_callback, &state);

    dvz_app_run(app, example_frame_count(argc, argv));

    status = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    dvz_free(base_rgba);
    dvz_free(overlay_rgba);
    dvz_free(pick_rgba);
    dvz_free(labels);
    return status;
}
