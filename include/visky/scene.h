#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

#include "builtin_visuals.h"
#include "interact.h"
#include "panel.h"
#include "ticks.h"
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
typedef enum
{
    VKL_CONTROLLER_NONE,
    VKL_CONTROLLER_PANZOOM,
    VKL_CONTROLLER_AXES_2D,
    VKL_CONTROLLER_ARCBALL,
    VKL_CONTROLLER_CAMERA,
    VKL_CONTROLLER_AXES_3D,
} VklControllerType;



// Visual flags.
typedef enum
{
    VKL_SCENE_VISUAL_FLAGS_NONE = 0x0000,
    VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE = 0x1000,
} VklSceneVisualFlags;



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

VKY_EXPORT VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols);

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
