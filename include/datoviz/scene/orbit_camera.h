/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene orbit camera controller                                                                */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/orbit_camera.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned orbit-camera controller.
 *
 * @param scene the scene
 * @param desc orbit camera descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DVZ_EXPORT DvzController* dvz_orbit_camera(
    DvzScene* scene, const DvzOrbitCameraDesc* desc);



EXTERN_C_OFF
