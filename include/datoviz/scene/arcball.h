/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene arcball controller                                                                     */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/arcball.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned arcball controller.
 *
 * @param scene the scene
 * @param desc arcball descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DVZ_EXPORT DvzController* dvz_arcball(DvzScene* scene, const DvzArcballDesc* desc);



EXTERN_C_OFF
