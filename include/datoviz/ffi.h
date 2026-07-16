/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Foreign-function interface helpers                                                           */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/common/types.h"
#include "datoviz/geom/types.h"
#include "datoviz/scene/field.h"
#include "datoviz/scene/overlay.h"
#include "datoviz/scene/types.h"



/*************************************************************************************************/
/*  Forward declarations                                                                         */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzFigure DvzFigure;
typedef struct DvzView DvzView;



EXTERN_C_ON

/*************************************************************************************************/
/*  Hosted surface helpers                                                                       */
/*************************************************************************************************/

/**
 * Create a hosted present view around an external Vulkan surface from FFI-friendly handles.
 *
 * Foreign-function-interface adapters may use this helper when constructing
 * DvzWindowExternalSurfaceInfo directly is undesirable. Native C and C++ callers should prefer
 * dvz_view_external_surface().
 *
 * @param app the app
 * @param figure the figure to render (borrowed)
 * @param instance borrowed VkInstance handle as an opaque pointer
 * @param surface borrowed or Datoviz-owned VkSurfaceKHR handle value
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return the view handle, or NULL on failure
 */
DVZ_EXPORT DvzView* dvz_ffi_view_external_surface(
    DvzApp* app, DvzFigure* figure, void* instance, uint64_t surface,
    uint32_t framebuffer_width, uint32_t framebuffer_height, float scale_x, float scale_y,
    bool owned_by_datoviz);


/**
 * Update a hosted external surface from FFI-friendly handles.
 *
 * Foreign-function-interface adapters may use this helper when constructing
 * DvzWindowExternalSurfaceInfo directly is undesirable. Native C and C++ callers should prefer
 * dvz_view_update_external_surface().
 *
 * @param view view created with dvz_view_external_surface() or dvz_ffi_view_external_surface()
 * @param instance borrowed VkInstance handle as an opaque pointer, or NULL for surface loss
 * @param surface borrowed or Datoviz-owned VkSurfaceKHR handle value, or zero for surface loss
 * @param framebuffer_width framebuffer width in physical pixels
 * @param framebuffer_height framebuffer height in physical pixels
 * @param scale_x horizontal content scale
 * @param scale_y vertical content scale
 * @param owned_by_datoviz whether Datoviz should destroy the surface
 * @return 0 on success, negative on error
 */
DVZ_EXPORT DvzResult dvz_ffi_view_update_external_surface(
    DvzView* view, void* instance, uint64_t surface, uint32_t framebuffer_width,
    uint32_t framebuffer_height, float scale_x, float scale_y, bool owned_by_datoviz);



/*************************************************************************************************/
/*  Descriptor initializers                                                                      */
/*************************************************************************************************/

/**
 * Initialize a default arrow geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_arrow_desc(DvzGeometryArrowDesc* out);


/**
 * Initialize a default cone geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_cone_desc(DvzGeometryConeDesc* out);


/**
 * Initialize a default cube geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_cube_desc(DvzGeometryCubeDesc* out);


/**
 * Initialize a default cylinder geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_cylinder_desc(DvzGeometryCylinderDesc* out);


/**
 * Initialize a default disc geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_disc_desc(DvzGeometryDiscDesc* out);


/**
 * Initialize a default plane geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_plane_desc(DvzGeometryPlaneDesc* out);


/**
 * Initialize a default regular-polygon geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_regular_polygon_desc(DvzGeometryRegularPolygonDesc* out);


/**
 * Initialize a default sector geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_sector_desc(DvzGeometrySectorDesc* out);


/**
 * Initialize a default sphere geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_sphere_desc(DvzGeometrySphereDesc* out);


/**
 * Initialize a default star geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_star_desc(DvzGeometryStarDesc* out);


/**
 * Initialize a default surface-grid geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_surface_grid_desc(DvzGeometrySurfaceGridDesc* out);


/**
 * Initialize a default torus geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_geometry_torus_desc(DvzGeometryTorusDesc* out);


/**
 * Initialize a default polygon descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_polygon_desc(DvzPolygonDesc* out);


/**
 * Initialize a default sampled-field geometry descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_field_geometry(DvzFieldGeometry* out);


/**
 * Initialize a default reference-grid descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_reference_grid_desc(DvzReferenceGridDesc* out);


/**
 * Initialize a default visual transform descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_visual_transform_desc(DvzVisualTransformDesc* out);


/**
 * Initialize a default panel background descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_panel_background_desc(DvzPanelBackgroundDesc* out);


/**
 * Initialize a default material descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_material_desc(DvzMaterialDesc* out);


/**
 * Initialize a default Phong material descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_phong_material_desc(DvzMaterialDesc* out);


/**
 * Initialize a default standard material descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_standard_material_desc(DvzMaterialDesc* out);


/**
 * Initialize a default limb material descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_limb_material_desc(DvzMaterialDesc* out);


/**
 * Initialize a default depth-cue descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_depth_cue_desc(DvzDepthCueDesc* out);


/**
 * Initialize a default scale-bar descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_scale_bar_desc(DvzScaleBarDesc* out);


/**
 * Initialize a default overlay-card style descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_overlay_card_style(DvzOverlayCardStyle* out);


/**
 * Initialize a default overlay-card descriptor through an out pointer.
 *
 * @param out output descriptor
 * @return true on success, false when out is NULL
 */
DVZ_EXPORT bool dvz_ffi_overlay_card_desc(DvzOverlayCardDesc* out);


EXTERN_C_OFF
