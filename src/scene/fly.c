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

#define DVZ_FLY_MOVE_SPEED  0.1f
#define DVZ_FLY_MOUSE_SPEED 0.5f
#define DVZ_FLY_ROLL_SPEED  30.0f
#define DVZ_FLY_WHEEL_SPEED 3.0f



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
    glm_vec3_scale(front, amount * DVZ_FLY_MOVE_SPEED, move);
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
    glm_vec3_scale(right, amount * DVZ_FLY_MOVE_SPEED, move);
    glm_vec3_add(fly->position, move, fly->position);
}



void dvz_fly_move_up(DvzFly* fly, float amount)
{
    ANN(fly);
    vec3 front, up;
    _update_vectors(fly, front, up);

    // Move along up vector
    vec3 move;
    glm_vec3_scale(up, amount * DVZ_FLY_MOVE_SPEED, move);
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
    fly->yaw = fly->yaw_init + dx;
    fly->pitch = fly->pitch_init + dy;

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
    case DVZ_MOUSE_EVENT_PRESS:
        if (ev->button == DVZ_MOUSE_BUTTON_LEFT || ev->button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            // Store initial angles at press
            fly->yaw_init = fly->yaw;
            fly->pitch_init = fly->pitch;
            fly->roll_init = fly->roll;
            return true;
        }
        break;

    case DVZ_MOUSE_EVENT_DRAG:
        if (ev->button == DVZ_MOUSE_BUTTON_LEFT)
        {
            // Calculate the normalized displacement from press position.
            float dx = DVZ_FLY_MOUSE_SPEED * (ev->pos[0] - ev->content.d.press_pos[0]) /
                       fly->viewport_size[0];
            float dy = DVZ_FLY_MOUSE_SPEED * (ev->pos[1] - ev->content.d.press_pos[1]) /
                       fly->viewport_size[1];

            // Mouse look (yaw/pitch)
            dvz_fly_rotate(fly, dx * M_PI, -dy * M_PI);
            return true;
        }
        else if (ev->button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            // Roll with right mouse drag (horizontal only), normalized by viewport width
            float dx = DVZ_FLY_ROLL_SPEED * //
                       (ev->pos[0] - ev->content.d.last_pos[0]) / fly->viewport_size[0];
            float dy = -DVZ_FLY_ROLL_SPEED * //
                       (ev->pos[1] - ev->content.d.last_pos[1]) / fly->viewport_size[1];

            dvz_fly_move_right(fly, dx);
            dvz_fly_move_up(fly, dy);
            // float roll = fly->roll_init + dx * M_PI;

            // // Normalize roll angle to [-PI, PI]
            // if (roll > M_PI)
            //     roll -= 2 * M_PI;
            // if (roll < -M_PI)
            //     roll += 2 * M_PI;

            // fly->roll = roll;
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
        dvz_fly_move_forward(fly, 1.0f);
        return true;
    case DVZ_KEY_DOWN:
        dvz_fly_move_forward(fly, -1.0f);
        return true;
    case DVZ_KEY_LEFT:
        dvz_fly_move_right(fly, -1.0f);
        return true;
    case DVZ_KEY_RIGHT:
        dvz_fly_move_right(fly, 1.0f);
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
