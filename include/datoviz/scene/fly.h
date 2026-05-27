/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene fly camera controller                                                                  */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/fly.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned fly camera controller.
 *
 * @param scene the scene
 * @param desc fly descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DVZ_EXPORT DvzController* dvz_fly(DvzScene* scene, const DvzFlyDesc* desc);



EXTERN_C_OFF
