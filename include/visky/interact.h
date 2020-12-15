#ifndef VKL_INTERACT_HEADER
#define VKL_INTERACT_HEADER

#include "canvas.h"
#include "graphics.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKY_INTERACT_MIN_DELAY .01


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
    vec3 camera_pos;
    vec3 last_camera_pos;

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
    VklInteractType type;
    VklCanvas* canvas;
    VklMouseLocal mouse_local;
    VklInteractCallback callback;
    VklMVP mvp;
    VklInteractUnion u;
    bool to_update;
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


#endif
