/*************************************************************************************************/
/*  Scene API                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_SCENE_HEADER
#define DVZ_SCENE_HEADER

#include "builtin_visuals.h"
#include "interact.h"
#include "panel.h"
#include "ticks_types.h"
#include "transforms.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_VISUALS_PER_CONTROLLER 64



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
    DVZ_CONTROLLER_NONE,    // static panel
    DVZ_CONTROLLER_PANZOOM, // pan and zoom with the mouse
    DVZ_CONTROLLER_AXES_2D, // panzoom + 2D axes with ticks, grid, etc.
    DVZ_CONTROLLER_ARCBALL, // 3D rotating model with the mouse (uses quaternions)
    DVZ_CONTROLLER_CAMERA,  // 3D camera with keyboard for movement and mouse for view
    DVZ_CONTROLLER_AXES_3D, // 3D arcball with axes (NOT IMPLEMENTED YET)
} DvzControllerType;



// Visual flags.
typedef enum
{
    DVZ_VISUAL_FLAGS_TRANSFORM_AUTO = 0x0000,
    DVZ_VISUAL_FLAGS_TRANSFORM_NONE = 0x0010,
} DvzVisualFlags;



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

typedef struct DvzScene DvzScene;
typedef struct DvzController DvzController;
typedef struct DvzTransformOLD DvzTransformOLD;
typedef struct DvzAxes2D DvzAxes2D;
typedef union DvzControllerUnion DvzControllerUnion;

typedef void (*DvzControllerCallback)(DvzController* controller, DvzEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzAxes2D
{
    DvzAxesContext ctx[2]; // one per dimension
    DvzAxesTicks ticks[2];
    DvzBox box; // box, in data coordinates, corresponding to the box showed with initial panzoom
    float font_size;
};



union DvzControllerUnion
{
    DvzAxes2D axes_2D;
};



struct DvzController
{
    DvzObject obj;
    DvzPanel* panel;
    int flags;

    DvzControllerType type;

    uint32_t visual_count;
    DvzVisual* visuals[DVZ_MAX_VISUALS_PER_CONTROLLER];

    uint32_t interact_count;
    DvzInteract interacts[DVZ_MAX_VISUALS_PER_CONTROLLER];

    // may call dvz_visual_update() on all visuals in the panel
    DvzControllerCallback callback;
    DvzControllerUnion u;
};



struct DvzScene
{
    DvzObject obj;
    DvzCanvas* canvas;

    // The grid contains the panels.
    DvzGrid grid;

    // Visuals.
    DvzContainer visuals;

    // Controllers.
    DvzContainer controllers;
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
DVZ_EXPORT DvzScene* dvz_scene(DvzCanvas* canvas, uint32_t n_rows, uint32_t n_cols);



/**
 * Destroy a scene.
 *
 * Destroy all panels and visuals in the scene.
 *
 * @param scene the scene
 */
DVZ_EXPORT void dvz_scene_destroy(DvzScene* scene);



/*************************************************************************************************/
/*  Controller                                                                                   */
/*************************************************************************************************/

/**
 * Create a custom controller.
 *
 * @param panel the panel
 * @returns a controller structure
 */
DVZ_EXPORT DvzController dvz_controller(DvzPanel* panel);

/**
 * Add a visual to a controller.
 *
 * @param controller the controller
 * @param visual the visual
 */
DVZ_EXPORT void dvz_controller_visual(DvzController* controller, DvzVisual* visual);

/**
 * Add an interact to a controller.
 *
 * @param controller the controller
 * @param interact the interact
 */
DVZ_EXPORT void dvz_controller_interact(DvzController* controller, DvzInteractType type);

/**
 * Specify a controller frame callback.
 *
 * Callback signature: `void(DvzController* controller, DvzEvent ev);`
 *
 * @param controller the controller
 * @param callback the callback
 */
DVZ_EXPORT void dvz_controller_callback(DvzController* controller, DvzControllerCallback callback);

/**
 * Update a controller.
 *
 * !!! missing "Not yet implemented"
 *
 * @param controller the controller
 */
DVZ_EXPORT void dvz_controller_update(DvzController* controller);

/**
 * Destroy a controller.
 *
 * @param controller the controller
 */
DVZ_EXPORT void dvz_controller_destroy(DvzController* controller);


/**
 * Create a builtin controller.
 *
 * @param panel the panel
 * @param type the controller type
 * @param flags flags for the builtin controller
 * @returns the controller
 */
DVZ_EXPORT DvzController
dvz_controller_builtin(DvzPanel* panel, DvzControllerType type, int flags);



/*************************************************************************************************/
/*  High-level functions                                                                         */
/*************************************************************************************************/

/**
 * Add a panel to the scene grid.
 *
 * @param controller the scene
 * @param row the row index (0-based)
 * @param col the column index (0-based)
 * @param type the controller type
 * @param flags flags for the builtin controller
 * @returns the panel
 */
DVZ_EXPORT DvzPanel*
dvz_scene_panel(DvzScene* scene, uint32_t row, uint32_t col, DvzControllerType type, int flags);

/**
 * Create a builtin visual and add it to a panel.
 *
 * @param panel the panel
 * @param type the type of visual
 * @param flags flags for the builtin visual
 * @returns the visual
 */
DVZ_EXPORT DvzVisual* dvz_scene_visual(DvzPanel* panel, DvzVisualType type, int flags);

/**
 * Create a blank graphics (used when creating custom graphics and visuals).
 *
 * @param scene the scene
 * @param flags graphics flags
 * @returns a blank graphics
 */
DVZ_EXPORT DvzGraphics* dvz_blank_graphics(DvzScene* scene, int flags);

/**
 * Create a blank visual (used when creating custom visuals).
 *
 * @param scene the scene
 * @param flags visual flags
 * @returns a blank visual
 */
DVZ_EXPORT DvzVisual* dvz_blank_visual(DvzScene* scene, int flags);

/**
 * Make a custom graphics and add it to a visual.
 *
 * @param visual a visual
 * @param graphics the custom graphics
 */
DVZ_EXPORT void dvz_custom_graphics(DvzVisual* visual, DvzGraphics* graphics);

/**
 * Make a custom visual and add it to a panel.
 *
 * @param panel the panel
 * @param visual the custom visual
 */
DVZ_EXPORT void dvz_custom_visual(DvzPanel* panel, DvzVisual* visual);


// DVZ_EXPORT void dvz_visual_toggle(DvzVisual* visual, DvzVisualVisibility visibility);



/*************************************************************************************************/
/*  Interact functions                                                                           */
/*************************************************************************************************/

/**
 * Set the camera position.
 *
 * @param panel the panel
 * @param pos the position in scene coordinates
 */
DVZ_EXPORT void dvz_camera_pos(DvzPanel* panel, vec3 pos);

/**
 * Set the camera center position (the position the camera points to).
 *
 * @param panel the panel
 * @param center the center position
 */
DVZ_EXPORT void dvz_camera_look(DvzPanel* panel, vec3 center);

/**
 * Set the arcball rotation.
 *
 * @param panel the panel
 * @param angle the rotation angle
 * @param axis the rotation angle
 */
DVZ_EXPORT void dvz_arcball_rotate(DvzPanel* panel, float angle, vec3 axis);

// TODO: panzoom functions



static void _default_controller_callback(DvzController* controller, DvzEvent ev)
{
    DvzScene* scene = controller->panel->scene;
    DvzCanvas* canvas = scene->canvas;

    // Controller interactivity.
    DvzInteract* interact = NULL;

    // Use all interact of the controllers.
    for (uint32_t i = 0; i < controller->interact_count; i++)
    {
        interact = &controller->interacts[i];
        // float delay = canvas->clock.elapsed - interact->last_update;

        // Update the interact using the current panel's viewport.
        DvzViewport viewport = controller->panel->viewport;
        dvz_interact_update(interact, viewport, &canvas->mouse, &canvas->keyboard);
        // NOTE: the CPU->GPU transfer occurs at every frame, in another callback below
    }
}



#endif
