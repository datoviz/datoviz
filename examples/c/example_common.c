/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Example common helpers                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "example_common.h"

#ifndef DVZ_EXAMPLE_NO_APP
#include "datoviz/app.h"
#endif
#include "datoviz/geom.h"
#include "datoviz/input/pointer.h"
#include "datoviz/math/_cglm.h"
#include "datoviz/scene.h"

#include "_alloc.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Parse a bounded unsigned 32-bit integer.
 *
 * @param text input text
 * @param out parsed value output
 * @return true when the full string is a valid unsigned integer
 */
bool example_parse_u32(const char* text, uint32_t* out)
{
    if (text == NULL || text[0] == '\0' || out == NULL)
        return false;

    char* end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0'))
        return false;

    if (value > UINT32_MAX)
        value = UINT32_MAX;

    *out = (uint32_t)value;
    return true;
}



/**
 * Return whether a command-line token is present.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param name token to find
 * @return true when the token appears after argv[0]
 */
bool example_arg_has(int argc, char** argv, const char* name)
{
    if (argc < 2 || argv == NULL || name == NULL)
        return false;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL && strcmp(argv[i], name) == 0)
            return true;
    }
    return false;
}



/**
 * Return the value following a command-line token.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param name token to find
 * @param out value output
 * @return true when the token and a non-NULL following value are present
 */
bool example_arg_value(int argc, char** argv, const char* name, const char** out)
{
    if (argc < 3 || argv == NULL || name == NULL || out == NULL)
        return false;

    for (int i = 1; i + 1 < argc; i++)
    {
        if (argv[i] != NULL && strcmp(argv[i], name) == 0 && argv[i + 1] != NULL)
        {
            *out = argv[i + 1];
            return true;
        }
    }
    return false;
}



/**
 * Return the value suffix following a command-line token prefix.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param prefix token prefix to find
 * @param out value output
 * @return true when a token with the prefix is present
 */
bool example_arg_value_prefix(int argc, char** argv, const char* prefix, const char** out)
{
    if (argc < 2 || argv == NULL || prefix == NULL || out == NULL)
        return false;

    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0)
        return false;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL && strncmp(argv[i], prefix, prefix_len) == 0)
        {
            *out = argv[i] + prefix_len;
            return true;
        }
    }
    return false;
}



/**
 * Build an output path next to the example executable.
 *
 * @param exe executable path from argv[0]
 * @param name output file name
 * @param out destination path buffer
 * @param size destination path buffer size
 */
void example_outpath(const char* exe, const char* name, char* out, size_t size)
{
    if (name == NULL || out == NULL || size == 0)
        return;

    const char* capture_dir = getenv("DVZ_CAPTURE_DIR");
    const char* capture_basename = getenv("DVZ_CAPTURE_BASENAME");
    const char* dot = strrchr(name, '.');
    if (capture_dir != NULL && capture_dir[0] != '\0' && capture_basename != NULL &&
        capture_basename[0] != '\0' && dot != NULL && dot[1] != '\0')
    {
        dvz_snprintf(out, size, "%s/%s%s", capture_dir, capture_basename, dot);
        return;
    }

    const char* slash = exe != NULL ? strrchr(exe, '/') : NULL;
    if (slash != NULL)
    {
        size_t prefix_len = (size_t)(slash - exe);
        if (prefix_len > (size_t)INT_MAX)
            prefix_len = (size_t)INT_MAX;
        dvz_snprintf(out, size, "%.*s/%s", (int)prefix_len, exe, name);
    }
    else
    {
        dvz_snprintf(out, size, "%s", name);
    }
}



/**
 * Return whether an example should record a DVZR stream.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @param default_path default path next to the executable
 * @param out output recording path
 * @param size output buffer size
 * @return whether DVZR recording was requested
 */
bool example_recording_path(
    int argc, char** argv, const char* default_path, char* out, size_t size)
{
    if (argc < 2 || argv == NULL || default_path == NULL || out == NULL || size == 0)
        return false;

    const char* value = NULL;
    if (example_arg_has(argc, argv, "record"))
    {
        dvz_snprintf(out, size, "%s", default_path);
        return true;
    }
    if (example_arg_value_prefix(argc, argv, "record=", &value))
    {
        dvz_snprintf(out, size, "%s", value);
        return true;
    }
    if (example_arg_value(argc, argv, "--record", &value))
    {
        dvz_snprintf(out, size, "%s", value);
        return true;
    }
    return false;
}



/**
 * Parse one command-line token as a frame count.
 *
 * @param text command-line text, or NULL
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count_from_text(const char* text)
{
    uint32_t out = 0;
    return example_parse_u32(text, &out) ? out : 0;
}



/**
 * Parse the first positional argument as a frame count.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;

    return example_frame_count_from_text(argv[1]);
}



/**
 * Parse the first numeric command-line argument as a frame count.
 *
 * @param argc command-line argument count
 * @param argv command-line argument vector
 * @return requested frame count, or 0 for the interactive loop
 */
uint32_t example_frame_count_any(int argc, char** argv)
{
    if (argc < 2 || argv == NULL)
        return 0;

    uint32_t out = 0;
    for (int i = 1; i < argc; i++)
    {
        if (example_parse_u32(argv[i], &out))
            return out;
    }
    return 0;
}


/**
 * Run an app view with the configured capture lifecycle.
 *
 * @param app app to run
 * @param view view receiving capture commands
 * @param frame_count requested frame count, or 0 for interactive
 * @param capture capture configuration
 * @return true when capture start/stop succeeded
 */
#ifndef DVZ_EXAMPLE_NO_APP
bool example_run_with_capture(
    DvzApp* app, DvzView* view, uint32_t frame_count, const DvzAppCaptureConfig* capture)
{
    if (app == NULL || view == NULL)
        return false;

    if (dvz_view_capture_start(view, capture) != 0)
        return false;
    dvz_app_run(app, frame_count);
    return dvz_view_capture_stop(view) == 0;
}
#endif



/**
 * Upload one CPU geometry object into a mesh visual.
 *
 * @param visual target mesh visual
 * @param geometry CPU geometry object
 * @return true on success, false on error
 */
bool example_mesh_geometry(DvzVisual* visual, const DvzGeometry* geometry)
{
    return dvz_mesh_set_geometry(visual, geometry) == 0;
}



/**
 * Convert one raw pointer event to panel-local figure coordinates.
 *
 * @param panel target panel
 * @param event pointer event in logical window coordinates
 * @param out_x output panel-local x coordinate
 * @param out_y output panel-local y coordinate
 * @return true when the pointer is inside the panel rectangle
 */
bool example_panel_pointer_position(
    const DvzPanel* panel, const DvzPointerEvent* event, double* out_x, double* out_y)
{
    if (panel == NULL || event == NULL || out_x == NULL || out_y == NULL)
        return false;

    DvzRect rect = {0};
    if (!dvz_panel_inner_rect_px(panel, &rect) || rect.width <= 0.0f || rect.height <= 0.0f)
        return false;

    float x = event->pos[0];
    float y = event->pos[1];

    x -= rect.x;
    y -= rect.y;
    if (x < 0.0f || x >= rect.width || y < 0.0f || y >= rect.height)
        return false;

    *out_x = (double)x;
    *out_y = (double)y;
    return true;
}


/**
 * Return the default perspective camera for 3D gallery examples.
 *
 * @param extent approximate scene extent in world units
 * @return camera descriptor
 */
DvzCameraDesc example_default_3d_camera_desc(float extent)
{
    if (extent <= 0.0f)
        extent = 1.0f;

    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = -0.25f * extent;
    camera.view.eye[1] = +2.0f * extent;
    camera.view.eye[2] = +4.0f * extent;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = 0.0f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 1.0f;
    camera.view.up[2] = 0.0f;
    camera.projection.fov_y = 0.66f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f * extent;
    return camera;
}


/**
 * Apply the default 3D gallery camera to one panel.
 *
 * @param panel target panel
 * @param extent approximate scene extent in world units
 * @return scene-owned camera, or NULL on error
 */
DvzCamera* example_set_default_3d_camera(DvzPanel* panel, float extent)
{
    if (panel == NULL)
        return NULL;

    DvzCameraDesc camera = example_default_3d_camera_desc(extent);
    return dvz_panel_set_camera(panel, &camera);
}


/**
 * Return the shared initial camera for controller feature examples.
 *
 * @return camera descriptor
 */
DvzCameraDesc example_controller_camera_desc(void)
{
    DvzCameraDesc camera = dvz_camera_desc();
    camera.view.eye[0] = 0.0f;
    camera.view.eye[1] = +3.0f;
    camera.view.eye[2] = +5.0f;
    camera.view.target[0] = 0.0f;
    camera.view.target[1] = 0.0f;
    camera.view.target[2] = +0.3f;
    camera.view.up[0] = 0.0f;
    camera.view.up[1] = 1.0f;
    camera.view.up[2] = 0.0f;
    camera.projection.fov_y = 0.66f;
    camera.projection.near_clip = 0.05f;
    camera.projection.far_clip = 100.0f;
    return camera;
}


/**
 * Apply the shared initial camera for controller feature examples.
 *
 * @param panel target panel
 * @return scene-owned camera, or NULL on error
 */
DvzCamera* example_set_controller_camera(DvzPanel* panel)
{
    if (panel == NULL)
        return NULL;

    DvzCameraDesc camera = example_controller_camera_desc();
    return dvz_panel_set_camera(panel, &camera);
}


/**
 * Return the shared XZ reference-grid height for controller feature examples.
 *
 * @return Y coordinate of the grid plane
 */
float example_controller_grid_origin_y(void) { return -0.55f; }


/**
 * Return the shared cube size for controller feature examples.
 *
 * @return cube edge length
 */
double example_controller_cube_size(void) { return 1.10; }


/**
 * Return the shared cube face color roles for controller feature examples.
 *
 * @param out six output color roles
 */
void example_controller_cube_face_roles(ExampleStyleColorRole out[6])
{
    if (out == NULL)
        return;

    out[0] = EXAMPLE_STYLE_COLOR_ACCENT_PRIMARY;
    out[1] = EXAMPLE_STYLE_COLOR_ACCENT_SECONDARY;
    out[2] = EXAMPLE_STYLE_COLOR_WARNING;
    out[3] = EXAMPLE_STYLE_COLOR_ERROR;
    out[4] = EXAMPLE_STYLE_COLOR_TEXT;
    out[5] = EXAMPLE_STYLE_COLOR_MINOR_TICK;
}


/**
 * Return the default example light direction.
 *
 * @param out output light direction
 */
void example_default_light_direction(vec3 out)
{
    if (out == NULL)
        return;

    out[0] = -0.45f;
    out[1] = +0.35f;
    out[2] = +0.82f;
}


/**
 * Return the default Phong material for lit gallery meshes.
 *
 * @return material descriptor
 */
DvzMaterialDesc example_default_phong_material_desc(void)
{
    DvzMaterialDesc material = dvz_phong_material_desc();
    example_default_light_direction(material.light_direction);
    material.phong.ambient = 0.5f;
    material.phong.diffuse = 0.8f;
    material.phong.specular = 0.3f;
    material.phong.shininess = 24.0f;
    return material;
}


/**
 * Return the default standard material for lit gallery meshes.
 *
 * @return material descriptor
 */
DvzMaterialDesc example_default_standard_material_desc(void)
{
    DvzMaterialDesc material = dvz_standard_material_desc();
    example_default_light_direction(material.light_direction);
    material.standard.roughness = 0.38f;
    material.standard.specular = 0.42f;
    material.standard.rim_strength = 0.26f;
    return material;
}


/**
 * Apply the default Phong material to one visual.
 *
 * @param visual target visual
 * @return true on success
 */
bool example_apply_default_phong_material(DvzVisual* visual)
{
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = example_default_phong_material_desc();
    return dvz_visual_set_material(visual, &material) == 0;
}


/**
 * Apply the default standard material to one visual.
 *
 * @param visual target visual
 * @return true on success
 */
bool example_apply_default_standard_material(DvzVisual* visual)
{
    if (visual == NULL)
        return false;

    DvzMaterialDesc material = example_default_standard_material_desc();
    return dvz_visual_set_material(visual, &material) == 0;
}


/**
 * Add a finite XZ reference grid using the shared example palette.
 *
 * @param panel target panel
 * @param origin_y grid plane Y coordinate
 * @param size grid side length in world units
 * @return true when the grid was created
 */
bool example_add_xz_reference_grid(DvzPanel* panel, float origin_y, float size)
{
    if (panel == NULL || size <= 0.0f)
        return false;

    DvzReferenceGridDesc grid = dvz_reference_grid_desc();
    grid.plane = DVZ_REFERENCE_GRID_XZ;
    grid.origin[0] = 0.0f;
    grid.origin[1] = origin_y;
    grid.origin[2] = 0.0f;
    grid.size[0] = size;
    grid.size[1] = size;
    grid.spacing = 0.25f;
    grid.major_every = 4;
    grid.minor_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_GRID);
    grid.minor_color.a = 72u;
    grid.major_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_MINOR_TICK);
    grid.major_color.a = 130u;
    grid.axis_color = example_graphite_cyan_color(EXAMPLE_STYLE_COLOR_TEXT);
    grid.axis_color.a = 170u;
    grid.minor_width_px = 1.0f;
    grid.major_width_px = 1.4f;
    grid.axis_width_px = 2.0f;
    grid.depth_test = true;
    return dvz_reference_grid(panel, &grid) != NULL;
}


/**
 * Add the default finite XZ reference grid using the shared example palette.
 *
 * @param panel target panel
 * @param origin_y grid plane Y coordinate
 * @return true when the grid was created
 */
bool example_add_default_xz_reference_grid(DvzPanel* panel, float origin_y)
{
    return example_add_xz_reference_grid(panel, origin_y, 8.0f);
}


/**
 * Add a compact fixed panel label.
 *
 * @param panel target panel
 * @param label label text
 * @param x_px panel-local x position
 * @param y_px panel-local y position
 * @return true when the label was created
 */
bool example_add_panel_label(DvzPanel* panel, const char* label, float x_px, float y_px)
{
    return example_add_sized_panel_label(panel, label, x_px, y_px, 0.0f);
}


/**
 * Add a fixed panel label with an explicit optional text size.
 *
 * @param panel target panel
 * @param label label text
 * @param x_px panel-local x position
 * @param y_px panel-local y position
 * @param size_px optional text size override, or 0 for the shared default
 * @return true when the label was created
 */
bool example_add_sized_panel_label(
    DvzPanel* panel, const char* label, float x_px, float y_px, float size_px)
{
    if (panel == NULL || label == NULL || label[0] == '\0')
        return false;

    DvzLabelDesc desc = dvz_label_desc();
    desc.text = label;
    desc.style = example_graphite_cyan_text_style(EXAMPLE_STYLE_TEXT_PANEL_LABEL);
    desc.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    if (size_px > 0.0f)
        desc.style.size_px = size_px;
    desc.placement.mode = DVZ_TEXT_PLACEMENT_SCREEN;
    desc.placement.anchor = DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT;
    desc.placement.position[0] = x_px;
    desc.placement.position[1] = y_px;
    desc.placement.text_anchor[0] = 0.0f;
    desc.placement.text_anchor[1] = 0.0f;
    desc.placement.has_text_anchor = true;
    return dvz_annotation_label(panel, &desc) != NULL;
}


/**
 * Add a compact fixed panel label at the default top-left position.
 *
 * @param panel target panel
 * @param label label text
 * @return true when the label was created
 */
bool example_add_default_panel_label(DvzPanel* panel, const char* label)
{
    return example_add_panel_label(panel, label, 20.0f, 20.0f);
}


/**
 * Add a large fixed panel label at the shared top-left position.
 *
 * @param panel target panel
 * @param label label text
 * @return true when the label was created
 */
bool example_add_large_panel_label(DvzPanel* panel, const char* label)
{
    return example_add_sized_panel_label(panel, label, 24.0f, 24.0f, 30.0f);
}


/**
 * Add a compact data-positioned label using the shared graphite-cyan text style.
 *
 * @param panel target panel
 * @param label label text
 * @param position data coordinate
 * @param offset_x screen-space X offset
 * @param offset_y screen-space Y offset
 * @param role semantic text role
 * @param color text color
 * @return true when the label was created
 */
bool example_add_data_label(
    DvzPanel* panel, const char* label, const vec3 position, float offset_x, float offset_y,
    ExampleStyleTextRole role, DvzColor color)
{
    if (panel == NULL || label == NULL || label[0] == '\0' || position == NULL)
        return false;

    DvzLabelDesc desc = dvz_label_desc();
    desc.text = label;
    desc.style = example_graphite_cyan_text_style(role);
    desc.style.renderer = DVZ_TEXT_RENDERER_MSDF_ATLAS;
    desc.style.color[0] = color.r;
    desc.style.color[1] = color.g;
    desc.style.color[2] = color.b;
    desc.style.color[3] = color.a;
    desc.placement.mode = DVZ_TEXT_PLACEMENT_DATA;
    desc.placement.position[0] = position[0];
    desc.placement.position[1] = position[1];
    desc.placement.position[2] = position[2];
    desc.placement.offset[0] = offset_x;
    desc.placement.offset[1] = offset_y;
    desc.placement.text_anchor[0] = 0.5f;
    desc.placement.text_anchor[1] = 0.5f;
    desc.placement.has_text_anchor = true;
    desc.placement.depth_test = false;
    return dvz_annotation_label(panel, &desc) != NULL;
}


/**
 * Configure a graphite-cyan panel with an equal-aspect data-domain fit.
 *
 * @param panel target panel
 * @param x x data domain
 * @param y y data domain
 * @param padding domain fit padding
 * @return true when the panel was configured
 */
bool example_configure_equal_aspect_panel(
    DvzPanel* panel, DvzDataDomain x, DvzDataDomain y, double padding)
{
    if (panel == NULL)
        return false;

    example_graphite_cyan_set_panel_background(panel);
    if (dvz_panel_set_domain(panel, DVZ_DIM_X, x.min, x.max) != 0)
        return false;
    if (dvz_panel_set_domain(panel, DVZ_DIM_Y, y.min, y.max) != 0)
        return false;

    DvzPanelView2D view = dvz_panel_view2d();
    view.aspect = DVZ_PANEL_VIEW2D_ASPECT_EQUAL;
    view.padding = padding;
    return dvz_panel_set_view2d(panel, &view) == 0;
}


/**
 * Configure compact, homogeneous outer grid margins for feature examples.
 *
 * @param grid target grid
 * @param gutter_x_px horizontal gutter in pixels
 * @param gutter_y_px vertical gutter in pixels
 * @return true when margins and gutters were configured
 */
bool example_configure_compact_grid(DvzGrid* grid, float gutter_x_px, float gutter_y_px)
{
    if (grid == NULL)
        return false;
    if (!dvz_grid_set_margins(
            grid,
            &(DvzPanelReserve){
                .left_px = 56.0f, .right_px = 56.0f, .top_px = 56.0f, .bottom_px = 56.0f}))
    {
        return false;
    }
    return dvz_grid_set_gutter(grid, gutter_x_px, gutter_y_px);
}


/**
 * Link two controllers in both directions.
 *
 * @param scene scene owning both controllers
 * @param a first controller
 * @param b second controller
 * @param components linked controller state components
 * @return true when the link was created
 */
bool example_link_controllers_bidirectional(
    DvzScene* scene, DvzController* a, DvzController* b, uint32_t components)
{
    if (scene == NULL || a == NULL || b == NULL)
        return false;
    return dvz_controller_link(scene, a, b, components, DVZ_CONTROLLER_LINK_TWO_WAY) != NULL;
}


/**
 * Return the default arcball initial angles for 3D gallery examples.
 *
 * @param out output initial angles
 */
void example_default_arcball_initial(vec3 out)
{
    if (out == NULL)
        return;

    // out[0] = +0.56f;
    // out[1] = -0.16f;
    // out[2] = +0.24f;
}


/**
 * Apply the default arcball initial angles.
 *
 * @param arcball target arcball controller state
 */
void example_set_default_arcball(DvzArcball* arcball)
{
    if (arcball == NULL)
        return;

    vec3 initial = {0};
    example_default_arcball_initial(initial);
    dvz_arcball_initial(arcball, initial);
}


/**
 * Create a mesh visual backed by one graphite-cyan colored cube geometry.
 *
 * @param scene scene owning the visual
 * @param size cube edge length
 * @param face_roles six graphite-cyan face color roles
 * @param out_geometry geometry handle for cleanup on failure before upload completes
 * @return uploaded mesh visual, or NULL on error
 */
DvzVisual* example_graphite_cyan_cube_mesh(
    DvzScene* scene, double size, const ExampleStyleColorRole face_roles[6],
    DvzGeometry** out_geometry)
{
    if (out_geometry != NULL)
        *out_geometry = NULL;
    if (scene == NULL || face_roles == NULL)
        return NULL;

    DvzVisual* visual = dvz_mesh(scene, 0);
    if (visual == NULL)
        return NULL;

    DvzColor face_colors[DVZ_GEOM_CUBE_FACE_COUNT] = {0};
    for (uint32_t i = 0; i < DVZ_GEOM_CUBE_FACE_COUNT; i++)
        face_colors[i] = example_graphite_cyan_color(face_roles[i]);

    DvzGeometry* cube = dvz_geom_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = size,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return NULL;
    if (out_geometry != NULL)
        *out_geometry = cube;

    if (!example_mesh_geometry(visual, cube))
        return NULL;

    dvz_geometry_destroy(cube);
    if (out_geometry != NULL)
        *out_geometry = NULL;
    return visual;
}



/**
 * Create a visual transform spin animation.
 *
 * @param scene owning scene
 * @param visual visual to transform
 * @param axis spin axis
 * @param speed_rad_per_sec angular speed in radians per second
 * @param controller optional controller whose interactions pause the animation
 * @param out output spin handle
 * @return true on success
 */
bool example_visual_spin(
    DvzScene* scene, DvzVisual* visual, vec3 axis, float speed_rad_per_sec,
    DvzController* controller, DvzExampleVisualSpin* out)
{
    if (scene == NULL || visual == NULL || out == NULL)
        return false;
    memset(out, 0, sizeof(*out));

    DvzTrackRotationDesc rotation_desc = dvz_track_rotation_desc();
    glm_vec3_copy(axis, rotation_desc.axis);
    rotation_desc.speed_rad_per_sec = 1.0f;
    out->rotation = dvz_track_rotation(&rotation_desc);
    if (out->rotation == NULL)
        return false;

    DvzTransformMotionDesc transform_desc = dvz_transform_motion_desc();
    transform_desc.rotation = out->rotation;
    out->animation = dvz_anim_visual_transform(scene, visual, &transform_desc);
    if (out->animation == NULL)
    {
        example_visual_spin_destroy(out);
        return false;
    }
    if (controller != NULL)
        dvz_anim_set_interaction_policy(
            out->animation, controller, DVZ_ANIM_INTERACTION_PAUSE, 0.0);
    example_visual_spin_set_speed(out, speed_rad_per_sec);
    return true;
}



/**
 * Start or restart a visual spin animation.
 *
 * @param spin spin handle
 * @param t_start scene-clock start time, or 0 for immediate start
 */
void example_visual_spin_start(DvzExampleVisualSpin* spin, double t_start)
{
    if (spin != NULL && spin->animation != NULL)
        dvz_anim_start(spin->animation, t_start);
}



/**
 * Stop a visual spin animation.
 *
 * @param spin spin handle
 */
void example_visual_spin_stop(DvzExampleVisualSpin* spin)
{
    if (spin != NULL && spin->animation != NULL)
        dvz_anim_stop(spin->animation);
}



/**
 * Set the angular speed of a visual spin animation.
 *
 * @param spin spin handle
 * @param speed_rad_per_sec angular speed in radians per second
 */
void example_visual_spin_set_speed(DvzExampleVisualSpin* spin, float speed_rad_per_sec)
{
    if (spin != NULL && spin->animation != NULL)
        dvz_anim_set_speed(spin->animation, speed_rad_per_sec);
}



/**
 * Destroy resources owned by a visual spin helper.
 *
 * @param spin spin handle
 */
void example_visual_spin_destroy(DvzExampleVisualSpin* spin)
{
    if (spin == NULL)
        return;
    dvz_track_destroy(spin->rotation);
    memset(spin, 0, sizeof(*spin));
}
