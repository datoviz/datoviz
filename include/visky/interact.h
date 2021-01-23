#ifndef VKL_INTERACT_HEADER
#define VKL_INTERACT_HEADER

#include "canvas.h"
#include "graphics.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKY_INTERACT_MIN_DELAY .01
#define VKL_CAMERA_EYE                                                                            \
    (vec3) { 0, 0, 4 }
#define VKL_CAMERA_UP                                                                             \
    (vec3) { 0, 1, 0 }

#define VKL_PANZOOM_MOUSE_WHEEL_FACTOR .2
#define VKL_PANZOOM_MIN_ZOOM           1e-5
#define VKL_PANZOOM_MAX_ZOOM           1e+5



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklPanzoom VklPanzoom;
typedef struct VklArcball VklArcball;
typedef struct VklCamera VklCamera;
typedef struct VklInteract VklInteract;
typedef union VklInteractUnion VklInteractUnion;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Interact type.
typedef enum
{
    VKL_INTERACT_NONE,
    VKL_INTERACT_PANZOOM,
    VKL_INTERACT_PANZOOM_FIXED_ASPECT,
    VKL_INTERACT_ARCBALL,
    VKL_INTERACT_TURNTABLE,
    VKL_INTERACT_FLY,
    VKL_INTERACT_FPS,
} VklInteractType;



// Interact callback.
typedef void (*VklInteractCallback)(
    VklInteract* interact, VklViewport viewport, //
    VklMouse* mouse, VklKeyboard* keyboard);



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

struct VklPanzoom
{
    VklCanvas* canvas;

    vec3 camera_pos;
    vec3 press_pos;

    vec2 zoom;
    vec2 last_zoom;

    bool lim_reached[2];
    bool fixed_aspect;
};



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

struct VklCamera
{
    VklCanvas* canvas;

    vec3 eye; // smoothly follows target
    vec3 forward;
    vec3 up;
    vec3 target; // requested eye position modified by mouse and keyboard, used for smooth move
    float speed;
};



/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

struct VklArcball
{
    VklCanvas* canvas;

    mat4 center_translation, translation;
    versor rotation;
    mat4 mat;
    VklCamera camera;
};



/*************************************************************************************************/
/*  Interact                                                                                     */
/*************************************************************************************************/

union VklInteractUnion
{
    VklPanzoom p;
    VklArcball a;
    VklCamera c;
};



struct VklInteract
{
    VklCanvas* canvas;
    VklInteractType type;

    VklMouseLocal mouse_local;
    VklInteractCallback callback;

    VklMVP mvp;

    VklInteractUnion u;

    bool is_active;
    double last_update;
    void* user_data;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklInteract vkl_interact(VklCanvas* canvas, void* user_data);

VKY_EXPORT void vkl_interact_callback(VklInteract* interact, VklInteractCallback callback);

VKY_EXPORT VklInteract vkl_interact_builtin(VklCanvas* canvas, VklInteractType type);

VKY_EXPORT void vkl_interact_update(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard);

VKY_EXPORT void vkl_interact_destroy(VklInteract* interact);


#endif
