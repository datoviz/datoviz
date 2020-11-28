#include "../include/visky/graphics.h"
#include "../include/visky/canvas.h"


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define SHADER(name)                                                                              \
    char path[1024];                                                                              \
    snprintf(path, sizeof(path), "%s/spirv/%s.vert.spv", DATA_DIR, name);                         \
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);                              \
    snprintf(path, sizeof(path), "%s/spirv/%s.frag.spv", DATA_DIR, name);                         \
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);



/*************************************************************************************************/
/*  Points                                                                                     */
/*************************************************************************************************/

static void _graphics_points(VklCanvas* canvas, VklGraphics* graphics)
{
    ASSERT(canvas != NULL);
    ASSERT(graphics != NULL);

    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    SHADER("points")

    // TODO
    // vkl_graphics_vertex_binding(graphics, 0, sizeof(VklVertex));
    // vkl_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VklVertex,
    // pos)); vkl_graphics_vertex_attr(
    //     graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VklVertex, color));
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(type != VKL_GRAPHICS_NONE);

    VklGraphics* graphics = NULL;

    int32_t idx = (int32_t)type;
    ASSERT(idx > 0);

    graphics = &canvas->graphics[idx];
    if (is_obj_created(&graphics->obj))
        return graphics;
    ASSERT(!is_obj_created(&graphics->obj));

    vkl_graphics(canvas->gpu);

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
