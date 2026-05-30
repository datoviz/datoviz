/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Shared example GUI controls                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "datoviz/common/macros.h"
#include "datoviz/gui.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzExampleGuiMaterialControls
{
    bool standard_material;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float roughness;
    float rim_strength;
} DvzExampleGuiMaterialControls;



typedef struct DvzExampleGuiMsaaControls
{
    bool enabled;
    bool alpha_to_coverage;
    float samples;
    float min_samples;
    float max_samples;
} DvzExampleGuiMsaaControls;


typedef struct DvzExampleGuiEdlControls
{
    bool enabled;
    float radius;
    float strength;
    float depth_scale;
} DvzExampleGuiEdlControls;



typedef struct DvzExampleGuiSsaoControls
{
    bool enabled;
    bool blur;
    bool debug_view;
    bool show_blur_sigmas;
    bool show_debug_view;
    float radius;
    float strength;
    float bias;
    float power;
    float min_visibility;
    float samples;
    float min_samples;
    float max_samples;
    float blur_radius;
    float blur_radius_max;
    float blur_depth_sigma;
    float blur_normal_sigma;
} DvzExampleGuiSsaoControls;



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool dvz_example_gui_material(DvzGui* gui, DvzExampleGuiMaterialControls* controls);

bool dvz_example_gui_msaa(DvzGui* gui, DvzExampleGuiMsaaControls* controls);

bool dvz_example_gui_edl(DvzGui* gui, DvzExampleGuiEdlControls* controls);

bool dvz_example_gui_ssao(DvzGui* gui, DvzExampleGuiSsaoControls* controls);

bool dvz_example_gui_clip_box(DvzGui* gui, const char* label, float clip_min[3], float clip_max[3]);

bool dvz_example_gui_vec3(
    DvzGui* gui, const char* label, float value[3], float min, float max, const char* format);

EXTERN_C_OFF
