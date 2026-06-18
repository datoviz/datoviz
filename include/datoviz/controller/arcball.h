/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Arcball controller                                                                           */
/*************************************************************************************************/
/* Advanced/unstable standalone controller internals. This standalone API includes legacy
 * camera-view pan/zoom helpers; scene-owned DvzArcball remains object/model oriented. */

// References:
// https://github.com/Twinklebear/arcball-cpp
// https://tommyhinks.com/wp-content/uploads/2012/02/shoemake92_arcball.pdf

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/input/pointer.h"
#include "datoviz/input/router.h"
#include "datoviz/math/types.h"
#include "datoviz/controller/panzoom.h" /* for DvzMVP */



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_ARCBALL_FLAGS_NONE      = 0x00,
    DVZ_ARCBALL_FLAGS_CONSTRAIN = 0x01,
} DvzArcballFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArcball DvzArcball;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArcball
{
    vec2   viewport_size;
    int    flags;

    mat4 mat;        /* accumulated model matrix */
    vec3 init;       /* initial Euler angles (used by reset) */
    vec4 rotation;   /* in-flight quaternion (while dragging); same layout as cglm versor */
    vec3 constrain;  /* constrain axis; null if no constraint */
    mat4 view;        /* optional camera view used to interpret drag axes */
    float zoom;      /* multiplicative camera dolly factor */
    vec2 pan;        /* camera view-plane pan offset */
    vec2 pan_center; /* committed pan baseline used during right/middle drag */
    bool has_view;   /* true when view carries a camera-space drag basis */
    bool interacting; /* true while the pointer is controlling the arcball */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

typedef struct DvzArcballDesc DvzArcballDesc;



struct DvzArcballDesc
{
    uint32_t struct_size;
    uint32_t flags;
    float width;
    float height;
    uint32_t controller_flags;
};



DVZ_EXPORT DvzArcballDesc dvz_arcball_desc(void);



/**
 * Create a standalone arcball controller.
 *
 * @param desc arcball descriptor, or NULL for defaults
 * @return the controller, or NULL on allocation failure
 */
DVZ_EXPORT DvzArcball* dvz_arcball_create(const DvzArcballDesc* desc);



/**
 * Set the initial Euler angles and reset.
 */
DVZ_EXPORT void dvz_arcball_initial(DvzArcball* arcball, vec3 angles);



/**
 * Reset to the initial orientation.
 */
DVZ_EXPORT void dvz_arcball_reset(DvzArcball* arcball);



/**
 * Set the orientation directly from Euler angles.
 */
DVZ_EXPORT void dvz_arcball_set(DvzArcball* arcball, vec3 angles);


/**
 * Apply an incremental rotation around an axis to the accumulated orientation.
 *
 * @param arcball arcball controller
 * @param angle rotation angle in radians
 * @param axis rotation axis
 */
DVZ_EXPORT void dvz_arcball_rotate_axis(DvzArcball* arcball, float angle, vec3 axis);


/**
 * Set the camera dolly factor.
 *
 * @param arcball arcball controller
 * @param zoom uniform zoom factor
 */
DVZ_EXPORT void dvz_arcball_zoom(DvzArcball* arcball, float zoom);


/**
 * Set the camera view-plane pan offset.
 *
 * @param arcball arcball controller
 * @param pan panel-plane pan offset
 */
DVZ_EXPORT void dvz_arcball_pan(DvzArcball* arcball, vec2 pan);


/**
 * Apply an incremental panel-plane pan shift in pixels.
 *
 * @param arcball arcball controller
 * @param shift_px shift in viewport pixels
 */
DVZ_EXPORT void dvz_arcball_pan_shift(DvzArcball* arcball, vec2 shift_px);



/**
 * Update the viewport size (call on window resize).
 */
DVZ_EXPORT void dvz_arcball_resize(DvzArcball* arcball, float width, float height);



/**
 * Set a rotation constraint axis.
 */
DVZ_EXPORT void dvz_arcball_constrain(DvzArcball* arcball, vec3 axis);



/**
 * Read current Euler angles.
 */
DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles);



/**
 * Apply an in-flight rotation from two NDC screen positions.
 */
DVZ_EXPORT void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);



/**
 * Compute the model matrix (accumulated × in-flight rotation).
 *
 * Arcball pan and zoom are camera view state and are not included here.
 */
DVZ_EXPORT void dvz_arcball_model(DvzArcball* arcball, mat4 model);



/**
 * Commit the in-flight rotation into the accumulated matrix (call at drag stop).
 */
DVZ_EXPORT void dvz_arcball_end(DvzArcball* arcball);



/**
 * Fill the MVP struct from the current arcball state.
 * Rotation is applied to the model matrix; pan and zoom are applied to view.
 */
DVZ_EXPORT void dvz_arcball_mvp(DvzArcball* arcball, DvzMVP* mvp);


/**
 * Return whether the pointer is currently interacting with the arcball.
 *
 * @returns true while the user is pressing or dragging the arcball
 */
DVZ_EXPORT bool dvz_arcball_is_interacting(DvzArcball* arcball);



/**
 * Process a pointer event and update arcball state.
 *
 * @returns true if the event was consumed
 */
DVZ_EXPORT bool dvz_arcball_pointer(DvzArcball* arcball, const DvzPointerEvent* ev);



/**
 * Subscribe the arcball to an input router.
 */
DVZ_EXPORT void dvz_arcball_connect(DvzArcball* arcball, DvzInputRouter* router);



/**
 * Unsubscribe the arcball from a router.
 */
DVZ_EXPORT void dvz_arcball_disconnect(DvzArcball* arcball, DvzInputRouter* router);



/**
 * Destroy the arcball.
 */
DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* arcball);



EXTERN_C_OFF
