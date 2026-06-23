/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example common helpers                                                                       */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "_compat.h"
#include "datoviz/math/types.h"
#include "datoviz/scene/types.h"
#include "example_style.h"


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGeometry DvzGeometry;
typedef struct DvzVisual   DvzVisual;
typedef struct DvzScene    DvzScene;
typedef struct DvzPanel    DvzPanel;
typedef struct DvzCamera   DvzCamera;
typedef struct DvzApp      DvzApp;
typedef struct DvzView     DvzView;
typedef struct DvzPointerEvent DvzPointerEvent;
typedef struct DvzController DvzController;
typedef struct DvzArcball    DvzArcball;
typedef struct DvzAnimation  DvzAnimation;
typedef struct DvzTrack      DvzTrack;
typedef struct DvzAppCaptureConfig DvzAppCaptureConfig;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DvzExampleVisualSpin
{
    DvzTrack* rotation;
    DvzAnimation* animation;
} DvzExampleVisualSpin;



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define EXAMPLE_CHECK(condition, message)                                                        \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            dvz_fprintf(stderr, "%s\n", message);                                                \
            goto cleanup;                                                                         \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool example_parse_u32(const char* text, uint32_t* out);

bool example_arg_has(int argc, char** argv, const char* name);

bool example_arg_value(int argc, char** argv, const char* name, const char** out);

bool example_arg_value_prefix(int argc, char** argv, const char* prefix, const char** out);

void example_outpath(const char* exe, const char* name, char* out, size_t size);

bool example_recording_path(int argc, char** argv, const char* default_path, char* out, size_t size);

uint32_t example_frame_count_from_text(const char* text);

uint32_t example_frame_count(int argc, char** argv);

uint32_t example_frame_count_any(int argc, char** argv);

bool example_run_with_capture(
    DvzApp* app, DvzView* view, uint32_t frame_count, const DvzAppCaptureConfig* capture);

bool example_mesh_geometry(DvzVisual* visual, const DvzGeometry* geometry);

bool example_panel_pointer_position(
    const DvzPanel* panel, const DvzPointerEvent* event, double* out_x, double* out_y);

DvzCameraDesc example_default_3d_camera_desc(float extent);

DvzCamera* example_set_default_3d_camera(DvzPanel* panel, float extent);

DvzCameraDesc example_controller_camera_desc(void);

DvzCamera* example_set_controller_camera(DvzPanel* panel);

float example_controller_grid_origin_y(void);

double example_controller_cube_size(void);

void example_controller_cube_face_roles(ExampleStyleColorRole out[6]);

void example_default_light_direction(vec3 out);

DvzMaterialDesc example_default_phong_material_desc(void);

DvzMaterialDesc example_default_standard_material_desc(void);

bool example_apply_default_phong_material(DvzVisual* visual);

bool example_apply_default_standard_material(DvzVisual* visual);

bool example_add_xz_reference_grid(DvzPanel* panel, float origin_y, float size);

bool example_add_default_xz_reference_grid(DvzPanel* panel, float origin_y);

bool example_add_panel_label(DvzPanel* panel, const char* label, float x_px, float y_px);

bool example_add_sized_panel_label(
    DvzPanel* panel, const char* label, float x_px, float y_px, float size_px);

bool example_add_default_panel_label(DvzPanel* panel, const char* label);

bool example_add_large_panel_label(DvzPanel* panel, const char* label);

bool example_add_data_label(
    DvzPanel* panel,
    const char* label,
    const vec3 position,
    float offset_x,
    float offset_y,
    ExampleStyleTextRole role,
    DvzColor color);

bool example_configure_equal_aspect_panel(
    DvzPanel* panel,
    DvzDataDomain x,
    DvzDataDomain y,
    double padding);

bool example_link_controllers_bidirectional(
    DvzScene* scene, DvzController* a, DvzController* b, uint32_t components);

void example_default_arcball_initial(vec3 out);

void example_set_default_arcball(DvzArcball* arcball);

DvzVisual* example_graphite_cyan_cube_mesh(
    DvzScene* scene,
    double size,
    const ExampleStyleColorRole face_roles[6],
    DvzGeometry** out_geometry);

bool example_visual_spin(
    DvzScene* scene,
    DvzVisual* visual,
    vec3 axis,
    float speed_rad_per_sec,
    DvzController* controller,
    DvzExampleVisualSpin* out);

void example_visual_spin_start(DvzExampleVisualSpin* spin, double t_start);

void example_visual_spin_stop(DvzExampleVisualSpin* spin);

void example_visual_spin_set_speed(DvzExampleVisualSpin* spin, float speed_rad_per_sec);

void example_visual_spin_destroy(DvzExampleVisualSpin* spin);
