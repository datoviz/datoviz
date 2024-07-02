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
#include "scene/camera.h"
#include "scene/mvp.h"



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
    vec3 pos_init, lookat_init, up_init; // initial camera parameters
    float fov;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzCamera* dvz_camera(float width, float height, int flags);



/**
 * Function.
 *
 * @param camera the camera
 */
DVZ_EXPORT void dvz_camera_destroy(DvzCamera* camera);



#endif
