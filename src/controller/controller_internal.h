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
    mat4 projection;  /* camera projection used for pixel-proportional panning */
    vec2 orbit_center; /* frozen pan-neutral pointer sphere center */
    mat4 orbit_view;        /* frozen camera basis for the current pointer rotation */
    vec3 orbit_pivot;       /* retained target in post-model world coordinates */
    vec3 orbit_translation; /* committed world translation, separate from orientation */
    bool orbit_active;      /* pointer rotation has a frozen basis and control center */
    float zoom;       /* multiplicative camera dolly factor */
    float zoom_speed; /* wheel sensitivity multiplier, independent of programmatic zoom */
    float orbit_zoom_start; /* adaptive orbit magnification threshold, default 1 */
    float orbit_zoom_end; /* fully viewport-centered magnification, default 3 */
    vec2 zoom_offset; /* view-plane dolly displacement along the screen-pan center ray */
    vec2 pan;         /* camera view-plane pan offset */
    vec2 pan_center;  /* committed pan baseline used during right/middle drag */
    float pan_depth; /* positive clip W of the press-time surface plane */
    float pan_depth_fallback; /* press-time pivot plane when no depth is available */
    vec2 pan_pixel_scale; /* frozen per-pixel scale before multiplying by clip W */
    vec2 pan_pending_shift; /* latest press-relative shift, also while query is pending */
    bool pan_depth_pending;
    bool pan_plane_active;
    bool pan_pending_stopped;
    uint64_t pan_epoch; /* invalidates pending navigation queries */
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

/**
 * Evaluate the world translation for the current pointer orbit.
 * @param arcball arcball controller
 * @param out current world translation
 */
void _dvz_arcball_orbit_translation(DvzArcball* arcball, vec3 out);

/**
 * Set the live world translation while preserving an active pointer rotation baseline.
 * @param arcball arcball controller
 * @param translation requested live world translation
 */
void _dvz_arcball_orbit_translation_set(DvzArcball* arcball, vec3 translation);

/**
 * Freeze the pan frame while awaiting GPU surface depth.
 * @param arcball arcball controller
 */
void _dvz_arcball_pan_depth_wait(DvzArcball* arcball);

/**
 * Resolve a press-time pan plane and apply the latest accumulated shift.
 * @param arcball arcball controller
 * @param clip_w positive surface clip W, or zero for the captured fallback plane
 */
void _dvz_arcball_pan_depth_resolve(DvzArcball* arcball, float clip_w);

/**
 * Invalidate a pending navigation depth without changing the rendered pose.
 * @param arcball arcball controller
 */
void _dvz_arcball_pan_depth_cancel(DvzArcball* arcball);
