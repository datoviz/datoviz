#ifndef VKY_CONSTANTS_HEADER
#define VKY_CONSTANTS_HEADER

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

// This should be set by cmake
#ifndef DATA_DIR
#define DATA_DIR ""
#endif



/*************************************************************************************************/
/*  Built-in fixed constants                                                                     */
/*************************************************************************************************/

#define ENGINE_NAME         "Visky"
#define APPLICATION_NAME    "Visky prototype"
#define APPLICATION_VERSION VK_MAKE_VERSION(1, 0, 0)


/*************************************************************************************************/
/*  Visky environment variables                                                                  */
/*************************************************************************************************/

#define VKY_FPS                 (vky_check_env("VKY_FPS", false))
#define VKY_VSYNC               (vky_check_env("VKY_VSYNC", true) && !VKY_FPS)
#define VKY_DEBUG_TEST          (vky_check_env("VKY_DEBUG_TEST", false))
#define VKY_INVERSE_MOUSE_WHEEL vky_check_env("VKY_INVERSE_MOUSE_WHEEL", false)


/*************************************************************************************************/
/*  Math                                                                                         */
/*************************************************************************************************/

#ifndef M_PI
#define M_PI 3.141592653589793
#endif
#define M_2PI 6.283185307179586

#define VKY_NEVER -1000000



/*************************************************************************************************/
/*  Builtin limits                                                                               */
/*************************************************************************************************/

#define VKY_MAX_CONSTANTS         1000
#define VKY_MAX_CANVASES          1000
#define VKY_MAX_PANEL_LINKS       1000
#define VKY_MAX_EVENT_CALLBACKS   100
#define VKY_MAX_FRAMES_IN_FLIGHT  2
#define VKY_MAX_ATTRIBUTE_COUNT   100
#define VKY_MAX_BINDING_COUNT     100
#define VKY_MAX_SHADER_COUNT      20
#define VKY_MAX_GUI_COUNT         100
#define VKY_MAX_GUI_CONTROLS      1000
#define VKY_AXES_MAX_USER_TICKS   100
#define VKY_AXES_MAX_LABEL_LENGTH 256

// Number of matrices per viewport in the MVP dynamic uniform buffer
#define VKY_MVP_BUFFER_SIZE (3 * sizeof(mat4) + 3 * sizeof(vec4) + sizeof(cvec4))
// Note that the params buffer must be < 65K as it is passed as an UBO to the GPU.
// Should be a multiple of 4 for alignement reasons.
#define VKY_RAW_PATH_MAX_PATHS 4 * 500



/*************************************************************************************************/
/*  Defaults                                                                                     */
/*************************************************************************************************/

#define VKY_DEFAULT_VERTEX_FORMAT_POS   VK_FORMAT_R32G32B32_SFLOAT
#define VKY_DEFAULT_VERTEX_FORMAT_CMAP  VK_FORMAT_R8G8_UINT
#define VKY_DEFAULT_VERTEX_FORMAT_COLOR VK_FORMAT_R8G8B8A8_UNORM
#define VKY_DEFAULT_BACKEND             VKY_BACKEND_GLFW, NULL
#define VKY_DEFAULT_COLORMAP            VKY_CMAP_HSV
#define VKY_DEFAULT_TRIANGLE_PARAMS     "pzqAQ"
#define VKY_AXES_DEFAULT_SCALE                                                                    \
    {                                                                                             \
        -1.0, +1.0, false                                                                         \
    }


/*************************************************************************************************/
/*  Misc                                                                                         */
/*************************************************************************************************/

#define VKY_TIME vky_get_timer()

#define VKY_FONT_TEXTURE_SHAPE                                                                    \
    {                                                                                             \
        6, 16                                                                                     \
    }

#ifdef __cplusplus
#define VKY_CLEAR_COLOR_WHITE                                                                     \
    {                                                                                             \
        255, 255, 255, 255                                                                        \
    }
#define VKY_CLEAR_COLOR_BLACK                                                                     \
    {                                                                                             \
        0, 0, 0, 0                                                                                \
    }
#else
#define VKY_CLEAR_COLOR_WHITE                                                                     \
    (VkyColorBytes) { 255, 255, 255, 255 }
#define VKY_CLEAR_COLOR_BLACK                                                                     \
    (VkyColorBytes) { 0, 0, 0, 0 }
#endif

#define VKY_FONT_MAP_FILENAME "font_inconsolata.png"



/*************************************************************************************************/
/*  Constant system                                                                              */
/*************************************************************************************************/

typedef enum
{
    VKY_ANTIALIAS_ID = 1,
    VKY_ARCBALL_PAN_FACTOR_ID,
    VKY_AXES_COVERAGE_NTICKS_X_ID,
    VKY_AXES_COVERAGE_NTICKS_Y_ID,
    VKY_AXES_DECIMAL_FORMAT_MAX_ID,
    VKY_AXES_DECIMAL_FORMAT_MIN_ID,
    VKY_AXES_DEFAULT_TICK_COUNT_ID,
    VKY_AXES_DYAD_TRIGGER_ID,
    VKY_AXES_FONT_SIZE_ID,
    VKY_AXES_GRID_COLOR_A_ID,
    VKY_AXES_GRID_COLOR_B_ID,
    VKY_AXES_GRID_COLOR_G_ID,
    VKY_AXES_GRID_COLOR_R_ID,
    VKY_AXES_LABEL_COLOR_R_ID,
    VKY_AXES_LABEL_COLOR_G_ID,
    VKY_AXES_LABEL_COLOR_B_ID,
    VKY_AXES_LABEL_COLOR_A_ID,
    VKY_AXES_LABEL_FONT_SIZE_ID,
    VKY_AXES_LABEL_HMARGIN_ID,
    VKY_AXES_LABEL_VMARGIN_ID,
    VKY_AXES_LIM_COLOR_A_ID,
    VKY_AXES_LIM_COLOR_B_ID,
    VKY_AXES_LIM_COLOR_G_ID,
    VKY_AXES_LIM_COLOR_R_ID,
    VKY_AXES_MARGIN_BOTTOM_ID,
    VKY_AXES_MARGIN_LEFT_ID,
    VKY_AXES_MARGIN_RIGHT_ID,
    VKY_AXES_MARGIN_TOP_ID,
    VKY_AXES_MAX_GLYPHS_PER_TICK_ID,
    VKY_AXES_MAX_TICKS_ID,
    VKY_AXES_MAX_USER_TICKS_ID,
    VKY_AXES_MINOR_TICKS_COUNT_ID,
    VKY_AXES_PHYSICAL_DENSITY_MAX_ID,
    VKY_AXES_TEXT_COLOR_A_ID,
    VKY_AXES_TEXT_COLOR_B_ID,
    VKY_AXES_TEXT_COLOR_G_ID,
    VKY_AXES_TEXT_COLOR_R_ID,
    VKY_AXES_TICK_ANCHOR_X_ID,
    VKY_AXES_TICK_ANCHOR_Y_ID,
    VKY_AXES_TICK_COLOR_A_ID,
    VKY_AXES_TICK_COLOR_B_ID,
    VKY_AXES_TICK_COLOR_G_ID,
    VKY_AXES_TICK_COLOR_R_ID,
    VKY_AXES_TICK_LENGTH_MAJOR_ID,
    VKY_AXES_TICK_LENGTH_MINOR_ID,
    VKY_AXES_TICK_LINEWIDTH_GRID_ID,
    VKY_AXES_TICK_LINEWIDTH_LIM_ID,
    VKY_AXES_TICK_LINEWIDTH_MAJOR_ID,
    VKY_AXES_TICK_LINEWIDTH_MINOR_ID,
    VKY_CAMERA_SENSITIVITY_ID,
    VKY_CAMERA_SPEED_ID,
    VKY_CAMERA_YMAX_ID,
    VKY_CAMERA_YMIN_ID,
    VKY_COLORBAR_HMARGIN_ID,
    VKY_COLORBAR_TICK_LENGTH_ID,
    VKY_COLORBAR_VMARGIN_ID,
    VKY_COLORBAR_WIDTH_ID,
    VKY_DEFAULT_BACKEND_ID,
    VKY_DEFAULT_CAMERA_CENTER_X_ID,
    VKY_DEFAULT_CAMERA_CENTER_Y_ID,
    VKY_DEFAULT_CAMERA_CENTER_Z_ID,
    VKY_DEFAULT_CAMERA_POS_X_ID,
    VKY_DEFAULT_CAMERA_POS_Y_ID,
    VKY_DEFAULT_CAMERA_POS_Z_ID,
    VKY_DEFAULT_CAMERA_UP_X_ID,
    VKY_DEFAULT_CAMERA_UP_Y_ID,
    VKY_DEFAULT_CAMERA_UP_Z_ID,
    VKY_DEFAULT_HEIGHT_ID,
    VKY_DEFAULT_LIGHT_COEFFS_W_ID,
    VKY_DEFAULT_LIGHT_COEFFS_X_ID,
    VKY_DEFAULT_LIGHT_COEFFS_Y_ID,
    VKY_DEFAULT_LIGHT_COEFFS_Z_ID,
    VKY_DEFAULT_LIGHT_POSITION_W_ID,
    VKY_DEFAULT_LIGHT_POSITION_X_ID,
    VKY_DEFAULT_LIGHT_POSITION_Y_ID,
    VKY_DEFAULT_LIGHT_POSITION_Z_ID,
    VKY_DEFAULT_WIDTH_ID,
    VKY_DPI_SCALING_FACTOR_ID,
    VKY_IMAGE_FORMAT_ID,
    VKY_IMGUI_FONT_SIZE_ID,
    VKY_KEY_PRESS_DELAY_ID,
    VKY_MAX_BUFFER_COUNT_ID,
    VKY_MAX_CAMERA_COUNT_ID,
    VKY_MAX_GRID_VIEWPORTS_ID,
    VKY_MAX_MESH_OBJECTS_ID,
    VKY_MAX_TEXTURE_COUNT_ID,
    VKY_MAX_VISUAL_COUNT_ID,
    VKY_MAX_VISUAL_RESOURCES_ID,
    VKY_MOUSE_CLICK_MAX_DELAY_ID,
    VKY_MOUSE_CLICK_MAX_SHIFT_ID,
    VKY_MOUSE_DOUBLE_CLICK_MAX_DELAY_ID,
    VKY_PANZOOM_LOOKAT_CENTER_ID,
    VKY_PANZOOM_LOOKAT_POS_X_ID,
    VKY_PANZOOM_LOOKAT_POS_Y_ID,
    VKY_PANZOOM_LOOKAT_POS_Z_ID,
    VKY_PANZOOM_LOOKAT_UP_X_ID,
    VKY_PANZOOM_LOOKAT_UP_Y_ID,
    VKY_PANZOOM_LOOKAT_UP_Z_ID,
    VKY_PANZOOM_MAX_ZOOM_ID,
    VKY_PANZOOM_MIN_ZOOM_ID,
    VKY_PANZOOM_MOUSE_RIGHT_DRAG_FACTOR_ID,
    VKY_PANZOOM_MOUSE_WHEEL_FACTOR_ID,
    VKY_PANZOOM_ORTHO_FAR_ID,
    VKY_PANZOOM_ORTHO_NEAR_ID,
    VKY_PERSPECTIVE_FAR_VAL_ID,
    VKY_PERSPECTIVE_NEAR_VAL_ID,
    VKY_RAW_PATH_MAX_PATHS_ID,
} VkyConstantName;

VKY_EXPORT double vky_get_constant(VkyConstantName name, double default_value);

VKY_EXPORT void vky_set_constant(VkyConstantName name, double value);

VKY_EXPORT void vky_reset_constant(VkyConstantName name);

VKY_EXPORT void vky_reset_all_constants(void);

#define VKY_CONST(name, value) vky_get_constant(name, value)

#define VKY_CONST_INT(name, value) ((uint32_t)round(vky_get_constant(name, value)))

VKY_INLINE bool vky_check_env(const char* name, bool default_value)
{
    const char* value = getenv(name);
    return value == NULL ? default_value : strcmp(value, "0") != 0;
}

VKY_INLINE double vky_get_env(const char* name, bool default_value)
{
    const char* value = getenv(name);
    if (value == NULL)
        return default_value;
    double number = 0;
    sscanf(value, "%lf", &number);
    return number;
}



/*************************************************************************************************/
/*  Customizable constants                                                                       */
/*************************************************************************************************/

#define VKY_DPI_SCALING_FACTOR                                                                    \
    vky_get_env("VKY_DPI_FACTOR", VKY_CONST(VKY_DPI_SCALING_FACTOR_ID, 1.00))

#define VKY_IMAGE_FORMAT                 VKY_CONST(VKY_IMAGE_FORMAT_ID, VK_FORMAT_B8G8R8A8_UNORM)
#define VKY_DEFAULT_WIDTH                VKY_CONST_INT(VKY_DEFAULT_WIDTH_ID, 1280)
#define VKY_DEFAULT_HEIGHT               VKY_CONST_INT(VKY_DEFAULT_HEIGHT_ID, 720)
#define VKY_IMGUI_FONT_SIZE              VKY_CONST_INT(VKY_IMGUI_FONT_SIZE_ID, 10)
#define VKY_ANTIALIAS                    VKY_CONST(VKY_ANTIALIAS_ID, 1.0)
#define VKY_MAX_MESH_OBJECTS             VKY_CONST_INT(VKY_MAX_MESH_OBJECTS_ID, 10000)
#define VKY_DEFAULT_LIGHT_POSITION_X     VKY_CONST(VKY_DEFAULT_LIGHT_POSITION_X_ID, -3)
#define VKY_DEFAULT_LIGHT_POSITION_Y     VKY_CONST(VKY_DEFAULT_LIGHT_POSITION_Y_ID, -5)
#define VKY_DEFAULT_LIGHT_POSITION_Z     VKY_CONST(VKY_DEFAULT_LIGHT_POSITION_Z_ID, -8)
#define VKY_DEFAULT_LIGHT_POSITION_W     VKY_CONST(VKY_DEFAULT_LIGHT_POSITION_W_ID, 0)
#define VKY_DEFAULT_LIGHT_COEFFS_X       VKY_CONST(VKY_DEFAULT_LIGHT_COEFFS_X_ID, .3)
#define VKY_DEFAULT_LIGHT_COEFFS_Y       VKY_CONST(VKY_DEFAULT_LIGHT_COEFFS_Y_ID, .6)
#define VKY_DEFAULT_LIGHT_COEFFS_Z       VKY_CONST(VKY_DEFAULT_LIGHT_COEFFS_Z_ID, .3)
#define VKY_DEFAULT_LIGHT_COEFFS_W       VKY_CONST(VKY_DEFAULT_LIGHT_COEFFS_W_ID, 0)
#define VKY_DEFAULT_CAMERA_POS_X         VKY_CONST(VKY_DEFAULT_CAMERA_POS_X_ID, 0)
#define VKY_DEFAULT_CAMERA_POS_Y         VKY_CONST(VKY_DEFAULT_CAMERA_POS_Y_ID, 0)
#define VKY_DEFAULT_CAMERA_POS_Z         VKY_CONST(VKY_DEFAULT_CAMERA_POS_Z_ID, 5)
#define VKY_DEFAULT_CAMERA_CENTER_X      VKY_CONST(VKY_DEFAULT_CAMERA_CENTER_X_ID, 0)
#define VKY_DEFAULT_CAMERA_CENTER_Y      VKY_CONST(VKY_DEFAULT_CAMERA_CENTER_Y_ID, 0)
#define VKY_DEFAULT_CAMERA_CENTER_Z      VKY_CONST(VKY_DEFAULT_CAMERA_CENTER_Z_ID, 0)
#define VKY_DEFAULT_CAMERA_UP_X          VKY_CONST(VKY_DEFAULT_CAMERA_UP_X_ID, 0)
#define VKY_DEFAULT_CAMERA_UP_Y          VKY_CONST(VKY_DEFAULT_CAMERA_UP_Y_ID, -1)
#define VKY_DEFAULT_CAMERA_UP_Z          VKY_CONST(VKY_DEFAULT_CAMERA_UP_Z_ID, 0)
#define VKY_MAX_VISUAL_COUNT             VKY_CONST_INT(VKY_MAX_VISUAL_COUNT_ID, 10000)
#define VKY_MAX_VISUAL_BUNDLE_COUNT      VKY_CONST_INT(VKY_MAX_VISUAL_COUNT_ID, 10000)
#define VKY_MAX_VISUALS_PER_BUNDLE       VKY_CONST_INT(VKY_MAX_VISUAL_COUNT_ID, 100)
#define VKY_MAX_CAMERA_COUNT             VKY_CONST_INT(VKY_MAX_CAMERA_COUNT_ID, 1000)
#define VKY_MAX_GRID_VIEWPORTS           VKY_CONST_INT(VKY_MAX_GRID_VIEWPORTS_ID, 1000)
#define VKY_MAX_BUFFER_COUNT             VKY_CONST_INT(VKY_MAX_BUFFER_COUNT_ID, 100)
#define VKY_MAX_TEXTURE_COUNT            VKY_CONST_INT(VKY_MAX_TEXTURE_COUNT_ID, 100)
#define VKY_MAX_VISUAL_RESOURCES         VKY_CONST_INT(VKY_MAX_VISUAL_RESOURCES_ID, 100)
#define VKY_PERSPECTIVE_NEAR_VAL         VKY_CONST(VKY_PERSPECTIVE_NEAR_VAL_ID, 0.1f)
#define VKY_PERSPECTIVE_FAR_VAL          VKY_CONST(VKY_PERSPECTIVE_FAR_VAL_ID, 100.0f)
#define VKY_MOUSE_CLICK_MAX_SHIFT        VKY_CONST(VKY_MOUSE_CLICK_MAX_SHIFT_ID, 10)
#define VKY_MOUSE_CLICK_MAX_DELAY        VKY_CONST(VKY_MOUSE_CLICK_MAX_DELAY_ID, .5)
#define VKY_MOUSE_DOUBLE_CLICK_MAX_DELAY VKY_CONST(VKY_MOUSE_DOUBLE_CLICK_MAX_DELAY_ID, 0.2)
#define VKY_KEY_PRESS_DELAY              VKY_CONST(VKY_KEY_PRESS_DELAY_ID, 0.05)

#define VKY_DEFAULT_LIGHT_POSITION                                                                \
    {                                                                                             \
        VKY_DEFAULT_LIGHT_POSITION_X, VKY_DEFAULT_LIGHT_POSITION_Y, VKY_DEFAULT_LIGHT_POSITION_Z, \
            VKY_DEFAULT_LIGHT_POSITION_W                                                          \
    }
#define VKY_DEFAULT_LIGHT_COEFFS                                                                  \
    {                                                                                             \
        VKY_DEFAULT_LIGHT_COEFFS_X, VKY_DEFAULT_LIGHT_COEFFS_Y, VKY_DEFAULT_LIGHT_COEFFS_Z,       \
            VKY_DEFAULT_LIGHT_COEFFS_W                                                            \
    }
#define VKY_DEFAULT_CAMERA_POS                                                                    \
    {                                                                                             \
        VKY_DEFAULT_CAMERA_POS_X, VKY_DEFAULT_CAMERA_POS_Y, VKY_DEFAULT_CAMERA_POS_Z              \
    }
#define VKY_DEFAULT_CAMERA_CENTER                                                                 \
    {                                                                                             \
        VKY_DEFAULT_CAMERA_CENTER_X, VKY_DEFAULT_CAMERA_CENTER_Y, VKY_DEFAULT_CAMERA_CENTER_Z     \
    }
#define VKY_DEFAULT_CAMERA_UP                                                                     \
    {                                                                                             \
        VKY_DEFAULT_CAMERA_UP_X, VKY_DEFAULT_CAMERA_UP_Y, VKY_DEFAULT_CAMERA_UP_Z                 \
    }



/*************************************************************************************************/
/*  Axes constants                                                                               */
/*************************************************************************************************/

#define VKY_AXES_DYAD_TRIGGER      VKY_CONST_INT(VKY_AXES_DYAD_TRIGGER_ID, 2)
#define VKY_AXES_MINOR_TICKS_COUNT VKY_CONST_INT(VKY_AXES_MINOR_TICKS_COUNT_ID, 5)
#define VKY_AXES_MAX_TICKS                                                                        \
    VKY_CONST_INT(                                                                                \
        VKY_AXES_MAX_TICKS_ID,                                                                    \
        50) // max number of major ticks on 1 axis, without counting the cache
#define VKY_AXES_MAX_GLYPHS_PER_TICK VKY_CONST_INT(VKY_AXES_MAX_GLYPHS_PER_TICK_ID, 16)

// The parameters below are automatically obtained from the user-customizable parameters above.
#define VKY_AXES_CACHE_LEN  2 * (VKY_AXES_DYAD_TRIGGER) + 1
#define VKY_AXES_CACHE_SIZE (VKY_AXES_CACHE_LEN) * (VKY_AXES_CACHE_LEN)
#define VKY_AXES_MAX_VERTICES                                                                     \
    2 * (VKY_AXES_MAX_TICKS) * (VKY_AXES_MINOR_TICKS_COUNT - 1) * (VKY_AXES_CACHE_SIZE)
#define VKY_AXES_MAX_STRINGS 2 * (VKY_AXES_MAX_TICKS) * (VKY_AXES_CACHE_LEN)
#define VKY_AXES_MAX_GLYPHS  (VKY_AXES_MAX_STRINGS) * (VKY_AXES_MAX_GLYPHS_PER_TICK)

// Initial number of ticks, which will be normalized by the window size on each axis.
#define VKY_AXES_DEFAULT_TICK_COUNT VKY_CONST_INT(VKY_AXES_DEFAULT_TICK_COUNT_ID, 12)

// Range of ticks within which we use decimal rather than scientific format for tick labels.
#define VKY_AXES_DECIMAL_FORMAT_MIN VKY_CONST(VKY_AXES_DECIMAL_FORMAT_MIN_ID, 0.01)
#define VKY_AXES_DECIMAL_FORMAT_MAX VKY_CONST(VKY_AXES_DECIMAL_FORMAT_MAX_ID, 1000)

// Maximally accepted physical coverage of the tick labels on each axes. The higher the more ticks.
// Note that the tick cache must be taken into account when zooming.
#define VKY_AXES_PHYSICAL_DENSITY_MAX VKY_CONST(VKY_AXES_PHYSICAL_DENSITY_MAX_ID, .350)


// The factors below is used to compute the physical coverage of the axes ticks
// to determine the appropriate number of ticks. The higher the value, the sparsier the number of
// ticks on the associated axis is.
#define VKY_AXES_COVERAGE_NTICKS_X    VKY_CONST(VKY_AXES_COVERAGE_NTICKS_X_ID, 2.0)
#define VKY_AXES_COVERAGE_NTICKS_Y    VKY_CONST(VKY_AXES_COVERAGE_NTICKS_Y_ID, 2.0)
#define VKY_AXES_MARGIN_TOP           VKY_CONST(VKY_AXES_MARGIN_TOP_ID, 20.0)
#define VKY_AXES_MARGIN_RIGHT         VKY_CONST(VKY_AXES_MARGIN_RIGHT_ID, 20.0)
#define VKY_AXES_MARGIN_BOTTOM        VKY_CONST(VKY_AXES_MARGIN_BOTTOM_ID, 50.0)
#define VKY_AXES_MARGIN_LEFT          VKY_CONST(VKY_AXES_MARGIN_LEFT_ID, 90.0)
#define VKY_AXES_FONT_SIZE            VKY_CONST(VKY_AXES_FONT_SIZE_ID, 10.0)
#define VKY_AXES_TICK_LENGTH_MAJOR    VKY_CONST(VKY_AXES_TICK_LENGTH_MAJOR_ID, 12.0)
#define VKY_AXES_TICK_LENGTH_MINOR    VKY_CONST(VKY_AXES_TICK_LENGTH_MINOR_ID, 8.0)
#define VKY_AXES_TICK_LINEWIDTH_MINOR VKY_CONST(VKY_AXES_TICK_LINEWIDTH_MINOR_ID, 1.0)
#define VKY_AXES_TICK_LINEWIDTH_MAJOR VKY_CONST(VKY_AXES_TICK_LINEWIDTH_MAJOR_ID, 1.25)
#define VKY_AXES_TICK_LINEWIDTH_GRID  VKY_CONST(VKY_AXES_TICK_LINEWIDTH_GRID_ID, 1.0)
#define VKY_AXES_TICK_LINEWIDTH_LIM   VKY_CONST(VKY_AXES_TICK_LINEWIDTH_LIM_ID, 1.35)
#define VKY_AXES_TICK_ANCHOR_X        VKY_CONST(VKY_AXES_TICK_ANCHOR_X_ID, +2)
#define VKY_AXES_TICK_ANCHOR_Y        VKY_CONST(VKY_AXES_TICK_ANCHOR_Y_ID, -1)
#define VKY_AXES_TICK_COLOR_R         VKY_CONST(VKY_AXES_TICK_COLOR_R_ID, 0.10f)
#define VKY_AXES_TICK_COLOR_G         VKY_CONST(VKY_AXES_TICK_COLOR_G_ID, 0.10f)
#define VKY_AXES_TICK_COLOR_B         VKY_CONST(VKY_AXES_TICK_COLOR_B_ID, 0.10f)
#define VKY_AXES_TICK_COLOR_A         VKY_CONST(VKY_AXES_TICK_COLOR_A_ID, 0.50f)
#define VKY_AXES_GRID_COLOR_R         VKY_CONST(VKY_AXES_GRID_COLOR_R_ID, 0.25f)
#define VKY_AXES_GRID_COLOR_G         VKY_CONST(VKY_AXES_GRID_COLOR_G_ID, 0.25f)
#define VKY_AXES_GRID_COLOR_B         VKY_CONST(VKY_AXES_GRID_COLOR_B_ID, 0.25f)
#define VKY_AXES_GRID_COLOR_A         VKY_CONST(VKY_AXES_GRID_COLOR_A_ID, 0.25f)
#define VKY_AXES_LABEL_HMARGIN        VKY_CONST(VKY_AXES_LABEL_HMARGIN_ID, 40)
#define VKY_AXES_LABEL_VMARGIN        VKY_CONST(VKY_AXES_LABEL_VMARGIN_ID, 60)
#define VKY_AXES_LABEL_COLOR_R        VKY_CONST(VKY_AXES_LABEL_COLOR_R_ID, 0.0f)
#define VKY_AXES_LABEL_COLOR_G        VKY_CONST(VKY_AXES_LABEL_COLOR_G_ID, 0.0f)
#define VKY_AXES_LABEL_COLOR_B        VKY_CONST(VKY_AXES_LABEL_COLOR_B_ID, 0.0f)
#define VKY_AXES_LABEL_COLOR_A        VKY_CONST(VKY_AXES_LABEL_COLOR_A_ID, 1.0f)
#define VKY_AXES_LABEL_FONT_SIZE      VKY_CONST(VKY_AXES_LABEL_FONT_SIZE_ID, 12)
#define VKY_AXES_LIM_COLOR_R          VKY_CONST(VKY_AXES_LIM_COLOR_R_ID, 0.25f)
#define VKY_AXES_LIM_COLOR_G          VKY_CONST(VKY_AXES_LIM_COLOR_G_ID, 0.25f)
#define VKY_AXES_LIM_COLOR_B          VKY_CONST(VKY_AXES_LIM_COLOR_B_ID, 0.25f)
#define VKY_AXES_LIM_COLOR_A          VKY_CONST(VKY_AXES_LIM_COLOR_A_ID, 1.00f)
#define VKY_AXES_TEXT_COLOR_R         VKY_CONST(VKY_AXES_TEXT_COLOR_R_ID, 0.25f)
#define VKY_AXES_TEXT_COLOR_G         VKY_CONST(VKY_AXES_TEXT_COLOR_G_ID, 0.25f)
#define VKY_AXES_TEXT_COLOR_B         VKY_CONST(VKY_AXES_TEXT_COLOR_B_ID, 0.25f)
#define VKY_AXES_TEXT_COLOR_A         VKY_CONST(VKY_AXES_TEXT_COLOR_A_ID, 1.00f)
#define VKY_COLORBAR_HMARGIN          VKY_CONST(VKY_COLORBAR_HMARGIN_ID, 100)
#define VKY_COLORBAR_TICK_LENGTH      VKY_CONST(VKY_COLORBAR_TICK_LENGTH_ID, 10)
#define VKY_COLORBAR_VMARGIN          VKY_CONST(VKY_COLORBAR_VMARGIN_ID, 50)
#define VKY_COLORBAR_WIDTH            VKY_CONST(VKY_COLORBAR_WIDTH_ID, 80)



/*************************************************************************************************/
/*  Interact constants                                                                           */
/*************************************************************************************************/

// Panzoom
#define VKY_PANZOOM_MIN_ZOOM                VKY_CONST(VKY_PANZOOM_MIN_ZOOM_ID, 1e-4)
#define VKY_PANZOOM_MAX_ZOOM                VKY_CONST(VKY_PANZOOM_MAX_ZOOM_ID, 1e+4)
#define VKY_PANZOOM_ORTHO_NEAR              VKY_CONST(VKY_PANZOOM_ORTHO_NEAR_ID, 0.1f)
#define VKY_PANZOOM_ORTHO_FAR               VKY_CONST(VKY_PANZOOM_ORTHO_FAR_ID, 10.0f)
#define VKY_PANZOOM_MOUSE_WHEEL_FACTOR      VKY_CONST(VKY_PANZOOM_MOUSE_WHEEL_FACTOR_ID, 2)
#define VKY_PANZOOM_MOUSE_RIGHT_DRAG_FACTOR VKY_CONST(VKY_PANZOOM_MOUSE_RIGHT_DRAG_FACTOR_ID, 3)
#define VKY_PANZOOM_LOOKAT_POS_X            VKY_CONST(VKY_PANZOOM_LOOKAT_POS_X_ID, +0)
#define VKY_PANZOOM_LOOKAT_POS_Y            VKY_CONST(VKY_PANZOOM_LOOKAT_POS_Y_ID, +0)
#define VKY_PANZOOM_LOOKAT_POS_Z            VKY_CONST(VKY_PANZOOM_LOOKAT_POS_Z_ID, -2)
#define VKY_PANZOOM_LOOKAT_CENTER_X         VKY_CONST(VKY_PANZOOM_LOOKAT_CENTER_X_ID, +0)
#define VKY_PANZOOM_LOOKAT_CENTER_Y         VKY_CONST(VKY_PANZOOM_LOOKAT_CENTER_Y_ID, +0)
#define VKY_PANZOOM_LOOKAT_CENTER_Z         VKY_CONST(VKY_PANZOOM_LOOKAT_CENTER_Z_ID, +0)
#define VKY_PANZOOM_LOOKAT_UP_X             VKY_CONST(VKY_PANZOOM_LOOKAT_UP_X_ID, +0)
#define VKY_PANZOOM_LOOKAT_UP_Y             VKY_CONST(VKY_PANZOOM_LOOKAT_UP_Y_ID, -1)
#define VKY_PANZOOM_LOOKAT_UP_Z             VKY_CONST(VKY_PANZOOM_LOOKAT_UP_Z_ID, +0)

// Arcball
#define VKY_ARCBALL_PAN_FACTOR VKY_CONST(VKY_ARCBALL_PAN_FACTOR_ID, 1)

// Camera
#define VKY_CAMERA_SPEED       VKY_CONST(VKY_CAMERA_SPEED_ID, 1)
#define VKY_CAMERA_SENSITIVITY VKY_CONST(VKY_CAMERA_SENSITIVITY_ID, .001)
#define VKY_CAMERA_YMIN        VKY_CONST(VKY_CAMERA_YMIN_ID, -10)
#define VKY_CAMERA_YMAX        VKY_CONST(VKY_CAMERA_YMAX_ID, +10)

#endif
