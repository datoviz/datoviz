/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Shared example GUI controls                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_gui_controls.h"

#include <stdbool.h>
#include <stdint.h>

#include "_assertions.h"
#include "_compat.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a fallback value when a control maximum is not positive.
 *
 * @param value configured value
 * @param fallback fallback value
 * @return configured value, or fallback
 */
static float _control_max(float value, float fallback)
{
    return value > 0.0f ? value : fallback;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Show reusable material controls used by the lit scene examples.
 *
 * @param gui GUI overlay
 * @param controls material controls edited in place
 * @return whether a material field changed
 */
bool dvz_example_gui_material(DvzGui* gui, DvzExampleGuiMaterialControls* controls)
{
    ANN(gui);
    ANN(controls);
    bool changed = false;

    changed |= dvz_gui_checkbox(gui, "Standard material", &controls->standard_material);
    if (controls->standard_material)
    {
        changed |= dvz_gui_slider_float(gui, "Roughness", &controls->roughness, 0.02f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Specular", &controls->specular, 0.0f, 1.5f);
        changed |= dvz_gui_slider_float(gui, "Rim", &controls->rim_strength, 0.0f, 1.0f);
    }
    else
    {
        changed |= dvz_gui_slider_float(gui, "Ambient", &controls->ambient, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Diffuse", &controls->diffuse, 0.0f, 1.5f);
        changed |= dvz_gui_slider_float(gui, "Specular", &controls->specular, 0.0f, 1.5f);
        changed |= dvz_gui_slider_float(gui, "Shininess", &controls->shininess, 1.0f, 160.0f);
    }
    return changed;
}



/**
 * Show reusable MSAA controls used by the graphics examples.
 *
 * @param gui GUI overlay
 * @param controls MSAA controls edited in place
 * @return whether an MSAA field changed
 */
bool dvz_example_gui_msaa(DvzGui* gui, DvzExampleGuiMsaaControls* controls)
{
    ANN(gui);
    ANN(controls);
    bool changed = false;
    const float min_samples = controls->min_samples > 0.0f ? controls->min_samples : 2.0f;
    const float max_samples = _control_max(controls->max_samples, 16.0f);

    changed |= dvz_gui_checkbox(gui, "Enable MSAA", &controls->enabled);
    changed |=
        dvz_gui_slider_float(gui, "MSAA samples", &controls->samples, min_samples, max_samples);
    changed |= dvz_gui_checkbox(gui, "Alpha-to-coverage", &controls->alpha_to_coverage);
    return changed;
}



/**
 * Show reusable EDL controls used by depth-rich examples.
 *
 * @param gui GUI overlay
 * @param controls EDL controls edited in place
 * @return whether an EDL field changed
 */
bool dvz_example_gui_edl(DvzGui* gui, DvzExampleGuiEdlControls* controls)
{
    ANN(gui);
    ANN(controls);
    bool changed = false;

    changed |= dvz_gui_checkbox(gui, "Enable EDL", &controls->enabled);
    changed |= dvz_gui_slider_float(gui, "Radius", &controls->radius, 1.0f, 8.0f);
    changed |= dvz_gui_slider_float(gui, "Strength", &controls->strength, 0.0f, 160.0f);
    changed |= dvz_gui_slider_float(gui, "Depth scale", &controls->depth_scale, 0.1f, 8.0f);
    return changed;
}



/**
 * Show reusable SSAO controls used by depth-rich examples.
 *
 * @param gui GUI overlay
 * @param controls SSAO controls edited in place
 * @return whether an SSAO field changed
 */
bool dvz_example_gui_ssao(DvzGui* gui, DvzExampleGuiSsaoControls* controls)
{
    ANN(gui);
    ANN(controls);
    bool changed = false;
    const float min_samples = controls->min_samples > 0.0f ? controls->min_samples : 4.0f;
    const float max_samples = _control_max(controls->max_samples, 32.0f);
    const float blur_radius_max = _control_max(controls->blur_radius_max, 16.0f);

    changed |= dvz_gui_checkbox(gui, "Enable SSAO", &controls->enabled);
    changed |= dvz_gui_slider_float(gui, "Radius", &controls->radius, 0.05f, 4.0f);
    changed |= dvz_gui_slider_float(gui, "Strength", &controls->strength, 0.0f, 6.0f);
    changed |= dvz_gui_slider_float(gui, "Bias", &controls->bias, 0.0f, 0.12f);
    changed |= dvz_gui_slider_float(gui, "Power", &controls->power, 0.1f, 8.0f);
    changed |= dvz_gui_slider_float(gui, "Min visibility", &controls->min_visibility, 0.0f, 1.0f);
    changed |= dvz_gui_slider_float(gui, "Samples", &controls->samples, min_samples, max_samples);
    changed |= dvz_gui_checkbox(gui, "Blur", &controls->blur);
    changed |=
        dvz_gui_slider_float(gui, "Blur radius", &controls->blur_radius, 1.0f, blur_radius_max);
    if (controls->show_blur_sigmas)
    {
        changed |=
            dvz_gui_slider_float(gui, "Depth sigma", &controls->blur_depth_sigma, 0.01f, 2.5f);
        changed |=
            dvz_gui_slider_float(gui, "Normal sigma", &controls->blur_normal_sigma, 0.01f, 1.0f);
    }
    if (controls->show_debug_view)
        changed |= dvz_gui_checkbox(gui, "Raw SSAO", &controls->debug_view);
    return changed;
}



/**
 * Show reusable volume clip-box controls with one min/max editor per axis.
 *
 * @param gui GUI overlay
 * @param label section label
 * @param clip_min clip lower bounds edited in place
 * @param clip_max clip upper bounds edited in place
 * @return whether a clip bound changed
 */
bool dvz_example_gui_clip_box(DvzGui* gui, const char* label, float clip_min[3], float clip_max[3])
{
    ANN(gui);
    ANN(label);
    ANN(clip_min);
    ANN(clip_max);
    bool changed = false;
    char axis_label[64] = {0};
    static const char* const axes[] = {"X", "Y", "Z"};

    dvz_gui_separator_text(gui, label);
    for (uint32_t i = 0; i < 3; i++)
    {
        dvz_snprintf(axis_label, sizeof(axis_label), "%s range", axes[i]);
        changed |=
            dvz_gui_range_float(gui, axis_label, &clip_min[i], &clip_max[i], 0.01f, 0.0f, 1.0f,
                                "%.2f");
    }
    return changed;
}



/**
 * Show reusable three-component vector controls as one compact slider.
 *
 * @param gui GUI overlay
 * @param label slider label
 * @param value vector edited in place
 * @param min minimum component value
 * @param max maximum component value
 * @param format optional display format
 * @return whether a vector component changed
 */
bool dvz_example_gui_vec3(
    DvzGui* gui, const char* label, float value[3], float min, float max, const char* format)
{
    ANN(gui);
    ANN(label);
    ANN(value);
    if (format != NULL)
    {
        // The curated wrapper has no formatted SliderFloat3 variant yet; raw cimgui is unnecessary
        // for current examples, so keep the argument for API symmetry and future extension.
        (void)format;
    }
    return dvz_gui_slider_float3(gui, label, value, min, max);
}
