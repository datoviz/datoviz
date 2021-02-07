/*************************************************************************************************/
/*  Interactivity tools (panzoom, arcball, ...)                                                  */
/*************************************************************************************************/

#ifndef DVZ_INTERACT_HEADER
#define DVZ_INTERACT_HEADER

#include "canvas.h"
#include "graphics.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_INTERACT_MIN_DELAY .01
#define DVZ_CAMERA_EYE                                                                            \
    (vec3) { 0, 0, 4 }
#define DVZ_CAMERA_UP                                                                             \
    (vec3) { 0, 1, 0 }

#define DVZ_PANZOOM_MOUSE_WHEEL_FACTOR .2
#define DVZ_PANZOOM_MIN_ZOOM           1e-5
#define DVZ_PANZOOM_MAX_ZOOM           1e+5



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzPanzoom DvzPanzoom;
typedef struct DvzArcball DvzArcball;
typedef struct DvzCamera DvzCamera;
typedef struct DvzInteract DvzInteract;
typedef union DvzInteractUnion DvzInteractUnion;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Interact type.
typedef enum
{
    DVZ_INTERACT_NONE,
    DVZ_INTERACT_PANZOOM,
    DVZ_INTERACT_PANZOOM_FIXED_ASPECT,
    DVZ_INTERACT_ARCBALL,
    DVZ_INTERACT_TURNTABLE,
    DVZ_INTERACT_FLY,
    DVZ_INTERACT_FPS,
} DvzInteractType;



// Interact callback.
typedef void (*DvzInteractCallback)(
    DvzInteract* interact, DvzViewport viewport, //
    DvzMouse* mouse, DvzKeyboard* keyboard);



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

struct DvzPanzoom
{
    DvzCanvas* canvas;

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

struct DvzCamera
{
    DvzCanvas* canvas;

    vec3 eye; // smoothly follows target
    vec3 forward;
    vec3 up;
    vec3 target; // requested eye position modified by mouse and keyboard, used for smooth move
    float speed;
};



/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

struct DvzArcball
{
    DvzCanvas* canvas;
    versor rotation;
    mat4 translate;
    mat4 mat;
    mat4 inv_model;
    DvzCamera camera;
};



/*************************************************************************************************/
/*  Interact                                                                                     */
/*************************************************************************************************/

union DvzInteractUnion
{
    DvzPanzoom p;
    DvzArcball a;
    DvzCamera c;
};



struct DvzInteract
{
    DvzCanvas* canvas;
    DvzInteractType type;

    DvzMouseLocal mouse_local;
    DvzInteractCallback callback;

    DvzMVP mvp;

    DvzInteractUnion u;

    bool is_active;
    double last_update;
    void* user_data;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create an interactivity object.
 *
 * @param canvas the canvas
 * @param user_data arbitrary user pointer
 * @returns an interactivity object
 */
DVZ_EXPORT DvzInteract dvz_interact(DvzCanvas* canvas, void* user_data);

/**
 * Add an interactivity callback.
 *
 * The callback function is called at every frame and is meant to update the `mvp` object.
 *
 * Callback function signature: `(DvzInteract*, DvzViewport, DvzMouse*, DvzKeyboard*)`
 *
 * @param interact the interactivity object
 * @param callback the interactivity callback function
 */
DVZ_EXPORT void dvz_interact_callback(DvzInteract* interact, DvzInteractCallback callback);

/**
 * Create a builtin interactivity object.
 *
 * @param canvas the canvas
 * @param type the builtin interactivity type
 * @returns an interactivity object
 */
DVZ_EXPORT DvzInteract dvz_interact_builtin(DvzCanvas* canvas, DvzInteractType type);

/**
 * Call the interactivity callback.
 *
 * Normally called at every frame.
 *
 * @param interact the interact object
 * @param viewport the viewport
 * @param mouse the mouse object
 * @param keyboard the keyboard object
 */
DVZ_EXPORT void dvz_interact_update(
    DvzInteract* interact, DvzViewport viewport, DvzMouse* mouse, DvzKeyboard* keyboard);

/**
 * Destroy an interactivity object.
 *
 * @param canvas the interactivity object
 */
DVZ_EXPORT void dvz_interact_destroy(DvzInteract* interact);


#endif
