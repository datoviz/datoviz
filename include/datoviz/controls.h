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

#define CONTROL_MAX 32



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_GUI_FLAGS_NONE = 0x0000,
    DVZ_GUI_FLAGS_FIXED = 0x0001,
    DVZ_GUI_FLAGS_CORNER_UL = 0x0010,
    DVZ_GUI_FLAGS_CORNER_UR = 0x0020,
    DVZ_GUI_FLAGS_CORNER_LR = 0x0030,
    DVZ_GUI_FLAGS_CORNER_LL = 0x0040,
} DvzGuiFlags;



typedef enum
{
    DVZ_GUI_CONTROL_NONE,
    DVZ_GUI_CONTROL_SLIDER_FLOAT,
    DVZ_GUI_CONTROL_SLIDER_INT,
} DvzGuiControlType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiControl DvzGuiControl;

typedef struct DvzGuiControlSliderFloat DvzGuiControlSliderFloat;
typedef struct DvzGuiControlSliderInt DvzGuiControlSliderInt;
typedef union DvzGuiControlUnion DvzGuiControlUnion;



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



union DvzGuiControlUnion
{
    DvzGuiControlSliderFloat sf;
    DvzGuiControlSliderInt si;
};



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
};



/*************************************************************************************************/
/*  Gui functions                                                                              */
/*************************************************************************************************/


DVZ_EXPORT DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags);

DVZ_EXPORT void dvz_gui_slider_float(DvzGui* gui, const char* name, float vmin, float vmax);

DVZ_EXPORT void dvz_gui_slider_int(DvzGui* gui, const char* name, int vmin, int vmax);

DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



#ifdef __cplusplus
}
#endif

#endif
