/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* labels - live labels visual with categorical legend and working selection.
 *
 * This example keeps segmentation IDs in a signed integer sampled field and renders them through
 * dvz_labels(). A separate transparent image overlay provides immediate selection feedback until
 * selected/hidden/boundary labels uniforms are wired into the labels shader.
 *
 * Build:  cmake --build build --target labels
 * Run:    ./build/examples/c/showcase/labels
 * Smoke:  ./build/examples/c/showcase/labels 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
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
#include "datoviz/scene/panzoom.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH             1120
#define HEIGHT            820
#define TEX_W             768
#define TEX_H             768
#define IMAGE_MIN_NDC    -0.92f
#define IMAGE_MAX_NDC    +0.92f
#define REGION_SEED_COUNT 34
#define CATEGORY_COUNT    9
#define SELECT_ITEM_COUNT (CATEGORY_COUNT + 1)



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct RegionSeed RegionSeed;
typedef struct LabelsDemoState LabelsDemoState;


struct RegionSeed
{
    DvzCategoryId id;
    float x;
    float y;
    float weight;
};


struct LabelsDemoState
{
    DvzScene* scene;
    DvzView* win;
    DvzPanzoom* panzoom;
    DvzVisual* labels_visual;
    DvzVisual* selection_visual;
    int32_t* labels;
    uint8_t* selection_rgba;
    DvzCategoryId hover_id;
    DvzCategoryId selected_id;
    int selected_index;
    int outline_width;
    bool labels_visible;
    bool selection_visible;
};


static const DvzCategoryId CATEGORY_IDS[CATEGORY_COUNT] = {
    -100, -7, 17, 42, 89, 256, 1009, 2048, 8191,
};


static const char* CATEGORY_NAMES[CATEGORY_COUNT] = {
    "artifact",
    "unassigned",
    "cell 17",
    "cell 42",
    "cell 89",
    "layer 256",
    "cell 1009",
    "island 2048",
    "region 8191",
};


static const char* SELECT_ITEMS[SELECT_ITEM_COUNT] = {
    "none",
    "artifact (-100)",
    "unassigned (-7)",
    "cell 17",
    "cell 42",
    "cell 89",
    "layer 256",
    "cell 1009",
    "island 2048",
    "region 8191",
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
 * Convert one integer label ID to the same fallback color family as the labels shader.
 *
 * @param id signed label ID
 * @return RGBA color
 */
static DvzColor _label_color(DvzCategoryId id)
{
    uint32_t h = _hash_u32((uint32_t)(int32_t)id);
    return (DvzColor){
        .r = (uint8_t)(46u + (209u * ((h >> 0) & 0xffu)) / 255u),
        .g = (uint8_t)(46u + (209u * ((h >> 8) & 0xffu)) / 255u),
        .b = (uint8_t)(46u + (209u * ((h >> 16) & 0xffu)) / 255u),
        .a = 209,
    };
}



/**
 * Return normalized coordinate for one texture index.
 *
 * @param i integer coordinate
 * @param n texture extent
 * @return normalized coordinate in [-1, +1]
 */
static float _coord01(uint32_t i, uint32_t n)
{
    return (2.0f * ((float)i + 0.5f) / (float)n) - 1.0f;
}



/**
 * Return whether a texel is on a label boundary.
 *
 * @param labels signed label map
 * @param x texel x coordinate
 * @param y texel y coordinate
 * @return whether the texel touches a different label
 */
static bool _label_boundary(const int32_t* labels, uint32_t x, uint32_t y)
{
    ANN(labels);
    int32_t id = labels[y * TEX_W + x];
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
 * Fill deterministic region seeds for a tissue-like synthetic segmentation.
 *
 * @param seeds output region seeds
 */
static void _generate_seeds(RegionSeed seeds[REGION_SEED_COUNT])
{
    ANN(seeds);
    for (uint32_t i = 0; i < REGION_SEED_COUNT; i++)
    {
        uint32_t h0 = _hash_u32(313u + 37u * i);
        uint32_t h1 = _hash_u32(911u + 53u * i);
        float x = -0.78f + 1.56f * (float)(h0 & 0xffffu) / 65535.0f;
        float y = -0.72f + 1.44f * (float)(h1 & 0xffffu) / 65535.0f;
        seeds[i] = (RegionSeed){
            .id = CATEGORY_IDS[i % CATEGORY_COUNT],
            .x = x,
            .y = y,
            .weight = 0.80f + 0.45f * (float)((h0 >> 18) & 0xffu) / 255.0f,
        };
    }
}



/**
 * Return the closest seeded region for a normalized coordinate.
 *
 * @param seeds region seed array
 * @param x normalized x coordinate
 * @param y normalized y coordinate
 * @return signed label ID, or 0 for background
 */
static int32_t _nearest_region(const RegionSeed seeds[REGION_SEED_COUNT], float x, float y)
{
    float tissue = (x * x) / 0.92f + (y * y) / 0.72f;
    if (tissue > 1.0f)
        return 0;

    float best = 1e30f;
    DvzCategoryId best_id = 0;
    for (uint32_t i = 0; i < REGION_SEED_COUNT; i++)
    {
        float dx = x - seeds[i].x;
        float dy = y - seeds[i].y;
        float ripple = 0.035f * sinf(17.0f * x + 11.0f * y + (float)i);
        float d = (dx * dx + dy * dy) * seeds[i].weight + ripple;
        if (d < best)
        {
            best = d;
            best_id = seeds[i].id;
        }
    }
    return (int32_t)best_id;
}



/**
 * Generate a smooth microscopy-like underlay and a signed labels texture.
 *
 * @param labels output signed label field
 * @param base_rgba output RGBA8 base image
 */
static void _generate_dataset(int32_t* labels, uint8_t* base_rgba)
{
    ANN(labels);
    ANN(base_rgba);

    RegionSeed seeds[REGION_SEED_COUNT] = {0};
    _generate_seeds(seeds);

    for (uint32_t y = 0; y < TEX_H; y++)
    {
        float yy = _coord01(y, TEX_H);
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            float xx = _coord01(x, TEX_W);
            labels[y * TEX_W + x] = _nearest_region(seeds, xx, yy);
        }
    }

    for (uint32_t y = 0; y < TEX_H; y++)
    {
        float yy = _coord01(y, TEX_H);
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            float xx = _coord01(x, TEX_W);
            float r2 = xx * xx + yy * yy;
            uint32_t h = _hash_u32(x * 1973u + y * 9277u + 19u);
            float texture = 0.08f * sinf(18.0f * xx + 4.0f * yy) +
                            0.06f * sinf(9.0f * yy - 3.0f * xx);
            float value = 0.22f + 0.46f * fmaxf(0.0f, 1.0f - 0.68f * r2) + texture;
            value += 0.08f * (float)(h & 0xffu) / 255.0f;
            if (labels[y * TEX_W + x] != 0)
                value += 0.08f;
            if (_label_boundary(labels, x, y))
                value += 0.16f;
            if (value < 0.0f)
                value = 0.0f;
            if (value > 1.0f)
                value = 1.0f;

            uint8_t v = (uint8_t)(255.0f * value + 0.5f);
            uint64_t p = 4ull * ((uint64_t)y * TEX_W + x);
            base_rgba[p + 0] = (uint8_t)(0.82f * (float)v);
            base_rgba[p + 1] = (uint8_t)(0.92f * (float)v);
            base_rgba[p + 2] = v;
            base_rgba[p + 3] = 255;
        }
    }
}



/**
 * Fill one categorical scale entry.
 *
 * @param category category entry to fill
 * @param id category ID
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
    category->color = _label_color(id);
    category->flags = 0;
}



/**
 * Return label ID for one selection combo index.
 *
 * @param index combo index
 * @return selected label ID, or 0 for none
 */
static DvzCategoryId _selected_id_from_index(int index)
{
    if (index <= 0 || index >= SELECT_ITEM_COUNT)
        return 0;
    return CATEGORY_IDS[(uint32_t)index - 1u];
}



/**
 * Return combo index for a label ID.
 *
 * @param id label ID
 * @return combo index, or 0 for none/unknown
 */
static int _selected_index_from_id(DvzCategoryId id)
{
    for (uint32_t i = 0; i < CATEGORY_COUNT; i++)
    {
        if (CATEGORY_IDS[i] == id)
            return (int)i + 1;
    }
    return 0;
}



/**
 * Return the display label for a label ID.
 *
 * @param id label ID
 * @return label text
 */
static const char* _category_name(DvzCategoryId id)
{
    if (id == 0)
        return "background";
    for (uint32_t i = 0; i < CATEGORY_COUNT; i++)
    {
        if (CATEGORY_IDS[i] == id)
            return CATEGORY_NAMES[i];
    }
    return "unknown";
}



/**
 * Convert a pointer position to a label ID using the current panzoom extent.
 *
 * @param state demo state
 * @param px pointer x in window pixels
 * @param py pointer y in window pixels
 * @return label ID under the pointer, or 0 outside the labels quad
 */
static DvzCategoryId _label_at_pointer(const LabelsDemoState* state, double px, double py)
{
    ANN(state);
    if (state->labels == NULL)
        return 0;

    float extent[4] = {-1.0f, +1.0f, -1.0f, +1.0f};
    if (state->panzoom != NULL)
        (void)dvz_panzoom_extent(state->panzoom, extent);

    float sx = (float)(px / (double)WIDTH);
    float sy = (float)(py / (double)HEIGHT);
    float vx = extent[0] + sx * (extent[1] - extent[0]);
    float vy = extent[3] - sy * (extent[3] - extent[2]);

    if (vx < IMAGE_MIN_NDC || vx > IMAGE_MAX_NDC || vy < IMAGE_MIN_NDC || vy > IMAGE_MAX_NDC)
        return 0;

    float u = (vx - IMAGE_MIN_NDC) / (IMAGE_MAX_NDC - IMAGE_MIN_NDC);
    float v = (vy - IMAGE_MIN_NDC) / (IMAGE_MAX_NDC - IMAGE_MIN_NDC);
    uint32_t x = (uint32_t)floorf(u * (float)TEX_W);
    uint32_t y = (uint32_t)floorf(v * (float)TEX_H);
    if (x >= TEX_W)
        x = TEX_W - 1;
    if (y >= TEX_H)
        y = TEX_H - 1;
    return (DvzCategoryId)state->labels[y * TEX_W + x];
}



/**
 * Rebuild the transparent selected-label overlay.
 *
 * @param state demo state
 */
static void _rebuild_selection_overlay(LabelsDemoState* state)
{
    ANN(state);
    ANN(state->selection_rgba);
    dvz_memset(
        state->selection_rgba, TEX_W * TEX_H * 4ull, 0, TEX_W * TEX_H * 4ull);

    if (state->selected_id == 0 || !state->selection_visible)
        return;

    int radius = state->outline_width > 0 ? state->outline_width : 1;
    radius += 2;
    for (uint32_t y = 0; y < TEX_H; y++)
    {
        for (uint32_t x = 0; x < TEX_W; x++)
        {
            bool selected = (DvzCategoryId)state->labels[y * TEX_W + x] == state->selected_id;
            int best_d2 = (radius + 1) * (radius + 1);
            int xi = (int)x;
            int yi = (int)y;
            for (int dy = -radius; dy <= radius; dy++)
            {
                int yy = yi + dy;
                if (yy < 0 || yy >= (int)TEX_H)
                    continue;
                for (int dx = -radius; dx <= radius; dx++)
                {
                    int xx = xi + dx;
                    if (xx < 0 || xx >= (int)TEX_W)
                        continue;
                    bool other_selected =
                        (DvzCategoryId)state->labels[(uint32_t)yy * TEX_W + (uint32_t)xx] ==
                        state->selected_id;
                    if (other_selected == selected)
                        continue;
                    int d2 = dx * dx + dy * dy;
                    if (d2 < best_d2)
                        best_d2 = d2;
                }
            }

            if (best_d2 > radius * radius)
                continue;

            float d = sqrtf((float)best_d2);
            float t = 1.0f - fminf(1.0f, d / (float)radius);
            float alpha = selected ? 42.0f + 170.0f * t : 18.0f + 210.0f * t;
            uint64_t p = 4ull * ((uint64_t)y * TEX_W + x);
            state->selection_rgba[p + 0] = 255;
            state->selection_rgba[p + 1] = 242;
            state->selection_rgba[p + 2] = 48;
            state->selection_rgba[p + 3] = (uint8_t)(alpha + 0.5f);
        }
    }
}



/**
 * Apply current selection state to the overlay visual and retained labels state.
 *
 * @param state demo state
 */
static void _apply_selection(LabelsDemoState* state)
{
    ANN(state);
    if (state->labels_visual != NULL)
    {
        if (state->selected_id == 0)
            (void)dvz_labels_clear_selected(state->labels_visual);
        else
            (void)dvz_labels_set_selected(state->labels_visual, state->selected_id);
    }

    if (state->selection_visual != NULL)
    {
        _rebuild_selection_overlay(state);
        int rc =
            dvz_visual_set_texture(state->selection_visual, state->selection_rgba, TEX_W, TEX_H);
        if (rc != 0)
            dvz_fprintf(stderr, "selection overlay texture update failed\n");
        dvz_visual_set_visible(
            state->selection_visual, state->selection_visible && state->selected_id != 0);
    }
    if (state->win != NULL)
        dvz_view_request_frame(state->win);
}



/**
 * Select a label ID and refresh GUI-visible state.
 *
 * @param state demo state
 * @param id label ID
 */
static void _select_label(LabelsDemoState* state, DvzCategoryId id)
{
    ANN(state);
    state->selected_id = id;
    state->selected_index = _selected_index_from_id(id);
    _apply_selection(state);
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
    int rc = dvz_visual_set_data(visual, "position", positions, 4);
    if (rc != 0)
        return NULL;
    rc = dvz_visual_set_data(visual, "texcoords", texcoords, 4);
    if (rc != 0)
        return NULL;
    rc = dvz_visual_set_texture(visual, pixels, TEX_W, TEX_H);
    if (rc != 0)
        return NULL;
    rc = dvz_visual_set_alpha_mode(visual, alpha_mode);
    if (rc != 0)
        return NULL;
    return visual;
}



/**
 * Assign common quad geometry to a labels visual.
 *
 * @param visual labels visual
 * @return whether the data was accepted
 */
static bool _set_quad_geometry(DvzVisual* visual)
{
    ANN(visual);
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
    int rc = dvz_visual_set_data(visual, "position", positions, 4);
    if (rc != 0)
        return false;
    rc = dvz_visual_set_data(visual, "texcoords", texcoords, 4);
    return rc == 0;
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Update hover state and select labels on click.
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
    if (event->type != DVZ_POINTER_EVENT_MOVE)
        return;

    state->hover_id = _label_at_pointer(state, event->pos[0], event->pos[1]);
}



/**
 * Select labels from derived pointer-click events.
 *
 * @param router input router emitting the input event
 * @param event input event payload
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

    state->hover_id = _label_at_pointer(state, pointer->pos[0], pointer->pos[1]);
    _select_label(state, state->hover_id);
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

    bool selection_changed = false;
    if (dvz_gui_begin(gui, "Labels", NULL, 0))
    {
        if (dvz_gui_checkbox(gui, "Labels overlay", &state->labels_visible))
        {
            if (state->labels_visual != NULL)
                dvz_visual_set_visible(state->labels_visual, state->labels_visible);
            if (state->win != NULL)
                dvz_view_request_frame(state->win);
        }

        int previous = state->selected_index;
        selection_changed |= dvz_gui_combo(
            gui, "Selected", &state->selected_index, SELECT_ITEMS, SELECT_ITEM_COUNT);
        if (state->selected_index != previous)
            state->selected_id = _selected_id_from_index(state->selected_index);

        selection_changed |=
            dvz_gui_checkbox(gui, "Selection highlight", &state->selection_visible);
        selection_changed |= dvz_gui_slider_int(gui, "Outline width", &state->outline_width, 1, 8);

        if (dvz_gui_button(gui, "Clear selection"))
        {
            state->selected_index = 0;
            state->selected_id = 0;
            selection_changed = true;
        }
        if (dvz_gui_button(gui, "Reset view"))
        {
            if (state->panzoom != NULL)
                dvz_panzoom_reset(state->panzoom);
            if (state->win != NULL)
                dvz_view_request_frame(state->win);
        }

        dvz_gui_separator_text(gui, "Readout");
        char line[160] = {0};
        dvz_snprintf(
            line, sizeof(line), "Hover: %" PRIi64 "  %s", state->hover_id,
            _category_name(state->hover_id));
        dvz_gui_text(gui, line);
        dvz_snprintf(
            line, sizeof(line), "Selected: %" PRIi64 "  %s", state->selected_id,
            _category_name(state->selected_id));
        dvz_gui_text(gui, line);
    }
    dvz_gui_end(gui);

    if (selection_changed)
        _apply_selection(state);
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
    uint8_t* selection_rgba = (uint8_t*)dvz_calloc(TEX_W * TEX_H * 4ull, 1);
    int32_t* labels = (int32_t*)dvz_calloc(TEX_W * TEX_H, sizeof(int32_t));
    EXAMPLE_CHECK(
        base_rgba != NULL && selection_rgba != NULL && labels != NULL,
        "texture allocation failed");
    _generate_dataset(labels, base_rgba);

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "scene panel setup failed");

    LabelsDemoState state = {
        .scene = scene,
        .labels = labels,
        .selection_rgba = selection_rgba,
        .outline_width = 3,
        .labels_visible = true,
        .selection_visible = true,
    };

    DvzVisual* base = _image_visual(scene, base_rgba, DVZ_ALPHA_OPAQUE);
    DvzVisual* selection = _image_visual(scene, selection_rgba, DVZ_ALPHA_BLENDED);
    EXAMPLE_CHECK(base != NULL && selection != NULL, "image visual setup failed");
    state.selection_visual = selection;
    dvz_visual_set_visible(selection, false);

    DvzScale* labels_scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   .kind = DVZ_SCALE_CATEGORICAL,
                   .label = "labels",
               });
    EXAMPLE_CHECK(labels_scale != NULL, "dvz_scale() failed");
    DvzScaleCategory categories[CATEGORY_COUNT] = {0};
    for (uint32_t i = 0; i < CATEGORY_COUNT; i++)
        _set_category(&categories[i], CATEGORY_IDS[i], i, CATEGORY_NAMES[i]);
    bool ok = dvz_scale_set_categories(labels_scale, categories, CATEGORY_COUNT);
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
    ok = _set_quad_geometry(labels_visual);
    EXAMPLE_CHECK(ok, "labels visual geometry setup failed");
    ok = dvz_visual_set_field(labels_visual, "field", label_field);
    EXAMPLE_CHECK(ok, "dvz_visual_set_field(labels) failed");
    int rc = dvz_visual_set_scale(labels_visual, "labels", labels_scale);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_scale(labels) failed");
    state.labels_visual = labels_visual;

    rc = dvz_panel_add_visual(panel, base, &(DvzVisualAttachDesc){.z_layer = 0});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(base) failed");
    rc = dvz_panel_add_visual(panel, labels_visual, &(DvzVisualAttachDesc){.z_layer = 1});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(labels) failed");
    rc = dvz_panel_add_visual(panel, selection, &(DvzVisualAttachDesc){.z_layer = 2});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(selection) failed");

    DvzLegend* legend = dvz_legend(
        panel, labels_scale,
        &(DvzLegendDesc){
            .title = "Labels",
            .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
            .reserve_px = 168.0f,
        });
    EXAMPLE_CHECK(legend != NULL, "dvz_legend() failed");
    dvz_panel_set_background_color(panel, 0.025f, 0.027f, 0.03f, 1.0f);

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "labels");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");
    state.win = win;

    DvzInputRouter* router = dvz_view_input(win);
    EXAMPLE_CHECK(router != NULL, "dvz_view_input() failed");
    DvzPanzoomDesc panzoom_desc = dvz_panzoom_desc();
    panzoom_desc.flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, &panzoom_desc);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    state.panzoom = panzoom;
    dvz_input_subscribe_pointer(router, _pointer_callback, &state);
    dvz_input_subscribe_event(router, _input_event_callback, &state);

    DvzGui* gui = dvz_view_gui(win, NULL);
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
    dvz_free(selection_rgba);
    dvz_free(labels);
    return status;
}
