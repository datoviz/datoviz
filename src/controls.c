#include <inttypes.h>

#include "../include/datoviz/canvas.h"
#include "../include/datoviz/controls.h"

// NOTE: This file should NOT import gui.h (C++)



/*************************************************************************************************/
/*  Gui controls API                                                                             */
/*************************************************************************************************/

DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags)
{
    ASSERT(canvas != NULL);
    DvzGui* gui = (DvzGui*)dvz_container_alloc(&canvas->guis);
    gui->canvas = canvas;
    gui->title = title;
    gui->flags = flags;
    dvz_obj_init(&gui->obj);
    return gui;
}



void dvz_gui_slider_float(DvzGui* gui, const char* name, double vmin, double vmax)
{
    ASSERT(gui != NULL);
    ASSERT(vmin < vmax);

    DvzGuiControl* control = &gui->controls[gui->control_count++];
    control->gui = gui;
    control->name = name;
    control->type = DVZ_GUI_CONTROL_SLIDER_FLOAT;
    control->value = (float*)calloc(1, sizeof(float));
    control->u.sf.vmin = (float)vmin;
    control->u.sf.vmax = (float)vmax;
    ASSERT(control->u.sf.vmin < control->u.sf.vmax);
    *((float*)control->value) = .5 * (vmax + vmin);
}



void dvz_gui_destroy(DvzGui* gui)
{
    ASSERT(gui != NULL);
    for (uint32_t i = 0; i < gui->control_count; i++)
    {
        FREE(gui->controls[i].value);
    }
}
