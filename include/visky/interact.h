#ifndef VKY_INTERACT_HEADER
#define VKY_INTERACT_HEADER

#include "scene.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VkyPanzoom
{
    vec3 camera_pos;
    vec2 zoom;
    mat4 model;
    bool lim_reached[2];
};


struct VkyArcball
{
    VkyScene* scene;
    mat4 center_translation, translation;
    versor rotation;
    mat4 mat_arcball;
    mat4 mat_user;
    vec3 eye_init;
    VkyMVPMatrix
        which_matrix; // whether the model or the view matrix should be affected by the arcball
    VkyMVP mvp;
};


struct VkyCamera
{
    vec3 eye; // smoothly follows target
    vec3 forward;
    vec3 up;
    vec3 target; // requested eye position modified by mouse and keyboard, used for smooth move
    float speed;
    VkyMVP mvp;
};



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyPanzoom* vky_panzoom_init(void);
VKY_EXPORT void vky_panzoom_reset(VkyPanzoom*);
VKY_EXPORT void vky_panzoom_update(VkyPanel*, VkyPanzoom*, VkyViewportType viewport_type);
VKY_EXPORT void vky_panzoom_mvp(VkyPanel*, VkyPanzoom*, VkyViewportType viewport_type);

VKY_EXPORT VkyBox2D vky_panzoom_get_box(VkyPanel*, VkyPanzoom*, VkyViewportType);
VKY_EXPORT void vky_panzoom_set_box(VkyPanzoom*, VkyViewportType, VkyBox2D);


/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

VKY_EXPORT VkyArcball* vky_arcball_init(VkyMVPMatrix);
VKY_EXPORT void vky_screen_to_arcball(vec2 p, versor q);
VKY_EXPORT void vky_arcball_update(VkyPanel*, VkyArcball*, VkyViewportType);



/*************************************************************************************************/
/*  FPS                                                                                          */
/*************************************************************************************************/

VKY_EXPORT VkyCamera* vky_camera_init(void);
VKY_EXPORT void vky_camera_update(VkyPanel*, VkyCamera*, VkyViewportType);



#endif
