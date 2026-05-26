/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* labels - napari-style Labels visual with categorical legend.
 *
 * This demo keeps label IDs in a signed integer sampled field, binds them to the retained labels
 * visual, attaches a categorical scale/legend, and exposes the labels presentation state through a
 * compact GUI. The hidden image probe path is temporary until labels get raw integer GPU probing.
 *
 * Build:  cmake --build build --target labels
 * Run:    ./build/examples/c/showcase/labels
 * Smoke:  ./build/examples/c/showcase/labels 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <inttypes.h>
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
#define LEGEND_CATEGORY_COUNT 6
#define SELECTED_ITEM_COUNT  6



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LabelCell LabelCell;
typedef struct LabelsDemoState LabelsDemoState;


struct LabelCell
{
    int32_t id;
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
    DvzVisual* labels_visual;
    int32_t* labels;
    float opacity;
    int selected_index;
    int background_id;
    int fallback_seed;
    float boundary_width;
    bool labels_visible;
    bool boundary_mode;
    bool hide_unassigned;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    DvzCategoryId hover_label;
    uint64_t next_probe_request_id;
    uint64_t select_probe_request_id;
};


static const DvzCategoryId LABEL_CATEGORY_IDS[LEGEND_CATEGORY_COUNT] = {
    -100, -7, 17, 42, 89, 1009,
};


static const char* LABEL_CATEGORY_NAMES[LEGEND_CATEGORY_COUNT] = {
    "artifact", "unassigned", "cell 17", "cell 42", "cell 89", "cell 1009",
};


static const DvzCategoryId SELECTED_IDS[SELECTED_ITEM_COUNT] = {
    0, -100, -7, 17, 42, 1009,
};


static const char* SELECTED_LABELS[SELECTED_ITEM_COUNT] = {
    "none", "artifact (-100)", "unassigned (-7)", "cell 17", "cell 42", "cell 1009",
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
static void _label_color(DvzCategoryId id, DvzColor* out)
{
    ANN(out);
    uint32_t h = _hash_u32((uint32_t)(int32_t)id);
    out->r = (uint8_t)(46u + (209u * ((h >> 0) & 0xffu)) / 255u);
    out->g = (uint8_t)(46u + (209u * ((h >> 8) & 0xffu)) / 255u);
    out->b = (uint8_t)(46u + (209u * ((h >> 16) & 0xffu)) / 255u);
    out->a = 209;
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
        int32_t id = (int32_t)i + 1;
        if (i % 41u == 0)
            id = -100;
        else if (i % 29u == 0)
            id = -7;
        else if (i % 31u == 0)
            id = 1009;
        cells[i] = (LabelCell){
            .id = id,
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
    const LabelCell cells[CELL_COUNT], int32_t* labels, uint8_t* base_rgba)
{
    dvz_memset(labels, TEX_W * TEX_H * sizeof(int32_t), 0, TEX_W * TEX_H * sizeof(int32_t));

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
 * Fill one categorical scale entry.
 *
 * @param category category entry to fill
 * @param id category id
 * @param order legend display order
 * @param label legend label
 */
static void _set_category(
    DvzScaleCategory* category, DvzCategoryId id, uint32_t order, const char* label)
{
    ANN(category);
    category->category_id = id;
    category->order = order;
    category->label = label;
    _label_color(id, &category->color);
    category->flags = 0;
}



/**
 * Encode label IDs into an RGBA8 texture for scene segment probing.
 *
 * @param labels source signed label map
 * @param rgba output RGBA8 pick texture
 */
static void _encode_pick_texture(const int32_t* labels, uint8_t* rgba)
{
    if (labels == NULL || rgba == NULL)
        return;

    for (uint32_t y = 0; y < TEX_H; y++)
    {
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            /* Match the displayed image orientation used by the scene image visual. */
            uint32_t src_y = TEX_H - 1u - y;
            uint32_t id = (uint32_t)labels[src_y * TEX_W + x];
            uint64_t p = 4ull * ((uint64_t)y * TEX_W + x);
            rgba[p + 0] = (uint8_t)(id & 0xffu);
            rgba[p + 1] = (uint8_t)((id >> 8) & 0xffu);
            rgba[p + 2] = (uint8_t)((id >> 16) & 0xffu);
            rgba[p + 3] = id == 0 ? 0 : 255;
        }
    }
}



/**
 * Apply labels visual state after a GUI change.
 *
 * @param state demo state
 */
static void _apply_labels_state(LabelsDemoState* state)
{
    if (state == NULL || state->labels_visual == NULL)
        return;

    dvz_visual_set_visible(state->labels_visual, state->labels_visible);
    (void)dvz_labels_set_opacity(state->labels_visual, state->opacity);
    (void)dvz_labels_set_background(state->labels_visual, (DvzCategoryId)state->background_id);
    if (state->selected_index > 0 && state->selected_index < SELECTED_ITEM_COUNT)
        (void)dvz_labels_set_selected(state->labels_visual, SELECTED_IDS[state->selected_index]);
    else
        (void)dvz_labels_clear_selected(state->labels_visual);

    DvzCategoryId hidden[1] = {-7};
    (void)dvz_labels_set_hidden(
        state->labels_visual, state->hide_unassigned ? hidden : NULL,
        state->hide_unassigned ? 1u : 0u);

    DvzColor boundary = {255, 255, 255, 255};
    (void)dvz_labels_set_boundary(
        state->labels_visual, state->boundary_mode, state->boundary_width, boundary);
    (void)dvz_labels_set_fallback_seed(state->labels_visual, (uint32_t)state->fallback_seed);
    if (state->win != NULL)
        dvz_view_request_frame(state->win);
}



/**
 * Select a decoded label and update the labels visual state.
 *
 * @param state demo state
 * @param label_id label id, or 0 to clear the selection
 */
static void _select_label(LabelsDemoState* state, DvzCategoryId label_id)
{
    if (state == NULL)
        return;

    int selected = 0;
    for (int i = 1; i < SELECTED_ITEM_COUNT; i++)
    {
        if (SELECTED_IDS[i] == label_id)
        {
            selected = i;
            break;
        }
    }
    if (state->selected_index != selected)
    {
        state->selected_index = selected;
        _apply_labels_state(state);
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
 * Track cursor movement for label probing.
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
 * Poll deferred probe results after input events.
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

        DvzCategoryId label_id = probe.hit ? probe.category_id : 0;
        state->hover_label = label_id;
        if (state->select_probe_request_id != 0 &&
            probe.request_id == state->select_probe_request_id)
        {
            _select_label(state, label_id);
            state->select_probe_request_id = 0;
        }
    }
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
        dvz_gui_separator_text(gui, "Presentation");
        changed |= dvz_gui_checkbox(gui, "Labels", &state->labels_visible);
        changed |= dvz_gui_slider_float(gui, "Opacity", &state->opacity, 0.0f, 1.0f);
        changed |= dvz_gui_combo(
            gui, "Selected", &state->selected_index, SELECTED_LABELS, SELECTED_ITEM_COUNT);
        changed |= dvz_gui_slider_int(gui, "Background ID", &state->background_id, -10, 0);
        changed |= dvz_gui_checkbox(gui, "Hide unassigned", &state->hide_unassigned);
        changed |= dvz_gui_checkbox(gui, "Boundaries", &state->boundary_mode);
        if (state->boundary_mode)
            changed |= dvz_gui_slider_float(gui, "Boundary width", &state->boundary_width, 1.0f, 8.0f);
        changed |= dvz_gui_slider_int(gui, "Fallback seed", &state->fallback_seed, 0, 64);

        if (dvz_gui_button(gui, "Clear selection"))
        {
            state->selected_index = 0;
            changed = true;
        }
        if (dvz_gui_button(gui, "Reset"))
        {
            state->opacity = 0.82f;
            state->selected_index = 0;
            state->background_id = 0;
            state->fallback_seed = 0;
            state->boundary_width = 1.0f;
            state->labels_visible = true;
            state->boundary_mode = false;
            state->hide_unassigned = false;
            changed = true;
        }

        dvz_gui_separator_text(gui, "Probe");
        char line[128] = {0};
        snprintf(line, sizeof(line), "Hover: label %" PRIi64, state->hover_label);
        dvz_gui_text(gui, line);
    }
    dvz_gui_end(gui);

    if (changed)
        _apply_labels_state(state);
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
    uint8_t* pick_rgba = (uint8_t*)dvz_calloc(TEX_W * TEX_H * 4ull, 1);
    int32_t* labels = (int32_t*)dvz_calloc(TEX_W * TEX_H, sizeof(int32_t));
    EXAMPLE_CHECK(
        base_rgba != NULL && pick_rgba != NULL && labels != NULL, "texture allocation failed");

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
        .opacity = 0.82f,
        .selected_index = 0,
        .background_id = 0,
        .fallback_seed = 0,
        .boundary_width = 1.0f,
        .labels_visible = true,
    };

    DvzVisual* base = _image_visual(scene, base_rgba, DVZ_ALPHA_OPAQUE);
    DvzVisual* label_pick = _image_visual(scene, pick_rgba, DVZ_ALPHA_OPAQUE);
    EXAMPLE_CHECK(base != NULL && label_pick != NULL, "image visual setup failed");
    dvz_visual_set_pick_capabilities(label_pick, DVZ_PICK_CAPABILITY_GROUP);
    dvz_visual_set_visible(label_pick, false);

    DvzScale* labels_scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CATEGORICAL,
                   .label = "cell labels",
               });
    EXAMPLE_CHECK(labels_scale != NULL, "dvz_scale() failed");
    DvzScaleCategory categories[LEGEND_CATEGORY_COUNT] = {0};
    for (uint32_t i = 0; i < LEGEND_CATEGORY_COUNT; i++)
        _set_category(&categories[i], LABEL_CATEGORY_IDS[i], i, LABEL_CATEGORY_NAMES[i]);
    bool ok = dvz_scale_set_categories(labels_scale, categories, LEGEND_CATEGORY_COUNT);
    EXAMPLE_CHECK(ok, "dvz_scale_set_categories() failed");

    DvzSampledField* label_field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = TEX_W,
                   .height = TEX_H,
                   .depth = 1,
               });
    EXAMPLE_CHECK(label_field != NULL, "dvz_sampled_field(labels) failed");
    ok = dvz_sampled_field_set_data(
        label_field, &(DvzFieldDataView){
                         .data = labels,
                         .bytes_per_row = TEX_W * sizeof(int32_t),
                         .rows_per_image = TEX_H,
                     });
    EXAMPLE_CHECK(ok, "dvz_sampled_field_set_data(labels) failed");

    DvzVisual* labels_visual = dvz_labels(scene, 0);
    EXAMPLE_CHECK(labels_visual != NULL, "dvz_labels() failed");
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
    int rc = dvz_visual_set_data(labels_visual, "position", positions, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(labels position) failed");
    rc = dvz_visual_set_data(labels_visual, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(labels texcoords) failed");
    ok = dvz_visual_set_field(labels_visual, "field", label_field);
    EXAMPLE_CHECK(ok, "dvz_visual_set_field(labels) failed");
    rc = dvz_visual_set_scale(labels_visual, "labels", labels_scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale(labels) failed");
    state.labels_visual = labels_visual;
    _apply_labels_state(&state);

    rc = dvz_panel_add_visual(panel, base, &(DvzVisualAttachDesc){.z_layer = 0});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(base) failed");

    rc = dvz_panel_add_visual(panel, labels_visual, &(DvzVisualAttachDesc){.z_layer = 1});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(labels) failed");

    rc = dvz_panel_add_visual(panel, label_pick, &(DvzVisualAttachDesc){.z_layer = 2});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(label_pick) failed");
    DvzLegend* legend = dvz_legend(
        panel, labels_scale,
        &(DvzLegendDesc){
            .title = "Labels",
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .reserve_px = 150.0f,
        });
    EXAMPLE_CHECK(legend != NULL, "dvz_legend() failed");
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
    dvz_free(pick_rgba);
    dvz_free(labels);
    return status;
}
