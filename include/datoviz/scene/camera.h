/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Scene camera                                                                                 */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/camera.h"
#include "datoviz/scene/types.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Set or replace the camera descriptor attached to a panel.
 *
 * @param panel the panel
 * @param desc the camera descriptor, or NULL for defaults
 * @return 0 on success, -1 on validation or allocation error
 */
DVZ_EXPORT DvzResult dvz_panel_set_camera_desc(DvzPanel* panel, const DvzCameraDesc* desc);


/**
 * Return the camera attached to a panel.
 *
 * The returned camera is borrowed panel-owned state. Do not pass it to `dvz_camera_destroy()`.
 *
 * @param panel the panel
 * @return the panel-owned camera, or NULL
 */
DVZ_EXPORT DvzCamera* dvz_panel_camera(DvzPanel* panel);



EXTERN_C_OFF
