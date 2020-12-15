#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols)
{
    ASSERT(canvas != NULL);
    canvas->scene = calloc(1, sizeof(VklScene));
    canvas->scene->canvas = canvas;
    canvas->scene->grid = vkl_grid(canvas, 2, 3);

    INSTANCES_INIT(
        VklVisual, canvas->scene, visuals, max_visuals, //
        VKL_MAX_VISUALS, VKL_OBJECT_TYPE_VISUAL)

    INSTANCES_INIT(
        VklController, canvas->scene, controllers, max_controllers, //
        VKL_MAX_CONTROLLERS, VKL_OBJECT_TYPE_CONTROLLER)

    return canvas->scene;
}



void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
    INSTANCES_DESTROY(scene->visuals)
    obj_destroyed(&scene->obj);
    FREE(scene);
}
