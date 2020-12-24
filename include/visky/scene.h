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



// Coordinate system
typedef enum
{
    VKL_CDS_DATA = 1,       // data coordinate system
    VKL_CDS_GPU = 2,        // data coordinates normalized to -1,+1 and sent to the GPU
    VKL_CDS_PANZOOM = 3,    // normalized coords within the panel inner's viewport (w/ panzoom)
    VKL_CDS_PANEL = 4,      // NDC coordinates within the outer panel viewport
    VKL_CDS_CANVAS_NDC = 5, // normalized coords within the canvas
    VKL_CDS_CANVAS_PX = 6,  // same but in pixels, origin at the upper left
} VklCDS;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklScene VklScene;
typedef struct VklController VklController;
typedef struct VklTransform VklTransform;
typedef struct VklAxes2D VklAxes2D;

typedef void (*VklControllerCallback)(VklController* controller, VklEvent ev);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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

    VklDataCoords coords;
    // may call vkl_visual_update() on all visuals in the panel
    VklControllerCallback callback;
    // union; // axes
};



struct VklAxes2D
{
    VklPanel* panel;
    VklVisual* visualx;
    VklVisual* visualy;
    float* xticks;
    float* yticks;
    char* str_buf;
    char** text;
};



struct VklScene
{
    VklObject obj;
    VklCanvas* canvas;

    // The grid contains the panels.
    VklGrid grid;

    // Visuals.
    uint32_t max_visuals;
    VklVisual* visuals;

    // Controllers.
    uint32_t max_controllers;
    VklController* controllers;
};



/*************************************************************************************************/
/*  Transform definitions                                                                        */
/*************************************************************************************************/

struct VklTransform
{
    dvec2 scale, shift;
};



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VklTransform vkl_transform(VklPanel* panel, VklCDS source, VklCDS target);

VklTransform vkl_transform_inv(VklTransform);

VklTransform vkl_transform_mul(VklTransform, VklTransform);

VklTransform vkl_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout);

void vkl_transform_apply(VklTransform*, dvec2 in, dvec2 out);



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

// VKY_EXPORT VklController* vkl_scene_controller(VklPanel* panel, VklControllerType type, int
// flags);

VKY_EXPORT VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags);

VKY_EXPORT VklPanel*
vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags);

// VKY_EXPORT void vkl_visual_toggle(VklVisual* visual, VklVisualVisibility visibility);



#endif
