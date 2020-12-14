#include "../include/visky/scene.h"
#include "../include/visky/canvas.h"
#include "../include/visky/visuals.h"
#include "../include/visky/vklite.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklScene vkl_scene(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklScene scene = {0};

    INSTANCES_INIT(
        VklVisual, (&scene), visuals, max_visuals, VKL_MAX_VISUALS, VKL_OBJECT_TYPE_VISUAL)

    return scene;
}



void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
    INSTANCES_DESTROY(scene->visuals)
    obj_destroyed(&scene->obj);
}
