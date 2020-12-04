#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Default callbacks                                                                            */
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



static void _default_visual_bake(VklVisual* visual, VklVisualDataEvent ev)
{
    vkl_bake_vertex_attr(visual);
}



/*************************************************************************************************/
/*  Utils                                                                                        */
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
        return 1 * 3;
    case VKL_DTYPE_CVEC4:
        return 1 * 4;


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

    if (dtype != VKL_DTYPE_NONE)
        log_error("could not find the size of dtype %d", dtype);
    return 0;
}



static VklSource* _get_source(VklVisual* visual, VklPropType type, uint32_t idx)
{
    for (uint32_t i = 0; i < visual->source_count; i++)
    {
        if (visual->sources[i].prop == type && visual->sources[i].prop_idx == idx)
            return &visual->sources[i];
    }
    return NULL;
}



static void _copy_contiguous(
    void* dst, const void* src, uint32_t offset, uint32_t count, VkDeviceSize item_size)
{
    memcpy((void*)((int64_t)dst + (int64_t)(offset * item_size)), src, count * item_size);
}



static void _copy_strided(
    const void* src, VkDeviceSize src_offset, VkDeviceSize src_stride, //
    void* dst, VkDeviceSize dst_offset, VkDeviceSize dst_stride,       //
    VkDeviceSize item_size, uint32_t item_count)
{
    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    ASSERT(src_stride > 0);
    ASSERT(dst_stride > 0);
    ASSERT(item_size > 0);
    ASSERT(item_count > 0);

    log_trace(
        "copy src offset %d stride %d, dst offset %d stride %d, item size %d count %d", src_offset,
        src_stride, dst_offset, dst_stride, item_size, item_count);

    int64_t src_byte = (int64_t)src + (int64_t)src_offset;
    int64_t dst_byte = (int64_t)dst + (int64_t)dst_offset;
    for (uint32_t i = 0; i < item_count; i++)
    {
        memcpy((void*)dst_byte, (void*)src_byte, item_size);
        src_byte += (int64_t)src_stride;
        dst_byte += (int64_t)dst_stride;
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};
    visual.canvas = canvas;

    // Default callbacks.
    visual.fill_callback = _default_visual_fill;
    visual.bake_callback = _default_visual_bake;

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

void vkl_visual_vertex(VklVisual* visual, VkDeviceSize vertex_size)
{
    ASSERT(visual != NULL);
    ASSERT(vertex_size > 0);
    visual->vertex_size = vertex_size;

    VklSource source = {0};
    source.prop = VKL_PROP_VERTEX;
    source.prop_idx = 0;
    source.dtype = VKL_DTYPE_NONE;
    source.dtype_size = vertex_size;
    source.loc = VKL_PROP_LOC_VERTEX;
    source.is_set = true;
    visual->sources[visual->source_count++] = source;
}



void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx,   //
    VklDataType dtype, VklPropLoc loc,                 //
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset)
{
    ASSERT(visual != NULL);
    if (visual->source_count >= VKL_MAX_VISUAL_SOURCES)
    {
        log_error("maximum number of props per visual reached");
        return;
    }
    if (_get_source(visual, prop, idx) != NULL)
    {
        log_error("visual prop %d #%d already exists", prop, idx);
        return;
    }
    VklSource source = {0};
    source.prop = prop;
    source.prop_idx = idx;
    source.pipeline_type = pipeline;
    source.pipeline_idx = pipeline_idx;
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
    visual->graphics[visual->graphics_count] = graphics;
    visual->gbindings[visual->graphics_count] = vkl_bindings(&graphics->slots, 1);
    visual->graphics_count++;
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



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    uint32_t first_item, uint32_t item_count, const void* data)
{
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    if (source == NULL)
        log_error("Data source for prop %d #%d could not be found", type, idx);
    ASSERT(source != NULL);
    ASSERT(source->dtype_size > 0);

    source->is_set = true;
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
    _copy_contiguous(source->u.a.data_original, data, first_item, item_count, source->dtype_size);
}



void vkl_visual_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklBufferRegions br)
{
    vkl_visual_buffer_partial(visual, type, idx, br, 0, br.size);
}



void vkl_visual_buffer_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    if (source == NULL)
        log_error("Data source for prop %d #%d could not be found", type, idx);
    ASSERT(source != NULL);
    if (size == 0)
        size = br.size;

    source->is_set = true;
    source->binding = VKL_PROP_BINDING_BUFFER;
    source->u.b.br = br;
    source->u.b.offset = offset;
    source->u.b.size = size;
}



void vkl_visual_texture(VklVisual* visual, VklPropType type, uint32_t idx, VklTexture* texture)
{
    vkl_visual_texture_partial(visual, type, idx, texture, (uvec3){0}, (uvec3){0});
}



void vkl_visual_texture_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, //
    VklTexture* texture, uvec3 offset, uvec3 shape)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _get_source(visual, type, idx);
    if (source == NULL)
        log_error("Data source for prop %d #%d could not be found", type, idx);
    ASSERT(source != NULL);
    if (shape[0] == 0)
        shape[0] = texture->image->width;
    if (shape[1] == 0)
        shape[1] = texture->image->height;
    if (shape[2] == 0)
        shape[2] = texture->image->depth;

    source->is_set = true;
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



/*************************************************************************************************/
/*  Data update and baking                                                                       */
/*************************************************************************************************/

// To be called by the baking callbacks.
void vkl_bake_alloc(VklVisual* visual, uint32_t vertex_count, uint32_t index_count)
{
    // Allocate the vertex data and index data CPU arrays of the visual.

    ASSERT(visual != NULL);
    ASSERT(vertex_count > 0);
    log_trace(
        "allocate vertex and index buffers with %d vertices and % indices", vertex_count,
        index_count);

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

    visual->vertex_count = vertex_count;
    visual->index_count = index_count;
}



void vkl_bake_vertex_attr(VklVisual* visual)
{
    ASSERT(visual != NULL);
    uint32_t item_count = visual->item_count;
    if (visual->item_count_triangulated != 0)
        item_count = visual->item_count_triangulated;
    ASSERT(item_count > 0);
    VkDeviceSize item_size = 0;

    log_debug("bake visual");
    vkl_bake_alloc(visual, item_count, 0);

    VklSource* source = NULL;
    void* src = NULL;
    for (uint32_t s = 0; s < visual->source_count; s++)
    {
        source = &visual->sources[s];
        if (source->loc == VKL_PROP_LOC_VERTEX_ATTR)
        {
            // Source data array.
            src = source->u.a.data_original;
            if (source->u.a.data_transformed != NULL)
                src = source->u.a.data_transformed;
            if (source->u.a.data_triangulated != NULL)
                src = source->u.a.data_triangulated;
            if (src == NULL)
            {
                log_error("vertex attr #%d not set, skipping", source->field_idx);
                continue;
            }
            ASSERT(src != NULL);
            ASSERT(visual->vertex_data != NULL);

            // Item size.
            item_size = source->dtype_size;
            ASSERT(item_size > 0);

            // Copy the source data array to the vertex data.
            _copy_strided(
                src, 0, item_size,                                        //
                visual->vertex_data, source->offset, visual->vertex_size, //
                item_size, item_count);
        }
    }
}



void vkl_visual_data_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data)
{
    ASSERT(visual != NULL);
    VklContext* ctx = visual->canvas->gpu->context;
    VklVisualDataEvent ev = {0};
    ev.viewport = viewport;
    ev.coords = coords;
    ev.user_data = user_data;

    // uint32_t initial_vertex_count = visual->vertex_count;
    if (visual->transform_callback != NULL)
    {
        log_trace("visual transform callback");
        // This callback updates VklDataSource.data_transformed
        visual->transform_callback(visual, ev);
    }

    if (visual->triangulation_callback != NULL)
    {
        log_trace("visual triangulation callback");
        // This callback updates VklDataSource.data_triangulated and
        // VklVisual.item_count_triangulated. It also updates all CPU data sources (not just POS)
        // with new values in data_triangulated
        visual->triangulation_callback(visual, ev);
    }

    if (visual->bake_callback != NULL)
    {
        log_trace("visual bake callback");
        // This callback allocates and updates VklVisual.vertex_data/index_data
        // TODO: baking callback should reuse functions to copy data into the vertex data array etc
        // NOTE: must take item_count_triangulated into account
        visual->bake_callback(visual, ev);
    }

    // Vertex and index buffer.
    ASSERT(visual->vertex_data != NULL);
    ASSERT(visual->vertex_count > 0);
    ASSERT(
        (visual->index_count > 0 && visual->index_data != NULL) ||
        (visual->index_count == 0 && visual->index_data == NULL));

    // Allocate a vertex buffer if needed.
    {
        VkDeviceSize vertex_buf_size = visual->vertex_count * visual->vertex_size;
        ASSERT(vertex_buf_size > 0);
        if (visual->vertex_buf.count == 0)
        {
            log_trace("allocating vertex buffer with %d vertices", visual->vertex_count);
            visual->vertex_buf =
                vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, vertex_buf_size);
        }
        // Need to reallocate the vertex buffer if there are more vertices.
        else if (visual->vertex_buf.size < vertex_buf_size)
        {
            log_trace("reallocating vertex buffer with %d vertices", visual->vertex_count);
            // NOTE: we waste some space as the previous buffer region with the old vertices is
            // lost.
            visual->vertex_buf =
                vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, vertex_buf_size);
        }
        log_trace("uploading vertex data");
        ASSERT(visual->vertex_buf.count > 0);
        ASSERT(visual->vertex_buf.buffer != VK_NULL_HANDLE);
        vkl_upload_buffers(ctx, visual->vertex_buf, 0, vertex_buf_size, visual->vertex_data);
    }

    // Allocate an index buffer if needed.
    if (visual->index_count > 0)
    {
        VkDeviceSize index_buf_size = visual->index_count * sizeof(VklIndex);
        ASSERT(index_buf_size > 0);
        if (visual->index_buf.buffer == NULL)
        {
            log_trace("allocating index buffer with %d indices", visual->index_count);
            visual->index_buf = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_INDEX, 1, index_buf_size);
        }
        // Need to reallocate the vertex buffer if there are more vertices.
        else if (visual->index_buf.size < index_buf_size)
        {
            log_trace("reallocating index buffer with %d indices", visual->index_count);
            // NOTE: we waste some space as the previous buffer region with the old indices is
            // lost.
            visual->index_buf = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_INDEX, 1, index_buf_size);
        }
        log_trace("uploading index data");
        ASSERT(visual->index_buf.count > 0);
        ASSERT(visual->index_buf.buffer != VK_NULL_HANDLE);
        vkl_upload_buffers(ctx, visual->index_buf, 0, index_buf_size, visual->index_data);
    }

    // Update the bindings.
    // NOTE: only UNIFORM/TEXTURE for now, not CPU
    {
        VklSource* source = NULL;
        for (uint32_t i = 0; i < visual->source_count; i++)
        {
            source = &visual->sources[i];

            // Get the associated VklBinding struct.
            VklBindings* bindings = NULL;
            if (source->pipeline_type == VKL_PIPELINE_GRAPHICS)
                bindings = &visual->gbindings[source->pipeline_idx];
            else if (source->pipeline_type == VKL_PIPELINE_COMPUTE)
                bindings = &visual->cbindings[source->pipeline_idx];
            ASSERT(bindings != NULL);
            ASSERT(is_obj_created(&bindings->obj));

            if (!source->is_set)
            {
                log_error("binding #%d not set, skipping", source->binding_idx);
                continue;
            }

            if (source->loc == VKL_PROP_LOC_UNIFORM || source->loc == VKL_PROP_LOC_STORAGE)
            {
                // TODO: support CPU too
                ASSERT(source->binding == VKL_PROP_BINDING_BUFFER);
                vkl_bindings_buffer(bindings, source->binding_idx, source->u.b.br);
            }

            if (source->loc == VKL_PROP_LOC_SAMPLER)
            {
                // TODO: support CPU too
                ASSERT(source->binding == VKL_PROP_BINDING_TEXTURE);
                vkl_bindings_texture(
                    bindings, source->binding_idx, //
                    source->u.t.texture->image, source->u.t.texture->sampler);
            }

            if (source->loc == VKL_PROP_LOC_PUSH)
            {
                ASSERT(source->binding == VKL_PROP_BINDING_CPU);
                log_error("not implemented yet");
            }
        }
        // Update the bindings.
        for (uint32_t i = 0; i < visual->graphics_count; i++)
            vkl_bindings_update(&visual->gbindings[i]);
        for (uint32_t i = 0; i < visual->compute_count; i++)
            vkl_bindings_update(&visual->cbindings[i]);
    }
}
