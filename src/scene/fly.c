/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "datoviz/scene/fly.h"
#include "datoviz.h"
#include "datoviz/scene/camera.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FLY_LOOK_SPEED     0.5f
#define DVZ_FLY_MOVE_SPEED     5.0f
#define DVZ_FLY_KEYBOARD_SPEED .25f

#if OS_MACOS
#define DVZ_FLY_WHEEL_SPEED -0.1f
#else
#define DVZ_FLY_WHEEL_SPEED 0.3f
#endif


/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _update_vectors(DvzFly* fly, vec3 front, vec3 up)
{
    // Calculate front vector from yaw and pitch (spherical coordinates)
    vec3 front_ = {
        cos(fly->yaw) * cos(fly->pitch), sin(fly->pitch), sin(fly->yaw) * cos(fly->pitch)};
    glm_vec3_normalize(front_);
    glm_vec3_copy(front_, front);

    // Right vector
    vec3 world_up = {0.0f, 1.0f, 0.0f}; // temp up vector
    vec3 right;
    glm_vec3_cross(front, world_up, right);
    glm_vec3_normalize(right);

    // Up vector
    glm_vec3_cross(right, front, up);
    glm_vec3_normalize(up);

    // Apply roll rotation to both right and up vectors
    mat4 roll_matrix;
    glm_mat4_identity(roll_matrix);
    glm_rotate(roll_matrix, fly->roll, front);

    // Update right vector with roll
    glm_mat4_mulv3(roll_matrix, right, 1.0f, right);
    // Update up vector with roll
    glm_mat4_mulv3(roll_matrix, up, 1.0f, up);
}



/*************************************************************************************************/
/*  Fly camera functions                                                                         */
/*************************************************************************************************/

DvzFly* dvz_fly(int flags)
{
    DvzFly* fly = calloc(1, sizeof(DvzFly));
    fly->flags = flags;

    // Default initial position and angles
    vec3 pos = {DVZ_CAMERA_DEFAULT_POSITION};
    dvz_fly_initial(fly, pos, -M_PI_2, 0.0f, 0.0f); // Look along -Z

    // Default viewport size
    fly->viewport_size[0] = 800;
    fly->viewport_size[1] = 600;

    return fly;
}



void dvz_fly_resize(DvzFly* fly, float width, float height)
{
    ANN(fly);
    fly->viewport_size[0] = width;
    fly->viewport_size[1] = height;
}



void dvz_fly_initial(DvzFly* fly, vec3 position, float yaw, float pitch, float roll)
{
    ANN(fly);
    glm_vec3_copy(position, fly->position_init);
    fly->yaw_init = yaw;
    fly->pitch_init = pitch;
    fly->roll_init = roll;

    dvz_fly_reset(fly);
}



void dvz_fly_initial_lookat(DvzFly* fly, vec3 position, vec3 lookat)
{
    ANN(fly);
    glm_vec3_copy(position, fly->position_init);
    glm_vec3_copy(position, fly->position);

    // Compute the direction vector from the position to the lookat point
    vec3 dir;
    glm_vec3_sub(lookat, fly->position, dir);
    glm_vec3_normalize(dir);

    // Compute pitch and yaw from the direction vector
    // pitch is the angle from the XZ plane (vertical)
    // yaw is the angle from the +X axis in the XZ plane (horizontal)

    fly->pitch_init = asinf(dir[1]); // y component directly gives the pitch

    // yaw = atan2(z, x)
    fly->yaw_init = atan2f(dir[2], dir[0]);

    // Note: roll is not affected

    dvz_fly_reset(fly);
}



void dvz_fly_reset(DvzFly* fly)
{
    ANN(fly);
    glm_vec3_copy(fly->position_init, fly->position);
    fly->yaw = fly->yaw_init;
    fly->pitch = fly->pitch_init;
    fly->roll = fly->roll_init;
}



void dvz_fly_move_forward(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);

    // Move in the direction we're looking
    vec3 move;
    glm_vec3_scale(front, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
}



void dvz_fly_move_right(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);

    // Move perpendicular to view direction and up vector
    vec3 right, move;
    glm_vec3_cross(front, up, right);
    glm_vec3_normalize(right);
    glm_vec3_scale(right, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
}



void dvz_fly_move_up(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);

    // Move along up vector
    vec3 move;
    glm_vec3_scale(up, amount, move);
    glm_vec3_add(fly->position, move, fly->position);
}



void dvz_fly_rotate(DvzFly* fly, float dx, float dy)
{
    ANN(fly);

    if (fly->flags & DVZ_FLY_FLAGS_INVERT_MOUSE)
    {
        dx = -dx;
        dy = -dy;
    }

    // Set absolute angles based on displacement from press position
    fly->yaw = fly->yaw + dx;
    fly->pitch = fly->pitch + dy;

    // Constrain pitch to avoid gimbal lock
    fly->pitch = CLIP(fly->pitch, -M_PI_2 + 0.1, M_PI_2 - 0.1);
}



void dvz_fly_roll(DvzFly* fly, float dx)
{
    ANN(fly);
    fly->roll += dx;

    // Normalize roll angle to [-PI, PI]
    if (fly->roll > M_PI)
        fly->roll -= 2 * M_PI;
    if (fly->roll < -M_PI)
        fly->roll += 2 * M_PI;
}



void dvz_fly_get_position(DvzFly* fly, vec3 out_pos)
{
    ANN(fly);
    glm_vec3_copy(fly->position, out_pos);
}



void dvz_fly_get_lookat(DvzFly* fly, vec3 out_lookat)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);

    // Position + front direction = lookat point
    glm_vec3_add(fly->position, front, out_lookat);
}



void dvz_fly_set_lookat(DvzFly* fly, vec3 lookat)
{
    ANN(fly);

    // Compute the direction vector from the position to the lookat point
    vec3 dir;
    glm_vec3_sub(lookat, fly->position, dir);
    glm_vec3_normalize(dir);

    // Compute pitch and yaw from the direction vector
    // pitch is the angle from the XZ plane (vertical)
    // yaw is the angle from the +X axis in the XZ plane (horizontal)

    fly->pitch = asinf(dir[1]); // y component directly gives the pitch

    // yaw = atan2(z, x)
    fly->yaw = atan2f(dir[2], dir[0]);

    // Note: roll is not affected
}



void dvz_fly_get_up(DvzFly* fly, vec3 out_up)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);
    glm_vec3_copy(up, out_up);
}



bool dvz_fly_mouse(DvzFly* fly, DvzMouseEvent* ev)
{
    ANN(fly);

    switch (ev->type)
    {
    case DVZ_MOUSE_EVENT_DRAG:
        if (ev->button == DVZ_MOUSE_BUTTON_LEFT)
        {
            // Calculate the normalized displacement from press position.
            float dx = DVZ_FLY_LOOK_SPEED * (ev->pos[0] - ev->content.d.last_pos[0]) /
                       fly->viewport_size[0];
            float dy = DVZ_FLY_LOOK_SPEED * (ev->pos[1] - ev->content.d.last_pos[1]) /
                       fly->viewport_size[1];

            // Mouse look (yaw/pitch)
            dvz_fly_rotate(fly, dx * M_PI, -dy * M_PI);
            return true;
        }
        else if (
            ev->button == DVZ_MOUSE_BUTTON_MIDDLE ||
            (ev->button == DVZ_MOUSE_BUTTON_RIGHT && (ev->mods & DVZ_KEY_MODIFIER_CONTROL)))
        {
            // Roll with right mouse drag (horizontal only), normalized by viewport width
            float dx = DVZ_FLY_MOVE_SPEED * //
                       (ev->pos[0] - ev->content.d.last_pos[0]) / fly->viewport_size[0];
            float dy = -DVZ_FLY_MOVE_SPEED * //
                       (ev->pos[1] - ev->content.d.last_pos[1]) / fly->viewport_size[1];

            dvz_fly_move_right(fly, dx);
            dvz_fly_move_up(fly, dy);

            return true;
        }
        else if (ev->button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            // Compute drag deltas normalized by viewport size
            float dx = (ev->pos[0] - ev->content.d.last_pos[0]) / fly->viewport_size[0];
            float dy = (ev->pos[1] - ev->content.d.last_pos[1]) / fly->viewport_size[1];

            // Sensitivity scaling (adjust if needed)
            float speed = M_2PI * DVZ_FLY_LOOK_SPEED; // One full drag moves 360 degrees
            dx *= -speed;
            dy *= speed;

            // Step 1: compute initial distance (|V| = distance from initial pos to origin)
            vec3 V = {-fly->position_init[0], -fly->position_init[1], -fly->position_init[2]};
            float radius = glm_vec3_norm(V);

            // Step 2: compute current front vector (from position to lookat)
            vec3 lookat;
            dvz_fly_get_lookat(fly, lookat);
            vec3 front;
            glm_vec3_sub(lookat, fly->position, front);
            glm_vec3_normalize(front);

            // Step 3: compute orbit center = position + radius * front
            vec3 center;
            glm_vec3_scale(front, radius, center);
            glm_vec3_add(fly->position, center, center);

            // Step 4: get spherical coordinates of position relative to center
            vec3 rel;
            glm_vec3_sub(fly->position, center, rel);
            float r = glm_vec3_norm(rel);
            float azimuth = atan2f(rel[2], rel[0]);             // around Y axis
            float inclination = acosf(CLIP(rel[1] / r, -1, 1)); // from +Y

            // Step 5: apply deltas
            azimuth -= dx;
            inclination -= dy;

            // Clamp inclination to avoid pole singularities
            float eps = 0.01f;
            inclination = CLIP(inclination, eps, M_PI - eps);

            // Step 6: convert back to Cartesian and update position
            fly->position[0] = center[0] + r * sinf(inclination) * cosf(azimuth);
            fly->position[1] = center[1] + r * cosf(inclination);
            fly->position[2] = center[2] + r * sinf(inclination) * sinf(azimuth);

            // Always look at the dynamic center
            dvz_fly_set_lookat(fly, center);

            return true;
        }



        break;

    case DVZ_MOUSE_EVENT_DOUBLE_CLICK:
        dvz_fly_reset(fly);
        return true;

    case DVZ_MOUSE_EVENT_WHEEL:;
        float w = ev->content.w.dir[1];
        float dx = DVZ_FLY_WHEEL_SPEED * w;
        dvz_fly_move_forward(fly, dx);
        return true;

    default:
        break;
    }

    return false;
}



bool dvz_fly_keyboard(DvzFly* fly, DvzKeyboardEvent* ev)
{
    ANN(fly);

    if (ev->type != DVZ_KEYBOARD_EVENT_PRESS && ev->type != DVZ_KEYBOARD_EVENT_REPEAT)
        return false;

    switch (ev->key)
    {
    case DVZ_KEY_UP:
        dvz_fly_move_forward(fly, DVZ_FLY_KEYBOARD_SPEED);
        return true;
    case DVZ_KEY_DOWN:
        dvz_fly_move_forward(fly, -DVZ_FLY_KEYBOARD_SPEED);
        return true;
    case DVZ_KEY_LEFT:
        dvz_fly_move_right(fly, -DVZ_FLY_KEYBOARD_SPEED);
        return true;
    case DVZ_KEY_RIGHT:
        dvz_fly_move_right(fly, DVZ_FLY_KEYBOARD_SPEED);
        return true;
    default:
        break;
    }

    return false;
}



void dvz_fly_destroy(DvzFly* fly)
{
    ANN(fly);
    FREE(fly);
}
