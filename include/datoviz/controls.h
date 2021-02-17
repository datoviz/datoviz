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
    DVZ_GUI_CONTROL_CHECKBOX,
    DVZ_GUI_CONTROL_SLIDER_FLOAT,
    DVZ_GUI_CONTROL_SLIDER_INT,
    DVZ_GUI_CONTROL_INPUT_FLOAT,
    DVZ_GUI_CONTROL_LABEL,
    DVZ_GUI_CONTROL_TEXTBOX,
    DVZ_GUI_CONTROL_BUTTON,
    DVZ_GUI_CONTROL_COLORMAP,
} DvzGuiControlType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiControl DvzGuiControl;

typedef struct DvzGuiControlSliderFloat DvzGuiControlSliderFloat;
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

    const char* title;
    int flags;
    uint32_t control_count;
    DvzGuiControl controls[CONTROL_MAX];
    bool show_imgui_demo;
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
 * Add a checkbox control.
 *
 * @param gui the GUI
 * @param name the control label
 * @param value whether the checkbox is initially checked
 */
DVZ_EXPORT void dvz_gui_checkbox(DvzGui* gui, const char* name, bool value);

/**
 * Add a slider for float number input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param value the initial value
 */
DVZ_EXPORT void
dvz_gui_slider_float(DvzGui* gui, const char* name, float vmin, float vmax, float value);

/**
 * Add a slider for integer input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param vmin the minimum value
 * @param vmax the maximum value
 * @param value the initial value
 */
DVZ_EXPORT void dvz_gui_slider_int(DvzGui* gui, const char* name, int vmin, int vmax, int value);

/**
 * Add an input float control.
 *
 * @param gui the GUI
 * @param name the control label
 * @param step the step
 * @param step_fast the step for fast scrolling
 * @param value the initial value
 */
DVZ_EXPORT void
dvz_gui_input_float(DvzGui* gui, const char* name, float step, float step_fast, float value);

/**
 * Add a static, non-modifiable label.
 *
 * @param gui the GUI
 * @param name the control label
 * @param text the control text
 */
DVZ_EXPORT void dvz_gui_label(DvzGui* gui, const char* name, char* text);

/**
 * Add a textbox control for text input.
 *
 * @param gui the GUI
 * @param name the control label
 * @param value the initial text
 */
DVZ_EXPORT void dvz_gui_textbox(DvzGui* gui, const char* name, char* value);

/**
 * Add a button.
 *
 * @param gui the GUI
 * @param name the control label
 * @param flags optional flags
 */
DVZ_EXPORT void dvz_gui_button(DvzGui* gui, const char* name, int flags);

/**
 * Add a colormap image.
 *
 * @param gui the GUI
 * @param cmap the colormap
 */
DVZ_EXPORT void dvz_gui_colormap(DvzGui* gui, DvzColormap cmap);



/**
 * Display the Dear ImGUI demo with all supported controls.
 *
 * @param gui the GUI
 */
DVZ_EXPORT void dvz_gui_demo(DvzGui* gui);

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
