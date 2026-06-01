/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Controller internals                                                                         */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/controller/arcball.h"
#include "datoviz/controller/camera.h"
#include "datoviz/controller/fly.h"
#include "datoviz/controller/orbit_camera.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/controller/turntable.h"



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzCamera* _dvz_camera(const DvzCameraDesc* desc);

DvzPanzoom* _dvz_panzoom(float width, float height, int flags);

DvzArcball* _dvz_arcball(float width, float height, int flags);

void _dvz_arcball_view(DvzArcball* arcball, mat4 view);

void _dvz_arcball_clear_view(DvzArcball* arcball);

DvzFly* _dvz_fly(const DvzFlyDesc* desc);

DvzTurntable* _dvz_turntable(const DvzTurntableDesc* desc);

void dvz_orbit_camera_connect(DvzOrbitCamera* orbit, DvzInputRouter* router);

void dvz_orbit_camera_disconnect(DvzOrbitCamera* orbit, DvzInputRouter* router);



EXTERN_C_OFF
