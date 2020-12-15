#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

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
    VklScene* scene;
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



#endif
