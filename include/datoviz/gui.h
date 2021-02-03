#ifndef DVZ_IMGUI_HEADER
#define DVZ_IMGUI_HEADER

#include "../include/datoviz/canvas.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_MAX_GUI_CONTROLS 32



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    DVZ_GUI_STANDARD = 0,
    DVZ_GUI_PROMPT = 1,
    DVZ_GUI_FIXED_TL = 10,
    DVZ_GUI_FIXED_TR = 11,
    DVZ_GUI_FIXED_LL = 12,
    DVZ_GUI_FIXED_LR = 13,
} DvzGuiStyle;

typedef enum
{
    DVZ_GUI_CONTROL_NONE,
    DVZ_GUI_CONTROL_FLOAT_SLIDER,
} DvzGuiControlType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzGui DvzGui;
typedef struct DvzGuiControl DvzGuiControl;

typedef struct DvzGuiControlFloatSlider DvzGuiControlFloatSlider;
typedef union DvzGuiControlUnion DvzGuiControlUnion;



/*************************************************************************************************/
/*  Gui style                                                                                  */
/*************************************************************************************************/

struct DvzGuiControlFloatSlider
{
    float vmin;
    float vmax;
};



union DvzGuiControlUnion
{
    DvzGuiControlFloatSlider fs;
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
    DvzGuiControl controls[DVZ_MAX_GUI_CONTROLS];
};



/*************************************************************************************************/
/*  Gui functions                                                                              */
/*************************************************************************************************/

DVZ_EXPORT void dvz_imgui_init(DvzCanvas* canvas);

DVZ_EXPORT void dvz_gui_begin(const char* title, DvzGuiStyle style);

DVZ_EXPORT void dvz_gui_end();

DVZ_EXPORT void dvz_imgui_destroy();

DVZ_EXPORT void dvz_gui_callback(DvzCanvas* canvas, DvzEvent event);

DVZ_EXPORT void dvz_gui_callback_fps(DvzCanvas* canvas, DvzEvent event);



DVZ_EXPORT DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags);

DVZ_EXPORT void dvz_gui_float_slider(DvzGui* gui, const char* name, double vmin, double vmax);

DVZ_EXPORT void dvz_gui_destroy(DvzGui* gui);



#ifdef __cplusplus
}
#endif

#endif
