/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/arcball.h"



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



/*************************************************************************************************/
/*  Arcball functions                                                                            */
/*************************************************************************************************/

DvzArcball dvz_arcball(float width, float height, int flags)
{
    // width, width are the inner viewport size
    DvzArcball arcball = {0};
    arcball.flags = flags;
    arcball.viewport_size[0] = width;
    arcball.viewport_size[1] = height;

    dvz_arcball_reset(&arcball);

    return arcball;
}



void dvz_arcball_reset(DvzArcball* arcball)
{
    ANN(arcball);

    dvz_arcball_angles(arcball, (vec3){0});

    // glm_mat4_identity(arcball->mat);
    glm_quat_identity(arcball->rotation);
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



void dvz_arcball_angles(DvzArcball* arcball, vec3 angles)
{
    ANN(arcball);

    mat4 m4;
    glm_vec3_negate(angles);
    glm_euler(angles, m4);
    mat3 m;
    glm_mat4_pick3(m4, m);
    glm_mat3_transpose(m);
    glm_mat3_quat(m, arcball->rotation);
    glm_quat_normalize(arcball->rotation);
}



void dvz_arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    ANN(arcball);

    versor mouse_cur_ball = {0}, mouse_prev_ball = {0};
    _screen_to_arcball(cur_pos, mouse_cur_ball);
    _screen_to_arcball(last_pos, mouse_prev_ball);

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

    glm_quat_mat4(arcball->rotation, model);
    // glm_mat4_copy(arcball->mat, model);
    // glm_mat4_inv(model, arcball->inv_model);
    // glm_mat4_mul(model, arcball->translate, model);
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



void dvz_arcball_destroy(DvzArcball* arcball) { ANN(arcball); }
