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

#include "datoviz/app.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzFigure DvzFigure;
typedef struct DvzPanel DvzPanel;
typedef struct DvzController DvzController;
typedef struct DvzScenarioContext DvzScenarioContext;

typedef bool (*DvzScenarioInitFn)(DvzScenarioContext* ctx, void** out_user);
typedef void (*DvzScenarioFrameFn)(DvzScenarioContext* ctx, void* user);
typedef void (*DvzScenarioDestroyFn)(DvzScenarioContext* ctx, void* user);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DvzRunnerPresentation
{
    DVZ_RUNNER_PRESENT_GLFW,
    DVZ_RUNNER_PRESENT_OFFSCREEN,
} DvzRunnerPresentation;


typedef enum DvzRunnerCaptureKind
{
    DVZ_RUNNER_CAPTURE_NONE,
    DVZ_RUNNER_CAPTURE_VIDEO,
    DVZ_RUNNER_CAPTURE_PNG,
    DVZ_RUNNER_CAPTURE_DVZR,
} DvzRunnerCaptureKind;



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

    DvzScenarioInitFn init;
    DvzScenarioFrameFn frame;
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

int dvz_scenario_run_native(const DvzScenarioSpec* spec, const DvzRunnerConfig* config);

int dvz_scenario_run_native_cli(const DvzScenarioSpec* spec, int argc, char** argv);
