#include "../include/visky/graphics.h"
#include "../include/visky/canvas.h"


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

extern const unsigned char* VKL_BINARY_SHADER_graphics_points_vert;
extern const unsigned char* VKL_BINARY_SHADER_graphics_points_frag;



static inline void _load_shader(
    VklGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const unsigned char* buffer)
{
    uint32_t* code = (uint32_t*)calloc(size, 1);
    memcpy(code, buffer, size);
    ASSERT(size % 4 == 0);
    vkl_graphics_shader_spirv(graphics, stage, size / 4, code);
    FREE(code);
}



#define SHADER(x)                                                                                 \
    _load_shader(                                                                                 \
        graphics, VK_SHADER_STAGE_VERTEX_BIT, sizeof(VKL_BINARY_SHADER_graphics_##x##_vert),      \
        (const unsigned char*)VKL_BINARY_SHADER_graphics_##x##_vert);                             \
    _load_shader(                                                                                 \
        graphics, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VKL_BINARY_SHADER_graphics_##x##_frag),    \
        (const unsigned char*)VKL_BINARY_SHADER_graphics_##x##_frag);



/*************************************************************************************************/
/*  Points */
/*************************************************************************************************/

static void _graphics_points(VklCanvas* canvas, VklGraphics* graphics)
{
    ASSERT(canvas != NULL);
    ASSERT(graphics != NULL);

    SHADER(points)

    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    vkl_graphics_vertex_binding(graphics, 0, sizeof(VklVertex));
    vkl_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VklVertex, pos));
    vkl_graphics_vertex_attr(graphics, 0, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VklVertex, color));

    // Create the slots.
    visual->slots = vkl_slots(gpu);
    vkl_slots_create(&visual->slots);
    vkl_graphics_slots(&visual->graphics, &visual->slots);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual->graphics);
}



/*************************************************************************************************/
/*  Graphics */
/*************************************************************************************************/

VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(type != VKL_GRAPHICS_NONE);

    int32_t idx = (int32_t)type;
    ASSERT(idx > 0);

    VklGraphics* graphics = &canvas->graphics[idx];
    ASSERT(graphics != NULL);
    if (is_obj_created(&graphics->obj))
        return graphics;
    ASSERT(!is_obj_created(&graphics->obj));

    // Common initialization.
    *graphics = vkl_graphics(canvas->gpu);

    switch (type)
    {
    case VKL_GRAPHICS_POINTS:
        _graphics_points(canvas, graphics);
        break;
    default:
        log_error("no graphics type specified");
        break;
    }

    ASSERT(is_obj_created(&graphics->obj));
    return graphics;
}
