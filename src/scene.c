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



VklVisual* vkl_visual_builtin(VklScene* scene, VklVisualBuiltin type, int flags)
{
    ASSERT(scene != NULL);
    ASSERT(scene->canvas != NULL);
    VklCanvas* canvas = scene->canvas;
    ASSERT(type != VKL_VISUAL_NONE);

    for (uint32_t i = 0; i < VKL_VISUAL_COUNT; i++)
    {
        if (scene->visuals[i].obj.status == VKL_OBJECT_STATUS_NONE)
            scene->visuals[i].obj.status = VKL_OBJECT_STATUS_INIT;
    }

    int32_t idx = (int32_t)type;
    ASSERT(idx > 0);

    VklVisual* visual = &scene->visuals[idx];
    ASSERT(visual != NULL);
    if (is_obj_created(&visual->obj))
        return visual;
    ASSERT(!is_obj_created(&visual->obj));

    // Common initialization.
    *visual = vkl_visual(canvas);

    switch (type)
    {
        // TODO
    default:
        log_error("no visuals type specified");
        break;
    }

    ASSERT(is_obj_created(&visual->obj));

    return visual;
}



void vkl_scene_destroy(VklScene* scene)
{
    ASSERT(scene != NULL);
    INSTANCES_DESTROY(scene->visuals)
    obj_destroyed(&scene->obj);
}
