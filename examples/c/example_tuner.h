/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Modular example tuner                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "_compat.h"
#include "datoviz/app.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/controller/camera.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/gui.h"
#include "datoviz/input/router.h"
#include "datoviz/scene.h"
#include "example_gui_controls.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define EXAMPLE_TUNER_MAX_COMPONENTS 16u
#define EXAMPLE_TUNER_MAX_ARCBALLS   8u
#define EXAMPLE_TUNER_MAX_PANZOOMS   8u
#define EXAMPLE_TUNER_MAX_CAMERAS    8u
#define EXAMPLE_TUNER_MAX_EDL        8u



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct ExampleTuner ExampleTuner;

typedef void (*ExampleTunerSyncFn)(void* user);
typedef bool (*ExampleTunerGuiFn)(DvzGui* gui, void* user);
typedef void (*ExampleTunerApplyFn)(void* user);
typedef void (*ExampleTunerResetFn)(void* user);
typedef void (*ExampleTunerPrintFn)(FILE* fp, void* user);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct ExampleTunerComponent
{
    const char* title;
    void* user;
    ExampleTunerSyncFn sync;
    ExampleTunerGuiFn gui;
    ExampleTunerApplyFn apply;
    ExampleTunerResetFn reset;
    ExampleTunerPrintFn print_c;
} ExampleTunerComponent;


typedef struct ExampleTunerArcball
{
    const char* name;
    DvzArcball* arcball;
    vec3 angles;
    vec2 pan;
    float zoom;
    vec3 reset_angles;
    vec2 reset_pan;
    float reset_zoom;
} ExampleTunerArcball;


typedef struct ExampleTunerPanzoom
{
    const char* name;
    DvzPanzoom* panzoom;
    vec2 pan;
    vec2 zoom;
    vec2 reset_pan;
    vec2 reset_zoom;
} ExampleTunerPanzoom;


typedef struct ExampleTunerCamera
{
    const char* name;
    DvzPanel* panel;
    DvzCamera* camera_ref;
    DvzCameraDesc camera;
    DvzCameraDesc reset_camera;
} ExampleTunerCamera;


typedef struct ExampleTunerEdl
{
    const char* name;
    DvzPanel* panel;
    DvzExampleGuiEdlControls* controls;
    DvzExampleGuiEdlControls reset_controls;
} ExampleTunerEdl;


struct ExampleTuner
{
    const char* name;
    DvzView* view;
    DvzInputRouter* input;
    DvzCallbackId input_subscription_id;
    bool installed;
    uint32_t screenshot_index;

    ExampleTunerComponent components[EXAMPLE_TUNER_MAX_COMPONENTS];
    uint32_t component_count;

    ExampleTunerArcball arcballs[EXAMPLE_TUNER_MAX_ARCBALLS];
    uint32_t arcball_count;

    ExampleTunerPanzoom panzooms[EXAMPLE_TUNER_MAX_PANZOOMS];
    uint32_t panzoom_count;

    ExampleTunerCamera cameras[EXAMPLE_TUNER_MAX_CAMERAS];
    uint32_t camera_count;

    ExampleTunerEdl edls[EXAMPLE_TUNER_MAX_EDL];
    uint32_t edl_count;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

ExampleTuner example_tuner(const char* name);

bool example_tuner_attach(ExampleTuner* tuner, DvzView* view);

void example_tuner_detach(ExampleTuner* tuner);

bool example_tuner_add_component(
    ExampleTuner* tuner,
    const char* title,
    void* user,
    ExampleTunerSyncFn sync,
    ExampleTunerGuiFn gui,
    ExampleTunerApplyFn apply,
    ExampleTunerResetFn reset,
    ExampleTunerPrintFn print_c);

void example_tuner_arcball(
    ExampleTuner* tuner,
    const char* name,
    DvzArcball* arcball,
    const float reset_angles[3],
    float reset_zoom,
    const float reset_pan[2]);

void example_tuner_panzoom(
    ExampleTuner* tuner,
    const char* name,
    DvzPanzoom* panzoom,
    const float reset_pan[2],
    const float reset_zoom[2]);

void example_tuner_camera(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    const DvzCameraDesc* camera_desc);

void example_tuner_camera_ref(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzCamera* camera,
    const DvzCameraDesc* camera_desc);

void example_tuner_edl(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiEdlControls* controls);

void example_tuner_sync(ExampleTuner* tuner);

void example_tuner_reset(ExampleTuner* tuner);

void example_tuner_print_c(ExampleTuner* tuner, FILE* fp);

EXTERN_C_OFF
