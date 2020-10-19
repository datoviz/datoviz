#ifndef VKY_IMGUI_HEADER
#define VKY_IMGUI_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include "app.h"
#include "offscreen.h"
#include "vklite.h"
#include <vulkan/vulkan.h>



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_GUI_STANDARD,
    VKY_GUI_FIXED_TL,
    VKY_GUI_FIXED_TR,
    VKY_GUI_FIXED_LL,
    VKY_GUI_FIXED_LR
} VkyGuiStyle;



typedef enum
{
    VKY_GUI_BUTTON = 1,
    VKY_GUI_CHECKBOX,
    VKY_GUI_RADIO,
    GKY_GUI_INT_STEPPER,
    VKY_GUI_INT_SLIDER,
    VKY_GUI_FLOAT_SLIDER,
    VKY_GUI_TEXT,
    VKY_GUI_COMBO,
    VKY_GUI_LISTBOX,
    VKY_GUI_COLOR,
    VKY_GUI_FPS,
} VkyGuiControlType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct VkyCanvas VkyCanvas;
typedef struct VkyImGuiTexture VkyImGuiTexture;
typedef struct VkyGuiControl VkyGuiControl;
typedef struct VkyGui VkyGui;



struct VkyImGuiTexture
{
    VkyTexture texture;
    void* id;
};



typedef struct VkyGuiParams VkyGuiParams;
struct VkyGuiParams
{
    VkyGuiStyle style;
    const char* title;
};



typedef struct VkyGuiListParams VkyGuiListParams;
struct VkyGuiListParams
{
    const uint32_t item_count;
    const char** item_names;
};



struct VkyGuiControl
{
    VkyGuiControlType control_type;
    const char* name;
    const void* params;
    void* value;
};



struct VkyGui
{
    const char* title;
    VkyGuiStyle style;

    uint32_t control_count;
    VkyGuiControl controls[VKY_MAX_GUI_CONTROLS];
};



/*************************************************************************************************/
/*  Internal ImGui                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vky_imgui_init(VkyCanvas* canvas);
VKY_EXPORT void vky_imgui_newframe(void);

VKY_EXPORT VkyImGuiTexture vky_imgui_image_from_texture(VkyTexture texture);
VKY_EXPORT VkyImGuiTexture vky_imgui_image_from_canvas(VkyCanvas* canvas);

VKY_EXPORT VkyCanvas* vky_imgui_canvas_create(VkyCanvas* canvas, uint32_t width, uint32_t height);
VKY_EXPORT void vky_imgui_canvas_init(VkyCanvas* canvas);
VKY_EXPORT void vky_imgui_canvas_next_frame(VkyCanvas* canvas);

VKY_EXPORT void vky_imgui_image(VkyImGuiTexture* imtexture, float, float);
VKY_EXPORT void vky_imgui_render(VkyCanvas* canvas, VkCommandBuffer cmd_buf);
VKY_EXPORT void vky_imgui_destroy(void);



/*************************************************************************************************/
/*  Public GUI API                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyGui* vky_create_gui(VkyCanvas*, VkyGuiParams);

VKY_EXPORT void
vky_gui_control(VkyGui*, VkyGuiControlType, const char*, const void* params, void* out_value);

VKY_EXPORT void vky_gui_fps(VkyGui* gui);

VKY_EXPORT void vky_destroy_guis();

#ifdef __cplusplus
}
#endif

#endif
