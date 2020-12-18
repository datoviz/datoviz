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
        const unsigned char* buffer = vkl_resource_shader(x, &size);                              \
        ASSERT(size > 0);                                                                         \
        ASSERT(buffer != NULL);                                                                   \
        _load_shader(graphics, VK_SHADER_STAGE_##stage##_BIT, size, buffer);                      \
    }

#define PRIMITIVE(x)                                                                              \
    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);                                    \
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_##x);                                   \
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

// TODO: common bindings
#define CREATE vkl_graphics_create(graphics);

#define ATTR_BEGIN(t)                                                                             \
    vkl_graphics_vertex_binding(graphics, 0, sizeof(t));                                          \
    uint32_t attr_idx = 0;

#define ATTR(t, fmt, f) vkl_graphics_vertex_attr(graphics, 0, attr_idx++, fmt, offsetof(t, f));

#define ATTR_POS(t, f) ATTR(t, VK_FORMAT_R32G32B32_SFLOAT, f)

#define ATTR_COL(t, f) ATTR(t, VK_FORMAT_R8G8B8A8_UNORM, f)



/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

// Number of common bindings
#define USER_BINDING 3

static void _common_bindings(VklGraphics* graphics)
{
    vkl_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // MVP
    vkl_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);         // viewport
    vkl_graphics_slot(graphics, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // color texture
}



/*************************************************************************************************/
/*  Basic graphics                                                                               */
/*************************************************************************************************/

static void _graphics_points(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_point_vert")
    SHADER(FRAGMENT, "graphics_point_frag")
    PRIMITIVE(POINT_LIST)

    ATTR_BEGIN(VklVertex)
    ATTR_POS(VklVertex, pos)
    ATTR_COL(VklVertex, color)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}

static void _graphics_basic(VklCanvas* canvas, VklGraphics* graphics, VkPrimitiveTopology topology)
{
    SHADER(VERTEX, "graphics_basic_vert")
    SHADER(FRAGMENT, "graphics_basic_frag")

    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);
    vkl_graphics_topology(graphics, topology);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    ATTR_BEGIN(VklVertex)
    ATTR_POS(VklVertex, pos)
    ATTR_COL(VklVertex, color)

    _common_bindings(graphics);

    CREATE
}

static void _graphics_marker(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_marker_vert")
    SHADER(FRAGMENT, "graphics_marker_frag")
    PRIMITIVE(POINT_LIST)

    ATTR_BEGIN(VklGraphicsMarkerVertex)
    ATTR_POS(VklGraphicsMarkerVertex, pos)
    ATTR_COL(VklGraphicsMarkerVertex, color)
    ATTR(VklGraphicsMarkerVertex, VK_FORMAT_R32_SFLOAT, size)
    ATTR(VklGraphicsMarkerVertex, VK_FORMAT_R8_UINT, marker)
    ATTR(VklGraphicsMarkerVertex, VK_FORMAT_R8_UNORM, angle)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}

static void _graphics_segment(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_segment_vert")
    SHADER(FRAGMENT, "graphics_segment_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(VklGraphicsSegmentVertex)
    ATTR_POS(VklGraphicsSegmentVertex, P0)
    ATTR_POS(VklGraphicsSegmentVertex, P1)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32G32B32A32_SFLOAT, shift)
    ATTR_COL(VklGraphicsSegmentVertex, color)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SFLOAT, linewidth)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap0)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap1)
    // vkl_graphics_vertex_attr(
    //     graphics, 0, 7, VK_FORMAT_R8_UINT, offsetof(VklGraphicsSegmentVertex, transform_mode));

    _common_bindings(graphics);

    CREATE
}

static void _graphics_text(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_text_vert")
    SHADER(FRAGMENT, "graphics_text_frag")
    PRIMITIVE(TRIANGLE_STRIP)

    ATTR_BEGIN(VklGraphicsTextVertex)
    ATTR_POS(VklGraphicsTextVertex, pos)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32B32A32_SFLOAT, shift)
    ATTR_COL(VklGraphicsTextVertex, color)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, glyph_size)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, anchor)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32_SFLOAT, angle)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R16G16B16A16_UINT, glyph)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkl_graphics_slot(graphics, USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Graphics                                                                                     */
/*************************************************************************************************/

VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type, int flags)
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

        // Basic graphics types.
    case VKL_GRAPHICS_POINTS:
        _graphics_points(canvas, graphics);
        break;

    case VKL_GRAPHICS_LINES:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        break;

    case VKL_GRAPHICS_LINE_STRIP:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        break;

    case VKL_GRAPHICS_TRIANGLES:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        break;

    case VKL_GRAPHICS_TRIANGLE_STRIP:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        break;

    case VKL_GRAPHICS_TRIANGLE_FAN:
        _graphics_basic(canvas, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        break;


        // Agg graphics types.
    case VKL_GRAPHICS_MARKER:
        _graphics_marker(canvas, graphics);
        break;

    case VKL_GRAPHICS_SEGMENT:
        _graphics_segment(canvas, graphics);
        break;

    case VKL_GRAPHICS_TEXT:
        _graphics_text(canvas, graphics);
        break;


    default:
        log_error("no graphics type specified");
        break;
    }

    ASSERT(is_obj_created(&graphics->obj));
    return graphics;
}



/*************************************************************************************************/
/*  Viewpor                                                                                      */
/*************************************************************************************************/

VklViewport vkl_viewport_full(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklViewport viewport = {0};

    viewport.viewport.x = 0;
    viewport.viewport.y = 0;
    viewport.viewport.minDepth = +0;
    viewport.viewport.maxDepth = +1;

    viewport.size_framebuffer[0] = viewport.viewport.width =
        (float)canvas->swapchain.images->width;
    viewport.size_framebuffer[1] = viewport.viewport.height =
        (float)canvas->swapchain.images->height;

    viewport.size_screen[0] = canvas->window->width;
    viewport.size_screen[1] = canvas->window->height;

    // TODO
    viewport.dpi_scaling = 1.0;

    return viewport;
}
