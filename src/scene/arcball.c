/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

// References:
// https://github.com/Twinklebear/arcball-cpp
// https://tommyhinks.com/wp-content/uploads/2012/02/shoemake92_arcball.pdf


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/arcball.h"
#include "datoviz.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/

static void _screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    // If we're on/in the sphere return the point on it
    if (dist <= 1.f)
    {
        glm_vec4_copy((vec4){p[0], p[1], sqrt(1 - dist), 0}, q);
    }
    else
    {
        // otherwise we project the point onto the sphere
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0, 0}, q);
    }
}



static void _constrain(versor q, vec3 axis)
{
    glm_vec3_normalize(axis);
    // adapted from http://www.talisman.org/~erlkonig/misc/shoemake92-arcball.pdf page 4
    float dot = glm_vec3_dot(q, axis);
    vec3 proj, t;
    glm_vec3_scale(axis, dot, t);
    glm_vec3_sub(q, t, proj);
    float norm = glm_vec3_norm(proj);
    if (norm > 0)
    {
        float s = proj[2] >= 0 ? 1 / norm : -1 / norm;
        glm_vec3_scale(proj, s, q);
    }
    else if (axis[2] == 1)
    {
        glm_vec3_copy((vec3){1, 0, 0}, q);
    }
    else
    {
        glm_vec3_normalize_to((vec3){-axis[1], axis[0]}, q);
    }
}



/*************************************************************************************************/
/*  Arcball functions                                                                            */
/*************************************************************************************************/

DvzArcball* dvz_arcball(float width, float height, int flags)
{
    // width, width are the inner viewport size
    DvzArcball* arcball = (DvzArcball*)calloc(1, sizeof(DvzArcball));

    arcball->flags = flags;
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;

    dvz_arcball_reset(arcball);

    return arcball;
}



void dvz_arcball_initial(DvzArcball* arcball, vec3 angles)
{
    ANN(arcball);
    glm_vec3_copy(angles, arcball->init);
    dvz_arcball_reset(arcball);
}



void dvz_arcball_reset(DvzArcball* arcball)
{
    ANN(arcball);

    dvz_arcball_set(arcball, arcball->init); // this sets arcball->mat

    // glm_mat4_identity(arcball->mat);
    glm_quat_identity(arcball->rotation);
}



void dvz_arcball_set(DvzArcball* arcball, vec3 angles)
{
    ANN(arcball);
    glm_euler(angles, arcball->mat);
}



void dvz_arcball_resize(DvzArcball* arcball, float width, float height)
{
    ANN(arcball);
    arcball->viewport_size[0] = width;
    arcball->viewport_size[1] = height;
}



void dvz_arcball_flags(DvzArcball* arcball, int flags)
{
    ANN(arcball);
    arcball->flags = flags;
}



void dvz_arcball_constrain(DvzArcball* arcball, vec3 constrain)
{
    ANN(arcball);
    if (glm_vec3_norm(constrain) == 0)
    {
        log_warn("null arcball constrain axis, ignoring constrain");
        return;
    }
    glm_vec3_normalize(arcball->constrain);
    glm_vec3_copy(constrain, arcball->constrain);
    arcball->flags |= DVZ_ARCBALL_FLAGS_CONSTRAIN;
}



void dvz_arcball_angles(DvzArcball* arcball, vec3 out_angles)
{
    ANN(arcball);
    // mat4 rot, model;
    // glm_quat_mat4(arcball->rotation, rot);
    // glm_mat4_mul(rot, arcball->mat, model);
    glm_euler_angles(arcball->mat, out_angles);
}



void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    ANN(arcball);

    versor mouse_cur_ball = {0}, mouse_prev_ball = {0};
    _screen_to_arcball(cur_pos, mouse_cur_ball);
    _screen_to_arcball(last_pos, mouse_prev_ball);

    if ((arcball->flags & DVZ_ARCBALL_FLAGS_CONSTRAIN) != 0)
    {
        _constrain(mouse_cur_ball, arcball->constrain);
        _constrain(mouse_prev_ball, arcball->constrain);
    }

    glm_quat_identity(arcball->rotation);
    glm_quat_mul(mouse_prev_ball, arcball->rotation, arcball->rotation);
    glm_quat_mul(mouse_cur_ball, arcball->rotation, arcball->rotation);
}



// void dvz_arcball_pan(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
// {
//     ANN(arcball);

//     vec3 delta = {0};
//     glm_vec2_sub(last_pos, cur_pos, delta);
//     // glm_vec2_scale(delta, -.5 * arcball->camera.eye[2], delta);
//     // Convert translation vector back to original coordinate system.
//     glm_mat4_mulv3(arcball->inv_model, delta, 1, delta);
//     glm_translate(arcball->translate, delta);
// }



void dvz_arcball_model(DvzArcball* arcball, mat4 model)
{
    ANN(arcball);

    mat4 rot;
    glm_quat_mat4(arcball->rotation, rot);

    // model=rot*mat
    glm_mat4_mul(rot, arcball->mat, model);
}



void dvz_arcball_end(DvzArcball* arcball)
{
    ANN(arcball);
    mat4 rot;
    glm_quat_mat4(arcball->rotation, rot);

    glm_mat4_mul(rot, arcball->mat, arcball->mat);
    // glm_mat4_inv(arcball->mat, arcball->mat_inv);
    glm_quat_identity(arcball->rotation);
}



void dvz_arcball_mvp(DvzArcball* arcball, DvzMVP* mvp)
{
    ANN(arcball);
    ANN(mvp);
    dvz_arcball_model(arcball, mvp->model);
}



void dvz_arcball_print(DvzArcball* arcball)
{
    ANN(arcball);
    mat4 model;
    dvz_arcball_model(arcball, model);
    glm_mat4_print(model, stdout);
}



static inline void _arcball_gui(DvzApp* app, DvzId canvas_id, DvzGuiEvent* ev)
{
    ANN(app);

    DvzArcball* arcball = (DvzArcball*)ev->user_data;
    ANN(arcball);

    DvzPanel* panel = (DvzPanel*)arcball->user_data;

    dvz_gui_corner(DVZ_DIALOG_CORNER_BOTTOM_RIGHT, (vec2){20, 20});
    dvz_gui_size((vec2){180, 120});
    dvz_gui_begin("Arcball angles", 0);

    vec3 angles = {0};
    dvz_arcball_angles(arcball, angles);
    dvz_gui_slider("x", -M_PI, M_PI, &angles[0]);
    dvz_gui_slider("y", -M_PI / 2 + .001, M_PI / 2 - .001, &angles[1]);
    dvz_gui_slider("z", -M_PI, M_PI, &angles[2]);
    dvz_arcball_set(arcball, angles);

    if (panel != NULL)
        dvz_panel_update(panel);

    dvz_gui_end();
}

void dvz_arcball_gui(DvzArcball* arcball, DvzApp* app, DvzId canvas_id, DvzPanel* panel)
{
    ANN(arcball);
    ANN(app);
    ASSERT(canvas_id != DVZ_ID_NONE);

    if (panel != NULL)
        arcball->user_data = (void*)panel;
    dvz_app_gui(app, canvas_id, _arcball_gui, arcball);
}



void dvz_arcball_destroy(DvzArcball* arcball)
{
    ANN(arcball);
    FREE(arcball);
}



/*************************************************************************************************/
/*  Arcball event functions                                                                      */
/*************************************************************************************************/

bool dvz_arcball_mouse(DvzArcball* arcball, DvzMouseEvent* ev)
{
    ANN(arcball);

    switch (ev->type)
    {
    // Dragging: pan.
    case DVZ_MOUSE_EVENT_DRAG:
        if (ev->button == DVZ_MOUSE_BUTTON_LEFT)
        {
            float width = arcball->viewport_size[0];
            float height = arcball->viewport_size[1];

            vec2 cur_pos, last_pos;
            cur_pos[0] = -1 + 2 * ev->pos[0] / width;
            cur_pos[1] = +1 - 2 * ev->pos[1] / height;
            last_pos[0] = -1 + 2 * ev->content.d.press_pos[0] / width; // press position
            last_pos[1] = +1 - 2 * ev->content.d.press_pos[1] / height;

            dvz_arcball_rotate(arcball, cur_pos, last_pos);
        }
        break;

    // Stop dragging.
    case DVZ_MOUSE_EVENT_DRAG_STOP:
        dvz_arcball_end(arcball);
        break;

    // Double-click
    case DVZ_MOUSE_EVENT_DOUBLE_CLICK:
        dvz_arcball_reset(arcball);
        break;

    default:
        return false;
    }

    return true;
}
