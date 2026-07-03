/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Internal controller layouts                                                                  */
/*************************************************************************************************/

#pragma once

#include "datoviz/controller/arcball.h"
#include "datoviz/controller/fly.h"
#include "datoviz/controller/panzoom.h"
#include "datoviz/controller/turntable.h"
#include "datoviz/input/router.h"



struct DvzPanzoom
{
    vec2 viewport_origin;
    vec2 viewport_size;
    int  flags;
    bool has_viewport;
    bool interacting;
    DvzInputRouter* input_router;
    DvzCallbackId input_subscription_id;

    vec2 pan;
    vec2 pan_center;
    vec2 zoom;
    vec2 zoom_center;
    vec2 zoom_min;
    vec2 zoom_max;
    bool has_zoom_limits;

    vec2 pan_lock;
    vec2 zoom_lock;
    bool pan_locked[2];
    bool zoom_locked[2];
};



struct DvzArcball
{
    vec2   viewport_size;
    int    flags;

    mat4 mat;         /* accumulated model matrix */
    vec3 init;        /* initial Euler angles used by reset */
    vec4 rotation;    /* in-flight quaternion while dragging; cglm versor layout */
    vec3 constrain;   /* constraint axis, or zero when unconstrained */
    mat4 view;        /* optional camera view used to interpret drag axes */
    float zoom;       /* multiplicative camera dolly factor */
    vec2 pan;         /* camera view-plane pan offset */
    vec2 pan_center;  /* committed pan baseline used during right/middle drag */
    bool has_view;    /* true when view carries a camera-space drag basis */
    bool interacting; /* true while the pointer is controlling the arcball */
    DvzInputRouter* input_router;
    DvzCallbackId input_subscription_id;
};



struct DvzFly
{
    DvzFlyMode mode;
    int flags;

    vec2 viewport_origin;
    vec2 viewport_size;
    bool has_viewport;
    bool interacting;
    DvzInputRouter* input_router;
    DvzCallbackId input_subscription_id;

    vec3 world_up;
    vec3 position;
    float yaw;
    float pitch;
    float roll;

    vec3 position_init;
    float yaw_init;
    float pitch_init;
    float roll_init;

    float speed;
    float fast_multiplier;
    float slow_multiplier;
    float look_speed;
    float wheel_speed;

    bool key_forward;
    bool key_backward;
    bool key_left;
    bool key_right;
    bool key_up;
    bool key_down;
    bool key_fast;
    bool key_slow;

    bool has_pivot;
    vec3 pivot;
    float pivot_distance;
    bool pivot_marker_visible;
    double pivot_marker_time_left;

    DvzCamera* camera;
};



struct DvzTurntable
{
    int flags;

    vec2 viewport_origin;
    vec2 viewport_size;
    bool has_viewport;
    bool interacting;
    DvzInputRouter* input_router;
    DvzCallbackId input_subscription_id;

    vec3 pivot;
    vec3 eye;
    vec3 up;

    vec3 pivot_init;
    float distance_init;
    float yaw_init;
    float pitch_init;

    float distance;
    float yaw;
    float pitch;

    float yaw_speed;
    float pitch_speed;
    float zoom_speed;
    float pan_speed;

    float min_pitch;
    float max_pitch;
    float min_distance;
    float max_distance;

    bool pivot_marker_visible;
    double pivot_marker_time_left;

    DvzCamera* camera;
};
