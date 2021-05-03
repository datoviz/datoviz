#ifndef DVZ_GUI_HEADER
#define DVZ_GUI_HEADER

// NOTE: we separate between gui.h and controls.h to isolate C++ compilation units from the rest of
// the library
// This file should NOT import gui.h (C++)

#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define CONTROL_MAX     32
#define MAX_TEXT_LENGTH 1024



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// GUI creation flags
typedef enum
{
    DVZ_GUI_FLAGS_NONE = 0x0000,
    DVZ_GUI_FLAGS_FIXED = 0x0001,
    DVZ_GUI_FLAGS_CORNER_UL = 0x0010,
    DVZ_GUI_FLAGS_CORNER_UR = 0x0020,
    DVZ_GUI_FLAGS_CORNER_LR = 0x0040,
    DVZ_GUI_FLAGS_CORNER_LL = 0x0030,
} DvzGuiFlags;



// GUI control type
typedef enum
{
    DVZ_GUI_CONTROL_NONE,
    DVZ_GUI_CONTROL_CHECKBOX,      /* checkbox */
    DVZ_GUI_CONTROL_SLIDER_FLOAT,  /* slider for 1 float */
    DVZ_GUI_CONTROL_SLIDER_FLOAT2, /* sliders for 2 floats */
    DVZ_GUI_CONTROL_SLIDER_INT,    /* slider for 1 int */
    DVZ_GUI_CONTROL_INPUT_FLOAT,   /* textbox for 1 float */
    DVZ_GUI_CONTROL_LABEL,         /* static text */
    DVZ_GUI_CONTROL_TEXTBOX,       /* input text */
    DVZ_GUI_CONTROL_BUTTON,        /* button */
    DVZ_GUI_CONTROL_COLORMAP,      /* static gradient with a preexisting colormap */
} DvzGuiControlType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiControl DvzGuiControl;

typedef struct DvzGuiControlSliderFloat DvzGuiControlSliderFloat;
typedef struct DvzGuiControlSliderFloat2 DvzGuiControlSliderFloat2;
typedef struct DvzGuiControlInputFloat DvzGuiControlInputFloat;
typedef struct DvzGuiControlSliderInt DvzGuiControlSliderInt;
typedef union DvzGuiControlUnion DvzGuiControlUnion;
// typedef struct DvzGuiControlLabel DvzGuiControlLabel;



/*************************************************************************************************/
/*  Gui style                                                                                  */
/*************************************************************************************************/

struct DvzGuiControlSliderFloat
{
    float vmin;
    float vmax;
};



struct DvzGuiControlSliderFloat2
{
    float vmin;
    float vmax;
    bool force_increasing;
};



struct DvzGuiControlSliderInt
{
    int vmin;
    int vmax;
};



struct DvzGuiControlInputFloat
{
    float step;
    float step_fast;
};



union DvzGuiControlUnion
{
    DvzGuiControlSliderFloat sf;
    DvzGuiControlSliderFloat2 sf2;
    DvzGuiControlSliderInt si;
    DvzGuiControlInputFloat f;
    // DvzGuiControlLabel l;
};



// struct DvzGuiControlLabel
// {
// };



struct DvzGuiControl
{
    DvzGui* gui;
    const char* name;
    int flags;
    void* value;
    DvzGuiControlType type;
    DvzGuiControlUnion u;
};



struct DvzGui
{
    DvzObject obj;
    DvzCanvas* canvas;

    char* title;
    int flags;
    uint32_t control_count;
    DvzGuiControl controls[CONTROL_MAX];
};



/*************************************************************************************************/
/*  Gui functions                                                                                */
/*************************************************************************************************/

/**
 * Create a new GUI dialog.
 *
 * @param canvas the canvas
 * @param title the GUI title
 * @param flags optional flags
 * @returns the GUI
 */
DVZ_EXPORT DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags);
/**
 * Return the pointer to the control's value.
 *
 * @param control the GUI
 * @returns the pointer to the value
 */
DVZ_EXPORT void* dvz_gui_value(DvzGuiControl* control);

/**
 * Add a checkbox control.
 *
 * @param gui the GUI
 * @param name the control label
 * @param value whether the checkbox is initially checked
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_checkbox(DvzGui* gui, const char* name, bool value);

/**
 * Add a slider for float number input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param value the initial value
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl*
dvz_gui_slider_float(DvzGui* gui, const char* name, float vmin, float vmax, float value);

/**
 * Add a slider for 2 float numbers.
 *
 * @param gui the GUI
 * @param name the control label
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param values the initial values
 * @param force_increasing whether the first value must always be lower than the second
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_slider_float2(
    DvzGui* gui, const char* name, float vmin, float vmax, vec2 value, bool force_increasing);

/**
 * Add a slider for integer input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param value the initial value
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl*
dvz_gui_slider_int(DvzGui* gui, const char* name, int vmin, int vmax, int value);

/**
 * Add an input float control.
 *
 * @param gui the GUI
 * @param name the control label
 * @param step the step
 * @param step_fast the step for fast scrolling
 * @param value the initial value
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl*
dvz_gui_input_float(DvzGui* gui, const char* name, float step, float step_fast, float value);

/**
 * Add a static, non-modifiable label.
 *
 * @param gui the GUI
 * @param name the control label
 * @param text the control text
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_label(DvzGui* gui, const char* name, char* text);

/**
 * Add a textbox control for text input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param value the initial text
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_textbox(DvzGui* gui, const char* name, char* text);

/**
 * Add a button.
 *
 * @param gui the GUI
 * @param name the control label
 * @param flags optional flags
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_button(DvzGui* gui, const char* name, int flags);

/**
 * Add a colormap image.
 *
 * @param gui the GUI
 * @param cmap the colormap
 * @returns the control
 */
DVZ_EXPORT DvzGuiControl* dvz_gui_colormap(DvzGui* gui, DvzColormap cmap);



/**
 * Destroy a GUI.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



#ifdef __cplusplus
}
#endif

#endif
