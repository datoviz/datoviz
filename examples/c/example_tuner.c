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
#include <string.h>

#include "_assertions.h"
#include "_compat.h"
#include "example_common.h"



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

    example_tuner_sync(tuner);
    if (dvz_gui_begin(gui, _tuner_name(tuner->name, "Example settings"), NULL, 0))
    {
        for (uint32_t i = 0; i < tuner->component_count; i++)
        {
            ExampleTunerComponent* component = &tuner->components[i];
            if (component->gui == NULL)
                continue;

            dvz_gui_separator_text(gui, _tuner_name(component->title, "Component"));
            const bool changed = component->gui(gui, component->user);
            if (changed && component->apply != NULL)
                component->apply(component->user);
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
    return (ExampleTuner){.name = name};
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

    if (tuner->installed && tuner->input != NULL)
        dvz_input_unsubscribe(tuner->input, tuner->input_subscription_id);
    if (tuner->view != NULL)
        dvz_view_set_gui_callback(tuner->view, NULL, NULL);

    tuner->input_subscription_id = DVZ_CALLBACK_ID_NONE;
    tuner->input = NULL;
    tuner->view = NULL;
    tuner->installed = false;
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
    if (tuner == NULL || panel == NULL || controls == NULL || tuner->edl_count >= EXAMPLE_TUNER_MAX_EDL)
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
