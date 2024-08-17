/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/graphics.h"
#include "fileio.h"
#include "scene/array.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Utils                                                                                       */
/*************************************************************************************************/

static inline void _load_shader(
    DvzGraphics* graphics, VkShaderStageFlagBits stage, //
    DvzSize size, const unsigned char* buffer)
{
    ANN(graphics);
    ANN(buffer);
    ASSERT(size > 0);
    uint32_t* code = (uint32_t*)calloc(size, 1);
    memcpy(code, buffer, size);
    ASSERT(size % 4 == 0);
    dvz_graphics_shader_spirv(graphics, stage, size, code);
    FREE(code);
}

#define SHADER(stage, x)                                                                          \
    {                                                                                             \
        unsigned long size = 0;                                                                   \
        unsigned char* buffer = dvz_resource_shader(x, &size);                                    \
        ASSERT(size > 0);                                                                         \
        ANN(buffer);                                                                              \
        _load_shader(graphics, VK_SHADER_STAGE_##stage##_BIT, size, buffer);                      \
    }



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

static void _common_slots(DvzGraphics* graphics)
{
    dvz_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    dvz_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport
    // dvz_graphics_slot(graphics, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // color texture
}



/*************************************************************************************************/
/*  Point graphics                                                                               */
/*************************************************************************************************/

static void _graphics_point(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    ANN(renderpass);
    ANN(graphics);

    SHADER(VERTEX, "graphics_point_vert")
    SHADER(FRAGMENT, "graphics_point_frag")

    dvz_graphics_renderpass(graphics, renderpass, 0);
    dvz_graphics_primitive(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    dvz_graphics_vertex_binding(
        graphics, 0, sizeof(DvzGraphicsPointVertex), VK_VERTEX_INPUT_RATE_VERTEX);

    dvz_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DvzGraphicsPointVertex, pos));

    dvz_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(DvzGraphicsPointVertex, color));

    dvz_graphics_vertex_attr(
        graphics, 0, 2, VK_FORMAT_R32_SFLOAT, offsetof(DvzGraphicsPointVertex, size));

    _common_slots(graphics);
}



static void _graphics_triangle(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    ANN(renderpass);
    ANN(graphics);

    SHADER(VERTEX, "graphics_trivial_vert")
    SHADER(FRAGMENT, "graphics_trivial_frag")

    dvz_graphics_renderpass(graphics, renderpass, 0);
    dvz_graphics_primitive(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    dvz_graphics_vertex_binding(graphics, 0, sizeof(DvzVertex), VK_VERTEX_INPUT_RATE_VERTEX);

    dvz_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(DvzVertex, pos));

    dvz_graphics_vertex_attr(graphics, 0, 1, VK_FORMAT_R8G8B8A8_UNORM, offsetof(DvzVertex, color));

    _common_slots(graphics);
}



/*************************************************************************************************/
/*  Graphics builtin                                                                             */
/*************************************************************************************************/

void dvz_graphics_builtin(
    DvzRenderpass* renderpass, DvzGraphics* graphics, DvzGraphicsType type, int flags)
{
    ANN(renderpass);
    ANN(graphics);
    ASSERT(type != DVZ_GRAPHICS_NONE);

    graphics->type = type;
    graphics->flags = flags;

    switch (type)
    {
    case DVZ_GRAPHICS_POINT:
        _graphics_point(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_TRIANGLE:
        _graphics_triangle(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_CUSTOM:
        dvz_graphics_renderpass(graphics, renderpass, 0);
        break;

    default:
        log_error("no graphics type specified");
        break;
    }
}
