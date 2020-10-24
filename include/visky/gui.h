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
    VKY_GUI_STANDARD = 0,
    VKY_GUI_PROMPT = 1,
    VKY_GUI_FIXED_TL = 10,
    VKY_GUI_FIXED_TR = 11,
    VKY_GUI_FIXED_LL = 12,
    VKY_GUI_FIXED_LR = 13,
} VkyGuiStyle;



typedef enum
{
    VKY_GUI_BUTTON = 1,
    VKY_GUI_CHECKBOX = 2,
    VKY_GUI_RADIO = 3,
    VKY_GUI_COMBO = 4,
    VKY_GUI_LISTBOX = 5,
    VKY_GUI_TEXTBOX = 6,
    VKY_GUI_TEXTBOX_PROMPT = 7,
    VKY_GUI_TEXT = 8,
    GKY_GUI_INT_STEPPER = 10,
    VKY_GUI_INT_SLIDER = 11,
    VKY_GUI_FLOAT_SLIDER = 12,
    VKY_GUI_COLOR = 20,
    VKY_GUI_FPS = 99,
} VkyGuiControlType;


typedef enum
{
    VKY_PROMPT_HIDDEN,
    VKY_PROMPT_SHOWN,
    VKY_PROMPT_ACTIVE
} VkyPromptState;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// typedef struct VkyCanvas VkyCanvas;
typedef struct VkyImGuiTexture VkyImGuiTexture;
typedef struct VkyGuiControl VkyGuiControl;
typedef struct VkyGui VkyGui;
typedef struct VkyPrompt VkyPrompt;



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
    void* callback;
};



struct VkyGui
{
    VkyCanvas* canvas;

    const char* title;
    VkyGuiStyle style;

    uint32_t control_count;
    VkyGuiControl controls[VKY_MAX_GUI_CONTROLS];

    bool is_visible;
    uint32_t frame_count;
};



struct VkyPrompt
{
    char text[VKY_MAX_PROMPT_SIZE];
    VkyPromptState state;
    VkyGui* gui;
};



/*************************************************************************************************/
/*  Internal ImGui                                                                               */
/*************************************************************************************************/

VKY_EXPORT void vky_imgui_init(VkyCanvas* canvas);
VKY_EXPORT void vky_imgui_newframe();

VKY_EXPORT VkyImGuiTexture vky_imgui_image_from_texture(VkyTexture texture);
VKY_EXPORT VkyImGuiTexture vky_imgui_image_from_canvas(VkyCanvas* canvas);

VKY_EXPORT VkyCanvas* vky_imgui_canvas_create(VkyCanvas* canvas, uint32_t width, uint32_t height);
VKY_EXPORT void vky_imgui_canvas_init(VkyCanvas* canvas);
VKY_EXPORT void vky_imgui_canvas_next_frame(VkyCanvas* canvas);
VKY_EXPORT void vky_imgui_capture(VkyCanvas* canvas);

VKY_EXPORT void vky_imgui_image(VkyImGuiTexture* imtexture, float, float);
VKY_EXPORT void vky_imgui_render(VkyCanvas* canvas, VkCommandBuffer cmd_buf);
VKY_EXPORT void vky_imgui_destroy();



/*************************************************************************************************/
/*  Public GUI API                                                                               */
/*************************************************************************************************/

VKY_EXPORT VkyGui* vky_create_gui(VkyCanvas* canvas, VkyGuiParams);

VKY_EXPORT void vky_gui_control(
    VkyGui* gui, VkyGuiControlType control, const char* title, const void* params,
    void* out_value);

VKY_EXPORT void vky_gui_fps(VkyGui* gui);

VKY_EXPORT void vky_destroy_guis(VkyCanvas* canvas);



/*************************************************************************************************/
/*  Prompt GUI                                                                                   */
/*************************************************************************************************/

VKY_EXPORT void vky_prompt(VkyCanvas* canvas);

VKY_EXPORT char* vky_prompt_get(VkyCanvas* canvas);



#ifdef __cplusplus
}
#endif

#endif
