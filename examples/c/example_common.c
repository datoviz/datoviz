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
 * When DVZ_CAPTURE_DIR and DVZ_CAPTURE_BASENAME are set, the requested basename is replaced
 * while preserving suffixes derived from the executable name. For example, record_replay_original
 * becomes ${DVZ_CAPTURE_BASENAME}_original, keeping multi-output examples collision-free.
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
        capture_basename[0] != '\0')
    {
        const char* suffix = "";
        int suffix_len = 0;

        const char* exe_name = exe != NULL ? strrchr(exe, '/') : NULL;
        exe_name = exe_name != NULL ? exe_name + 1 : exe;
        if (exe_name != NULL && exe_name[0] != '\0')
        {
            const char* exe_dot = strrchr(exe_name, '.');
            size_t exe_stem_len = exe_dot != NULL ? (size_t)(exe_dot - exe_name) : strlen(exe_name);
            size_t name_stem_len = dot != NULL ? (size_t)(dot - name) : strlen(name);
            if (exe_stem_len > 0 && name_stem_len >= exe_stem_len &&
                strncmp(name, exe_name, exe_stem_len) == 0)
            {
                suffix = name + exe_stem_len;
                suffix_len = (int)(name_stem_len - exe_stem_len);
            }
        }

        dvz_snprintf(
            out, size, "%s/%s%.*s%s", capture_dir, capture_basename, suffix_len, suffix,
            dot != NULL ? dot : "");
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
 * Bind or refresh an RGBA8 sampled field on a visual.
 *
 * @param scene scene owning the sampled field
 * @param visual destination visual
 * @param slot_name visual sampled-field slot
 * @param rgba tightly packed RGBA8 texels
 * @param width field width
 * @param height field height
 * @param field_io optional retained field pointer
 * @return true on success
 */
bool example_visual_set_rgba8_field(
    DvzScene* scene,
    DvzVisual* visual,
    const char* slot_name,
    const uint8_t* rgba,
    uint32_t width,
    uint32_t height,
    DvzSampledField** field_io)
{
    if (scene == NULL || visual == NULL || slot_name == NULL || rgba == NULL || width == 0 ||
        height == 0)
        return false;

    DvzFieldDataView view = dvz_field_data_view();
    view.data = rgba;
    view.bytes_per_row = (uint64_t)width * 4u;
    view.rows_per_image = height;

    DvzSampledField* field = field_io != NULL ? *field_io : NULL;
    if (field == NULL)
    {
        DvzSampledFieldDesc desc = dvz_sampled_field_desc();
        desc.dim = DVZ_FIELD_DIM_2D;
        desc.format = DVZ_FIELD_FORMAT_RGBA8_UNORM;
        desc.semantic = DVZ_FIELD_SEMANTIC_COLOR;
        desc.color_role = DVZ_COLOR_ROLE_SRGB_COLOR;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;

        field = dvz_sampled_field(scene, &desc);
        if (field == NULL)
            return false;
        if (dvz_sampled_field_set_data(field, &view) != DVZ_OK)
            return false;
    }
    else
    {
        DvzSampledFieldDesc desc = {0};
        if (
            !dvz_sampled_field_info(field, &desc) || desc.dim != DVZ_FIELD_DIM_2D ||
            desc.format != DVZ_FIELD_FORMAT_RGBA8_UNORM ||
            desc.semantic != DVZ_FIELD_SEMANTIC_COLOR)
        {
            return false;
        }

        bool ok = false;
        if (desc.width == width && desc.height == height && desc.depth == 1)
            ok = dvz_sampled_field_set_data(field, &view) == DVZ_OK;
        else
            ok = dvz_sampled_field_resize(field, width, height, 1, &view) == DVZ_OK;
        if (!ok)
            return false;
    }

    if (dvz_visual_set_field(visual, slot_name, field) != DVZ_OK)
        return false;
    if (field_io != NULL)
        *field_io = field;
    return true;
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
    if (dvz_panel_set_camera_desc(panel, &camera) != 0)
        return NULL;
    return dvz_panel_camera(panel);
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
 * Return the default Phong material for lit gallery meshes.
 *
 * @return material descriptor
 */
DvzMaterialDesc example_default_phong_material_desc(void)
{
    DvzMaterialDesc material = dvz_phong_material_desc();
    material.light_direction[0] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_X;
    material.light_direction[1] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_Y;
    material.light_direction[2] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_Z;
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
    material.light_direction[0] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_X;
    material.light_direction[1] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_Y;
    material.light_direction[2] = EXAMPLE_DEFAULT_LIGHT_DIRECTION_Z;
    material.standard.roughness = 0.38f;
    material.standard.specular = 0.42f;
    material.standard.rim_strength = 0.26f;
    return material;
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

    DvzPanelView2DDesc view = dvz_panel_view2d_desc();
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

    DvzGeometry* cube = dvz_geometry_cube(&(DvzGeometryCubeDesc){
        DVZ_STRUCT_INIT_FIELDS(DvzGeometryCubeDesc),
        .size = size,
        .face_colors = face_colors,
        .face_color_count = DVZ_GEOM_CUBE_FACE_COUNT,
    });
    if (cube == NULL)
        return NULL;
    if (out_geometry != NULL)
        *out_geometry = cube;

    if (dvz_mesh_set_geometry(visual, cube) != 0)
        return NULL;

    dvz_geometry_destroy(cube);
    if (out_geometry != NULL)
        *out_geometry = NULL;
    return visual;
}
