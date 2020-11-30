#include "../include/visky/graphics.h"
#include "../include/visky/canvas.h"


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Utils                                                                                       */
/*************************************************************************************************/

static inline void _load_shader(
    VklGraphics* graphics, VkShaderStageFlagBits stage, //
    VkDeviceSize size, const unsigned char* buffer)
{
    ASSERT(buffer != NULL);
    uint32_t* code = (uint32_t*)calloc(size, 1);
    memcpy(code, buffer, size);
    ASSERT(size % 4 == 0);
    vkl_graphics_shader_spirv(graphics, stage, size, code);
    FREE(code);
}

#define SHADER(stage, x)                                                                          \
    {                                                                                             \
        unsigned long size = 0;                                                                   \
        const unsigned char* buffer = vkl_binary_shader_load(x, &size);                           \
        ASSERT(size > 0);                                                                         \
        ASSERT(buffer != NULL);                                                                   \
        _load_shader(graphics, VK_SHADER_STAGE_##stage##_BIT, size, buffer);                      \
    }

#define PRIMITIVE(x)                                                                              \
    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);                                    \
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_##x);                                   \
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);                                    \
    _common_bindings(graphics);

// TODO: common bindings
#define CREATE vkl_graphics_create(graphics);



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

static void _common_bindings(VklGraphics* graphics)
{
    vkl_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    vkl_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport
    // vkl_graphics_slot(graphics, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // color texture
}



/*************************************************************************************************/
/*  Points                                                                                       */
/*************************************************************************************************/

static void _graphics_points(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_points_vert")
    SHADER(FRAGMENT, "graphics_points_frag")
    PRIMITIVE(POINT_LIST)

    vkl_graphics_vertex_binding(graphics, 0, sizeof(VklVertex));
    vkl_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VklVertex, pos));
    vkl_graphics_vertex_attr(graphics, 0, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VklVertex, color));

    CREATE
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(type != VKL_GRAPHICS_NONE);

    for (uint32_t i = 0; i < VKL_GRAPHICS_COUNT; i++)
    {
        if (canvas->graphics[i].obj.status == VKL_OBJECT_STATUS_NONE)
            canvas->graphics[i].obj.status = VKL_OBJECT_STATUS_INIT;
    }

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
