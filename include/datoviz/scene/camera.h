/*************************************************************************************************/
/* Camera                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CAMERA
#define DVZ_HEADER_CAMERA



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_log.h"
#include "_math.h"
#include "scene/mvp.h"
#include "scene/camera.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_CAMERA_DEFAULT_ZRANGE   0.1, 100
#define DVZ_CAMERA_DEFAULT_FOV      GLM_PI_4
#define DVZ_CAMERA_DEFAULT_POSITION 0, 0, 4
#define DVZ_CAMERA_DEFAULT_LOOKAT   0, 0, 0
#define DVZ_CAMERA_DEFAULT_UP       0, 1, 0



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzCamera DvzCamera;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzCamera
{
    vec2 viewport_size;
    int flags;

    float near, far; // zrange
    float aspect;

    // TODO: use type and union?
    // for orthographic camera
    float left, right, bottom, top;

    // for perspective camera
    vec3 pos, lookat, up;
    float fov;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DVZ_EXPORT DvzCamera* dvz_camera(float width, float height, int flags);



DVZ_EXPORT void dvz_camera_zrange(DvzCamera* camera, float near, float far);



DVZ_EXPORT void
dvz_camera_ortho(DvzCamera* camera, float left, float right, float bottom, float top);



DVZ_EXPORT void dvz_camera_resize(DvzCamera* camera, float width, float height);



DVZ_EXPORT void dvz_camera_position(DvzCamera* camera, vec3 pos);



DVZ_EXPORT void dvz_camera_lookat(DvzCamera* camera, vec3 lookat);



DVZ_EXPORT void dvz_camera_up(DvzCamera* camera, vec3 up);



DVZ_EXPORT void dvz_camera_perspective(DvzCamera* camera, float fov);
// field of view angle (in radians)



DVZ_EXPORT void dvz_camera_viewproj(DvzCamera* camera, mat4 view, mat4 proj);



DVZ_EXPORT void dvz_camera_mvp(DvzCamera* camera, DvzMVP* mvp);



DVZ_EXPORT void dvz_camera_print(DvzCamera* camera);



DVZ_EXPORT void dvz_camera_destroy(DvzCamera* camera);



EXTERN_C_OFF

#endif
