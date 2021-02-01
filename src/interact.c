#include "../include/datoviz/interact.h"
#include "../include/datoviz/canvas.h"
#include "interact_utils.h"



/*************************************************************************************************/
/*  Interact                                                                                     */
/*************************************************************************************************/

DvzInteract dvz_interact(DvzCanvas* canvas, void* user_data)
{
    ASSERT(canvas != NULL);
    DvzInteract interact = {0};
    interact.canvas = canvas;
    glm_mat4_identity(interact.mvp.model);
    glm_mat4_identity(interact.mvp.view);
    glm_mat4_identity(interact.mvp.proj);
    interact.user_data = user_data;
    return interact;
}



void dvz_interact_callback(DvzInteract* interact, DvzInteractCallback callback)
{
    ASSERT(interact != NULL);
    interact->callback = callback;
}



DvzInteract dvz_interact_builtin(DvzCanvas* canvas, DvzInteractType type)
{
    DvzInteract interact = dvz_interact(canvas, NULL);
    interact.type = type;
    switch (type)
    {
    case DVZ_INTERACT_PANZOOM:
    case DVZ_INTERACT_PANZOOM_FIXED_ASPECT:
        interact.u.p = _panzoom(canvas);
        if (type == DVZ_INTERACT_PANZOOM_FIXED_ASPECT)
            interact.u.p.fixed_aspect = true;
        interact.callback = _panzoom_callback;
        break;

    case DVZ_INTERACT_ARCBALL:
        interact.u.a = _arcball(canvas);
        interact.callback = _arcball_callback;
        break;

    case DVZ_INTERACT_FLY:
    case DVZ_INTERACT_FPS:
    case DVZ_INTERACT_TURNTABLE:
        interact.u.c = _camera(canvas, type);
        interact.callback = _camera_callback;
        break;

    default:
        break;
    }
    return interact;
}



void dvz_interact_update(
    DvzInteract* interact, DvzViewport viewport, DvzMouse* mouse, DvzKeyboard* keyboard)
{
    ASSERT(interact != NULL);

    // Update the local coordinates of the mouse before calling the interact callback.
    dvz_mouse_local(mouse, &interact->mouse_local, interact->canvas, viewport);

    if (interact->callback != NULL)
        interact->callback(interact, viewport, mouse, keyboard);
}



void dvz_interact_destroy(DvzInteract* interact)
{
    ASSERT(interact != NULL);
    //

    // if (interact->mmap != NULL)
    // {
    //     dvz_buffer_regions_unmap(&interact->br);
    //     interact->mmap = NULL;
    // }
}
