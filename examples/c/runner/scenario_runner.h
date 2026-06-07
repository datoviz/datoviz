/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Native runner support for portable C scenarios. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DVZ_EXAMPLE_NO_APP
#include "datoviz/app.h"
#else
typedef struct DvzApp DvzApp;
typedef struct DvzView DvzView;
typedef struct DvzAppCaptureConfig
{
    uint32_t unused;
} DvzAppCaptureConfig;
#endif



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;
typedef struct DvzController DvzController;
typedef struct DvzQueryRequest DvzQueryRequest;
typedef struct DvzQueryResult DvzQueryResult;
typedef struct DvzScenarioContext DvzScenarioContext;
typedef struct DvzScenarioEvent DvzScenarioEvent;

typedef bool (*DvzScenarioInitFn)(DvzScenarioContext* ctx, void** out_user);
typedef void (*DvzScenarioFrameFn)(DvzScenarioContext* ctx, void* user);
typedef void (*DvzScenarioEventFn)(
    DvzScenarioContext* ctx, const DvzScenarioEvent* event, void* user);
typedef void (*DvzScenarioPostFrameFn)(DvzScenarioContext* ctx, void* user);
typedef bool (*DvzScenarioNativeViewFn)(
    DvzScenarioContext* ctx, DvzApp* app, DvzView* view, void* user);
typedef void (*DvzScenarioDestroyFn)(DvzScenarioContext* ctx, void* user);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzRunnerPresentation
{
    DVZ_RUNNER_PRESENT_GLFW,
    DVZ_RUNNER_PRESENT_OFFSCREEN,
    DVZ_RUNNER_PRESENT_BROWSER,
} DvzRunnerPresentation;


typedef enum DvzRunnerCaptureKind
{
    DVZ_RUNNER_CAPTURE_NONE,
    DVZ_RUNNER_CAPTURE_VIDEO,
    DVZ_RUNNER_CAPTURE_PNG,
    DVZ_RUNNER_CAPTURE_DVZR,
} DvzRunnerCaptureKind;


typedef enum DvzScenarioRequirement
{
    DVZ_SCENARIO_REQ_POINT_VISUAL = 1ull << 0,
    DVZ_SCENARIO_REQ_MARKER_VISUAL = 1ull << 1,
    DVZ_SCENARIO_REQ_MESH_VISUAL = 1ull << 2,
    DVZ_SCENARIO_REQ_IMAGE_VISUAL = 1ull << 3,
    DVZ_SCENARIO_REQ_TEXT_VISUAL = 1ull << 4,
    DVZ_SCENARIO_REQ_SCENE_BUFFERS = 1ull << 5,
    DVZ_SCENARIO_REQ_STORAGE_BUFFERS = 1ull << 6,
    DVZ_SCENARIO_REQ_SCENE_COMPUTE = 1ull << 7,
    DVZ_SCENARIO_REQ_QUERY_READBACK = 1ull << 8,
    DVZ_SCENARIO_REQ_FRAME_CALLBACKS = 1ull << 9,
    DVZ_SCENARIO_REQ_NATIVE_CAPTURE = 1ull << 10,
    DVZ_SCENARIO_REQ_NATIVE_VIEW = 1ull << 11,
    DVZ_SCENARIO_REQ_CONTROLLER = 1ull << 12,
    DVZ_SCENARIO_REQ_PANZOOM = 1ull << 13,
    DVZ_SCENARIO_REQ_ARCBALL = 1ull << 14,
} DvzScenarioRequirement;


typedef enum DvzScenarioEventKind
{
    DVZ_SCENARIO_EVENT_NONE = 0,
    DVZ_SCENARIO_EVENT_POINTER,
    DVZ_SCENARIO_EVENT_KEY,
    DVZ_SCENARIO_EVENT_RESIZE,
    DVZ_SCENARIO_EVENT_QUERY_RESULT,
} DvzScenarioEventKind;


typedef enum DvzScenarioPointerType
{
    DVZ_SCENARIO_POINTER_NONE = 0,
    DVZ_SCENARIO_POINTER_RELEASE,
    DVZ_SCENARIO_POINTER_PRESS,
    DVZ_SCENARIO_POINTER_MOVE,
    DVZ_SCENARIO_POINTER_CLICK,
    DVZ_SCENARIO_POINTER_DOUBLE_CLICK,
    DVZ_SCENARIO_POINTER_DRAG_START,
    DVZ_SCENARIO_POINTER_DRAG,
    DVZ_SCENARIO_POINTER_DRAG_STOP,
    DVZ_SCENARIO_POINTER_WHEEL,
} DvzScenarioPointerType;



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_SCENARIO_MAX_CONTROLLER_BINDINGS 32u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzScenarioControllerBinding
{
    DvzPanel* panel;
    DvzController* controller;
    DvzDimMask dims;
} DvzScenarioControllerBinding;


typedef struct DvzScenarioPointerEvent
{
    DvzScenarioPointerType type;
    float x;
    float y;
    float dx;
    float dy;
    float content_scale;
    uint32_t button;
    uint32_t modifiers;
    uint64_t timestamp_ns;
} DvzScenarioPointerEvent;


typedef struct DvzScenarioKeyEvent
{
    uint32_t type;
    uint32_t key;
    uint32_t modifiers;
} DvzScenarioKeyEvent;


typedef struct DvzScenarioResizeEvent
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t window_width;
    uint32_t window_height;
    float content_scale_x;
    float content_scale_y;
} DvzScenarioResizeEvent;


struct DvzScenarioEvent
{
    DvzScenarioEventKind kind;
    union
    {
        DvzScenarioPointerEvent pointer;
        DvzScenarioKeyEvent key;
        DvzScenarioResizeEvent resize;
        const DvzQueryResult* query_result;
    } content;
};


struct DvzScenarioContext
{
    DvzScene* scene;
    DvzFigure* figure;

    uint32_t width;
    uint32_t height;

    double time;
    double dt;
    uint64_t frame_index;

    DvzScenarioControllerBinding controller_bindings[DVZ_SCENARIO_MAX_CONTROLLER_BINDINGS];
    uint32_t controller_binding_count;
};


typedef struct DvzScenarioSpec
{
    const char* id;
    const char* title;
    uint32_t width;
    uint32_t height;
    double fps;
    uint64_t requirements;

    DvzScenarioInitFn init;
    DvzScenarioFrameFn frame;
    DvzScenarioEventFn event;
    DvzScenarioPostFrameFn post_frame;
    DvzScenarioNativeViewFn native_view;
    DvzScenarioDestroyFn destroy;
} DvzScenarioSpec;


typedef struct DvzRunnerConfig
{
    DvzRunnerPresentation presentation;
    DvzRunnerCaptureKind capture_kind;

    uint32_t width;
    uint32_t height;
    uint32_t frame_count;
    double fps;

    DvzAppCaptureConfig capture;
    bool print_progress;
    bool pace_wall_time;
} DvzRunnerConfig;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzRunnerConfig dvz_runner_config(const DvzScenarioSpec* spec);

bool dvz_runner_capture_path(
    const DvzAppCaptureConfig* capture, DvzRunnerCaptureKind kind, char* out, size_t out_size,
    bool display);

/**
 * Bind a scene-owned controller to a scenario panel and register it for runner input connection.
 *
 * The scene/controller binding is applied immediately. Native live runners connect registered
 * panels to the view input router after creating the GLFW view; offscreen runners keep the scene
 * binding but do not attach input.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @param controller scene-owned controller
 * @param dims controlled dimensions
 * @return 0 on success, -1 on validation error
 */
int dvz_scenario_bind_controller(
    DvzScenarioContext* ctx, DvzPanel* panel, DvzController* controller, DvzDimMask dims);

/**
 * Create, bind, and register a scene-owned panzoom controller for a scenario panel.
 *
 * @param ctx scenario context
 * @param panel target panel
 * @param desc panzoom descriptor, or NULL for defaults
 * @param dims controlled dimensions, typically DVZ_DIM_MASK_XY
 * @return borrowed panzoom payload, or NULL on validation error
 */
DvzPanzoom* dvz_scenario_panzoom(
    DvzScenarioContext* ctx, DvzPanel* panel, const DvzPanzoomDesc* desc, DvzDimMask dims);

/**
 * Convert one portable pointer event to panel-local coordinates.
 *
 * @param panel target panel
 * @param event portable pointer event
 * @param out_x output panel-local x coordinate
 * @param out_y output panel-local y coordinate
 * @return true when the pointer is inside the panel rectangle
 */
bool dvz_scenario_panel_pointer_position(
    const DvzPanel* panel, const DvzScenarioPointerEvent* event, double* out_x, double* out_y);

/**
 * Queue one panel query in panel-local coordinates.
 *
 * @param panel target panel
 * @param x panel-local x coordinate
 * @param y panel-local y coordinate
 * @param request query request
 * @return 0 on success, negative on error
 */
int dvz_scenario_panel_query(
    DvzPanel* panel, double x, double y, const DvzQueryRequest* request);

int dvz_scenario_run_native(const DvzScenarioSpec* spec, const DvzRunnerConfig* config);

int dvz_scenario_run_native_cli(const DvzScenarioSpec* spec, int argc, char** argv);
