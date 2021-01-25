/*************************************************************************************************/
/*  Scene API                                                                                    */
/*************************************************************************************************/

#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

#include "builtin_visuals.h"
#include "interact.h"
#include "panel.h"
#include "ticks_types.h"
#include "transforms.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_VISUALS_PER_CONTROLLER 64



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Controller type
/**
 * Controller type.
 *
 * The controller type determines the way the user interacts with a panel.
 *
 */
typedef enum
{
    VKL_CONTROLLER_NONE,    // static panel
    VKL_CONTROLLER_PANZOOM, // pan and zoom with the mouse
    VKL_CONTROLLER_AXES_2D, // panzoom + 2D axes with ticks, grid, etc.
    VKL_CONTROLLER_ARCBALL, // 3D rotating model with the mouse (uses quaternions)
    VKL_CONTROLLER_CAMERA,  // 3D camera with keyboard for movement and mouse for view
    VKL_CONTROLLER_AXES_3D, // 3D arcball with axes (NOT IMPLEMENTED YET)
} VklControllerType;



// Visual flags.
typedef enum
{
    VKL_VISUAL_FLAGS_TRANSFORM_AUTO = 0x0000,
    VKL_VISUAL_FLAGS_TRANSFORM_NONE = 0x0010,
} VklVisualFlags;



/*
flags bits range:

panel flags:
0x00XX: transform
0xXX00: controller flags


visual flags:
0x00XX: visual level
    0x000X: visual specific
    0x00X0: auto CPU normalize
0xXX00: graphics level
    0x0100: depth test
    0xX000: interact axes

*/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklScene VklScene;
typedef struct VklController VklController;
typedef struct VklTransformOLD VklTransformOLD;
typedef struct VklAxes2D VklAxes2D;
typedef union VklControllerUnion VklControllerUnion;

typedef void (*VklControllerCallback)(VklController* controller, VklEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklAxes2D
{
    VklAxesContext ctx[2]; // one per dimension
    VklAxesTicks ticks[2];
    VklBox box; // box, in data coordinates, corresponding to the box showed with initial panzoom
    float font_size;
};



union VklControllerUnion
{
    VklAxes2D axes_2D;
};



struct VklController
{
    VklObject obj;
    VklPanel* panel;
    int flags;

    VklControllerType type;

    uint32_t visual_count;
    VklVisual* visuals[VKL_MAX_VISUALS_PER_CONTROLLER];

    uint32_t interact_count;
    VklInteract interacts[VKL_MAX_VISUALS_PER_CONTROLLER];

    // may call vkl_visual_update() on all visuals in the panel
    VklControllerCallback callback;
    VklControllerUnion u;
};



struct VklScene
{
    VklObject obj;
    VklCanvas* canvas;

    // The grid contains the panels.
    VklGrid grid;

    // Visuals.
    VklContainer visuals;

    // Controllers.
    VklContainer controllers;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a scene with a grid layout.
 *
 * The scene defines a 2D grid where each cell contains a panel (subplot). Panels may support
 * various kinds of interactivity.
 *
 * @param canvas the canvas
 * @param n_rows number of rows in the grid
 * @param n_cols number of columns in the grid
 * @returns a pointer to the created scene
 */
VKY_EXPORT VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols);



/**
 * Destroy a scene.
 *
 * Destroy all panels and visuals in the scene.
 *
 * @param scene the scene
 */
VKY_EXPORT void vkl_scene_destroy(VklScene* scene);



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

VKY_EXPORT VklController vkl_controller(VklPanel* panel);

VKY_EXPORT void vkl_controller_visual(VklController* controller, VklVisual* visual);

VKY_EXPORT void vkl_controller_interact(VklController* controller, VklInteractType type);

VKY_EXPORT void vkl_controller_callback(VklController* controller, VklControllerCallback callback);

VKY_EXPORT void vkl_controller_update(VklController* controller);

VKY_EXPORT void vkl_controller_destroy(VklController* controller);



VKY_EXPORT VklController
vkl_controller_builtin(VklPanel* panel, VklControllerType type, int flags);



/*************************************************************************************************/
/*  High-level functions                                                                         */
/*************************************************************************************************/

VKY_EXPORT VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags);

VKY_EXPORT VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags);

// VKY_EXPORT void vkl_visual_toggle(VklVisual* visual, VklVisualVisibility visibility);



/*************************************************************************************************/
/*  Interact functions                                                                           */
/*************************************************************************************************/

VKY_EXPORT void vkl_camera_pos(VklPanel* panel, vec3 pos);

VKY_EXPORT void vkl_camera_look(VklPanel* panel, vec3 center);

VKY_EXPORT void vkl_arcball_rotate(VklPanel* panel, float angle, vec3 axis);

// TODO: panzoom functions



static void _default_controller_callback(VklController* controller, VklEvent ev)
{
    VklScene* scene = controller->panel->scene;
    VklCanvas* canvas = scene->canvas;

    // Controller interactivity.
    VklInteract* interact = NULL;

    // Use all interact of the controllers.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        interact = &controller->interacts[i];
        // float delay = canvas->clock.elapsed - interact->last_update;

        // Update the interact using the current panel's viewport.
        VklViewport viewport = controller->panel->viewport;
        vkl_interact_update(interact, viewport, &canvas->mouse, &canvas->keyboard);
        // NOTE: the CPU->GPU transfer occurs at every frame, in another callback below
    }
}



#endif
