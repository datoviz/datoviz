/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example debug shortcuts                                                                      */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/app.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/controller/camera.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/input/router.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EXAMPLE_DEBUG_MAX_ARCBALLS 8u
#define EXAMPLE_DEBUG_MAX_PANZOOMS 8u
#define EXAMPLE_DEBUG_MAX_CAMERAS  8u



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ExampleDebugArcball
{
    const char* name;
    DvzArcball* arcball;
} ExampleDebugArcball;


typedef struct ExampleDebugPanzoom
{
    const char* name;
    DvzPanzoom* panzoom;
} ExampleDebugPanzoom;


typedef struct ExampleDebugCamera
{
    const char* name;
    DvzCameraDesc camera;
} ExampleDebugCamera;


typedef struct ExampleDebug
{
    DvzView* view;
    DvzInputRouter* input;
    const char* exe;
    const char* basename;
    bool installed;
    uint32_t screenshot_index;

    ExampleDebugArcball arcballs[EXAMPLE_DEBUG_MAX_ARCBALLS];
    uint32_t arcball_count;

    ExampleDebugPanzoom panzooms[EXAMPLE_DEBUG_MAX_PANZOOMS];
    uint32_t panzoom_count;

    ExampleDebugCamera cameras[EXAMPLE_DEBUG_MAX_CAMERAS];
    uint32_t camera_count;
} ExampleDebug;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool example_debug_arg(const char* arg);

bool example_debug_requested(int argc, char** argv);

ExampleDebug example_debug(DvzView* view, const char* exe, const char* basename);

void example_debug_arcball(ExampleDebug* debug, const char* name, DvzArcball* arcball);

void example_debug_panzoom(ExampleDebug* debug, const char* name, DvzPanzoom* panzoom);

void example_debug_camera(
    ExampleDebug* debug, const char* name, const DvzCameraDesc* camera_desc);

bool example_debug_install(ExampleDebug* debug, int argc, char** argv);

void example_debug_uninstall(ExampleDebug* debug);
