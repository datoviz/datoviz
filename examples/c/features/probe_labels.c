/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* probe_labels - categorical labels query proof with deterministic label-id readout.
 *
 * Scenario: probe_labels
 * Style: features, graphite_cyan, 1600x1200 capture target
 *
 * Build:  just example-c features/probe_labels
 * Run:    ./build/examples/c/features/probe_labels --live
 * Smoke:  ./build/examples/c/features/probe_labels --png
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_compat.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_style.h"
#include "runner/scenario_runner.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH            1600u
#define HEIGHT           1200u
#define FIELD_WIDTH      256u
#define FIELD_HEIGHT     192u
#define LABEL_COUNT      5u
#define PROBE_X          0.68f
#define PROBE_Y          0.56f
#define PROBE_REQUEST_ID 1u

static const float TAU = 6.28318530718f;

static const DvzCategoryId LABEL_IDS[LABEL_COUNT] = {7, 11, 17, 23, 31};
static const char* LABEL_NAMES[LABEL_COUNT] = {
    "cortex", "fiber", "nucleus", "vessel", "island"};



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct LabelProbeState LabelProbeState;

typedef struct LabelProbeMarker LabelProbeMarker;

typedef struct LabelProbePointer LabelProbePointer;

struct LabelProbeMarker
{
    DvzVisual* ring;
    DvzVisual* dot;
};

struct LabelProbePointer
{
    bool valid;
    double panel_px[2];
    double data[2];
};

struct LabelProbeState
{
    DvzScene* scene;
    DvzPanel* panel;
    LabelProbeMarker marker;
    int32_t* labels;
    bool cursor_valid;
    double cursor_x;
    double cursor_y;
    bool has_last_result;
    bool last_hit;
    DvzCategoryId last_category_id;
};



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a generated categorical label for one normalized sample coordinate.
 *
 * @param x normalized X coordinate
 * @param y normalized Y coordinate
 * @return category ID, or 0 for transparent background
 */
static DvzCategoryId _sample_label(float x, float y)
{
    const float dx = x - 0.50f;
    const float dy = y - 0.50f;
    const float tissue = dx * dx / 0.43f + dy * dy / 0.31f;
    if (tissue > 1.0f)
        return 0;

    const float island_dx = x - 0.68f;
    const float island_dy = y - 0.56f;
    if (island_dx * island_dx + 1.6f * island_dy * island_dy < 0.014f)
        return LABEL_IDS[4];

    const float wave = 0.5f + 0.5f * sinf(TAU * (1.7f * x + 0.9f * y));
    const uint32_t band = (uint32_t)floorf(4.0f * x + 1.4f * y + 0.45f * wave);
    return LABEL_IDS[band % 4u];
}



/**
 * Fill the deterministic label field.
 *
 * @param labels output signed label field
 */
static void _fill_labels(int32_t labels[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(labels);

    for (uint32_t y = 0; y < FIELD_HEIGHT; y++)
    {
        for (uint32_t x = 0; x < FIELD_WIDTH; x++)
        {
            const float u = FIELD_WIDTH > 1u ? (float)x / (float)(FIELD_WIDTH - 1u) : 0.0f;
            const float v = FIELD_HEIGHT > 1u ? (float)y / (float)(FIELD_HEIGHT - 1u) : 0.0f;
            labels[y * FIELD_WIDTH + x] = (int32_t)_sample_label(u, v);
        }
    }
}



/**
 * Set a normalized data domain on the probe panel.
 *
 * @param panel target panel
 * @return true when both domain calls succeed
 */
static bool _set_probe_domain(DvzPanel* panel)
{
    ANN(panel);
    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 1.0);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 1.0);
    return rc == 0;
}



/**
 * Create the categorical labels scale.
 *
 * @param scene scene owning the scale
 * @return created scale, or NULL on failure
 */
static DvzScale* _add_labels_scale(DvzScene* scene)
{
    ANN(scene);

    DvzScale* scale = dvz_scale(
        scene, &(DvzScaleDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                   .kind = DVZ_SCALE_CATEGORICAL,
                   .label = "labels",
               });
    if (scale == NULL)
        return NULL;

    const ExampleStyleColorRole roles[LABEL_COUNT] = {
        EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY,
        EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY,
        EXAMPLE_STYLE_COLOR_WARNING,
        EXAMPLE_STYLE_COLOR_ERROR,
        EXAMPLE_STYLE_COLOR_TEXT,
    };
    DvzScaleCategory categories[LABEL_COUNT] = {0};
    for (uint32_t i = 0; i < LABEL_COUNT; i++)
    {
        categories[i].category_id = LABEL_IDS[i];
        categories[i].order = i;
        categories[i].label = LABEL_NAMES[i];
        categories[i].color = example_graphite_cyan_color(roles[i]);
        categories[i].color.a = 224u;
    }
    return dvz_scale_set_categories(scale, categories, LABEL_COUNT) ? scale : NULL;
}



/**
 * Add the retained labels visual and enable label-ID queries.
 *
 * @param scene scene owning the visual and field
 * @param panel panel receiving the visual
 * @param scale categorical label scale
 * @param labels signed label field
 * @return true when the labels visual was added
 */
static bool _add_labels(
    DvzScene* scene, DvzPanel* panel, DvzScale* scale, int32_t labels[FIELD_WIDTH * FIELD_HEIGHT])
{
    ANN(scene);
    ANN(panel);
    ANN(scale);
    ANN(labels);

    vec3 data_positions[4] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    vec2 texcoords[4] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };

    DvzSampledField* field = dvz_sampled_field(
        scene, &(DvzSampledFieldDesc){
                   DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                   .dim = DVZ_FIELD_DIM_2D,
                   .format = DVZ_FIELD_FORMAT_R32_SINT,
                   .semantic = DVZ_FIELD_SEMANTIC_LABEL,
                   .width = FIELD_WIDTH,
                   .height = FIELD_HEIGHT,
                   .depth = 1,
               });
    if (field == NULL)
        return false;
    if (!dvz_sampled_field_set_data(
            field, &(DvzFieldDataView){
                       DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                       .data = labels,
                       .bytes_per_row = FIELD_WIDTH * sizeof(int32_t),
                       .rows_per_image = FIELD_HEIGHT,
                   }))
    {
        return false;
    }

    DvzVisual* visual = dvz_labels(scene, 0);
    if (visual == NULL)
        return false;
    DvzVisualDataUpdate updates[] = {
        {.attr_name = "position", .data = data_positions, .item_count = 4},
        {.attr_name = "texcoords", .data = texcoords, .item_count = 4},
    };
    if (dvz_visual_set_data_many(visual, updates, 2) != 0)
        return false;
    if (!dvz_visual_set_field(visual, "field", field))
        return false;
    if (dvz_visual_set_scale(visual, "labels", scale) != 0)
        return false;
    if (dvz_labels_set_opacity(visual, 0.92f) != 0)
        return false;
    if (dvz_labels_set_background(visual, 0) != 0)
        return false;
    if (dvz_labels_set_boundary(
            visual, true, 1.25f, example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_PANEL_BG)) != 0)
    {
        return false;
    }
    if (dvz_visual_set_depth_test(visual, false) != 0)
        return false;
    if (dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED) != 0)
        return false;
    dvz_visual_set_query_capabilities(visual, DVZ_QUERY_CAPABILITY_ITEM);
    return dvz_panel_add_visual(panel, visual, NULL) == 0;
}



/**
 * Update the visible probe marker to a normalized data position.
 *
 * @param marker visible probe marker
 * @param x normalized probe X coordinate
 * @param y normalized probe Y coordinate
 */
static void _update_probe_marker(LabelProbeMarker* marker, float x, float y)
{
    if (marker == NULL || marker->ring == NULL || marker->dot == NULL)
        return;

    vec3 ring_data[1] = {{x, y, 0.04f}};
    vec3 dot_data[1] = {{x, y, 0.05f}};

    (void)dvz_visual_set_data(marker->ring, "position", ring_data, 1);
    (void)dvz_visual_set_data(marker->dot, "position", dot_data, 1);
}



/**
 * Convert one figure-coordinate pointer position to panel pixels and normalized data coordinates.
 *
 * @param panel target panel
 * @param figure_x pointer X in figure pixels
 * @param figure_y pointer Y in figure pixels
 * @param out converted pointer state
 * @return true when the pointer is inside the probe panel
 */
static bool _probe_pointer_from_figure(
    DvzPanel* panel, double figure_x, double figure_y, LabelProbePointer* out)
{
    ANN(panel);
    ANN(out);
    *out = (LabelProbePointer){0};

    out->valid =
        dvz_panel_transform_point(
            panel, DVZ_PANEL_COORD_FIGURE_PX, DVZ_PANEL_COORD_PANEL_PX,
            (const double[2]){figure_x, figure_y}, out->panel_px) &&
        dvz_panel_position_to_data(panel, DVZ_PANEL_COORD_PANEL_PX, out->panel_px, out->data);
    return out->valid;
}



/**
 * Add the visible fixed probe marker.
 *
 * @param scene scene owning the visual
 * @param panel panel receiving the visual
 * @param out_marker output marker visuals
 * @return true when the marker was added
 */
static bool _add_probe_marker(DvzScene* scene, DvzPanel* panel, LabelProbeMarker* out_marker)
{
    ANN(scene);
    ANN(panel);
    ANN(out_marker);

    vec3 ring_data[1] = {{PROBE_X, PROBE_Y, 0.04f}};

    DvzVisual* ring = dvz_marker(scene, 0);
    if (ring == NULL)
        return false;
    DvzColor ring_color[1] = {example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT)};
    ring_color[0].a = 245u;
    float ring_diameter[1] = {24.0f};
    float ring_angle[1] = {0.0f};
    uint32_t ring_shape[1] = {DVZ_MARKER_SHAPE_RING};
    DvzVisualDataUpdate ring_updates[] = {
        {.attr_name = "position", .data = ring_data, .item_count = 1},
        {.attr_name = "color", .data = ring_color, .item_count = 1},
        {.attr_name = "diameter_px", .data = ring_diameter, .item_count = 1},
        {.attr_name = "angle", .data = ring_angle, .item_count = 1},
        {.attr_name = "shape", .data = ring_shape, .item_count = 1},
    };
    if (dvz_visual_set_data_many(ring, ring_updates, 5) != 0)
        return false;
    if (dvz_visual_set_depth_test(ring, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, ring, NULL) != 0)
        return false;

    vec3 dot_data[1] = {{PROBE_X, PROBE_Y, 0.05f}};

    DvzVisual* dot = dvz_point(scene, 0);
    if (dot == NULL)
        return false;
    DvzColor dot_color[1] = {example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY)};
    dot_color[0].a = 245u;
    float dot_diameter[1] = {6.0f};
    DvzVisualDataUpdate dot_updates[] = {
        {.attr_name = "position", .data = dot_data, .item_count = 1},
        {.attr_name = "color", .data = dot_color, .item_count = 1},
        {.attr_name = "diameter_px", .data = dot_diameter, .item_count = 1},
    };
    if (dvz_visual_set_data_many(dot, dot_updates, 3) != 0)
        return false;
    DvzPointStyleDesc point_style = dvz_point_style_desc();
    point_style.aspect = DVZ_SHAPE_ASPECT_FILLED;
    point_style.stroke_width_px = 0.0f;
    if (dvz_point_set_style(dot, &point_style) != 0)
        return false;
    if (dvz_visual_set_depth_test(dot, false) != 0)
        return false;
    if (dvz_panel_add_visual(panel, dot, NULL) != 0)
        return false;

    out_marker->ring = ring;
    out_marker->dot = dot;
    return true;
}



/**
 * Return whether a query result differs from the last printed category.
 *
 * @param state label query example state
 * @param query query result to compare
 * @return true when the result should be printed
 */
static bool _query_changed(LabelProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return false;
    if (!state->has_last_result || state->last_hit != query->hit)
        return true;
    if (!query->hit)
        return false;
    return state->last_category_id != query->category_id;
}



/**
 * Remember the last printed query result.
 *
 * @param state label query example state
 * @param query query result to store
 */
static void _store_query_result(LabelProbeState* state, const DvzQueryResult* query)
{
    if (state == NULL || query == NULL)
        return;
    state->has_last_result = true;
    state->last_hit = query->hit;
    if (query->hit)
        state->last_category_id = query->category_id;
}



/**
 * Queue one label-ID query at the latest probe position.
 *
 * @param state label query example state
 */
static void _queue_probe(LabelProbeState* state)
{
    if (state == NULL || !state->cursor_valid)
        return;

    DvzQueryRequest request = dvz_query_request();
    request.request_id = PROBE_REQUEST_ID;
    request.target = DVZ_SCENE_TARGET_SEGMENT;

    const int rc = dvz_panel_query(state->panel, state->cursor_x, state->cursor_y, &request);
    if (rc != 0)
        dvz_fprintf(stderr, "dvz_panel_query() failed\n");
}



/*************************************************************************************************/
/*  Callbacks                                                                                    */
/*************************************************************************************************/

/**
 * Record the latest cursor position and move the live probe marker.
 *
 * @param router input router emitting the event
 * @param event pointer event payload
 * @param user_data label probe example state
 */
static void
_labels_probe_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    LabelProbeState* state = (LabelProbeState*)user_data;
    if (state == NULL || event == NULL)
        return;
    if (event->type != DVZ_POINTER_EVENT_MOVE && event->type != DVZ_POINTER_EVENT_CLICK)
        return;

    LabelProbePointer pointer = {0};
    if (_probe_pointer_from_figure(state->panel, event->pos[0], event->pos[1], &pointer))
    {
        state->cursor_valid = true;
        state->cursor_x = pointer.panel_px[0];
        state->cursor_y = pointer.panel_px[1];
        _update_probe_marker(&state->marker, (float)pointer.data[0], (float)pointer.data[1]);
    }
    else
    {
        state->cursor_valid = false;
    }
}



/**
 * Poll label query results and queue the next probe.
 *
 * @param win view whose frame just completed
 * @param user_data label probe example state
 */
static void _labels_probe_frame(DvzView* win, void* user_data)
{
    (void)win;
    LabelProbeState* state = (LabelProbeState*)user_data;
    if (state == NULL)
        return;

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (!_query_changed(state, &query))
            continue;

        if (query.hit && query.value_kind == DVZ_QUERY_VALUE_CATEGORY)
        {
            dvz_fprintf(
                stdout, "probe label_id=%" PRId64 " label=\"%s\" uv=(%0.3f,%0.3f)\n",
                query.category_id, query.label, query.uvw[0], query.uvw[1]);
        }
        else
        {
            dvz_fprintf(
                stdout, "probe miss panel=(%0.1f,%0.1f)\n", query.panel_position[0],
                query.panel_position[1]);
        }
        _store_query_result(state, &query);
    }

    _queue_probe(state);
}



/*************************************************************************************************/
/*  Scenario callbacks                                                                           */
/*************************************************************************************************/

/**
 * Initialize the categorical labels probe feature scenario.
 *
 * @param ctx scenario context
 * @param out_user scenario state output
 * @return true on success
 */
static bool _scenario_init(DvzScenarioContext* ctx, void** out_user)
{
    if (ctx == NULL)
        return false;
    if (out_user != NULL)
        *out_user = NULL;

    LabelProbeState* state = (LabelProbeState*)dvz_calloc(1, sizeof(*state));
    if (state == NULL)
        return false;
    if (out_user != NULL)
        *out_user = state;

    state->labels = (int32_t*)dvz_calloc(FIELD_WIDTH * FIELD_HEIGHT, sizeof(*state->labels));
    if (state->labels == NULL)
        return false;
    _fill_labels(state->labels);

    ctx->figure = dvz_figure(ctx->scene, ctx->width, ctx->height, 0);
    if (ctx->figure == NULL)
        return false;

    DvzPanel* panel = dvz_panel_full(ctx->figure);
    if (panel == NULL)
        return false;

    bool ok = _set_probe_domain(panel);
    if (!ok)
        return false;

    example_graphite_cyan_set_panel_background(panel);

    DvzScale* scale = _add_labels_scale(ctx->scene);
    if (scale == NULL)
        return false;

    ok = _add_labels(ctx->scene, panel, scale, state->labels);
    if (!ok)
        return false;

    ok = _add_probe_marker(ctx->scene, panel, &state->marker);
    if (!ok)
        return false;

    double initial_probe_px[2] = {0};
    if (!dvz_panel_data_to_position(
            panel, DVZ_PANEL_COORD_PANEL_PX, (const double[2]){PROBE_X, PROBE_Y},
            initial_probe_px))
    {
        return false;
    }

    state->scene = ctx->scene;
    state->panel = panel;
    state->cursor_valid = true;
    state->cursor_x = initial_probe_px[0];
    state->cursor_y = initial_probe_px[1];
    return true;
}



/**
 * Attach native GLFW callbacks for the categorical labels probe feature scenario.
 *
 * @param ctx scenario context
 * @param app native app
 * @param view native view
 * @param user scenario state
 * @return true on success
 */
static bool _scenario_native_view(
    DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user)
{
    (void)ctx;
    (void)app;
    LabelProbeState* state = (LabelProbeState*)user;
    if (state == NULL || view == NULL)
        return false;

    DvzInputRouter* router = dvz_view_input(view);
    if (router == NULL)
        return true;

    dvz_input_subscribe_pointer(router, _labels_probe_pointer, state);
    dvz_view_set_frame_callback(view, _labels_probe_frame, state);
    return true;
}



/**
 * Destroy the categorical labels probe feature scenario state.
 *
 * @param ctx scenario context
 * @param user scenario state
 */
static void _scenario_destroy(DvzScenarioContext* ctx, void* user)
{
    (void)ctx;
    LabelProbeState* state = (LabelProbeState*)user;
    if (state == NULL)
        return;
    dvz_free(state->labels);
    dvz_free(state);
}



/**
 * Return the categorical labels probe scenario specification.
 *
 * @return scenario specification
 */
static DvzScenarioSpec _probe_labels_scenario(void)
{
    return (DvzScenarioSpec){
        .id = "feature_probe_labels",
        .title = "probe_labels",
        .width = WIDTH,
        .height = HEIGHT,
        .fps = 60.0,
        .init = _scenario_init,
        .native_view = _scenario_native_view,
        .destroy = _scenario_destroy,
    };
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run the categorical labels probe feature example through the native scenario runner.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return process exit code
 */
int main(int argc, char** argv)
{
    DvzScenarioSpec spec = _probe_labels_scenario();
    return dvz_scenario_run_native_cli(&spec, argc, argv) == 0 ? 0 : 1;
}
