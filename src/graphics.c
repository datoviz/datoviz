/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "graphics.h"
#include "array.h"
#include "fileio.h"
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

#define PRIMITIVE(x)                                                                              \
    dvz_graphics_renderpass(graphics, renderpass, 0);                                             \
    dvz_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_##x);                                   \
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);


// NOTE: the graphics should be created manually instead
#define CREATE
// dvz_graphics_create(graphics);

#define ATTR_BEGIN(t)                                                                             \
    dvz_graphics_vertex_binding(graphics, 0, sizeof(t));                                          \
    uint32_t attr_idx = 0;

#define ATTR(t, fmt, f) dvz_graphics_vertex_attr(graphics, 0, attr_idx++, fmt, offsetof(t, f));

#define ATTR_POS(t, f) ATTR(t, VK_FORMAT_R32G32B32_SFLOAT, f)

#define ATTR_COL(t, f) ATTR(t, VK_FORMAT_R8G8B8A8_UNORM, f)



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
    SHADER(VERTEX, "graphics_point_vert")
    SHADER(FRAGMENT, "graphics_point_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsPointVertex)
    ATTR_POS(DvzGraphicsPointVertex, pos)
    ATTR_COL(DvzGraphicsPointVertex, color)
    ATTR(DvzGraphicsPointVertex, VK_FORMAT_R32_SFLOAT, size)

    _common_slots(graphics);
    // dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Basic graphics                                                                               */
/*************************************************************************************************/

static void
_graphics_basic(DvzRenderpass* renderpass, DvzGraphics* graphics, VkPrimitiveTopology topology)
{
    ANN(renderpass);
    ANN(graphics);

    SHADER(VERTEX, "graphics_basic_vert")
    SHADER(FRAGMENT, "graphics_basic_frag")

    dvz_graphics_renderpass(graphics, renderpass, 0);
    dvz_graphics_topology(graphics, topology);
    dvz_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzVertex)
    ATTR_POS(DvzVertex, pos)
    ATTR_COL(DvzVertex, color)

    _common_slots(graphics);

    CREATE
}



/*************************************************************************************************/
/*  Raster graphics                                                                              */
/*************************************************************************************************/

static void _graphics_raster(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_raster_vert")
    SHADER(FRAGMENT, "graphics_raster_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsRasterVertex)
    ATTR_POS(DvzGraphicsRasterVertex, pos)
    ATTR(DvzGraphicsRasterVertex, VK_FORMAT_R8_SNORM, depth)
    ATTR(DvzGraphicsRasterVertex, VK_FORMAT_R8_UNORM, cmap_val)
    ATTR(DvzGraphicsRasterVertex, VK_FORMAT_R8_UNORM, alpha)
    ATTR(DvzGraphicsRasterVertex, VK_FORMAT_R8_UNORM, size)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Agg marker graphics                                                                          */
/*************************************************************************************************/

static void _graphics_marker(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_marker_vert")
    SHADER(FRAGMENT, "graphics_marker_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & DVZ_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsMarkerVertex)
    ATTR_POS(DvzGraphicsMarkerVertex, pos)
    ATTR_COL(DvzGraphicsMarkerVertex, color)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R32_SFLOAT, size)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UINT, marker)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UNORM, angle)
    ATTR(DvzGraphicsMarkerVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Segment graphics                                                                             */
/*************************************************************************************************/

static void _graphics_segment(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_segment_vert")
    SHADER(FRAGMENT, "graphics_segment_frag")
    PRIMITIVE(TRIANGLE_LIST)
    // PRIMITIVE(POINT_LIST) // DEBUG

    ATTR_BEGIN(DvzGraphicsSegmentVertex)
    ATTR_POS(DvzGraphicsSegmentVertex, P0)
    ATTR_POS(DvzGraphicsSegmentVertex, P1)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32G32B32A32_SFLOAT, shift)
    ATTR_COL(DvzGraphicsSegmentVertex, color)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SFLOAT, linewidth)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap0)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap1)
    ATTR(DvzGraphicsSegmentVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);

    CREATE
}



/*************************************************************************************************/
/*  Path graphics                                                                                */
/*************************************************************************************************/

static void _graphics_path(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_path_vert")
    SHADER(FRAGMENT, "graphics_path_frag")
    PRIMITIVE(TRIANGLE_STRIP)
    // PRIMITIVE(POINT_LIST)

    ATTR_BEGIN(DvzGraphicsPathVertex)
    ATTR_POS(DvzGraphicsPathVertex, p0)
    ATTR_POS(DvzGraphicsPathVertex, p1)
    ATTR_POS(DvzGraphicsPathVertex, p2)
    ATTR_POS(DvzGraphicsPathVertex, p3)
    ATTR_COL(DvzGraphicsPathVertex, color)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Text graphics                                                                             */
/*************************************************************************************************/

static void _graphics_text(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_text_vert")
    SHADER(FRAGMENT, "graphics_text_frag")
    PRIMITIVE(TRIANGLE_STRIP)

    ATTR_BEGIN(DvzGraphicsTextVertex)
    ATTR_POS(DvzGraphicsTextVertex, pos)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, shift)
    ATTR_COL(DvzGraphicsTextVertex, color)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, glyph_size)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, anchor)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R32_SFLOAT, angle)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R16G16B16A16_UINT, glyph)
    ATTR(DvzGraphicsTextVertex, VK_FORMAT_R8_UINT, transform)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

static void _graphics_image(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_image_vert")
    SHADER(FRAGMENT, "graphics_image_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(DvzGraphicsImageVertex)
    ATTR_POS(DvzGraphicsImageVertex, pos)
    ATTR(DvzGraphicsImageVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_graphics_slot(
            graphics, DVZ_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



static void _graphics_image_cmap(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_image_cmap_vert")
    SHADER(FRAGMENT, "graphics_image_cmap_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(DvzGraphicsImageVertex)
    ATTR_POS(DvzGraphicsImageVertex, pos)
    ATTR(DvzGraphicsImageVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_slots(graphics);

    // Params buffer.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Colormap texture.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Scalar image.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Volume slice                                                                                 */
/*************************************************************************************************/

static void _graphics_volume_slice(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_volume_slice_vert")
    SHADER(FRAGMENT, "graphics_volume_slice_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(DvzGraphicsVolumeSliceVertex)
    ATTR_POS(DvzGraphicsVolumeSliceVertex, pos)
    ATTR(DvzGraphicsVolumeSliceVertex, VK_FORMAT_R32G32B32_SFLOAT, uvw)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Volume                                                                                       */
/*************************************************************************************************/

static void _graphics_volume(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_volume_vert")
    SHADER(FRAGMENT, "graphics_volume_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_DISABLE);
    // dvz_graphics_pick(graphics, true);

    ATTR_BEGIN(DvzGraphicsVolumeVertex)
    ATTR_POS(DvzGraphicsVolumeVertex, pos)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Density 3D texture.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Color 3D texture.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    // Transfer 1D texture.
    dvz_graphics_slot(graphics, DVZ_USER_BINDING + 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  3D mesh                                                                                      */
/*************************************************************************************************/

static void _graphics_mesh(DvzRenderpass* renderpass, DvzGraphics* graphics)
{
    SHADER(VERTEX, "graphics_mesh_vert")
    SHADER(FRAGMENT, "graphics_mesh_frag")
    PRIMITIVE(TRIANGLE_LIST)
    dvz_graphics_depth_test(graphics, DVZ_DEPTH_TEST_ENABLE);
    // dvz_graphics_front_face(graphics, VK_FRONT_FACE_CLOCKWISE);
    // dvz_graphics_cull_mode(graphics, VK_CULL_MODE_FRONT_BIT);

    ATTR_BEGIN(DvzGraphicsMeshVertex)
    ATTR_POS(DvzGraphicsMeshVertex, pos)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R32G32B32_SFLOAT, normal)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R32G32_SFLOAT, uv)
    ATTR(DvzGraphicsMeshVertex, VK_FORMAT_R8_UNORM, alpha)

    _common_slots(graphics);
    dvz_graphics_slot(graphics, DVZ_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        dvz_graphics_slot(
            graphics, DVZ_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
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

        // Basic graphics types.
    case DVZ_GRAPHICS_POINT:
        _graphics_point(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_LINE:
        _graphics_basic(renderpass, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        break;

    case DVZ_GRAPHICS_LINE_STRIP:
        _graphics_basic(renderpass, graphics, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        break;

    case DVZ_GRAPHICS_TRIANGLE:
        _graphics_basic(renderpass, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        break;

    case DVZ_GRAPHICS_TRIANGLE_STRIP:
        _graphics_basic(renderpass, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        break;

    case DVZ_GRAPHICS_TRIANGLE_FAN:
        _graphics_basic(renderpass, graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
        break;


    case DVZ_GRAPHICS_RASTER:
        _graphics_raster(renderpass, graphics);
        break;


        // Agg graphics types.
    case DVZ_GRAPHICS_MARKER:
        _graphics_marker(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_SEGMENT:
        _graphics_segment(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_PATH:
        _graphics_path(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_TEXT:
        _graphics_text(renderpass, graphics);
        break;


        // Image
    case DVZ_GRAPHICS_IMAGE:
        _graphics_image(renderpass, graphics);
        break;

        // Image cmap
    case DVZ_GRAPHICS_IMAGE_CMAP:
        _graphics_image_cmap(renderpass, graphics);
        break;

        // Volume slice
    case DVZ_GRAPHICS_VOLUME_SLICE:
        _graphics_volume_slice(renderpass, graphics);
        break;

        // Volume
    case DVZ_GRAPHICS_VOLUME:
        _graphics_volume(renderpass, graphics);
        break;


        // 3D meshes
    case DVZ_GRAPHICS_MESH:
        _graphics_mesh(renderpass, graphics);
        break;

    case DVZ_GRAPHICS_CUSTOM:
        break;

    default:
        log_error("no graphics type specified");
        break;
    }
}
