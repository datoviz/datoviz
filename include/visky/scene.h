#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

#include "builtin_visuals.h"
#include "interact.h"
#include "panel.h"
#include "visuals.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_MAX_VISUALS                1024
#define VKL_MAX_CONTROLLERS            1024
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



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklScene VklScene;
typedef struct VklController VklController;


typedef void (*VklControllerCallback)(VklController* controller, VklEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklController
{
    VklObject obj;
    // VklScene* scene;
    VklPanel* panel;

    VklControllerType type;

    uint32_t visual_count;
    VklVisual* visuals[VKL_MAX_VISUALS_PER_CONTROLLER];

    uint32_t interact_count;
    VklInteract interacts[VKL_MAX_VISUALS_PER_CONTROLLER];

    VklDataCoords coords;
    // may call vkl_visual_update() on all visuals in the panel
    VklControllerCallback callback;
    // union; // axes
};



struct VklScene
{
    VklObject obj;
    VklCanvas* canvas;
    VklGrid grid;

    uint32_t max_visuals;
    VklVisual* visuals;

    uint32_t max_controllers;
    VklController* controllers;
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

VKY_EXPORT VklController* vkl_panel_controller(VklPanel* panel, VklControllerType type, int flags);

VKY_EXPORT VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags);

VKY_EXPORT VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags);

// VKY_EXPORT void vkl_visual_toggle(VklVisual* visual, VklVisualVisibility visibility);



#endif
