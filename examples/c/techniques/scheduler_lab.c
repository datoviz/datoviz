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
#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define LAB_WIDTH  980
#define LAB_HEIGHT 720
#define LAB_POINT_COUNT 5
#define LAB_IMAGE_SIZE  32



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct SchedulerLabState
{
    DvzScene* scene;
    DvzPanel* panel;
    DvzAppWindow* win;
    DvzVisual* points;
    DvzVisual* image;

    float diameters[LAB_POINT_COUNT];
    float point_size;
    float image_tint;
    uint8_t image_rgba[LAB_IMAGE_SIZE * LAB_IMAGE_SIZE * 4];

    bool show_points;
    bool show_image;
    bool continuous_repaint;
    bool mutate_after_frame;
    bool show_demo;

    uint32_t frame_count;
    uint32_t request_count;
    uint32_t mutation_count;
    uint32_t pick_request_count;
    uint32_t probe_request_count;
    uint32_t pick_result_count;
    uint32_t probe_result_count;

    uint64_t last_frame_ns;
    uint64_t fps_sample_ns;
    uint32_t fps_sample_frames;
    double fps;
    double frame_ms;

    char last_pick[128];
    char last_probe[160];
} SchedulerLabState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Parse an optional bounded frame count from the command line.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
static uint32_t _frame_count(int argc, char** argv)
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
 * Update the point diameter attribute from the current GUI state.
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
    if (dvz_visual_set_data(state->points, "diameter", state->diameters, LAB_POINT_COUNT) == 0)
        state->mutation_count++;
}



/**
 * Upload a small RGBA image texture used by probe requests.
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
        dvz_visual_set_texture(
            state->image, state->image_rgba, LAB_IMAGE_SIZE, LAB_IMAGE_SIZE) == 0)
        state->mutation_count++;
}



/**
 * Request a center pick on the point visual.
 *
 * @param state example state
 */
static void _lab_queue_pick(SchedulerLabState* state)
{
    if (state == NULL || state->panel == NULL)
        return;
    state->pick_request_count++;
    DvzPickRequest request = {.request_id = state->pick_request_count};
    if (dvz_panel_pick(state->panel, LAB_WIDTH * 0.5, LAB_HEIGHT * 0.5, &request) != 0)
        dvz_fprintf(stderr, "dvz_panel_pick() failed\n");
}



/**
 * Request a center probe on the image visual.
 *
 * @param state example state
 */
static void _lab_queue_probe(SchedulerLabState* state)
{
    if (state == NULL || state->panel == NULL)
        return;
    state->probe_request_count++;
    DvzProbeRequest request = {.request_id = state->probe_request_count};
    if (dvz_panel_probe(state->panel, LAB_WIDTH * 0.5, LAB_HEIGHT * 0.5, &request) != 0)
        dvz_fprintf(stderr, "dvz_panel_probe() failed\n");
}



/**
 * Record that the app requested another frame.
 *
 * @param win app-window requesting a frame
 * @param user_data example state
 */
static void _lab_request_frame(DvzAppWindow* win, void* user_data)
{
    (void)win;
    SchedulerLabState* state = (SchedulerLabState*)user_data;
    if (state != NULL)
        state->request_count++;
}



/**
 * Update FPS counters and consume pick/probe results after each frame.
 *
 * @param win app-window whose frame just completed
 * @param user_data example state
 */
static void _lab_frame(DvzAppWindow* win, void* user_data)
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

    DvzPickResult pick = {0};
    while (dvz_scene_poll_pick(state->scene, &pick))
    {
        state->pick_result_count++;
        (void)snprintf(
            state->last_pick, sizeof(state->last_pick), "pick #%u: %s item=%" PRIu64,
            state->pick_result_count, pick.hit ? "hit" : "miss", pick.resolved_id);
    }

    DvzProbeResult probe = {0};
    while (dvz_scene_poll_probe(state->scene, &probe))
    {
        state->probe_result_count++;
        if (probe.hit && probe.value_kind == DVZ_PROBE_VALUE_VEC4)
        {
            (void)snprintf(
                state->last_probe, sizeof(state->last_probe),
                "probe #%u: rgba=(%.2f, %.2f, %.2f, %.2f)", state->probe_result_count,
                probe.vector[0], probe.vector[1], probe.vector[2], probe.vector[3]);
        }
        else
        {
            (void)snprintf(
                state->last_probe, sizeof(state->last_probe), "probe #%u: miss",
                state->probe_result_count);
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
        dvz_app_window_request_frame(win);
}



/**
 * Render the Dear ImGui controls and scheduling status panel.
 *
 * @param gui GUI overlay
 * @param win app-window
 * @param user_data example state
 */
static void _lab_gui(DvzGui* gui, DvzAppWindow* win, void* user_data)
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
        bool wants_frame = _dvz_app_window_scheduler_should_render(
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
        dvz_gui_text(gui, state->last_pick);
        dvz_gui_text(gui, state->last_probe);

        dvz_gui_separator_text(gui, "Controls");
        if (dvz_gui_button(gui, "Request frame"))
            dvz_app_window_request_frame(win);
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
        if (dvz_gui_button(gui, "Queue pick"))
            _lab_queue_pick(state);
        if (dvz_gui_button(gui, "Queue probe"))
            _lab_queue_probe(state);

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
        dvz_app_window_request_frame(win);
}



/*************************************************************************************************/
/*  Main                                                                                         */
/*************************************************************************************************/

int main(int argc, char** argv)
{
    DvzScene* scene = dvz_scene();
    if (scene == NULL)
    {
        dvz_fprintf(stderr, "dvz_scene() failed\n");
        return 1;
    }

    DvzFigure* figure = dvz_figure(scene, LAB_WIDTH, LAB_HEIGHT, 0);
    DvzPanel* panel =
        figure != NULL ? dvz_panel(figure, (DvzPanelDesc){0.0f, 0.0f, 1.0f, 1.0f}) : NULL;
    if (figure == NULL || panel == NULL)
    {
        dvz_fprintf(stderr, "figure or panel creation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }
    dvz_panel_set_background_color(panel, 0.045f, 0.052f, 0.060f, 1.0f);

    DvzVisual* image = dvz_image(scene, 0);
    DvzVisual* points = dvz_point(scene, 0);
    if (image == NULL || points == NULL)
    {
        dvz_fprintf(stderr, "visual creation failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float image_pos[4][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
    };
    float texcoords[4][2] = {
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    if (
        dvz_visual_set_data(image, "position", image_pos, 4) != 0 ||
        dvz_visual_set_data(image, "texcoords", texcoords, 4) != 0 ||
        dvz_panel_add_visual(panel, image, &(DvzVisualAttachDesc){.z_layer = -1}) != 0)
    {
        dvz_fprintf(stderr, "image setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    float point_pos[LAB_POINT_COUNT][3] = {
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
    dvz_visual_set_pick_capabilities(points, DVZ_PICK_CAPABILITY_ITEM);
    if (
        dvz_visual_set_data(points, "position", point_pos, LAB_POINT_COUNT) != 0 ||
        dvz_visual_set_data(points, "color", point_color, LAB_POINT_COUNT) != 0 ||
        dvz_panel_add_visual(panel, points, NULL) != 0)
    {
        dvz_fprintf(stderr, "point setup failed\n");
        dvz_scene_destroy(scene);
        return 1;
    }

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
    (void)snprintf(state.last_pick, sizeof(state.last_pick), "pick: none");
    (void)snprintf(state.last_probe, sizeof(state.last_probe), "probe: none");
    _lab_update_points(&state);
    _lab_update_image(&state);
    state.mutation_count = 0;

    DvzApp* app = dvz_app(scene);
    if (app == NULL)
    {
        dvz_fprintf(stderr, "dvz_app() failed (no GPU or display?)\n");
        dvz_scene_destroy(scene);
        return 1;
    }

    DvzAppWindow* win = dvz_app_window_glfw(app, figure, LAB_WIDTH, LAB_HEIGHT, "scheduler_lab");
    if (win == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_glfw() failed (GLFW unavailable?)\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }
    state.win = win;

    DvzGui* gui = dvz_app_window_gui(win, NULL);
    if (gui == NULL)
    {
        dvz_fprintf(stderr, "dvz_app_window_gui() failed\n");
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 1;
    }

    dvz_app_window_set_request_frame_callback(win, _lab_request_frame, &state);
    dvz_app_window_set_frame_callback(win, _lab_frame, &state);
    dvz_app_window_set_gui_callback(win, _lab_gui, &state);
    dvz_app_window_request_frame(win);

    dvz_app_run(app, _frame_count(argc, argv));

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
