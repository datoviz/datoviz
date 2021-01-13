#define STB_IMAGE_IMPLEMENTATION
#include "../include/visky/graphics.h"
#include "../include/visky/atlas.h"
#include "../include/visky/canvas.h"
#include "../include/visky/context.h"


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

static void _common_bindings(VklGraphics* graphics)
{
    vkl_graphics_slot(graphics, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // MVP
    vkl_graphics_slot(graphics, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // viewport
    // vkl_graphics_slot(graphics, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // color texture
}



/*************************************************************************************************/
/*  Basic graphics                                                                               */
/*************************************************************************************************/

static void _graphics_points(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_point_vert")
    SHADER(FRAGMENT, "graphics_point_frag")
    PRIMITIVE(POINT_LIST)

    // Depth test flag.
    if ((graphics->flags & VKL_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        vkl_graphics_depth_test(graphics, VKL_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(VklVertex)
    ATTR_POS(VklVertex, pos)
    ATTR_COL(VklVertex, color)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}

static void _graphics_basic(VklCanvas* canvas, VklGraphics* graphics, VkPrimitiveTopology topology)
{
    SHADER(VERTEX, "graphics_basic_vert")
    SHADER(FRAGMENT, "graphics_basic_frag")

    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);
    vkl_graphics_topology(graphics, topology);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    // Depth test flag.
    if ((graphics->flags & VKL_GRAPHICS_FLAGS_DEPTH_TEST) != 0)
        vkl_graphics_depth_test(graphics, VKL_DEPTH_TEST_ENABLE);

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
    ATTR(VklGraphicsMarkerVertex, VK_FORMAT_R8_UINT, transform)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    CREATE
}



/*************************************************************************************************/
/*  Segment graphics                                                                             */
/*************************************************************************************************/

static void
_graphics_segment_callback(VklGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);
    ASSERT(data->indices != NULL);

    ASSERT(item_count > 0);
    vkl_array_resize(data->vertices, 4 * item_count);
    vkl_array_resize(data->indices, 6 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // Fill the vertices array by simply repeating them 4 times.
    vkl_array_data(data->vertices, 4 * data->current_idx, 4, 1, item);

    // Fill the indices array.
    VklIndex* indices = (VklIndex*)data->indices->data;
    uint32_t i = data->current_idx;
    indices[6 * i + 0] = 4 * i + 0;
    indices[6 * i + 1] = 4 * i + 1;
    indices[6 * i + 2] = 4 * i + 2;
    indices[6 * i + 3] = 4 * i + 0;
    indices[6 * i + 4] = 4 * i + 2;
    indices[6 * i + 5] = 4 * i + 3;

    data->current_idx++;
}

static void _graphics_segment(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_segment_vert")
    SHADER(FRAGMENT, "graphics_segment_frag")
    PRIMITIVE(TRIANGLE_LIST)
    // PRIMITIVE(POINT_LIST) // DEBUG

    ATTR_BEGIN(VklGraphicsSegmentVertex)
    ATTR_POS(VklGraphicsSegmentVertex, P0)
    ATTR_POS(VklGraphicsSegmentVertex, P1)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32G32B32A32_SFLOAT, shift)
    ATTR_COL(VklGraphicsSegmentVertex, color)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SFLOAT, linewidth)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap0)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R32_SINT, cap1)
    ATTR(VklGraphicsSegmentVertex, VK_FORMAT_R8_UINT, transform)

    _common_bindings(graphics);
    vkl_graphics_callback(graphics, _graphics_segment_callback);

    CREATE
}



/*************************************************************************************************/
/*  Text graphics                                                                             */
/*************************************************************************************************/

static void _graphics_text_callback(VklGraphicsData* data, uint32_t item_count, const void* item)
{
    // NOTE: item_count is the total number of glyphs

    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    vkl_array_resize(data->vertices, 4 * item_count);
    VklFontAtlas* atlas = &data->graphics->gpu->context->font_atlas;
    ASSERT(atlas != NULL);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // const char* str = item;
    const VklGraphicsTextItem* str_item = item;
    uint32_t n = strlen(str_item->string);
    VklGraphicsTextVertex vertex = {0};
    vertex = str_item->vertex;
    ASSERT(n > 0);
    ASSERT(data->current_idx + n <= item_count);
    for (uint32_t i = 0; i < n; i++)
    {
        size_t g = _font_atlas_glyph(atlas, str_item->string, i);

        // Glyph size.
        _font_atlas_glyph_size(atlas, str_item->font_size, vertex.glyph_size);

        // Glyph.
        vertex.glyph[0] = g;                   // char
        vertex.glyph[1] = i;                   // char idx
        vertex.glyph[2] = n;                   // str len
        vertex.glyph[3] = data->current_group; // str idx

        // Glyph colors.
        if (str_item->glyph_colors != NULL)
            memcpy(vertex.color, str_item->glyph_colors[i], sizeof(cvec4));

        // Fill the vertices array by simply repeating them 4 times.
        vkl_array_data(data->vertices, 4 * data->current_idx, 4, 1, &vertex);
        data->current_idx++; // glyph index
    }
    data->current_group++; // glyph index
}

static void _graphics_text(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_text_vert")
    SHADER(FRAGMENT, "graphics_text_frag")
    PRIMITIVE(TRIANGLE_STRIP)

    ATTR_BEGIN(VklGraphicsTextVertex)
    ATTR_POS(VklGraphicsTextVertex, pos)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, shift)
    ATTR_COL(VklGraphicsTextVertex, color)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, glyph_size)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32G32_SFLOAT, anchor)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R32_SFLOAT, angle)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R16G16B16A16_UINT, glyph)
    ATTR(VklGraphicsTextVertex, VK_FORMAT_R8_UINT, transform)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkl_graphics_slot(graphics, VKL_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    vkl_graphics_callback(graphics, _graphics_text_callback);

    CREATE
}



/*************************************************************************************************/
/*  Image                                                                                        */
/*************************************************************************************************/

static void _graphics_image(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_image_vert")
    SHADER(FRAGMENT, "graphics_image_frag")
    PRIMITIVE(TRIANGLE_LIST)

    ATTR_BEGIN(VklGraphicsImageVertex)
    ATTR_POS(VklGraphicsImageVertex, pos)
    ATTR(VklGraphicsImageVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        vkl_graphics_slot(
            graphics, VKL_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Volume image                                                                                 */
/*************************************************************************************************/

static void
_graphics_volume_image_callback(VklGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    ASSERT(item_count > 0);
    vkl_array_resize(data->vertices, 6 * item_count);

    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    const VklGraphicsVolumeItem* item_vert = (const VklGraphicsVolumeItem*)item;

    float xl = item_vert->pos_tl[0];
    float yt = item_vert->pos_tl[1];
    float zf = item_vert->pos_tl[2];

    float xr = item_vert->pos_br[0];
    float yb = item_vert->pos_br[1];
    float zb = item_vert->pos_br[2];

    float ul = item_vert->uvw_tl[0];
    float vt = item_vert->uvw_tl[1];
    float wf = item_vert->uvw_tl[2];

    float ur = item_vert->uvw_br[0];
    float vb = item_vert->uvw_br[1];
    float wb = item_vert->uvw_br[2];

    VklGraphicsVolumeVertex vertices[6] = {
        // vec3 pos, vec3 uvw
        {{xl, yb, zf}, {ul, vb, wf}}, // blf
        {{xr, yb, zb}, {ur, vb, wb}}, // brb
        {{xr, yt, zb}, {ur, vt, wb}}, // trb
        {{xr, yt, zb}, {ur, vt, wb}}, // trb
        {{xl, yt, zf}, {ul, vt, wf}}, // tlf
        {{xl, yb, zf}, {ul, vb, wf}}, // blf
    };

    vkl_array_data(data->vertices, 6 * data->current_idx, 6, 6, vertices);

    data->current_idx++;
}

static void _graphics_volume_image(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_volume_image_vert")
    SHADER(FRAGMENT, "graphics_volume_image_frag")
    PRIMITIVE(TRIANGLE_LIST)
    vkl_graphics_depth_test(graphics, VKL_DEPTH_TEST_ENABLE);

    ATTR_BEGIN(VklGraphicsVolumeVertex)
    ATTR_POS(VklGraphicsVolumeVertex, pos)
    ATTR(VklGraphicsVolumeVertex, VK_FORMAT_R32G32B32_SFLOAT, uvw)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkl_graphics_slot(graphics, VKL_USER_BINDING + 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    vkl_graphics_slot(graphics, VKL_USER_BINDING + 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE

    vkl_graphics_callback(graphics, _graphics_volume_image_callback);
}



/*************************************************************************************************/
/*  3D mesh                                                                                      */
/*************************************************************************************************/

static void _graphics_mesh(VklCanvas* canvas, VklGraphics* graphics)
{
    SHADER(VERTEX, "graphics_mesh_vert")
    SHADER(FRAGMENT, "graphics_mesh_frag")
    PRIMITIVE(TRIANGLE_LIST)
    vkl_graphics_depth_test(graphics, VKL_DEPTH_TEST_ENABLE);
    // vkl_graphics_front_face(graphics, VK_FRONT_FACE_CLOCKWISE);
    // vkl_graphics_cull_mode(graphics, VK_CULL_MODE_FRONT_BIT);

    ATTR_BEGIN(VklGraphicsMeshVertex)
    ATTR_POS(VklGraphicsMeshVertex, pos)
    ATTR(VklGraphicsMeshVertex, VK_FORMAT_R32G32B32_SFLOAT, normal)
    ATTR(VklGraphicsMeshVertex, VK_FORMAT_R32G32_SFLOAT, uv)

    _common_bindings(graphics);
    vkl_graphics_slot(graphics, VKL_USER_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (uint32_t i = 1; i <= 4; i++)
        vkl_graphics_slot(
            graphics, VKL_USER_BINDING + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    CREATE
}



/*************************************************************************************************/
/*  Graphics data                                                                                */
/*************************************************************************************************/

static void _default_callback(VklGraphicsData* data, uint32_t item_count, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(data->vertices != NULL);

    vkl_array_resize(data->vertices, item_count);
    if (item == NULL)
        return;
    ASSERT(item != NULL);
    ASSERT(data->current_idx < item_count);

    // Fill the vertices array by simply copying the current item (assumed to be a vertex).
    vkl_array_data(data->vertices, data->current_idx++, 1, 1, item);
}

// Used by graphics creator
void vkl_graphics_callback(VklGraphics* graphics, VklGraphicsCallback callback)
{
    // The callback must make sure the VklArray* are not NULL and resize them
    ASSERT(graphics != NULL);
    graphics->callback = callback;
}



// Used in visual bake:
VklGraphicsData
vkl_graphics_data(VklGraphics* graphics, VklArray* vertices, VklArray* indices, void* user_data)
{
    ASSERT(graphics != NULL);
    ASSERT(vertices != NULL);

    VklGraphicsData data = {0};
    data.graphics = graphics;
    data.vertices = vertices;
    data.indices = indices;
    data.user_data = user_data;

    if (graphics->callback == NULL)
        graphics->callback = _default_callback;
    return data;
}



void vkl_graphics_alloc(VklGraphicsData* data, uint32_t item_count)
{
    ASSERT(data != NULL);
    if (item_count == 0)
    {
        log_error("empty graphics allocation");
        return;
    }
    ASSERT(item_count > 0);
    data->item_count = item_count;
    VklGraphics* graphics = data->graphics;
    ASSERT(graphics != NULL);

    // The graphics callback should allocate the vertices and indices arrays.
    ASSERT(graphics->callback != NULL);
    graphics->callback(data, item_count, NULL);
}



void vkl_graphics_append(VklGraphicsData* data, const void* item)
{
    ASSERT(data != NULL);
    ASSERT(item != NULL);
    VklGraphics* graphics = data->graphics;
    ASSERT(graphics != NULL);

    // call the callback with item_count and item
    ASSERT(graphics->callback != NULL);
    graphics->callback(data, data->item_count, item);
}



/*************************************************************************************************/
/*  Graphics builtin                                                                             */
/*************************************************************************************************/

static VklGraphics* _find_graphics(VklCanvas* canvas, VklGraphicsType type, int flags)
{
    VklGraphics* graphics = vkl_container_iter_init(&canvas->graphics);
    if (graphics != NULL)
    {
        if (graphics->type == type && graphics->flags == flags)
            return graphics;
        graphics = vkl_container_iter(&canvas->graphics);
    }
    return NULL;
}



VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsType type, int flags)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->gpu != NULL);
    ASSERT(type != VKL_GRAPHICS_NONE);
    ASSERT(canvas->graphics.capacity > 0);

    // Try to find an existing graphics with the requested type and flags.
    VklGraphics* graphics = _find_graphics(canvas, type, flags);
    if (graphics != NULL)
        return graphics;

    // If there is none, create a new one.
    graphics = vkl_container_alloc(&canvas->graphics);
    ASSERT(graphics != NULL);
    ASSERT(!is_obj_created(&graphics->obj));
    *graphics = vkl_graphics(canvas->gpu);
    graphics->type = type;
    graphics->flags = flags;

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


        // Image
    case VKL_GRAPHICS_IMAGE:
        _graphics_image(canvas, graphics);
        break;

        // Volume image
    case VKL_GRAPHICS_VOLUME_IMAGE:
        _graphics_volume_image(canvas, graphics);
        break;


        // 3D meshes
    case VKL_GRAPHICS_MESH:
        _graphics_mesh(canvas, graphics);
        break;


    default:
        log_error("no graphics type specified");
        break;
    }

    return graphics;
}



void vkl_mvp_camera(VklViewport viewport, vec3 eye, vec3 center, vec2 near_far, VklMVP* mvp)
{
    vec3 up = {0, 1, 0};
    glm_lookat(eye, center, up, mvp->view);
    ASSERT(viewport.size_framebuffer[1] > 0);
    float ratio = viewport.size_framebuffer[0] / (float)viewport.size_framebuffer[1];
    glm_perspective(GLM_PI_4, ratio, near_far[0], near_far[1], mvp->proj);
}
