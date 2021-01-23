#include "../include/visky/interact.h"
#include "../include/visky/canvas.h"
#include "interact_utils.h"



/*************************************************************************************************/
/*  Interact                                                                                     */
/*************************************************************************************************/

VklInteract vkl_interact(VklCanvas* canvas, void* user_data)
{
    ASSERT(canvas != NULL);
    VklInteract interact = {0};
    interact.canvas = canvas;
    glm_mat4_identity(interact.mvp.model);
    glm_mat4_identity(interact.mvp.view);
    glm_mat4_identity(interact.mvp.proj);
    interact.user_data = user_data;
    return interact;
}



void vkl_interact_callback(VklInteract* interact, VklInteractCallback callback)
{
    ASSERT(interact != NULL);
    interact->callback = callback;
}



VklInteract vkl_interact_builtin(VklCanvas* canvas, VklInteractType type)
{
    VklInteract interact = vkl_interact(canvas, NULL);
    interact.type = type;
    switch (type)
    {
    case VKL_INTERACT_PANZOOM:
    case VKL_INTERACT_PANZOOM_FIXED_ASPECT:
        interact.u.p = _panzoom(canvas);
        if (type == VKL_INTERACT_PANZOOM_FIXED_ASPECT)
            interact.u.p.fixed_aspect = true;
        interact.callback = _panzoom_callback;
        break;

    case VKL_INTERACT_ARCBALL:
        interact.u.a = _arcball(canvas);
        interact.callback = _arcball_callback;
        break;

    case VKL_INTERACT_FLY:
    case VKL_INTERACT_FPS:
    case VKL_INTERACT_TURNTABLE:
        interact.u.c = _camera(canvas, type);
        interact.callback = _camera_callback;
        break;

    default:
        break;
    }
    return interact;
}



void vkl_interact_update(
    VklInteract* interact, VklViewport viewport, VklMouse* mouse, VklKeyboard* keyboard)
{
    ASSERT(interact != NULL);

    // Update the local coordinates of the mouse before calling the interact callback.
    vkl_mouse_local(mouse, &interact->mouse_local, interact->canvas, viewport);

    if (interact->callback != NULL)
        interact->callback(interact, viewport, mouse, keyboard);
}



void vkl_interact_destroy(VklInteract* interact)
{
    ASSERT(interact != NULL);
    //

    // if (interact->mmap != NULL)
    // {
    //     vkl_buffer_regions_unmap(&interact->br);
    //     interact->mmap = NULL;
    // }
}
