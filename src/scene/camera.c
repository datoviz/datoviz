/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/camera.h"
#include "_macros.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utility functions                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Camera functions                                                                             */
/*************************************************************************************************/

DvzCamera* dvz_camera(float width, float height, int flags)
{
    DvzCamera* camera = (DvzCamera*)calloc(1, sizeof(DvzCamera));

    camera->flags = flags;

    dvz_camera_resize(camera, width, height);

    dvz_camera_zrange(camera, DVZ_CAMERA_DEFAULT_ZRANGE);
    dvz_camera_perspective(camera, DVZ_CAMERA_DEFAULT_FOV);
    dvz_camera_initial(
        camera,                              //
        (vec3){DVZ_CAMERA_DEFAULT_POSITION}, //
        (vec3){DVZ_CAMERA_DEFAULT_LOOKAT},   //
        (vec3){DVZ_CAMERA_DEFAULT_UP});

    return camera;
}



void dvz_camera_initial(DvzCamera* camera, vec3 pos, vec3 lookat, vec3 up)
{
    ANN(camera);
    glm_vec3_copy(pos, camera->pos_init);
    glm_vec3_copy(lookat, camera->lookat_init);
    glm_vec3_copy(up, camera->up_init);

    dvz_camera_reset(camera);
}


void dvz_camera_reset(DvzCamera* camera)
{
    ANN(camera);
    glm_vec3_copy(camera->pos_init, camera->pos);
    glm_vec3_copy(camera->lookat_init, camera->lookat);
    glm_vec3_copy(camera->up_init, camera->up);
}



void dvz_camera_zrange(DvzCamera* camera, float near, float far)
{
    ANN(camera);
    camera->near = near;
    camera->far = far;
}



void dvz_camera_ortho(DvzCamera* camera, float left, float right, float bottom, float top)
{
    ANN(camera);
    camera->left = left;
    camera->right = right;
    camera->bottom = bottom;
    camera->top = top;
}



// void dvz_camera_aspect(DvzCamera* camera, float aspect)
// {
//     ANN(camera);
//     // if 1, fix aspect ratio, if 0, do not fix aspect ratio
//     camera->aspect = aspect;
// }



// void dvz_camera_ratio(DvzCamera* camera, vec2 viewport_size)
// {
//     ANN(camera);
//     ASSERT(viewport_size[1] > 0);
//     dvz_camera_aspect(camera, viewport_size[0] / viewport_size[1]);
// }


void dvz_camera_resize(DvzCamera* camera, float width, float height)
{
    ANN(camera);
    ASSERT(height > 0);

    camera->viewport_size[0] = width;
    camera->viewport_size[1] = height;
    camera->aspect = width / height;
}



void dvz_camera_position(DvzCamera* camera, vec3 pos)
{
    ANN(camera);
    glm_vec3_copy(pos, camera->pos);
}



void dvz_camera_lookat(DvzCamera* camera, vec3 lookat)
{
    ANN(camera);
    glm_vec3_copy(lookat, camera->lookat);
}



void dvz_camera_up(DvzCamera* camera, vec3 up)
{
    ANN(camera);
    glm_vec3_copy(up, camera->up);
}



void dvz_camera_perspective(DvzCamera* camera, float fov)
{
    ANN(camera);
    ASSERT(fov > 0);

    // field of view angle (in radians)
    camera->fov = fov;
}



void dvz_camera_viewproj(DvzCamera* camera, mat4 view, mat4 proj)
{
    ANN(camera);
    ASSERT(camera->aspect > 0);

    // View matrix.
    glm_lookat(camera->pos, camera->lookat, camera->up, view);

    // Projection matrix.
    glm_perspective(GLM_PI_4, camera->aspect, camera->near, camera->far, proj);
}



void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp)
{
    ANN(camera);
    ANN(mvp);

    dvz_camera_viewproj(camera, mvp->view, mvp->proj);
}



void dvz_camera_print(DvzCamera* camera)
{
    ANN(camera);
    mat4 view, proj;
    dvz_camera_viewproj(camera, view, proj);
    glm_mat4_print(view, stdout);
    glm_mat4_print(proj, stdout);
}



void dvz_camera_destroy(DvzCamera* camera)
{
    ANN(camera);
    FREE(camera);
}
