/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* scheduler_lab - small Dear ImGui diagnostic for app on-demand scheduling.
 *
 * Build:  just example-c scheduler_lab
 * Run:    ./build/examples/c/techniques/scheduler_lab
 * Smoke:  ./build/examples/c/techniques/scheduler_lab 120
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "_app.h"
#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/common/functions.h"
#include "datoviz/gui.h"
#include "datoviz/imgui.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define LAB_WIDTH  980
#define LAB_HEIGHT 720
#define LAB_POINT_COUNT 5
#define LAB_IMAGE_SIZE  32
#define LAB_HOVER_QUERY_INTERVAL_NS 33000000ULL



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SchedulerLabState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzView* win;
    DvzInputRouter* router;
    DvzVisual* points;
    DvzVisual* image;
    DvzSampledField* image_field;

    float diameters[LAB_POINT_COUNT];
    float point_size;
    float image_tint;
    uint8_t image_rgba[LAB_IMAGE_SIZE * LAB_IMAGE_SIZE * 4];
    DvzColor point_colors[LAB_POINT_COUNT];

    bool show_points;
    bool show_image;
    bool continuous_repaint;
    bool mutate_after_frame;
    bool show_demo;

    uint32_t frame_count;
    uint32_t request_count;
    uint32_t mutation_count;
    uint32_t item_query_request_count;
    uint32_t pixel_query_request_count;
    uint32_t item_query_result_count;
    uint32_t pixel_query_result_count;

    uint64_t last_frame_ns;
    uint64_t fps_sample_ns;
    uint32_t fps_sample_frames;
    double fps;
    double frame_ms;
    double cursor_x;
    double cursor_y;
    float hover_point_rgba[4];
    float hover_image_rgba[4];

    char last_item_query[128];
    char last_pixel_query[160];

    uint64_t hover_query_ns;
    bool cursor_valid;
    bool hover_point_color_valid;
    bool hover_image_color_valid;
} SchedulerLabState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Update the point diameter_px attribute from the current GUI state.
 *
 * @param state example state
 */
static void _lab_update_points(SchedulerLabState* state)
{
    if (state == NULL || state->points == NULL)
        return;

    float size = state->show_points ? state->point_size : 0.0f;
    for (uint32_t i = 0; i < LAB_POINT_COUNT; i++)
        state->diameters[i] = size + 4.0f * (float)(i % 3);
    if (dvz_visual_set_data(state->points, "diameter_px", state->diameters, LAB_POINT_COUNT) == 0)
        state->mutation_count++;
}



/**
 * Upload a small RGBA image texture used by pixel query requests.
 *
 * @param state example state
 */
static void _lab_update_image(SchedulerLabState* state)
{
    if (state == NULL || state->image == NULL)
        return;

    for (uint32_t y = 0; y < LAB_IMAGE_SIZE; y++)
    {
        for (uint32_t x = 0; x < LAB_IMAGE_SIZE; x++)
        {
            uint32_t idx = 4 * (y * LAB_IMAGE_SIZE + x);
            float fx = (float)x / (float)(LAB_IMAGE_SIZE - 1);
            float fy = (float)y / (float)(LAB_IMAGE_SIZE - 1);
            state->image_rgba[idx + 0] = (uint8_t)(255.0f * fx);
            state->image_rgba[idx + 1] = (uint8_t)(255.0f * fy);
            state->image_rgba[idx + 2] = (uint8_t)(255.0f * state->image_tint);
            state->image_rgba[idx + 3] = state->show_image ? 255 : 0;
        }
    }
    if (
        example_visual_set_rgba8_field(
            state->scene, state->image, "field", state->image_rgba, LAB_IMAGE_SIZE,
            LAB_IMAGE_SIZE, &state->image_field))
    {
        state->mutation_count++;
    }
}



/**
 * Convert a cursor y coordinate to the image-query y coordinate used by the displayed texture.
 *
 * @param y window y coordinate
 * @return image query y coordinate
 */
static double _lab_image_query_y(double y)
{
    return (double)LAB_HEIGHT - y;
}



/**
 * Store the frontmost hovered point color for the GUI swatch.
 *
 * @param state example state
 * @param item_id point item id
 */
static void _lab_set_hover_point_color(SchedulerLabState* state, uint64_t item_id)
{
    if (state == NULL || item_id >= LAB_POINT_COUNT)
        return;

    state->hover_point_rgba[0] = (float)state->point_colors[item_id].r / 255.0f;
    state->hover_point_rgba[1] = (float)state->point_colors[item_id].g / 255.0f;
    state->hover_point_rgba[2] = (float)state->point_colors[item_id].b / 255.0f;
    state->hover_point_rgba[3] = (float)state->point_colors[item_id].a / 255.0f;
    state->hover_point_color_valid = true;
}



/**
 * Queue a point item query request at a window coordinate.
 *
 * @param state example state
 * @param x window x coordinate
 * @param y window y coordinate
 */
static void _lab_queue_item_query_at(SchedulerLabState* state, double x, double y)
{
    if (state == NULL || state->panel == NULL)
        return;
    state->item_query_request_count++;
    DvzQueryRequest request = {
        .request_id = state->item_query_request_count,
        .target = DVZ_SCENE_TARGET_ITEM,
        .hit_policy = DVZ_QUERY_HIT_FRONTMOST,
    };
    if (dvz_panel_query_px(state->panel, x, y, &request) != 0)
        dvz_fprintf(stderr, "dvz_panel_query_px(item) failed\n");
}



/**
 * Request a center item query on the point visual.
 *
 * @param state example state
 */
static void _lab_queue_item_query(SchedulerLabState* state)
{
    _lab_queue_item_query_at(state, LAB_WIDTH * 0.5, LAB_HEIGHT * 0.5);
}



/**
 * Queue an image pixel query request at a window coordinate.
 *
 * @param state example state
 * @param x window x coordinate
 * @param y window y coordinate
 */
static void _lab_queue_pixel_query_at(SchedulerLabState* state, double x, double y)
{
    if (state == NULL || state->panel == NULL)
        return;
    state->pixel_query_request_count++;
    DvzQueryRequest request = {
        .request_id = state->pixel_query_request_count,
        .target = DVZ_SCENE_TARGET_PIXEL,
    };
    if (dvz_panel_query_px(state->panel, x, y, &request) != 0)
        dvz_fprintf(stderr, "dvz_panel_query_px(pixel) failed\n");
}



/**
 * Request a center pixel query on the image visual.
 *
 * @param state example state
 */
static void _lab_queue_pixel_query(SchedulerLabState* state)
{
    _lab_queue_pixel_query_at(state, LAB_WIDTH * 0.5, LAB_HEIGHT * 0.5);
}



/**
 * Queue click item queries and throttled hover pixel queries from pointer input.
 *
 * @param router input router that emitted the event
 * @param event pointer event payload
 * @param user_data example state
 */
static void _lab_pointer(DvzInputRouter* router, const DvzPointerEvent* event, void* user_data)
{
    (void)router;
    SchedulerLabState* state = (SchedulerLabState*)user_data;
    if (state == NULL || event == NULL)
        return;

    if (event->type == DVZ_POINTER_EVENT_MOVE || event->type == DVZ_POINTER_EVENT_PRESS)
    {
        state->cursor_valid = true;
        state->cursor_x = event->pos[0];
        state->cursor_y = event->pos[1];
    }

    if (event->type == DVZ_POINTER_EVENT_MOVE)
    {
        uint64_t now = dvz_time_monotonic_ns();
        if (
            state->hover_query_ns == 0 ||
            now - state->hover_query_ns >= LAB_HOVER_QUERY_INTERVAL_NS)
        {
            state->hover_query_ns = now;
            _lab_queue_item_query_at(state, event->pos[0], event->pos[1]);
            _lab_queue_pixel_query_at(state, event->pos[0], _lab_image_query_y(event->pos[1]));
        }
    }
    else if (event->type == DVZ_POINTER_EVENT_PRESS && event->button == DVZ_POINTER_BUTTON_LEFT)
    {
        _lab_queue_item_query_at(state, event->pos[0], event->pos[1]);
    }
}



/**
 * Record that the app requested another frame.
 *
 * @param win view requesting a frame
 * @param user_data example state
 */
static void _lab_request_frame(DvzView* win, void* user_data)
{
    (void)win;
    SchedulerLabState* state = (SchedulerLabState*)user_data;
    if (state != NULL)
        state->request_count++;
}



/**
 * Update FPS counters and consume query results after each frame.
 *
 * @param win view whose frame just completed
 * @param user_data example state
 */
static void _lab_frame(DvzView* win, void* user_data)
{
    SchedulerLabState* state = (SchedulerLabState*)user_data;
    if (state == NULL)
        return;

    uint64_t now = dvz_time_monotonic_ns();
    if (state->last_frame_ns != 0 && now > state->last_frame_ns)
        state->frame_ms = (double)(now - state->last_frame_ns) / 1000000.0;
    state->last_frame_ns = now;
    state->frame_count++;

    if (state->fps_sample_ns == 0)
        state->fps_sample_ns = now;
    state->fps_sample_frames++;
    uint64_t elapsed = now - state->fps_sample_ns;
    if (elapsed >= 500000000)
    {
        state->fps = 1000000000.0 * (double)state->fps_sample_frames / (double)elapsed;
        state->fps_sample_ns = now;
        state->fps_sample_frames = 0;
    }

    DvzQueryResult query = {0};
    while (dvz_scene_poll_query(state->scene, &query))
    {
        if (query.resolved_target == DVZ_SCENE_TARGET_PIXEL)
        {
            state->pixel_query_result_count++;
            if (
                query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
                query.value_kind == DVZ_QUERY_VALUE_VEC4)
            {
                state->hover_image_color_valid = true;
                for (uint32_t i = 0; i < 4; i++)
                    state->hover_image_rgba[i] = (float)query.vector[i];
                (void)snprintf(
                    state->last_pixel_query, sizeof(state->last_pixel_query),
                    "pixel query #%u: rgba=(%.2f, %.2f, %.2f, %.2f)",
                    state->pixel_query_result_count, query.vector[0], query.vector[1],
                    query.vector[2], query.vector[3]);
            }
            else
            {
                state->hover_image_color_valid = false;
                (void)snprintf(
                    state->last_pixel_query, sizeof(state->last_pixel_query),
                    "pixel query #%u: miss", state->pixel_query_result_count);
            }
            continue;
        }

        state->item_query_result_count++;
        if (
            query.status == DVZ_QUERY_STATUS_HIT && query.hit &&
            query.resolved_target == DVZ_SCENE_TARGET_ITEM && query.resolved_id < LAB_POINT_COUNT)
        {
            _lab_set_hover_point_color(state, query.resolved_id);
        }
        else
        {
            state->hover_point_color_valid = false;
        }
        if (query.status == DVZ_QUERY_STATUS_HIT && query.hit && query.has_data_position)
        {
            (void)snprintf(
                state->last_item_query, sizeof(state->last_item_query),
                "item query #%u: item=%" PRIu64 " data=(%.2f, %.2f)",
                state->item_query_result_count, query.resolved_id, query.data_position[0],
                query.data_position[1]);
        }
        else
        {
            (void)snprintf(
                state->last_item_query, sizeof(state->last_item_query),
                "item query #%u: %s item=%" PRIu64, state->item_query_result_count,
                query.hit ? "hit" : "miss", query.resolved_id);
        }
    }

    if (state->mutate_after_frame)
    {
        state->point_size += 1.0f;
        if (state->point_size > 54.0f)
            state->point_size = 18.0f;
        _lab_update_points(state);
    }
    if (state->continuous_repaint)
        dvz_view_request_frame(win);
}



/**
 * Render the Dear ImGui controls and scheduling status panel.
 *
 * @param gui GUI overlay
 * @param win view
 * @param user_data example state
 */
static void _lab_gui(DvzGui* gui, DvzView* win, void* user_data)
{
    SchedulerLabState* state = (SchedulerLabState*)user_data;
    if (state == NULL)
        return;

    char line[192] = {0};
    bool point_changed = false;
    bool image_changed = false;
    bool request_next = false;

    if (dvz_gui_begin(gui, "Scheduler", NULL, 0))
    {
        dvz_gui_separator_text(gui, "Status");
        bool wants_frame = _dvz_view_scheduler_should_render(
            win, state->continuous_repaint, dvz_time_monotonic_ns());
        (void)snprintf(line, sizeof(line), "state: drawing");
        dvz_gui_text(gui, line);
        (void)snprintf(
            line, sizeof(line), "next frame due: %s", wants_frame ? "yes" : "no");
        dvz_gui_text(gui, line);
        (void)snprintf(line, sizeof(line), "frames: %u", state->frame_count);
        dvz_gui_text(gui, line);
        (void)snprintf(line, sizeof(line), "requests: %u", state->request_count);
        dvz_gui_text(gui, line);
        (void)snprintf(
            line, sizeof(line), "fps: %.1f  frame: %.2f ms", state->fps, state->frame_ms);
        dvz_gui_text(gui, line);
        (void)snprintf(line, sizeof(line), "mutations: %u", state->mutation_count);
        dvz_gui_text(gui, line);
        dvz_gui_text(gui, state->last_item_query);
        dvz_gui_text(gui, state->last_pixel_query);
        if (state->cursor_valid)
        {
            (void)snprintf(
                line, sizeof(line), "cursor: %.1f, %.1f", state->cursor_x, state->cursor_y);
            dvz_gui_text(gui, line);
        }
        bool has_hover_color = state->hover_point_color_valid || state->hover_image_color_valid;
        float* hover_rgba =
            state->hover_point_color_valid ? state->hover_point_rgba : state->hover_image_rgba;
        ImVec4 hover_color = has_hover_color
                                 ? (ImVec4){
                                       hover_rgba[0],
                                       hover_rgba[1],
                                       hover_rgba[2],
                                       hover_rgba[3],
                                   }
                                 : (ImVec4){0.12f, 0.12f, 0.12f, 1.0f};
        (void)igColorButton(
            "hover color", hover_color,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
            (ImVec2){34.0f, 34.0f});
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        dvz_gui_text(
            gui, state->hover_point_color_valid
                     ? "hover point color"
                     : state->hover_image_color_valid ? "hover image color" : "hover: no sample");

        dvz_gui_separator_text(gui, "Controls");
        if (dvz_gui_button(gui, "Request frame"))
            dvz_view_request_frame(win);
        if (dvz_gui_button(gui, "Mutate points"))
        {
            state->point_size += 6.0f;
            if (state->point_size > 64.0f)
                state->point_size = 18.0f;
            _lab_update_points(state);
            request_next = true;
        }
        if (dvz_gui_button(gui, "Mutate image"))
        {
            state->image_tint += 0.17f;
            if (state->image_tint > 1.0f)
                state->image_tint = 0.05f;
            _lab_update_image(state);
            request_next = true;
        }
        if (dvz_gui_button(gui, "Queue item query"))
            _lab_queue_item_query(state);
        if (dvz_gui_button(gui, "Queue pixel query"))
            _lab_queue_pixel_query(state);

        point_changed |=
            dvz_gui_slider_float(gui, "Point size", &state->point_size, 4.0f, 72.0f);
        point_changed |= dvz_gui_checkbox(gui, "Show points", &state->show_points);
        image_changed |= dvz_gui_checkbox(gui, "Show image", &state->show_image);
        if (dvz_gui_checkbox(gui, "Continuous repaint", &state->continuous_repaint))
            request_next = state->continuous_repaint;
        (void)dvz_gui_checkbox(gui, "Mutate after frame", &state->mutate_after_frame);
        (void)dvz_gui_checkbox(gui, "ImGui demo", &state->show_demo);
    }
    dvz_gui_end(gui);

    if (state->show_demo)
        dvz_gui_demo(gui, &state->show_demo);

    if (point_changed)
    {
        _lab_update_points(state);
        request_next = true;
    }
    if (image_changed)
    {
        _lab_update_image(state);
        request_next = true;
    }
    if (request_next)
        dvz_view_request_frame(win);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, LAB_WIDTH, LAB_HEIGHT, 0);
    DvzPanel* panel = figure != NULL ? dvz_panel_full(figure) : NULL;
    EXAMPLE_CHECK(figure != NULL && panel != NULL, "figure or panel creation failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.045f, 0.052f, 0.060f, 1.0f));

    DvzVisual* image = dvz_image(scene, 0);
    DvzVisual* points = dvz_point(scene, 0);
    EXAMPLE_CHECK(image != NULL && points != NULL, "visual creation failed");

    vec3 image_pos[4] = {
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
    int rc = dvz_visual_set_data(image, "position", image_pos, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(image position) failed");

    rc = dvz_visual_set_data(image, "texcoords", texcoords, 4);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(image texcoords) failed");

    dvz_visual_set_query_capabilities(image, DVZ_QUERY_CAPABILITY_PIXEL);
    rc = dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1});
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(image) failed");

    vec3 point_pos[LAB_POINT_COUNT] = {
        {0.00f, 0.00f, 0.0f},
        {-0.45f, -0.30f, 0.0f},
        {+0.45f, -0.25f, 0.0f},
        {-0.25f, +0.38f, 0.0f},
        {+0.32f, +0.34f, 0.0f},
    };
    DvzColor point_color[LAB_POINT_COUNT] = {
        {255, 245, 180, 255},
        {255, 110,  95, 255},
        { 95, 210, 150, 255},
        { 95, 150, 245, 255},
        {220, 120, 245, 255},
    };
    dvz_visual_set_query_capabilities(points, DVZ_QUERY_CAPABILITY_ITEM);
    rc = dvz_visual_set_data(points, "position", point_pos, LAB_POINT_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(point position) failed");

    rc = dvz_visual_set_data(points, "color", point_color, LAB_POINT_COUNT);
    EXAMPLE_CHECK(rc == 0, "dvz_visual_set_data(point color) failed");

    rc = dvz_panel_add_visual(panel, points, NULL);
    EXAMPLE_CHECK(rc == 0, "dvz_panel_add_visual(points) failed");

    SchedulerLabState state = {
        .scene = scene,
        .panel = panel,
        .points = points,
        .image = image,
        .point_size = 34.0f,
        .image_tint = 0.35f,
        .show_points = true,
        .show_image = true,
    };
    (void)snprintf(state.last_item_query, sizeof(state.last_item_query), "item query: none");
    (void)snprintf(state.last_pixel_query, sizeof(state.last_pixel_query), "pixel query: none");
    for (uint32_t i = 0; i < LAB_POINT_COUNT; i++)
        state.point_colors[i] = point_color[i];
    _lab_update_points(&state);
    _lab_update_image(&state);
    state.mutation_count = 0;

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_window(app, figure, LAB_WIDTH, LAB_HEIGHT, "scheduler_lab");
    EXAMPLE_CHECK(win != NULL, "dvz_view_window() failed (GLFW unavailable?)");
    state.win = win;

    state.router = dvz_view_input(win);
    EXAMPLE_CHECK(state.router != NULL, "dvz_view_input() failed");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");
    dvz_input_subscribe_pointer(state.router, _lab_pointer, &state);

    DvzGui* gui = dvz_view_gui(win, NULL);
    EXAMPLE_CHECK(gui != NULL, "dvz_view_gui() failed");

    dvz_view_set_request_frame_callback(win, _lab_request_frame, &state);
    dvz_view_set_frame_callback(win, _lab_frame, &state);
    dvz_view_set_gui_callback(win, _lab_gui, &state);
    dvz_view_request_frame(win);

    dvz_app_run(app, example_frame_count(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
