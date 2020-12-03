#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _default_visual_fill(VklVisual* visual, VklVisualFillEvent ev)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    VklCommands* cmds = ev.cmds;
    uint32_t idx = ev.cmd_idx;
    VkViewport viewport = ev.viewport.viewport;

    ASSERT(viewport.width > 0);
    ASSERT(viewport.height > 0);
    ASSERT(is_obj_created(&visual->graphics[0]->obj));
    ASSERT(is_obj_created(&visual->gbindings[0].obj));
    ASSERT(visual->vertex_buf.size > 0);
    ASSERT(visual->vertex_count > 0);

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(cmds, idx, viewport);
    vkl_cmd_bind_vertex_buffer(cmds, idx, &visual->vertex_buf, 0);
    vkl_cmd_bind_graphics(cmds, idx, visual->graphics[0], &visual->gbindings[0], 0);
    vkl_cmd_draw(cmds, idx, 0, visual->vertex_count);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};
    visual.canvas = canvas;
    // Default fill callback.
    visual.fill_callback = _default_visual_fill;
    obj_created(&visual.obj);
    return visual;
}



void vkl_visual_destroy(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Free the data sources.
    VklSource* source = NULL;
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        source = &visual->sources[i];
        if (source->binding == VKL_PROP_BINDING_CPU)
        {
            FREE(source->u.a.data_original)
            FREE(source->u.a.data_transformed)
            FREE(source->u.a.data_triangulated)
        }
    }

    // Free the vertex buffer data.
    FREE(visual->vertex_data);

    // Free the index buffer data.
    FREE(visual->index_data);

    obj_destroyed(&visual->obj);
}



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

static VkDeviceSize _get_dtype_size(VklDataType dtype)
{
    switch (dtype)
    {
    case VKL_DTYPE_CHAR:
        return 1;
    case VKL_DTYPE_CVEC2:
        return 1 * 2;
    case VKL_DTYPE_CVEC3:
        return 2 * 3;
    case VKL_DTYPE_CVEC4:
        return 3 * 4;


    case VKL_DTYPE_FLOAT:
    case VKL_DTYPE_UINT:
    case VKL_DTYPE_INT:
        return 4;

    case VKL_DTYPE_VEC2:
    case VKL_DTYPE_UVEC2:
    case VKL_DTYPE_IVEC2:
        return 4 * 2;

    case VKL_DTYPE_VEC3:
    case VKL_DTYPE_UVEC3:
    case VKL_DTYPE_IVEC3:
        return 4 * 3;

    case VKL_DTYPE_VEC4:
    case VKL_DTYPE_UVEC4:
    case VKL_DTYPE_IVEC4:
        return 4 * 4;


    case VKL_DTYPE_DOUBLE:
        return 8;
    case VKL_DTYPE_DVEC2:
        return 8 * 2;
    case VKL_DTYPE_DVEC3:
        return 8 * 3;
    case VKL_DTYPE_DVEC4:
        return 8 * 4;

    default:
        break;
    }

    log_error("could not find the size of dtype %d", dtype);
    return 0;
}

void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset)
{
    ASSERT(visual != NULL);
    if (visual->source_count >= VKL_MAX_VISUAL_SOURCES)
    {
        log_error("maximum number of props per visual reached");
        return;
    }
    VklSource source = {0};
    source.prop = prop;
    source.prop_idx = idx;
    source.dtype = dtype;
    source.dtype_size = _get_dtype_size(dtype);
    source.loc = loc;
    source.binding_idx = binding_idx;
    source.field_idx = field_idx;
    source.offset = offset;
    visual->sources[visual->source_count++] = source;
}



void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(is_obj_created(&graphics->obj));
    if (visual->graphics_count >= VKL_MAX_GRAPHICS_PER_VISUAL)
    {
        log_error("maximum number of graphics per visual reached");
        return;
    }
    visual->graphics[visual->graphics_count++] = graphics;
}



void vkl_visual_compute(VklVisual* visual, VklCompute* compute)
{
    ASSERT(visual != NULL);
    ASSERT(compute != NULL);
    ASSERT(is_obj_created(&compute->obj));
    if (visual->compute_count >= VKL_MAX_COMPUTES_PER_VISUAL)
    {
        log_error("maximum number of computes per visual reached");
        return;
    }
    visual->computes[visual->compute_count++] = compute;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count)
{
    ASSERT(visual != NULL);
    bool to_realloc = item_count > visual->item_count;
    visual->item_count = item_count;
    visual->group_count = group_count;
    if (!to_realloc)
        return;
    // Resize all CPU sources that are already allocated.
    VklSource* source = NULL;
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        source = &visual->sources[i];
        if (source->binding == VKL_PROP_BINDING_CPU)
        {
            // Reallocate data_original.
            if (source->u.a.data_original != NULL)
            {
                log_trace("realloc data_original source #%d to %d items", i, item_count);
                REALLOC(source->u.a.data_original, item_count * source->dtype_size);
            }

            // Reallocate data_transformed.
            if (source->u.a.data_transformed != NULL)
            {
                log_trace("realloc data_transformed source #%d to %d items", i, item_count);
                REALLOC(source->u.a.data_transformed, item_count * source->dtype_size);
            }

            // Reallocate data_triangulated.
            if (source->u.a.data_triangulated != NULL)
            {
                log_trace("realloc data_triangulated source #%d to %d items", i, item_count);
                REALLOC(source->u.a.data_triangulated, item_count * source->dtype_size);
            }
        }
    }
}



void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
    if (group_idx >= VKL_MAX_VISUAL_GROUPS)
    {
        log_error("maximum number of groups reached");
        return;
    }
    visual->group_count = MAX(visual->group_count, group_idx + 1);
    visual->group_sizes[group_idx] = size;
}



void vkl_visual_data(VklVisual* visual, VklPropType type, uint32_t idx, const void* data)
{
    ASSERT(visual != NULL);
    vkl_visual_data_partial(visual, type, idx, 0, visual->item_count, data);
}



static VklSource* _get_source(VklVisual* visual, VklPropType type, uint32_t idx)
{
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        if (visual->sources[i].prop == type && visual->sources[i].prop_idx == idx)
            return &visual->sources[i];
    }
    log_error("Data source for prop %d #%d could not be found", type, idx);
    return NULL;
}



static void
_copy_array(void* dst, const void* src, uint32_t offset, uint32_t count, VkDeviceSize item_size)
{
    memcpy((void*)((int64_t)dst + (int64_t)(offset * item_size)), src, count * item_size);
}



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t first_item, uint32_t item_count,
    const void* data)
{
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_CPU;
    source->u.a.offset = first_item * source->dtype_size;
    source->u.a.size = item_count * source->dtype_size;

    // Ensure the source data_original array is allocated.
    if (source->u.a.data_original == NULL)
    {
        log_trace("allocating data_original for source");
        source->u.a.data_original = malloc(visual->item_count * source->dtype_size);
    }
    // Make a copy of the user-provided data.
    ASSERT(source->u.a.data_original != NULL);
    ASSERT(item_count * source->dtype_size <= visual->item_count * source->dtype_size);
    _copy_array(source->u.a.data_original, data, first_item, item_count, source->dtype_size);
}



void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_BUFFER;
    source->u.b.br = br;
    source->u.b.offset = offset;
    source->u.b.size = size;
}



void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklTexture* texture, uvec3 offset, uvec3 shape)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->binding = VKL_PROP_BINDING_TEXTURE;
    source->u.t.texture = texture;
    for (uint32_t i = 0; i < 3; i++)
    {
        source->u.t.offset[i] = offset[i];
        source->u.t.shape[i] = shape[i];
    }
}



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

void vkl_visual_transform_callback(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->transform_callback = callback;
}

void vkl_visual_triangulation_callback(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->triangulation_callback = callback;
}

void vkl_visual_bake_callback(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->bake_callback = callback;
}



// To be called by the baking callbacks.
void vkl_visual_data_alloc(VklVisual* visual, uint32_t vertex_count, uint32_t index_count)
{
    // Allocate the vertex data and index data CPU arrays of the visual.

    ASSERT(visual != NULL);
    ASSERT(vertex_count > 0);

    // Determine the vertex size.
    VkDeviceSize vertex_size = 0;
    VklSource* source = _get_source(visual, VKL_PROP_VERTEX, 0);
    if (source == NULL)
    {
        log_error("the PROP_VERTEX prop is mandatory");
        return;
    }
    ASSERT(source != NULL);
    vertex_size = source->dtype_size;
    ASSERT(vertex_size > 0);

    // Allocate the vertex data.
    if (visual->vertex_data == NULL)
    {
        visual->vertex_data = calloc(vertex_count, vertex_size);
    }
    // Reallocate.
    else if (vertex_count > visual->vertex_count)
    {
        REALLOC(visual->vertex_data, vertex_count * vertex_size);
    }

    // Allocate the index data.
    if (visual->index_data == NULL && index_count > 0)
    {
        visual->index_data = calloc(index_count, sizeof(VklIndex));
    }
    // Reallocate.
    else if (index_count > visual->index_count)
    {
        REALLOC(visual->index_data, index_count * sizeof(VklIndex));
    }
}



void vkl_visual_data_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data)
{
    ASSERT(visual != NULL);
    VklVisualDataEvent ev = {0};
    ev.viewport = viewport;
    ev.coords = coords;
    ev.user_data = user_data;

    if (visual->transform_callback != NULL)
    {
        log_trace("visual transform callback");
        // This callback updates VklDataSource.data_transformed
        visual->transform_callback(visual, ev);
    }

    if (visual->triangulation_callback != NULL)
    {
        log_trace("visual triangulation callback");
        // This callback updates VklDataSource.data_triangulated
        visual->triangulation_callback(visual, ev);
    }

    if (visual->bake_callback != NULL)
    {
        log_trace("visual bake callback");
        // This callback allocates and updates VklDataSource.vertex_data/index_data
        visual->bake_callback(visual, ev);
    }
}



void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    visual->fill_callback = callback;
}



void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data)
{
    // Called in a REFILL canvas callback.

    ASSERT(visual != NULL);
    ASSERT(visual->fill_callback != NULL);

    VklVisualFillEvent ev = {0};
    ev.clear_color = clear_color;
    ev.cmds = cmds;
    ev.cmd_idx = cmd_idx;
    ev.viewport = viewport;
    ev.user_data = user_data;

    visual->fill_callback(visual, ev);
    visual->canvas->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
}
