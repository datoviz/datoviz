/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Modular example tuner                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_tuner.h"

#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "datoviz/math/_cglm.h"
#include "example_common.h"

#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui/cimgui.h"



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a non-empty component label.
 *
 * @param name optional name
 * @param fallback fallback label
 * @return label
 */
static const char* _tuner_name(const char* name, const char* fallback)
{
    return name != NULL && name[0] != '\0' ? name : fallback;
}



/**
 * Copy a vec2, using a zero fallback for NULL.
 *
 * @param dst destination
 * @param src optional source
 */
static void _copy_vec2(vec2 dst, const float src[2])
{
    dst[0] = src != NULL ? src[0] : 0.0f;
    dst[1] = src != NULL ? src[1] : 0.0f;
}



/**
 * Copy a vec3, using a zero fallback for NULL.
 *
 * @param dst destination
 * @param src optional source
 */
static void _copy_vec3(vec3 dst, const float src[3])
{
    dst[0] = src != NULL ? src[0] : 0.0f;
    dst[1] = src != NULL ? src[1] : 0.0f;
    dst[2] = src != NULL ? src[2] : 0.0f;
}



/**
 * Recover the public turntable pose from the camera view it controls.
 *
 * @param view camera view
 * @param world_up stable turntable up vector
 * @param pivot output pivot
 * @param yaw output yaw in radians
 * @param pitch output pitch in radians
 * @param distance output eye-to-pivot distance
 * @return whether the view describes a valid pose
 */
static bool _turntable_pose_from_view(
    const DvzCameraView* view, const vec3 world_up, vec3 pivot, float* yaw, float* pitch,
    float* distance)
{
    ANN(view);
    ANN(yaw);
    ANN(pitch);
    ANN(distance);

    vec3 front = {0};
    glm_vec3_sub(
        (vec3){view->target[0], view->target[1], view->target[2]},
        (vec3){view->eye[0], view->eye[1], view->eye[2]}, front);
    const float norm = glm_vec3_norm(front);
    if (norm <= 1e-6f || !isfinite(norm))
        return false;
    glm_vec3_scale(front, 1.0f / norm, front);

    vec3 up = {world_up[0], world_up[1], world_up[2]};
    if (glm_vec3_norm(up) <= 1e-6f)
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, up);
    glm_vec3_normalize(up);
    vec3 axis0 = {0};
    if (fabsf(up[2]) > 0.9f)
        glm_vec3_cross((vec3){0.0f, 1.0f, 0.0f}, up, axis0);
    else
    {
        glm_vec3_cross((vec3){0.0f, 0.0f, 1.0f}, up, axis0);
        glm_vec3_scale(axis0, -1.0f, axis0);
    }
    glm_vec3_normalize(axis0);
    vec3 axis1 = {0};
    glm_vec3_cross(axis0, up, axis1);
    glm_vec3_normalize(axis1);

    const float sin_pitch = fminf(fmaxf(glm_vec3_dot(front, up), -1.0f), +1.0f);
    vec3 vertical = {0};
    vec3 horizontal = {0};
    glm_vec3_scale(up, sin_pitch, vertical);
    glm_vec3_sub(front, vertical, horizontal);
    _copy_vec3(pivot, view->target);
    *distance = norm;
    *pitch = asinf(sin_pitch);
    *yaw = atan2f(glm_vec3_dot(horizontal, axis1), glm_vec3_dot(horizontal, axis0));
    return true;
}



static bool _reserve_equal(DvzPanelReserve a, DvzPanelReserve b)
{
    return fabsf(a.left_px - b.left_px) < 0.5f && fabsf(a.right_px - b.right_px) < 0.5f &&
           fabsf(a.top_px - b.top_px) < 0.5f && fabsf(a.bottom_px - b.bottom_px) < 0.5f;
}



static ExampleTunerLayout _tuner_default_layout(void)
{
    return (ExampleTunerLayout){
        .dock_initially = true,
        .reserve_docked_area = true,
        .dock_slot = DVZ_GUI_DOCK_SLOT_LEFT,
        .dock_size_px = EXAMPLE_TUNER_DEFAULT_DOCK_WIDTH_PX,
    };
}



static DvzGuiDockSlot _tuner_valid_dock_slot(DvzGuiDockSlot slot)
{
    switch (slot)
    {
    case DVZ_GUI_DOCK_SLOT_LEFT:
    case DVZ_GUI_DOCK_SLOT_RIGHT:
    case DVZ_GUI_DOCK_SLOT_TOP:
    case DVZ_GUI_DOCK_SLOT_BOTTOM:
        return slot;
    default:
        return DVZ_GUI_DOCK_SLOT_LEFT;
    }
}



static void
_tuner_clamp_reserve(DvzFigure* figure, DvzPanelReserve* reserve, DvzGuiDockSlot dock_slot)
{
    ANN(reserve);
    if (figure == NULL)
        return;

    uint32_t width = 0;
    uint32_t height = 0;
    dvz_figure_size(figure, &width, &height);
    if (width > 1 && reserve->left_px + reserve->right_px >= (float)width)
    {
        if (dock_slot == DVZ_GUI_DOCK_SLOT_RIGHT)
        {
            const float max_right = (float)(width - 1u) - reserve->left_px;
            reserve->right_px = max_right > 0.0f ? max_right : 0.0f;
        }
        else
        {
            const float max_left = (float)(width - 1u) - reserve->right_px;
            reserve->left_px = max_left > 0.0f ? max_left : 0.0f;
        }
    }
    if (height > 1 && reserve->top_px + reserve->bottom_px >= (float)height)
    {
        if (dock_slot == DVZ_GUI_DOCK_SLOT_BOTTOM)
        {
            const float max_bottom = (float)(height - 1u) - reserve->top_px;
            reserve->bottom_px = max_bottom > 0.0f ? max_bottom : 0.0f;
        }
        else
        {
            const float max_top = (float)(height - 1u) - reserve->bottom_px;
            reserve->top_px = max_top > 0.0f ? max_top : 0.0f;
        }
    }
}



static void _tuner_snapshot_figure_reserve(ExampleTuner* tuner)
{
    ANN(tuner);
    if (tuner->figure == NULL || tuner->has_previous_figure_reserve)
        return;
    tuner->has_previous_figure_reserve =
        dvz_figure_get_reserve(tuner->figure, &tuner->previous_figure_reserve);
}



static void _tuner_restore_figure_reserve(ExampleTuner* tuner)
{
    if (tuner == NULL || tuner->figure == NULL || !tuner->has_previous_figure_reserve)
        return;
    if (dvz_figure_set_reserve(tuner->figure, &tuner->previous_figure_reserve) == DVZ_OK)
        tuner->reserve_applied = false;
}



static void _tuner_update_figure_reserve(ExampleTuner* tuner, DvzGui* gui)
{
    if (tuner == NULL || gui == NULL || tuner->figure == NULL || !tuner->layout.reserve_docked_area)
        return;

    _tuner_snapshot_figure_reserve(tuner);
    if (!tuner->has_previous_figure_reserve)
        return;

    DvzRect rect = {0};
    const bool docked =
        dvz_gui_current_window_docked(gui) && dvz_gui_current_window_rect(gui, &rect);
    const bool has_size = rect.width > 0.0f && rect.height > 0.0f;
    if (!docked || !has_size)
    {
        if (tuner->reserve_applied)
            _tuner_restore_figure_reserve(tuner);
        return;
    }

    DvzPanelReserve next = tuner->previous_figure_reserve;
    const DvzGuiDockSlot dock_slot = _tuner_valid_dock_slot(tuner->layout.dock_slot);
    switch (dock_slot)
    {
    case DVZ_GUI_DOCK_SLOT_RIGHT:
        next.right_px += rect.width;
        break;
    case DVZ_GUI_DOCK_SLOT_TOP:
        next.top_px += rect.height;
        break;
    case DVZ_GUI_DOCK_SLOT_BOTTOM:
        next.bottom_px += rect.height;
        break;
    case DVZ_GUI_DOCK_SLOT_LEFT:
    default:
        next.left_px += rect.width;
        break;
    }
    _tuner_clamp_reserve(tuner->figure, &next, dock_slot);

    if (tuner->reserve_applied && _reserve_equal(tuner->applied_figure_reserve, next))
        return;
    if (dvz_figure_set_reserve(tuner->figure, &next) != DVZ_OK)
        return;
    tuner->applied_figure_reserve = next;
    tuner->reserve_applied = true;
}



/**
 * Clamp an integer value.
 *
 * @param value value
 * @param min minimum
 * @param max maximum
 * @return clamped value
 */
static int _clamp_int(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}



/**
 * Clamp a depth-cue descriptor after GUI edits.
 *
 * @param desc descriptor edited in place
 */
static void _depth_cue_clamp(DvzDepthCueDesc* desc)
{
    if (desc == NULL)
        return;

    if (desc->near_depth < 0.0f)
        desc->near_depth = 0.0f;
    if (desc->far_depth <= desc->near_depth + 0.001f)
        desc->far_depth = desc->near_depth + 0.001f;
}



/**
 * Clamp a volume state after GUI edits.
 *
 * @param state state edited in place
 */
static void _volume_clamp(DvzVolumeState* state)
{
    if (state == NULL)
        return;

    if (state->step_count < 1u)
        state->step_count = 1u;
    if (state->value_max < state->value_min)
    {
        const double tmp = state->value_min;
        state->value_min = state->value_max;
        state->value_max = tmp;
    }
}



/**
 * Return conservative volume defaults when no retained state is available.
 *
 * @return volume state
 */
static DvzVolumeState _volume_state_default(void)
{
    DvzVolumeState state = {
        .opacity = 1.0f,
        .sampling = DVZ_VOLUME_SAMPLING_LINEAR,
        .render_mode = DVZ_VOLUME_RENDER_COMPOSITE,
        .slice_axis = DVZ_VOLUME_AXIS_Z,
        .slice_position = 0.5,
        .value_min = 0.0,
        .value_max = 1.0,
        .step_count = 64u,
    };
    return state;
}



/**
 * Apply an EDL control state to its panel.
 *
 * @param edl EDL tuner component state
 */
static void _edl_apply(ExampleTunerEdl* edl)
{
    if (edl == NULL || edl->panel == NULL || edl->controls == NULL)
        return;

    if (!edl->controls->enabled)
    {
        (void)dvz_panel_set_edl(edl->panel, NULL);
        return;
    }

    DvzEdlDesc desc = dvz_edl_desc();
    desc.radius = edl->controls->radius;
    desc.strength = edl->controls->strength;
    desc.depth_scale = edl->controls->depth_scale;
    (void)dvz_panel_set_edl(edl->panel, &desc);
}



/**
 * Dump one arcball component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _arcball_print(FILE* fp, const ExampleTunerArcball* state)
{
    ANN(fp);
    ANN(state);

    dvz_fprintf(fp, "/* arcball: %s */\n", _tuner_name(state->name, "arcball"));
    dvz_fprintf(
        fp, "dvz_arcball_initial(arcball, (vec3){%+.6ff, %+.6ff, %+.6ff});\n",
        state->angles[0], state->angles[1], state->angles[2]);
    dvz_fprintf(fp, "dvz_arcball_zoom(arcball, %.6ff);\n", state->zoom);
    dvz_fprintf(
        fp, "dvz_arcball_pan(arcball, (vec2){%+.6ff, %+.6ff});\n", state->pan[0],
        state->pan[1]);
}



/**
 * Dump one turntable descriptor at its current pose.
 *
 * @param fp output stream
 * @param state component state
 */
static void _turntable_print(FILE* fp, const ExampleTunerTurntable* state)
{
    ANN(fp);
    ANN(state);

    const DvzTurntableDesc* desc = &state->reset_desc;
    dvz_fprintf(fp, "/* turntable: %s */\n", _tuner_name(state->name, "turntable"));
    dvz_fprintf(
        fp, "/* yaw = %+.6ff; pitch = %+.6ff; distance = %.6ff */\n", state->yaw, state->pitch,
        state->distance);
    dvz_fprintf(fp, "DvzTurntableDesc turntable_desc = dvz_turntable_desc();\n");
    dvz_fprintf(
        fp,
        "turntable_desc.initial_view.eye[0] = %+.6ff; "
        "turntable_desc.initial_view.eye[1] = %+.6ff; "
        "turntable_desc.initial_view.eye[2] = %+.6ff;\n",
        state->view.eye[0], state->view.eye[1], state->view.eye[2]);
    dvz_fprintf(
        fp,
        "turntable_desc.initial_view.target[0] = %+.6ff; "
        "turntable_desc.initial_view.target[1] = %+.6ff; "
        "turntable_desc.initial_view.target[2] = %+.6ff;\n",
        state->pivot[0], state->pivot[1], state->pivot[2]);
    dvz_fprintf(
        fp,
        "turntable_desc.initial_view.up[0] = %+.6ff; "
        "turntable_desc.initial_view.up[1] = %+.6ff; "
        "turntable_desc.initial_view.up[2] = %+.6ff;\n",
        state->world_up[0], state->world_up[1], state->world_up[2]);
    dvz_fprintf(fp, "turntable_desc.yaw_speed = %.6ff;\n", desc->yaw_speed);
    dvz_fprintf(fp, "turntable_desc.pitch_speed = %.6ff;\n", desc->pitch_speed);
    dvz_fprintf(fp, "turntable_desc.zoom_speed = %.6ff;\n", desc->zoom_speed);
    dvz_fprintf(fp, "turntable_desc.pan_speed = %.6ff;\n", desc->pan_speed);
    dvz_fprintf(fp, "turntable_desc.min_pitch = %+.6ff;\n", desc->min_pitch);
    dvz_fprintf(fp, "turntable_desc.max_pitch = %+.6ff;\n", desc->max_pitch);
    dvz_fprintf(fp, "turntable_desc.min_distance = %.6ff;\n", desc->min_distance);
    dvz_fprintf(fp, "turntable_desc.max_distance = %.6ff;\n", desc->max_distance);
    dvz_fprintf(fp, "turntable_desc.controller_flags = %uu;\n", desc->controller_flags);
}



/**
 * Dump one panzoom component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _panzoom_print(FILE* fp, const ExampleTunerPanzoom* state)
{
    ANN(fp);
    ANN(state);

    dvz_fprintf(fp, "/* panzoom: %s */\n", _tuner_name(state->name, "panzoom"));
    dvz_fprintf(
        fp, "dvz_panzoom_pan(panzoom, (vec2){%+.6ff, %+.6ff});\n", state->pan[0],
        state->pan[1]);
    dvz_fprintf(
        fp, "dvz_panzoom_zoom(panzoom, (vec2){%.6ff, %.6ff});\n", state->zoom[0],
        state->zoom[1]);
}



/**
 * Dump one camera descriptor.
 *
 * @param fp output stream
 * @param state component state
 */
static void _camera_print(FILE* fp, const ExampleTunerCamera* state)
{
    ANN(fp);
    ANN(state);

    const DvzCameraDesc* camera = &state->camera;
    dvz_fprintf(fp, "/* camera: %s */\n", _tuner_name(state->name, "camera"));
    dvz_fprintf(fp, "DvzCameraDesc camera_desc = dvz_camera_desc();\n");
    dvz_fprintf(fp, "camera_desc.projection.type = %d;\n", (int)camera->projection.type);
    dvz_fprintf(
        fp,
        "camera_desc.view.eye[0] = %+.6ff; camera_desc.view.eye[1] = %+.6ff; "
        "camera_desc.view.eye[2] = %+.6ff;\n",
        camera->view.eye[0], camera->view.eye[1], camera->view.eye[2]);
    dvz_fprintf(
        fp,
        "camera_desc.view.target[0] = %+.6ff; camera_desc.view.target[1] = %+.6ff; "
        "camera_desc.view.target[2] = %+.6ff;\n",
        camera->view.target[0], camera->view.target[1], camera->view.target[2]);
    dvz_fprintf(
        fp,
        "camera_desc.view.up[0] = %+.6ff; camera_desc.view.up[1] = %+.6ff; "
        "camera_desc.view.up[2] = %+.6ff;\n",
        camera->view.up[0], camera->view.up[1], camera->view.up[2]);
    dvz_fprintf(
        fp,
        "camera_desc.projection.fov_y = %.6ff; camera_desc.projection.near_clip = %.6ff; "
        "camera_desc.projection.far_clip = %.6ff;\n",
        camera->projection.fov_y, camera->projection.near_clip, camera->projection.far_clip);
    dvz_fprintf(fp, "camera_desc.projection.ortho_height = %.6ff;\n",
                camera->projection.ortho_height);
    if (state->panel != NULL)
        dvz_fprintf(fp, "(void)dvz_panel_set_camera_desc(panel, &camera_desc);\n");
}



/**
 * Dump one EDL component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _edl_print(FILE* fp, const ExampleTunerEdl* state)
{
    ANN(fp);
    ANN(state);
    ANN(state->controls);

    dvz_fprintf(fp, "/* EDL: %s */\n", _tuner_name(state->name, "edl"));
    if (!state->controls->enabled)
    {
        dvz_fprintf(fp, "(void)dvz_panel_set_edl(panel, NULL);\n");
        return;
    }
    dvz_fprintf(fp, "DvzEdlDesc edl = dvz_edl_desc();\n");
    dvz_fprintf(fp, "edl.radius = %.6ff;\n", state->controls->radius);
    dvz_fprintf(fp, "edl.strength = %.6ff;\n", state->controls->strength);
    dvz_fprintf(fp, "edl.depth_scale = %.6ff;\n", state->controls->depth_scale);
    dvz_fprintf(fp, "(void)dvz_panel_set_edl(panel, &edl);\n");
}



/**
 * Convert MSAA controls to a descriptor.
 *
 * @param controls controls
 * @return MSAA descriptor
 */
static DvzMsaaDesc _msaa_desc_from_controls(const DvzExampleGuiMsaaControls* controls)
{
    DvzMsaaDesc desc = dvz_msaa_desc();
    if (controls == NULL)
        return desc;
    desc.enabled = controls->enabled;
    desc.sample_count = (uint32_t)(controls->samples + 0.5f);
    desc.alpha_to_coverage = controls->alpha_to_coverage;
    return desc;
}



/**
 * Convert AO controls to a descriptor.
 *
 * @param controls controls
 * @return AO descriptor
 */
static DvzAoDesc _ao_desc_from_controls(const DvzExampleGuiAoControls* controls)
{
    DvzAoDesc desc = dvz_ao_desc();
    if (controls == NULL)
        return desc;
    desc.radius = controls->radius;
    desc.intensity = controls->intensity;
    desc.thickness = controls->thickness;
    desc.min_visibility = controls->min_visibility;
    int quality = (int)(controls->quality + 0.5f);
    if (quality < (int)DVZ_AO_QUALITY_LOW)
        quality = (int)DVZ_AO_QUALITY_LOW;
    if (quality > (int)DVZ_AO_QUALITY_ULTRA)
        quality = (int)DVZ_AO_QUALITY_ULTRA;
    desc.quality = (DvzAoQuality)quality;
    desc.debug_mode = controls->debug_mode;
    return desc;
}



/**
 * Apply MSAA controls to their panel.
 *
 * @param msaa MSAA component state
 */
static void _msaa_apply(ExampleTunerMsaa* msaa)
{
    if (msaa == NULL || msaa->panel == NULL || msaa->controls == NULL)
        return;
    if (!msaa->controls->enabled)
    {
        (void)dvz_panel_set_msaa(msaa->panel, NULL);
        return;
    }
    DvzMsaaDesc desc = _msaa_desc_from_controls(msaa->controls);
    (void)dvz_panel_set_msaa(msaa->panel, &desc);
}



/**
 * Apply AO controls to their panel.
 *
 * @param ao AO component state
 */
static void _ao_apply(ExampleTunerAo* ao)
{
    if (ao == NULL || ao->panel == NULL || ao->controls == NULL)
        return;
    if (!ao->controls->enabled)
    {
        (void)dvz_panel_set_ao(ao->panel, NULL);
        return;
    }
    DvzAoDesc desc = _ao_desc_from_controls(ao->controls);
    (void)dvz_panel_set_ao(ao->panel, &desc);
}



/**
 * Apply material controls to their visual.
 *
 * @param material material component state
 */
static void _material_apply(ExampleTunerMaterial* material)
{
    if (material == NULL || material->visual == NULL || material->material == NULL)
        return;
    (void)dvz_visual_set_material(material->visual, material->material);
}



/**
 * Apply depth-cue controls to their visual.
 *
 * @param cue depth-cue component state
 */
static void _depth_cue_apply(ExampleTunerDepthCue* cue)
{
    if (cue == NULL || cue->visual == NULL || cue->enabled == NULL || cue->desc == NULL)
        return;
    (void)dvz_visual_set_depth_cue(cue->visual, *cue->enabled ? cue->desc : NULL);
}



/**
 * Apply volume controls to their visual.
 *
 * @param volume volume component state
 */
static void _volume_apply(ExampleTunerVolume* volume)
{
    if (volume == NULL || volume->visual == NULL)
        return;

    (void)dvz_volume_set_render_mode(volume->visual, volume->state.render_mode);
    (void)dvz_volume_set_sampling(volume->visual, volume->state.sampling);
    (void)dvz_volume_set_opacity(volume->visual, volume->state.opacity);
    (void)dvz_volume_set_step_count(volume->visual, volume->state.step_count);
    (void)dvz_volume_set_value_range(
        volume->visual, volume->state.value_min, volume->state.value_max);
    (void)dvz_volume_set_slice_axis(volume->visual, volume->state.slice_axis);
    (void)dvz_volume_set_slice_position(volume->visual, volume->state.slice_position);
}



/**
 * Dump one MSAA component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _msaa_print(FILE* fp, const ExampleTunerMsaa* state)
{
    ANN(fp);
    ANN(state);
    ANN(state->controls);

    dvz_fprintf(fp, "/* MSAA: %s */\n", _tuner_name(state->name, "msaa"));
    if (!state->controls->enabled)
    {
        dvz_fprintf(fp, "(void)dvz_panel_set_msaa(panel, NULL);\n");
        return;
    }
    dvz_fprintf(fp, "DvzMsaaDesc msaa = dvz_msaa_desc();\n");
    dvz_fprintf(fp, "msaa.enabled = true;\n");
    dvz_fprintf(fp, "msaa.sample_count = %uu;\n", (uint32_t)(state->controls->samples + 0.5f));
    dvz_fprintf(
        fp, "msaa.alpha_to_coverage = %s;\n",
        state->controls->alpha_to_coverage ? "true" : "false");
    dvz_fprintf(fp, "(void)dvz_panel_set_msaa(panel, &msaa);\n");
}



/**
 * Dump one AO component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _ao_print(FILE* fp, const ExampleTunerAo* state)
{
    ANN(fp);
    ANN(state);
    ANN(state->controls);

    dvz_fprintf(fp, "/* AO: %s */\n", _tuner_name(state->name, "ao"));
    if (!state->controls->enabled)
    {
        dvz_fprintf(fp, "(void)dvz_panel_set_ao(panel, NULL);\n");
        return;
    }
    dvz_fprintf(fp, "DvzAoDesc ao = dvz_ao_desc();\n");
    dvz_fprintf(fp, "ao.radius = %.6ff;\n", state->controls->radius);
    dvz_fprintf(fp, "ao.intensity = %.6ff;\n", state->controls->intensity);
    dvz_fprintf(fp, "ao.thickness = %.6ff;\n", state->controls->thickness);
    dvz_fprintf(fp, "ao.min_visibility = %.6ff;\n", state->controls->min_visibility);
    dvz_fprintf(fp, "ao.quality = (DvzAoQuality)%d;\n", (int)(state->controls->quality + 0.5f));
    dvz_fprintf(fp, "ao.debug_mode = (DvzAoDebugMode)%d;\n", (int)state->controls->debug_mode);
    dvz_fprintf(fp, "(void)dvz_panel_set_ao(panel, &ao);\n");
}



/**
 * Dump one material component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _material_print(FILE* fp, const ExampleTunerMaterial* state)
{
    ANN(fp);
    ANN(state);
    ANN(state->material);

    const DvzMaterialDesc* material = state->material;
    dvz_fprintf(fp, "/* material: %s */\n", _tuner_name(state->name, "material"));
    dvz_fprintf(fp, "DvzMaterialDesc material = dvz_material_desc();\n");
    dvz_fprintf(fp, "material.model = %d;\n", (int)material->model);
    dvz_fprintf(fp, "material.alpha_mode = %d;\n", (int)material->alpha_mode);
    dvz_fprintf(fp, "material.opacity = %.6ff;\n", material->opacity);
    dvz_fprintf(
        fp,
        "material.base_color_factor[0] = %.6ff; material.base_color_factor[1] = %.6ff; "
        "material.base_color_factor[2] = %.6ff; material.base_color_factor[3] = %.6ff;\n",
        material->base_color_factor[0], material->base_color_factor[1],
        material->base_color_factor[2], material->base_color_factor[3]);
    dvz_fprintf(
        fp,
        "material.light_direction[0] = %+.6ff; material.light_direction[1] = %+.6ff; "
        "material.light_direction[2] = %+.6ff;\n",
        material->light_direction[0], material->light_direction[1],
        material->light_direction[2]);
    dvz_fprintf(fp, "material.phong.ambient = %.6ff;\n", material->phong.ambient);
    dvz_fprintf(fp, "material.phong.diffuse = %.6ff;\n", material->phong.diffuse);
    dvz_fprintf(fp, "material.phong.specular = %.6ff;\n", material->phong.specular);
    dvz_fprintf(fp, "material.phong.shininess = %.6ff;\n", material->phong.shininess);
    dvz_fprintf(fp, "material.standard.roughness = %.6ff;\n", material->standard.roughness);
    dvz_fprintf(fp, "material.standard.specular = %.6ff;\n", material->standard.specular);
    dvz_fprintf(fp, "material.standard.metallic = %.6ff;\n", material->standard.metallic);
    dvz_fprintf(
        fp,
        "material.standard.emissive[0] = %.6ff; material.standard.emissive[1] = %.6ff; "
        "material.standard.emissive[2] = %.6ff;\n",
        material->standard.emissive[0], material->standard.emissive[1],
        material->standard.emissive[2]);
    dvz_fprintf(fp, "material.standard.rim_strength = %.6ff;\n", material->standard.rim_strength);
    if (state->visual != NULL)
        dvz_fprintf(fp, "(void)dvz_visual_set_material(visual, &material);\n");
}



/**
 * Dump one depth-cue component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _depth_cue_print(FILE* fp, const ExampleTunerDepthCue* state)
{
    ANN(fp);
    ANN(state);
    ANN(state->enabled);
    ANN(state->desc);

    dvz_fprintf(fp, "/* depth cue: %s */\n", _tuner_name(state->name, "depth_cue"));
    if (!*state->enabled)
    {
        dvz_fprintf(fp, "(void)dvz_visual_set_depth_cue(visual, NULL);\n");
        return;
    }
    const DvzDepthCueDesc* cue = state->desc;
    dvz_fprintf(fp, "DvzDepthCueDesc cue = dvz_depth_cue_desc();\n");
    dvz_fprintf(fp, "cue.mode = %d;\n", (int)cue->mode);
    dvz_fprintf(fp, "cue.metric = %d;\n", (int)cue->metric);
    dvz_fprintf(fp, "cue.falloff = %d;\n", (int)cue->falloff);
    dvz_fprintf(fp, "cue.near_depth = %.6ff;\n", cue->near_depth);
    dvz_fprintf(fp, "cue.far_depth = %.6ff;\n", cue->far_depth);
    dvz_fprintf(fp, "cue.strength = %.6ff;\n", cue->strength);
    dvz_fprintf(fp, "cue.density = %.6ff;\n", cue->density);
    dvz_fprintf(
        fp,
        "cue.background_color[0] = %.6ff; cue.background_color[1] = %.6ff; "
        "cue.background_color[2] = %.6ff; cue.background_color[3] = %.6ff;\n",
        cue->background_color[0], cue->background_color[1], cue->background_color[2],
        cue->background_color[3]);
    dvz_fprintf(fp, "(void)dvz_visual_set_depth_cue(visual, &cue);\n");
}



/**
 * Dump one volume component.
 *
 * @param fp output stream
 * @param state component state
 */
static void _volume_print(FILE* fp, const ExampleTunerVolume* state)
{
    ANN(fp);
    ANN(state);
    const DvzVolumeState* volume = &state->state;

    dvz_fprintf(fp, "/* volume: %s */\n", _tuner_name(state->name, "volume"));
    dvz_fprintf(fp, "(void)dvz_volume_set_render_mode(volume, %d);\n", (int)volume->render_mode);
    dvz_fprintf(fp, "(void)dvz_volume_set_sampling(volume, %d);\n", (int)volume->sampling);
    dvz_fprintf(fp, "(void)dvz_volume_set_opacity(volume, %.6ff);\n", volume->opacity);
    dvz_fprintf(fp, "(void)dvz_volume_set_step_count(volume, %uu);\n", volume->step_count);
    dvz_fprintf(
        fp, "(void)dvz_volume_set_value_range(volume, %.6f, %.6f);\n", volume->value_min,
        volume->value_max);
    dvz_fprintf(fp, "(void)dvz_volume_set_slice_axis(volume, %d);\n", (int)volume->slice_axis);
    dvz_fprintf(
        fp, "(void)dvz_volume_set_slice_position(volume, %.6f);\n",
        volume->slice_position);
}



/*************************************************************************************************/
/*  Component callbacks                                                                          */
/*************************************************************************************************/

static void _arcball_sync(void* user)
{
    ExampleTunerArcball* state = (ExampleTunerArcball*)user;
    if (state == NULL || state->arcball == NULL)
        return;

    dvz_arcball_angles(state->arcball, state->angles);
    DvzArcballState arcball = {0};
    if (dvz_arcball_state(state->arcball, &arcball))
    {
        state->zoom = arcball.zoom;
        state->pan[0] = arcball.pan[0];
        state->pan[1] = arcball.pan[1];
    }
}


static bool _arcball_gui(DvzGui* gui, void* user)
{
    ExampleTunerArcball* state = (ExampleTunerArcball*)user;
    if (gui == NULL || state == NULL)
        return false;

    bool changed = false;
    changed |= dvz_gui_slider_float3(gui, "Angles", state->angles, -3.14159f, +3.14159f);
    changed |= dvz_gui_slider_float(gui, "Zoom", &state->zoom, 0.20f, 4.0f);
    changed |= dvz_gui_slider_float2(gui, "Pan", state->pan, -2.0f, +2.0f);
    return changed;
}


static void _arcball_apply(void* user)
{
    ExampleTunerArcball* state = (ExampleTunerArcball*)user;
    if (state == NULL || state->arcball == NULL)
        return;

    (void)dvz_arcball_set(state->arcball, state->angles);
    (void)dvz_arcball_zoom(state->arcball, state->zoom);
    (void)dvz_arcball_pan(state->arcball, state->pan);
}


static void _arcball_reset(void* user)
{
    ExampleTunerArcball* state = (ExampleTunerArcball*)user;
    if (state == NULL || state->arcball == NULL)
        return;

    _copy_vec3(state->angles, state->reset_angles);
    _copy_vec2(state->pan, state->reset_pan);
    state->zoom = state->reset_zoom;
    _arcball_apply(state);
}


static void _arcball_print_cb(FILE* fp, void* user)
{
    ExampleTunerArcball* state = (ExampleTunerArcball*)user;
    if (state != NULL)
        _arcball_print(fp, state);
}


static void _turntable_sync(void* user)
{
    ExampleTunerTurntable* state = (ExampleTunerTurntable*)user;
    if (state == NULL || state->camera == NULL)
        return;

    dvz_camera_get_view(state->camera, &state->view);
    (void)_turntable_pose_from_view(
        &state->view, state->world_up, state->pivot, &state->yaw, &state->pitch, &state->distance);
}


static bool _turntable_gui(DvzGui* gui, void* user)
{
    ExampleTunerTurntable* state = (ExampleTunerTurntable*)user;
    if (gui == NULL || state == NULL)
        return false;

    const DvzTurntableDesc* desc = &state->reset_desc;
    const float min_pitch = desc->max_pitch > desc->min_pitch ? desc->min_pitch : -1.5707f;
    const float max_pitch = desc->max_pitch > desc->min_pitch ? desc->max_pitch : +1.5707f;
    const float min_distance = desc->min_distance > 0.0f ? desc->min_distance : 0.01f;
    const float max_distance = desc->max_distance > min_distance
                                   ? desc->max_distance
                                   : fmaxf(min_distance + 1.0f, state->distance * 4.0f);

    bool changed = false;
    changed |=
        dvz_gui_slider_float_format(gui, "Yaw", &state->yaw, -3.14159f, +3.14159f, "%.3f rad");
    changed |=
        dvz_gui_slider_float_format(gui, "Pitch", &state->pitch, min_pitch, max_pitch, "%.3f rad");
    changed |= dvz_gui_slider_float_format(
        gui, "Distance", &state->distance, min_distance, max_distance, "%.3f");
    changed |= dvz_gui_slider_float3(gui, "Pivot", state->pivot, -10.0f, +10.0f);
    return changed;
}


static void _turntable_apply(void* user)
{
    ExampleTunerTurntable* state = (ExampleTunerTurntable*)user;
    if (state == NULL || state->turntable == NULL || state->camera == NULL)
        return;

    (void)dvz_turntable_pivot(state->turntable, state->pivot);

    DvzCameraView baseline_view = {0};
    vec3 baseline_pivot = {0};
    float baseline_yaw = 0.0f;
    float baseline_pitch = 0.0f;
    float baseline_distance = 0.0f;
    dvz_camera_get_view(state->camera, &baseline_view);
    if (!_turntable_pose_from_view(
            &baseline_view, state->world_up, baseline_pivot, &baseline_yaw, &baseline_pitch,
            &baseline_distance))
        return;

    float pitch_delta = state->pitch - baseline_pitch;
    if ((state->reset_desc.controller_flags & DVZ_TURNTABLE_FLAGS_INVERT_Y) != 0)
        pitch_delta = -pitch_delta;
    (void)dvz_turntable_orbit(state->turntable, state->yaw - baseline_yaw, pitch_delta);
    (void)dvz_turntable_dolly(state->turntable, state->distance - baseline_distance);
    _turntable_sync(state);
}


static void _turntable_reset(void* user)
{
    ExampleTunerTurntable* state = (ExampleTunerTurntable*)user;
    if (state == NULL || state->turntable == NULL)
        return;

    (void)dvz_turntable_reset(state->turntable);
    _turntable_sync(state);
}


static void _turntable_print_cb(FILE* fp, void* user)
{
    ExampleTunerTurntable* state = (ExampleTunerTurntable*)user;
    if (state != NULL)
        _turntable_print(fp, state);
}


static void _panzoom_sync(void* user)
{
    ExampleTunerPanzoom* state = (ExampleTunerPanzoom*)user;
    if (state == NULL || state->panzoom == NULL)
        return;

    DvzPanzoomState panzoom = {0};
    if (dvz_panzoom_state(state->panzoom, &panzoom))
    {
        state->pan[0] = panzoom.pan[0];
        state->pan[1] = panzoom.pan[1];
        state->zoom[0] = panzoom.zoom[0];
        state->zoom[1] = panzoom.zoom[1];
    }
}


static bool _panzoom_gui(DvzGui* gui, void* user)
{
    ExampleTunerPanzoom* state = (ExampleTunerPanzoom*)user;
    if (gui == NULL || state == NULL)
        return false;

    bool changed = false;
    changed |= dvz_gui_slider_float2(gui, "Pan", state->pan, -2.0f, +2.0f);
    changed |= dvz_gui_slider_float2(gui, "Zoom", state->zoom, 0.05f, 32.0f);
    return changed;
}


static void _panzoom_apply(void* user)
{
    ExampleTunerPanzoom* state = (ExampleTunerPanzoom*)user;
    if (state == NULL || state->panzoom == NULL)
        return;

    (void)dvz_panzoom_pan(state->panzoom, state->pan);
    (void)dvz_panzoom_zoom(state->panzoom, state->zoom);
}


static void _panzoom_reset(void* user)
{
    ExampleTunerPanzoom* state = (ExampleTunerPanzoom*)user;
    if (state == NULL || state->panzoom == NULL)
        return;

    _copy_vec2(state->pan, state->reset_pan);
    _copy_vec2(state->zoom, state->reset_zoom);
    _panzoom_apply(state);
}


static void _panzoom_print_cb(FILE* fp, void* user)
{
    ExampleTunerPanzoom* state = (ExampleTunerPanzoom*)user;
    if (state != NULL)
        _panzoom_print(fp, state);
}


static void _camera_sync(void* user)
{
    ExampleTunerCamera* state = (ExampleTunerCamera*)user;
    if (state == NULL || state->camera_ref == NULL)
        return;

    dvz_camera_get_view(state->camera_ref, &state->camera.view);
    dvz_camera_get_projection(state->camera_ref, &state->camera.projection);
}


static bool _camera_gui(DvzGui* gui, void* user)
{
    ExampleTunerCamera* state = (ExampleTunerCamera*)user;
    if (gui == NULL || state == NULL)
        return false;

    bool changed = false;
    changed |= dvz_gui_slider_float3(gui, "Eye", state->camera.view.eye, -10.0f, +10.0f);
    changed |= dvz_gui_slider_float3(gui, "Target", state->camera.view.target, -10.0f, +10.0f);
    changed |= dvz_gui_slider_float3(gui, "Up", state->camera.view.up, -1.0f, +1.0f);
    changed |= dvz_gui_slider_float(
        gui, "FOV Y", &state->camera.projection.fov_y, 0.10f, 2.40f);
    changed |= dvz_gui_slider_float(
        gui, "Near clip", &state->camera.projection.near_clip, 0.001f, 10.0f);
    changed |= dvz_gui_slider_float(
        gui, "Far clip", &state->camera.projection.far_clip, 1.0f, 10000.0f);
    changed |= dvz_gui_slider_float(
        gui, "Ortho height", &state->camera.projection.ortho_height, 0.01f, 100.0f);
    return changed;
}


static void _camera_apply(void* user)
{
    ExampleTunerCamera* state = (ExampleTunerCamera*)user;
    if (state == NULL || state->panel == NULL)
        return;
    (void)dvz_panel_set_camera_desc(state->panel, &state->camera);
}


static void _camera_reset(void* user)
{
    ExampleTunerCamera* state = (ExampleTunerCamera*)user;
    if (state == NULL)
        return;
    state->camera = state->reset_camera;
    _camera_apply(state);
}


static void _camera_print_cb(FILE* fp, void* user)
{
    ExampleTunerCamera* state = (ExampleTunerCamera*)user;
    if (state != NULL)
        _camera_print(fp, state);
}


static bool _edl_gui(DvzGui* gui, void* user)
{
    ExampleTunerEdl* state = (ExampleTunerEdl*)user;
    if (gui == NULL || state == NULL || state->controls == NULL)
        return false;
    return example_gui_edl(gui, state->controls);
}


static void _edl_apply_cb(void* user)
{
    _edl_apply((ExampleTunerEdl*)user);
}


static void _edl_reset(void* user)
{
    ExampleTunerEdl* state = (ExampleTunerEdl*)user;
    if (state == NULL || state->controls == NULL)
        return;
    *state->controls = state->reset_controls;
    _edl_apply(state);
}


static void _edl_print_cb(FILE* fp, void* user)
{
    ExampleTunerEdl* state = (ExampleTunerEdl*)user;
    if (state != NULL && state->controls != NULL)
        _edl_print(fp, state);
}


static bool _msaa_gui(DvzGui* gui, void* user)
{
    ExampleTunerMsaa* state = (ExampleTunerMsaa*)user;
    if (gui == NULL || state == NULL || state->controls == NULL)
        return false;
    return example_gui_msaa(gui, state->controls);
}


static void _msaa_apply_cb(void* user)
{
    _msaa_apply((ExampleTunerMsaa*)user);
}


static void _msaa_reset(void* user)
{
    ExampleTunerMsaa* state = (ExampleTunerMsaa*)user;
    if (state == NULL || state->controls == NULL)
        return;
    *state->controls = state->reset_controls;
    _msaa_apply(state);
}


static void _msaa_print_cb(FILE* fp, void* user)
{
    ExampleTunerMsaa* state = (ExampleTunerMsaa*)user;
    if (state != NULL && state->controls != NULL)
        _msaa_print(fp, state);
}


static bool _ao_gui(DvzGui* gui, void* user)
{
    ExampleTunerAo* state = (ExampleTunerAo*)user;
    if (gui == NULL || state == NULL || state->controls == NULL)
        return false;
    return example_gui_ao(gui, state->controls);
}


static void _ao_apply_cb(void* user)
{
    _ao_apply((ExampleTunerAo*)user);
}


static void _ao_reset(void* user)
{
    ExampleTunerAo* state = (ExampleTunerAo*)user;
    if (state == NULL || state->controls == NULL)
        return;
    *state->controls = state->reset_controls;
    _ao_apply(state);
}


static void _ao_print_cb(FILE* fp, void* user)
{
    ExampleTunerAo* state = (ExampleTunerAo*)user;
    if (state != NULL && state->controls != NULL)
        _ao_print(fp, state);
}


static bool _material_gui(DvzGui* gui, void* user)
{
    ExampleTunerMaterial* state = (ExampleTunerMaterial*)user;
    if (gui == NULL || state == NULL || state->material == NULL)
        return false;

    static const char* const material_models[] = {"Unlit", "Phong", "Standard"};
    static const char* const alpha_modes[] = {"Opaque", "Blended", "WBOIT", "Depth peel", "Mask"};

    DvzMaterialDesc* material = state->material;
    bool changed = false;

    int model = _clamp_int((int)material->model, 0, 2);
    if (dvz_gui_combo(gui, "Model", &model, material_models, 3))
    {
        material->model = (DvzMaterialModel)model;
        changed = true;
    }

    int alpha_mode = _clamp_int((int)material->alpha_mode, 0, 4);
    if (dvz_gui_combo(gui, "Alpha", &alpha_mode, alpha_modes, 5))
    {
        material->alpha_mode = (DvzAlphaMode)alpha_mode;
        changed = true;
    }

    changed |= dvz_gui_slider_float(gui, "Opacity", &material->opacity, 0.0f, 1.0f);
    changed |= dvz_gui_color_edit4(gui, "Base color", material->base_color_factor, 0);

    if (material->model != DVZ_MATERIAL_MODEL_UNLIT)
        changed |=
            dvz_gui_slider_float3(gui, "Light direction", material->light_direction, -1.0f, +1.0f);

    if (material->model == DVZ_MATERIAL_MODEL_PHONG)
    {
        changed |= dvz_gui_slider_float(gui, "Ambient", &material->phong.ambient, 0.0f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Diffuse", &material->phong.diffuse, 0.0f, 1.5f);
        changed |= dvz_gui_slider_float(gui, "Specular", &material->phong.specular, 0.0f, 1.5f);
        changed |=
            dvz_gui_slider_float(gui, "Shininess", &material->phong.shininess, 1.0f, 160.0f);
    }
    else if (material->model == DVZ_MATERIAL_MODEL_STANDARD)
    {
        changed |=
            dvz_gui_slider_float(gui, "Roughness", &material->standard.roughness, 0.02f, 1.0f);
        changed |= dvz_gui_slider_float(gui, "Specular", &material->standard.specular, 0.0f, 1.5f);
        changed |= dvz_gui_slider_float(gui, "Metallic", &material->standard.metallic, 0.0f, 1.0f);
        changed |=
            dvz_gui_slider_float3(gui, "Emissive", material->standard.emissive, 0.0f, 4.0f);
        changed |=
            dvz_gui_slider_float(gui, "Rim", &material->standard.rim_strength, 0.0f, 1.0f);
    }

    return changed;
}


static void _material_apply_cb(void* user)
{
    _material_apply((ExampleTunerMaterial*)user);
}


static void _material_reset(void* user)
{
    ExampleTunerMaterial* state = (ExampleTunerMaterial*)user;
    if (state == NULL || state->material == NULL)
        return;
    *state->material = state->reset_material;
    _material_apply(state);
}


static void _material_print_cb(FILE* fp, void* user)
{
    ExampleTunerMaterial* state = (ExampleTunerMaterial*)user;
    if (state != NULL && state->material != NULL)
        _material_print(fp, state);
}


static bool _depth_cue_gui(DvzGui* gui, void* user)
{
    ExampleTunerDepthCue* state = (ExampleTunerDepthCue*)user;
    if (gui == NULL || state == NULL || state->enabled == NULL || state->desc == NULL)
        return false;

    static const char* const modes[] = {"None", "Fade", "Desaturate", "Darken"};
    static const char* const metrics[] = {"Clip depth", "Eye distance", "World distance"};
    static const char* const falloffs[] = {"Linear", "Exponential"};

    DvzDepthCueDesc* cue = state->desc;
    bool changed = false;
    changed |= dvz_gui_checkbox(gui, "Enable", state->enabled);

    int mode = _clamp_int((int)cue->mode, 0, 3);
    if (dvz_gui_combo(gui, "Mode", &mode, modes, 4))
    {
        cue->mode = (DvzDepthCueMode)mode;
        changed = true;
    }

    int metric = _clamp_int((int)cue->metric, 0, 2);
    if (dvz_gui_combo(gui, "Metric", &metric, metrics, 3))
    {
        cue->metric = (DvzDepthCueMetric)metric;
        changed = true;
    }

    int falloff = _clamp_int((int)cue->falloff, 0, 1);
    if (dvz_gui_combo(gui, "Falloff", &falloff, falloffs, 2))
    {
        cue->falloff = (DvzDepthCueFalloff)falloff;
        changed = true;
    }

    changed |= dvz_gui_slider_float(gui, "Near depth", &cue->near_depth, 0.0f, 16.0f);
    changed |= dvz_gui_slider_float(gui, "Far depth", &cue->far_depth, 0.0f, 16.0f);
    changed |= dvz_gui_slider_float(gui, "Strength", &cue->strength, 0.0f, 1.0f);
    changed |= dvz_gui_slider_float(gui, "Density", &cue->density, 0.0f, 8.0f);
    changed |= dvz_gui_color_edit4(gui, "Background", cue->background_color, 0);

    if (changed)
        _depth_cue_clamp(cue);
    return changed;
}


static void _depth_cue_apply_cb(void* user)
{
    ExampleTunerDepthCue* state = (ExampleTunerDepthCue*)user;
    if (state != NULL)
        _depth_cue_clamp(state->desc);
    _depth_cue_apply(state);
}


static void _depth_cue_reset(void* user)
{
    ExampleTunerDepthCue* state = (ExampleTunerDepthCue*)user;
    if (state == NULL || state->enabled == NULL || state->desc == NULL)
        return;
    *state->enabled = state->reset_enabled;
    *state->desc = state->reset_desc;
    _depth_cue_apply(state);
}


static void _depth_cue_print_cb(FILE* fp, void* user)
{
    ExampleTunerDepthCue* state = (ExampleTunerDepthCue*)user;
    if (state != NULL && state->enabled != NULL && state->desc != NULL)
        _depth_cue_print(fp, state);
}


static bool _volume_gui(DvzGui* gui, void* user)
{
    ExampleTunerVolume* state = (ExampleTunerVolume*)user;
    if (gui == NULL || state == NULL)
        return false;

    static const char* const render_modes[] = {"Slice", "MIP", "Composite"};
    static const char* const sampling_modes[] = {"Linear", "Nearest"};
    static const char* const axes[] = {"X", "Y", "Z"};

    DvzVolumeState* volume = &state->state;
    bool changed = false;

    int render_mode = _clamp_int((int)volume->render_mode, 0, 2);
    if (dvz_gui_combo(gui, "Render mode", &render_mode, render_modes, 3))
    {
        volume->render_mode = (DvzVolumeRenderMode)render_mode;
        changed = true;
    }

    int sampling = _clamp_int((int)volume->sampling, 0, 1);
    if (dvz_gui_combo(gui, "Sampling", &sampling, sampling_modes, 2))
    {
        volume->sampling = (DvzVolumeSamplingMode)sampling;
        changed = true;
    }

    changed |= dvz_gui_slider_float(gui, "Opacity", &volume->opacity, 0.0f, 1.0f);

    int step_count = (int)volume->step_count;
    if (dvz_gui_slider_int(gui, "Steps", &step_count, 1, 512))
    {
        volume->step_count = (uint32_t)_clamp_int(step_count, 1, 512);
        changed = true;
    }

    float value_min = (float)volume->value_min;
    float value_max = (float)volume->value_max;
    if (dvz_gui_slider_range_float(gui, "Value range", &value_min, &value_max, 0.0f, 1.0f, "%.3f"))
    {
        volume->value_min = (double)value_min;
        volume->value_max = (double)value_max;
        changed = true;
    }

    int axis = _clamp_int((int)volume->slice_axis, 0, 2);
    if (dvz_gui_combo(gui, "Slice axis", &axis, axes, 3))
    {
        volume->slice_axis = (DvzVolumeAxis)axis;
        changed = true;
    }
    float slice_position = (float)volume->slice_position;
    if (dvz_gui_slider_float(gui, "Slice", &slice_position, 0.0f, 1.0f))
    {
        volume->slice_position = slice_position;
        changed = true;
    }

    if (changed)
        _volume_clamp(volume);
    return changed;
}


static void _volume_apply_cb(void* user)
{
    ExampleTunerVolume* state = (ExampleTunerVolume*)user;
    if (state != NULL)
        _volume_clamp(&state->state);
    _volume_apply(state);
}


static void _volume_reset(void* user)
{
    ExampleTunerVolume* state = (ExampleTunerVolume*)user;
    if (state == NULL)
        return;
    state->state = state->reset_state;
    _volume_apply(state);
}


static void _volume_print_cb(FILE* fp, void* user)
{
    ExampleTunerVolume* state = (ExampleTunerVolume*)user;
    if (state != NULL)
        _volume_print(fp, state);
}



/*************************************************************************************************/
/*  Tuner callbacks                                                                              */
/*************************************************************************************************/

/**
 * Save a debug screenshot beside the example executable.
 *
 * @param tuner tuner
 */
static void _tuner_screenshot(ExampleTuner* tuner)
{
    ANN(tuner);
    if (tuner->view == NULL)
        return;

    char name[128] = {0};
    char path[1024] = {0};
    dvz_snprintf(
        name, sizeof(name), "%s_tuner_%04" PRIu32 ".png", _tuner_name(tuner->name, "example"),
        tuner->screenshot_index++);
    example_outpath(NULL, name, path, sizeof(path));

    if (dvz_view_capture_png(tuner->view, path) == 0)
        dvz_fprintf(stderr, "example tuner: saved %s\n", path);
    else
        dvz_fprintf(stderr, "example tuner: failed to save %s\n", path);
}



/**
 * Draw the tuner GUI.
 *
 * @param gui GUI
 * @param view view
 * @param user_data ExampleTuner pointer
 */
static void _tuner_gui(DvzGui* gui, DvzView* view, void* user_data)
{
    (void)view;
    ExampleTuner* tuner = (ExampleTuner*)user_data;
    if (gui == NULL || tuner == NULL)
        return;

    const char* title = _tuner_name(tuner->name, "Example settings");
    if (tuner->layout.dock_initially)
    {
        (void)dvz_gui_dock_window_once(
            gui, title, _tuner_valid_dock_slot(tuner->layout.dock_slot),
            tuner->layout.dock_size_px);
    }

    example_tuner_sync(tuner);
    if (dvz_gui_begin(gui, title, NULL, 0))
    {
        _tuner_update_figure_reserve(tuner, gui);
        for (uint32_t i = 0; i < tuner->component_count; i++)
        {
            ExampleTunerComponent* component = &tuner->components[i];
            if (component->gui == NULL)
                continue;

            igPushID_Int((int)i);
            dvz_gui_separator_text(gui, _tuner_name(component->title, "Component"));
            const bool changed = component->gui(gui, component->user);
            if (changed && component->apply != NULL)
                component->apply(component->user);
            igPopID();
        }

        dvz_gui_separator_text(gui, "Actions");
        if (dvz_gui_button(gui, "Print C defaults"))
            example_tuner_print_c(tuner, stdout);
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        if (dvz_gui_button(gui, "Reset"))
            example_tuner_reset(tuner);
        dvz_gui_same_line(gui, 0.0f, 8.0f);
        if (dvz_gui_button(gui, "Save PNG"))
            _tuner_screenshot(tuner);
    }
    else
    {
        _tuner_update_figure_reserve(tuner, gui);
    }
    dvz_gui_end(gui);
}



/**
 * Handle tuner keyboard shortcuts.
 *
 * @param router input router
 * @param event keyboard event
 * @param user_data ExampleTuner pointer
 */
static void
_tuner_keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    ExampleTuner* tuner = (ExampleTuner*)user_data;
    if (tuner == NULL || event == NULL || event->type != DVZ_KEYBOARD_EVENT_PRESS)
        return;

    switch (event->key)
    {
    case DVZ_KEY_D:
        example_tuner_print_c(tuner, stdout);
        break;
    case DVZ_KEY_S:
        _tuner_screenshot(tuner);
        break;
    case DVZ_KEY_R:
        example_tuner_reset(tuner);
        break;
    default:
        break;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Initialize an example tuner.
 *
 * @param name GUI window and output label
 * @return initialized tuner
 */
ExampleTuner example_tuner(const char* name)
{
    return (ExampleTuner){.name = name, .layout = _tuner_default_layout()};
}



/**
 * Register the figure whose content should avoid an initially docked tuner.
 *
 * @param tuner tuner
 * @param figure figure rendered by the native view
 */
void example_tuner_figure(ExampleTuner* tuner, DvzFigure* figure)
{
    if (tuner == NULL)
        return;
    if (tuner->reserve_applied)
        _tuner_restore_figure_reserve(tuner);
    tuner->figure = figure;
    tuner->has_previous_figure_reserve = false;
    tuner->reserve_applied = false;
    tuner->previous_figure_reserve = (DvzPanelReserve){0};
    tuner->applied_figure_reserve = (DvzPanelReserve){0};
}



/**
 * Override the tuner window's default docking and reserve behavior.
 *
 * @param tuner tuner
 * @param layout layout policy, or NULL to restore defaults
 */
void example_tuner_layout(ExampleTuner* tuner, const ExampleTunerLayout* layout)
{
    if (tuner == NULL)
        return;
    tuner->layout = layout != NULL ? *layout : _tuner_default_layout();
    tuner->layout.dock_slot = _tuner_valid_dock_slot(tuner->layout.dock_slot);
    if (tuner->layout.dock_size_px <= 0.0f || !isfinite(tuner->layout.dock_size_px))
        tuner->layout.dock_size_px = EXAMPLE_TUNER_DEFAULT_DOCK_WIDTH_PX;
}



/**
 * Attach the tuner to a native view.
 *
 * @param tuner tuner
 * @param view native app view
 * @return true when the tuner was attached
 */
bool example_tuner_attach(ExampleTuner* tuner, DvzView* view)
{
    if (tuner == NULL || view == NULL)
        return false;

    tuner->view = view;
    DvzGui* gui = dvz_view_gui(view, NULL);
    if (gui == NULL)
        return false;
    dvz_view_set_gui_callback(view, _tuner_gui, tuner);
    _tuner_snapshot_figure_reserve(tuner);

    tuner->input = dvz_view_input(view);
    if (tuner->input != NULL)
        tuner->input_subscription_id =
            dvz_input_subscribe_keyboard(tuner->input, _tuner_keyboard, tuner);
    tuner->installed = true;
    dvz_fprintf(stderr, "example tuner: D print C defaults, S save PNG, R reset\n");
    return true;
}



/**
 * Detach the tuner from its native view.
 *
 * @param tuner tuner
 */
void example_tuner_detach(ExampleTuner* tuner)
{
    if (tuner == NULL)
        return;

    DvzInputRouter* input = tuner->view != NULL ? dvz_view_input(tuner->view) : NULL;
    if (tuner->installed && input != NULL && input == tuner->input)
        dvz_input_unsubscribe(input, tuner->input_subscription_id);
    if (tuner->view != NULL)
        dvz_view_set_gui_callback(tuner->view, NULL, NULL);
    _tuner_restore_figure_reserve(tuner);

    tuner->input_subscription_id = DVZ_CALLBACK_ID_NONE;
    tuner->input = NULL;
    tuner->view = NULL;
    tuner->installed = false;
    tuner->has_previous_figure_reserve = false;
}



/**
 * Register one generic tuner component.
 *
 * @param tuner tuner
 * @param title component title
 * @param user component state
 * @param sync optional sync callback
 * @param gui optional GUI callback
 * @param apply optional apply callback
 * @param reset optional reset callback
 * @param print_c optional pasteable C dump callback
 * @return true when the component was registered
 */
bool example_tuner_add_component(
    ExampleTuner* tuner,
    const char* title,
    void* user,
    ExampleTunerSyncFn sync,
    ExampleTunerGuiFn gui,
    ExampleTunerApplyFn apply,
    ExampleTunerResetFn reset,
    ExampleTunerPrintFn print_c)
{
    if (tuner == NULL || tuner->component_count >= EXAMPLE_TUNER_MAX_COMPONENTS)
        return false;

    tuner->components[tuner->component_count++] = (ExampleTunerComponent){
        .title = title,
        .user = user,
        .sync = sync,
        .gui = gui,
        .apply = apply,
        .reset = reset,
        .print_c = print_c,
    };
    return true;
}



/**
 * Register an arcball tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param arcball arcball controller
 * @param reset_angles reset angles
 * @param reset_zoom reset zoom
 * @param reset_pan reset pan
 */
void example_tuner_arcball(
    ExampleTuner* tuner,
    const char* name,
    DvzArcball* arcball,
    const float reset_angles[3],
    float reset_zoom,
    const float reset_pan[2])
{
    if (tuner == NULL || arcball == NULL || tuner->arcball_count >= EXAMPLE_TUNER_MAX_ARCBALLS)
        return;

    ExampleTunerArcball* state = &tuner->arcballs[tuner->arcball_count++];
    state->name = name;
    state->arcball = arcball;
    _copy_vec3(state->reset_angles, reset_angles);
    _copy_vec2(state->reset_pan, reset_pan);
    state->reset_zoom = reset_zoom > 0.0f ? reset_zoom : 1.0f;
    _arcball_reset(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Arcball"), state, _arcball_sync, _arcball_gui, _arcball_apply,
        _arcball_reset, _arcball_print_cb);
}



/**
 * Register a turntable tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param turntable live turntable controller
 * @param panel panel whose camera is controlled by the turntable
 * @param desc turntable reset descriptor
 */
void example_tuner_turntable(
    ExampleTuner* tuner, const char* name, DvzTurntable* turntable, DvzPanel* panel,
    const DvzTurntableDesc* desc)
{
    if (tuner == NULL || turntable == NULL || panel == NULL || desc == NULL ||
        tuner->turntable_count >= EXAMPLE_TUNER_MAX_TURNTABLES)
        return;

    DvzCamera* camera = dvz_panel_camera(panel);
    if (camera == NULL)
        return;

    ExampleTunerTurntable* state = &tuner->turntables[tuner->turntable_count++];
    state->name = name;
    state->turntable = turntable;
    state->camera = camera;
    state->reset_desc = *desc;
    _copy_vec3(state->world_up, desc->initial_view.up);
    if (glm_vec3_norm(state->world_up) <= 1e-6f)
        glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, state->world_up);
    glm_vec3_normalize(state->world_up);
    _turntable_sync(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Turntable"), state, _turntable_sync, _turntable_gui,
        _turntable_apply, _turntable_reset, _turntable_print_cb);
}



/**
 * Register a panzoom tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panzoom panzoom controller
 * @param reset_pan reset pan
 * @param reset_zoom reset zoom
 */
void example_tuner_panzoom(
    ExampleTuner* tuner,
    const char* name,
    DvzPanzoom* panzoom,
    const float reset_pan[2],
    const float reset_zoom[2])
{
    if (tuner == NULL || panzoom == NULL || tuner->panzoom_count >= EXAMPLE_TUNER_MAX_PANZOOMS)
        return;

    ExampleTunerPanzoom* state = &tuner->panzooms[tuner->panzoom_count++];
    state->name = name;
    state->panzoom = panzoom;
    _copy_vec2(state->reset_pan, reset_pan);
    if (reset_zoom != NULL)
        _copy_vec2(state->reset_zoom, reset_zoom);
    else
    {
        state->reset_zoom[0] = 1.0f;
        state->reset_zoom[1] = 1.0f;
    }
    _panzoom_reset(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Panzoom"), state, _panzoom_sync, _panzoom_gui, _panzoom_apply,
        _panzoom_reset, _panzoom_print_cb);
}



/**
 * Register a camera descriptor tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panel target panel
 * @param camera_desc camera descriptor defaults
 */
void example_tuner_camera(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    const DvzCameraDesc* camera_desc)
{
    example_tuner_camera_ref(tuner, name, panel, NULL, camera_desc);
}



/**
 * Register a live camera tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panel target panel
 * @param camera live camera
 * @param camera_desc camera descriptor defaults
 */
void example_tuner_camera_ref(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzCamera* camera,
    const DvzCameraDesc* camera_desc)
{
    if (tuner == NULL || camera_desc == NULL || tuner->camera_count >= EXAMPLE_TUNER_MAX_CAMERAS)
        return;

    ExampleTunerCamera* state = &tuner->cameras[tuner->camera_count++];
    state->name = name;
    state->panel = panel;
    state->camera_ref = camera;
    state->camera = *camera_desc;
    state->reset_camera = *camera_desc;

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Camera"), state, _camera_sync, _camera_gui, _camera_apply,
        _camera_reset, _camera_print_cb);
}



/**
 * Register an Eye-Dome Lighting tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panel panel receiving EDL state
 * @param controls live EDL controls
 */
void example_tuner_edl(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiEdlControls* controls)
{
    if (tuner == NULL || panel == NULL || controls == NULL ||
        tuner->edl_count >= EXAMPLE_TUNER_MAX_EDL)
        return;

    ExampleTunerEdl* state = &tuner->edls[tuner->edl_count++];
    state->name = name;
    state->panel = panel;
    state->controls = controls;
    state->reset_controls = *controls;
    _edl_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "EDL"), state, NULL, _edl_gui, _edl_apply_cb, _edl_reset,
        _edl_print_cb);
}



/**
 * Register an MSAA tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panel panel receiving MSAA state
 * @param controls live MSAA controls
 */
void example_tuner_msaa(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiMsaaControls* controls)
{
    if (tuner == NULL || panel == NULL || controls == NULL ||
        tuner->msaa_count >= EXAMPLE_TUNER_MAX_MSAA)
        return;

    ExampleTunerMsaa* state = &tuner->msaas[tuner->msaa_count++];
    state->name = name;
    state->panel = panel;
    state->controls = controls;
    state->reset_controls = *controls;
    _msaa_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "MSAA"), state, NULL, _msaa_gui, _msaa_apply_cb, _msaa_reset,
        _msaa_print_cb);
}



/**
 * Register an AO tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param panel panel receiving AO state
 * @param controls live AO controls
 */
void example_tuner_ao(
    ExampleTuner* tuner,
    const char* name,
    DvzPanel* panel,
    DvzExampleGuiAoControls* controls)
{
    if (tuner == NULL || panel == NULL || controls == NULL ||
        tuner->ao_count >= EXAMPLE_TUNER_MAX_AO)
        return;

    ExampleTunerAo* state = &tuner->aos[tuner->ao_count++];
    state->name = name;
    state->panel = panel;
    state->controls = controls;
    state->reset_controls = *controls;
    _ao_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "AO"), state, NULL, _ao_gui, _ao_apply_cb, _ao_reset,
        _ao_print_cb);
}



/**
 * Register a material tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param visual visual receiving material state
 * @param material live material descriptor
 */
void example_tuner_material(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    DvzMaterialDesc* material)
{
    if (tuner == NULL || visual == NULL || material == NULL ||
        tuner->material_count >= EXAMPLE_TUNER_MAX_MATERIALS)
        return;

    ExampleTunerMaterial* state = &tuner->materials[tuner->material_count++];
    state->name = name;
    state->visual = visual;
    state->material = material;
    state->reset_material = *material;
    _material_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Material"), state, NULL, _material_gui, _material_apply_cb,
        _material_reset, _material_print_cb);
}



/**
 * Register a depth-cue tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param visual visual receiving depth-cue state
 * @param enabled live enable flag
 * @param desc live depth-cue descriptor
 */
void example_tuner_depth_cue(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    bool* enabled,
    DvzDepthCueDesc* desc)
{
    if (tuner == NULL || visual == NULL || enabled == NULL || desc == NULL ||
        tuner->depth_cue_count >= EXAMPLE_TUNER_MAX_DEPTH_CUES)
        return;

    ExampleTunerDepthCue* state = &tuner->depth_cues[tuner->depth_cue_count++];
    state->name = name;
    state->visual = visual;
    state->enabled = enabled;
    state->reset_enabled = *enabled;
    state->desc = desc;
    state->reset_desc = *desc;
    _depth_cue_clamp(state->desc);
    _depth_cue_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Depth cue"), state, NULL, _depth_cue_gui, _depth_cue_apply_cb,
        _depth_cue_reset, _depth_cue_print_cb);
}



/**
 * Register a volume tuner component.
 *
 * @param tuner tuner
 * @param name component name
 * @param visual volume visual
 * @param defaults optional volume defaults; NULL snapshots the current visual state
 */
void example_tuner_volume(
    ExampleTuner* tuner,
    const char* name,
    DvzVisual* visual,
    const DvzVolumeState* defaults)
{
    if (tuner == NULL || visual == NULL || tuner->volume_count >= EXAMPLE_TUNER_MAX_VOLUMES)
        return;

    ExampleTunerVolume* state = &tuner->volumes[tuner->volume_count++];
    state->name = name;
    state->visual = visual;
    const DvzVolumeState* current = defaults != NULL ? defaults : dvz_volume_state(visual);
    state->state = current != NULL ? *current : _volume_state_default();
    state->reset_state = state->state;
    _volume_clamp(&state->state);
    _volume_apply(state);

    (void)example_tuner_add_component(
        tuner, _tuner_name(name, "Volume"), state, NULL, _volume_gui, _volume_apply_cb,
        _volume_reset, _volume_print_cb);
}



/**
 * Synchronize registered tuner components from live runtime state.
 *
 * @param tuner tuner
 */
void example_tuner_sync(ExampleTuner* tuner)
{
    if (tuner == NULL)
        return;

    for (uint32_t i = 0; i < tuner->component_count; i++)
    {
        ExampleTunerComponent* component = &tuner->components[i];
        if (component->sync != NULL)
            component->sync(component->user);
    }
}



/**
 * Reset every component.
 *
 * @param tuner tuner
 */
void example_tuner_reset(ExampleTuner* tuner)
{
    if (tuner == NULL)
        return;

    for (uint32_t i = 0; i < tuner->component_count; i++)
    {
        ExampleTunerComponent* component = &tuner->components[i];
        if (component->reset != NULL)
            component->reset(component->user);
    }
    dvz_fprintf(stderr, "example tuner: reset registered components\n");
}



/**
 * Print pasteable C defaults for every component.
 *
 * @param tuner tuner
 * @param fp output stream
 */
void example_tuner_print_c(ExampleTuner* tuner, FILE* fp)
{
    if (tuner == NULL)
        return;
    if (fp == NULL)
        fp = stdout;

    example_tuner_sync(tuner);

    dvz_fprintf(fp, "\n/* %s tuned defaults */\n", _tuner_name(tuner->name, "example"));
    for (uint32_t i = 0; i < tuner->component_count; i++)
    {
        const ExampleTunerComponent* component = &tuner->components[i];
        if (component->print_c == NULL)
            continue;
        component->print_c(fp, component->user);
    }
    dvz_fprintf(fp, "\n");
    fflush(fp);
}
