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
#define EXAMPLE_TUNER_MAX_MSAA       8u
#define EXAMPLE_TUNER_MAX_SSAO       8u
#define EXAMPLE_TUNER_MAX_MATERIALS  16u
#define EXAMPLE_TUNER_MAX_DEPTH_CUES 8u
#define EXAMPLE_TUNER_MAX_VOLUMES    8u
#define EXAMPLE_TUNER_DEFAULT_DOCK_WIDTH_PX 360.0f



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


typedef struct ExampleTunerMsaa
{
    const char* name;
    DvzPanel* panel;
    DvzExampleGuiMsaaControls* controls;
    DvzExampleGuiMsaaControls reset_controls;
} ExampleTunerMsaa;


typedef struct ExampleTunerSsao
{
    const char* name;
    DvzPanel* panel;
    DvzExampleGuiSsaoControls* controls;
    DvzExampleGuiSsaoControls reset_controls;
} ExampleTunerSsao;


typedef struct ExampleTunerMaterial
{
    const char* name;
    DvzVisual* visual;
    DvzMaterialDesc* material;
    DvzMaterialDesc reset_material;
} ExampleTunerMaterial;


typedef struct ExampleTunerDepthCue
{
    const char* name;
    DvzVisual* visual;
    bool* enabled;
    bool reset_enabled;
    DvzDepthCueDesc* desc;
    DvzDepthCueDesc reset_desc;
} ExampleTunerDepthCue;


typedef struct ExampleTunerVolume
{
    const char* name;
    DvzVisual* visual;
    DvzVolumeState state;
    DvzVolumeState reset_state;
} ExampleTunerVolume;


typedef struct ExampleTunerLayout
{
    bool dock_initially;
    bool reserve_docked_area;
    DvzGuiDockSlot dock_slot;
    float dock_size_px;
} ExampleTunerLayout;


struct ExampleTuner
{
    const char* name;
    DvzView* view;
    DvzFigure* figure;
    DvzInputRouter* input;
    DvzCallbackId input_subscription_id;
    bool installed;
    uint32_t screenshot_index;
    ExampleTunerLayout layout;
    DvzPanelReserve previous_figure_reserve;
    DvzPanelReserve applied_figure_reserve;
    bool has_previous_figure_reserve;
    bool reserve_applied;

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

    ExampleTunerMsaa msaas[EXAMPLE_TUNER_MAX_MSAA];
    uint32_t msaa_count;

    ExampleTunerSsao ssaos[EXAMPLE_TUNER_MAX_SSAO];
    uint32_t ssao_count;

    ExampleTunerMaterial materials[EXAMPLE_TUNER_MAX_MATERIALS];
    uint32_t material_count;

    ExampleTunerDepthCue depth_cues[EXAMPLE_TUNER_MAX_DEPTH_CUES];
    uint32_t depth_cue_count;

    ExampleTunerVolume volumes[EXAMPLE_TUNER_MAX_VOLUMES];
    uint32_t volume_count;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

ExampleTuner example_tuner(const char* name);

void example_tuner_figure(ExampleTuner* tuner, DvzFigure* figure);

void example_tuner_layout(ExampleTuner* tuner, const ExampleTunerLayout* layout);

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

void example_tuner_msaa(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiMsaaControls* controls);

void example_tuner_ssao(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiSsaoControls* controls);

void example_tuner_material(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    DvzMaterialDesc* material);

void example_tuner_depth_cue(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    bool* enabled,
    DvzDepthCueDesc* desc);

void example_tuner_volume(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    const DvzVolumeState* defaults);

void example_tuner_sync(ExampleTuner* tuner);

void example_tuner_reset(ExampleTuner* tuner);

void example_tuner_print_c(ExampleTuner* tuner, FILE* fp);

EXTERN_C_OFF
