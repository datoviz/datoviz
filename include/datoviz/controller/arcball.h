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
#include "datoviz/common/types.h"
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
typedef struct DvzArcballState DvzArcballState;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArcballState
{
    float zoom;
    vec2 pan;
    bool interacting;
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
 *
 * @param arcball arcball controller
 * @param angles initial Euler angles
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_initial(DvzArcball* arcball, vec3 angles);



/**
 * Reset to the initial orientation.
 *
 * @param arcball arcball controller
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_reset(DvzArcball* arcball);



/**
 * Set the orientation directly from Euler angles.
 *
 * @param arcball arcball controller
 * @param angles Euler angles
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_set(DvzArcball* arcball, vec3 angles);


/**
 * Apply an incremental rotation around an axis to the accumulated orientation.
 *
 * @param arcball arcball controller
 * @param angle rotation angle in radians
 * @param axis rotation axis
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_rotate_axis(DvzArcball* arcball, float angle, vec3 axis);


/**
 * Set the camera dolly factor.
 *
 * @param arcball arcball controller
 * @param zoom uniform zoom factor
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_zoom(DvzArcball* arcball, float zoom);


/**
 * Set the camera view-plane pan offset.
 *
 * @param arcball arcball controller
 * @param pan panel-plane pan offset
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_pan(DvzArcball* arcball, vec2 pan);


/**
 * Copy the current arcball state.
 *
 * @param arcball arcball controller
 * @param out target state snapshot
 * @return whether the state was written
 */
DVZ_EXPORT bool dvz_arcball_state(const DvzArcball* arcball, DvzArcballState* out);


/**
 * Apply an incremental panel-plane pan shift in pixels.
 *
 * @param arcball arcball controller
 * @param shift_px shift in viewport pixels
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_pan_shift(DvzArcball* arcball, vec2 shift_px);



/**
 * Update the viewport size (call on window resize).
 *
 * @param arcball arcball controller
 * @param width viewport width in pixels
 * @param height viewport height in pixels
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_resize(DvzArcball* arcball, float width, float height);



/**
 * Set a rotation constraint axis.
 *
 * @param arcball arcball controller
 * @param axis rotation constraint axis
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_constrain(DvzArcball* arcball, vec3 axis);



/**
 * Read current Euler angles.
 */
DVZ_EXPORT void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles);



/**
 * Apply an in-flight rotation from two NDC screen positions.
 *
 * @param arcball arcball controller
 * @param cur_pos current pointer position in normalized device coordinates
 * @param last_pos previous pointer position in normalized device coordinates
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos);



/**
 * Compute the model matrix (accumulated × in-flight rotation).
 *
 * Arcball pan and zoom are camera view state and are not included here.
 *
 * @param arcball arcball controller
 * @param model output model matrix
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_model(DvzArcball* arcball, mat4 model);



/**
 * Commit the in-flight rotation into the accumulated matrix (call at drag stop).
 *
 * @param arcball arcball controller
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_end(DvzArcball* arcball);



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
 *
 * @param arcball arcball controller
 * @param router input router
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_connect(DvzArcball* arcball, DvzInputRouter* router);



/**
 * Unsubscribe the arcball from a router.
 *
 * @param arcball arcball controller
 * @param router input router
 * @return DVZ_OK on success, DVZ_ERROR on validation error
 */
DVZ_EXPORT DvzResult dvz_arcball_disconnect(DvzArcball* arcball, DvzInputRouter* router);



/**
 * Destroy the arcball.
 */
DVZ_EXPORT void dvz_arcball_destroy(DvzArcball* arcball);



EXTERN_C_OFF
