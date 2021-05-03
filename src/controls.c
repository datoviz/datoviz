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

    // Make a copy of the string to the GUI to avoid dangling pointer.
    gui->title = calloc(MAX_TEXT_LENGTH, sizeof(char));
    ASSERT(strlen(title) < 1024);
    strncpy((char*)gui->title, title, 1024);

    gui->flags = flags;
    dvz_obj_init(&gui->obj);
    return gui;
}



static DvzGuiControl* _add_control(
    DvzGui* gui, DvzGuiControlType type, const char* name, VkDeviceSize item_size, void* value)
{
    ASSERT(gui != NULL);

    DvzGuiControl* control = &gui->controls[gui->control_count++];
    control->gui = gui;
    control->name = name;
    control->type = type;
    control->value = calloc(1, item_size);
    if (value != NULL)
        memcpy(control->value, value, item_size);
    return control;
}


void* dvz_gui_value(DvzGuiControl* control)
{
    ASSERT(control != NULL);
    return control->value;
}



DvzGuiControl* dvz_gui_checkbox(DvzGui* gui, const char* name, bool value)
{
    ASSERT(gui != NULL);
    ASSERT(name != NULL);
    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_CHECKBOX, name, sizeof(bool), &value);
    ASSERT(control != NULL);
    return control;
}



DvzGuiControl*
dvz_gui_slider_float(DvzGui* gui, const char* name, float vmin, float vmax, float value)
{
    ASSERT(gui != NULL);
    ASSERT(vmin < vmax);
    value = CLIP(value, vmin, vmax);

    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_SLIDER_FLOAT, name, sizeof(float), &value);
    control->u.sf.vmin = vmin;
    control->u.sf.vmax = vmax;
    return control;
}



DvzGuiControl* dvz_gui_slider_float2(
    DvzGui* gui, const char* name, float vmin, float vmax, vec2 value, bool force_increasing)
{
    ASSERT(gui != NULL);
    ASSERT(vmin < vmax);
    for (uint32_t i = 0; i < 2; i++)
    {
        value[i] = CLIP(value[i], vmin, vmax);
    }

    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_SLIDER_FLOAT2, name, sizeof(vec2), value);
    control->u.sf2.vmin = vmin;
    control->u.sf2.vmax = vmax;
    control->u.sf2.force_increasing = force_increasing;
    return control;
}



DvzGuiControl*
dvz_gui_input_float(DvzGui* gui, const char* name, float step, float step_fast, float value)
{
    ASSERT(gui != NULL);
    ASSERT(step > 0);
    ASSERT(step_fast > 0);
    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_INPUT_FLOAT, name, sizeof(float), &value);
    control->u.f.step = step;
    control->u.f.step_fast = step_fast;
    return control;
}



DvzGuiControl* dvz_gui_slider_int(DvzGui* gui, const char* name, int vmin, int vmax, int value)
{
    ASSERT(gui != NULL);
    ASSERT(vmin < vmax);
    value = CLIP(value, vmin, vmax);

    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_SLIDER_INT, name, sizeof(int), &value);
    control->u.si.vmin = vmin;
    control->u.si.vmax = vmax;
    return control;
}



DvzGuiControl* dvz_gui_label(DvzGui* gui, const char* name, char* text)
{
    ASSERT(gui != NULL);
    ASSERT(name != NULL);
    ASSERT(text != NULL);
    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_LABEL, name, MAX_TEXT_LENGTH * sizeof(char), text);
    ASSERT(control != NULL);
    return control;
}



DvzGuiControl* dvz_gui_textbox(DvzGui* gui, const char* name, char* text)
{
    ASSERT(gui != NULL);
    ASSERT(name != NULL);
    ASSERT(text != NULL);
    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_TEXTBOX, name, MAX_TEXT_LENGTH * sizeof(char), text);
    ASSERT(control != NULL);
    return control;
}



DvzGuiControl* dvz_gui_button(DvzGui* gui, const char* name, int flags)
{
    ASSERT(gui != NULL);
    ASSERT(name != NULL);
    // TODO: flags
    DvzGuiControl* control = _add_control(gui, DVZ_GUI_CONTROL_BUTTON, name, sizeof(int), NULL);
    ASSERT(control != NULL);
    return control;
}



DvzGuiControl* dvz_gui_colormap(DvzGui* gui, DvzColormap cmap)
{
    ASSERT(gui != NULL);
    DvzGuiControl* control =
        _add_control(gui, DVZ_GUI_CONTROL_COLORMAP, "", sizeof(DvzColormap), &cmap);
    ASSERT(control != NULL);
    return control;
}



void dvz_gui_destroy(DvzGui* gui)
{
    ASSERT(gui != NULL);
    ASSERT(gui->title != NULL);
    FREE(gui->title);
    for (uint32_t i = 0; i < gui->control_count; i++)
    {
        FREE(gui->controls[i].value);
    }
}
