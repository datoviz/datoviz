/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene turntable controller                                                                   */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/turntable.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene-owned turntable controller.
 *
 * @param scene the scene
 * @param desc turntable descriptor, or NULL for defaults
 * @return the scene-owned controller handle
 */
DVZ_EXPORT DvzController* dvz_turntable(DvzScene* scene, const DvzTurntableDesc* desc);



EXTERN_C_OFF
