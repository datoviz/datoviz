#ifndef DVZ_VISUALS_UTILS_HEADER
#define DVZ_VISUALS_UTILS_HEADER

#include "../include/datoviz/visuals.h"



/*************************************************************************************************/
/*  Visual utils                                                                                 */
/*************************************************************************************************/

static bool _source_is_texture(DvzSourceKind source_kind)
{
    return source_kind == DVZ_SOURCE_KIND_TEXTURE_1D || //
           source_kind == DVZ_SOURCE_KIND_TEXTURE_2D || //
           source_kind == DVZ_SOURCE_KIND_TEXTURE_3D;
}



static bool _source_is_buffer(DvzSourceKind source_kind)
{
    return source_kind == DVZ_SOURCE_KIND_UNIFORM || source_kind == DVZ_SOURCE_KIND_STORAGE ||
           source_kind == DVZ_SOURCE_KIND_VERTEX || source_kind == DVZ_SOURCE_KIND_INDEX;
}



static bool _source_needs_binding(DvzSourceKind source_kind)
{
    return source_kind == DVZ_SOURCE_KIND_UNIFORM || //
           source_kind == DVZ_SOURCE_KIND_STORAGE;
}



// Return the source array.
static DvzArray* _source_array(DvzSource* source)
{
    ASSERT(source != NULL);
    return &source->arr;
}



static DvzSourceKind _get_source_kind(DvzSourceType type)
{
    switch (type)
    {
    case DVZ_SOURCE_TYPE_MVP:
    case DVZ_SOURCE_TYPE_VIEWPORT:
    case DVZ_SOURCE_TYPE_PARAM:
        return DVZ_SOURCE_KIND_UNIFORM;
        break;

    case DVZ_SOURCE_TYPE_VERTEX:
        return DVZ_SOURCE_KIND_VERTEX;

    case DVZ_SOURCE_TYPE_INDEX:
        return DVZ_SOURCE_KIND_INDEX;

    case DVZ_SOURCE_TYPE_IMAGE:
    case DVZ_SOURCE_TYPE_COLOR_TEXTURE:
    case DVZ_SOURCE_TYPE_FONT_ATLAS:
        return DVZ_SOURCE_KIND_TEXTURE_2D;

    case DVZ_SOURCE_TYPE_VOLUME:
        return DVZ_SOURCE_KIND_TEXTURE_3D;

    default:
        log_error("source type %d not yet supported", type);
        return DVZ_SOURCE_KIND_NONE;
        break;
    }
}



static uint32_t _get_texture_ndims(DvzSourceKind source_kind)
{
    uint32_t ndims = 1;
    if (source_kind == DVZ_SOURCE_KIND_TEXTURE_2D)
        ndims = 2;
    if (source_kind == DVZ_SOURCE_KIND_TEXTURE_3D)
        ndims = 3;
    return ndims;
}



static VkFormat _get_texture_format(DvzVisual* visual, DvzSource* source)
{
    ASSERT(source != NULL);
    ASSERT(_source_is_texture(source->source_kind));
    DvzDataType dtype = DVZ_DTYPE_NONE;

    DvzContainerIterator iter = dvz_container_iterator(&visual->props);
    DvzProp* prop = NULL;
    while (iter.item != NULL)
    {
        prop = iter.item;
        if (prop->source == source)
        {
            // Check that there is only 1 prop associated to the texture source.
            if (dtype != DVZ_DTYPE_NONE)
                log_error("multiple texture props not (yet) supported");
            dtype = prop->dtype;
        }
        dvz_container_iter(&iter);
    }

    ASSERT(dtype != DVZ_DTYPE_NONE);
    VkFormat format = VK_FORMAT_UNDEFINED;
    switch (dtype)
    {

    // 8 bit
    case DVZ_DTYPE_CHAR:
        format = VK_FORMAT_R8_UNORM;
        break;

    case DVZ_DTYPE_CVEC3:
        format = VK_FORMAT_R8G8B8_UNORM;
        break;

    case DVZ_DTYPE_CVEC4:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        break;


    // 16 bit signed
    case DVZ_DTYPE_SHORT:
        format = VK_FORMAT_R16_SNORM;
        break;

    case DVZ_DTYPE_SVEC3:
        format = VK_FORMAT_R16G16B16_SNORM;
        break;

    case DVZ_DTYPE_SVEC4:
        format = VK_FORMAT_R16G16B16A16_SNORM;
        break;


    // 16 bit unsigned
    case DVZ_DTYPE_USHORT:
        format = VK_FORMAT_R16_UNORM;
        break;

    case DVZ_DTYPE_USVEC3:
        format = VK_FORMAT_R16G16B16_UNORM;
        break;

    case DVZ_DTYPE_USVEC4:
        format = VK_FORMAT_R16G16B16A16_UNORM;
        break;


    default:
        break;
    }
    if (format == VK_FORMAT_UNDEFINED)
        log_error("unsupported texture format for dtype %d", dtype);
    return format;
}



static DvzBindings* _get_bindings(DvzVisual* visual, DvzSource* source)
{
    ASSERT(source != NULL);
    if (source->pipeline == DVZ_PIPELINE_GRAPHICS)
        return dvz_container_get(&visual->bindings, source->pipeline_idx);
    else if (source->pipeline == DVZ_PIPELINE_COMPUTE)
        return dvz_container_get(&visual->bindings_comp, source->pipeline_idx);
    log_error("could not find binding for source %d", source->source_type);
    return NULL;
}



// Return the prop array, transformed if it exists, otherwise original.
static DvzArray* _prop_array(DvzProp* prop)
{
    ASSERT(prop != NULL);
    if (prop->arr_trans.item_count > 0)
        return &prop->arr_trans;
    else
        return &prop->arr_orig;
}



static uint32_t _source_size(DvzVisual* visual, DvzSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    DvzArray* arr = NULL;
    uint32_t item_count = 0;

    DvzContainerIterator iter = dvz_container_iterator(&visual->props);
    DvzProp* prop = NULL;
    while (iter.item != NULL)
    {
        prop = iter.item;
        if (prop->source == source)
        {
            arr = _prop_array(prop);
            ASSERT(arr != NULL);
            item_count = MAX(item_count, arr->item_count * MAX(1, prop->reps));
        }
        dvz_container_iter(&iter);
    }
    return item_count;
}



static void _set_source_bindings(DvzVisual* visual, DvzSource* source)
{
    // Set bindings except for VERTEX and INDEX sources.
    if (_source_needs_binding(source->source_kind))
    {
        DvzBindings* bindings = _get_bindings(visual, source);
        // NOTE: the graphics must be created before.
        ASSERT(bindings != NULL);
        dvz_bindings_buffer(bindings, source->slot_idx, source->u.br);

        // Share the source's buffer regions with other pipelines.
        DvzBindings* other = NULL;
        DvzSource* other_source = NULL;
        for (uint32_t i = 0; i < source->other_count; i++)
        {
            other_source = dvz_source_get(visual, source->source_type, source->other_idxs[i]);
            ASSERT(other_source != NULL);
            // Get the binding corresponding to the pipeline of the other source.
            other = dvz_container_get(&visual->bindings, other_source->pipeline_idx);
            ASSERT(other != NULL);
            dvz_bindings_buffer(other, source->slot_idx, source->u.br);
        }
    }
}



static void _create_source_buffer(DvzCanvas* canvas, DvzSource* source, VkDeviceSize size)
{
    DvzContext* ctx = canvas->gpu->context;
    DvzBufferType type = DVZ_BUFFER_TYPE_UNDEFINED;
    bool mappable = (source->flags & DVZ_SOURCE_FLAG_MAPPABLE) != 0;
    switch (source->source_kind)
    {
    case DVZ_SOURCE_KIND_VERTEX:
        type = DVZ_BUFFER_TYPE_VERTEX;
        break;
    case DVZ_SOURCE_KIND_INDEX:
        type = DVZ_BUFFER_TYPE_INDEX;
        break;
    case DVZ_SOURCE_KIND_UNIFORM:
        type = mappable ? DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE : DVZ_BUFFER_TYPE_UNIFORM;
        break;
    case DVZ_SOURCE_KIND_STORAGE:
        type = DVZ_BUFFER_TYPE_STORAGE;
        break;
    default:
        log_error("invalid source kind %d", source->source_kind);
        return;
        break;
    }
    uint32_t buf_count = source->source_type == mappable ? canvas->swapchain.img_count : 1;
    source->u.br = dvz_ctx_buffers(ctx, type, buf_count, size);
}



static void _source_buffer(DvzVisual* visual, DvzSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);
    DvzCanvas* canvas = visual->canvas;

    ASSERT(source->source_kind < DVZ_SOURCE_KIND_TEXTURE_1D);
    ASSERT(source->arr.item_size > 0);

    uint32_t count = source->arr.item_count;
    ASSERT(count > 0);

    // Allocate the buffer if it doesn't exist yet, or if it is not large enough.
    if (source->u.br.buffer == VK_NULL_HANDLE || source->u.br.size < count * source->arr.item_size)
    {
        VkDeviceSize size = dvz_next_pow2(count * source->arr.item_size);
        ASSERT(size >= count * source->arr.item_size);
        log_debug(
            "need to %sallocate new buffer region to fit %d elements (%d bytes)",
            source->u.br.size > 0 ? "re" : "", count, size);
        _create_source_buffer(canvas, source, size);
        // Set the pipeline bindings with the source buffer.
        _set_source_bindings(visual, source);
    }
    ASSERT(source->u.br.buffer != VK_NULL_HANDLE);
}



static void _source_texture(DvzVisual* visual, DvzSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);
    DvzContext* ctx = visual->canvas->gpu->context;

    ASSERT(_source_is_texture(source->source_kind));

    // Find the numbe of dimensions.
    uint32_t ndims = _get_texture_ndims(source->source_kind);
    uvec3 shape = {source->arr.shape[0], source->arr.shape[1], source->arr.shape[2]};
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    // Find the texture format.
    VkFormat format = _get_texture_format(visual, source);
    ASSERT(format != VK_FORMAT_UNDEFINED);

    // Allocate the texture if it doesn't exist yet, or if it is not large enough.
    DvzTexture* tex = source->u.tex;
    if (tex == NULL ||                   //
        tex->image->width < shape[0] ||  //
        tex->image->height < shape[1] || //
        tex->image->depth < shape[2])    //
    {
        if (tex == NULL)
        {
            log_debug(
                "need to create new texture with shape %dx%dx%d", //
                shape[0], shape[1], shape[2]);
            tex = source->u.tex = dvz_ctx_texture(ctx, ndims, shape, format);
        }
        else
        {
            log_debug(
                "need to resize texture to new shape %dx%dx%d", //
                shape[0], shape[1], shape[2]);
            dvz_texture_resize(source->u.tex, shape);
        }
        ASSERT(tex != NULL);

        // Set bindings.
        DvzBindings* bindings = _get_bindings(visual, source);
        dvz_bindings_texture(bindings, source->slot_idx, tex);
    }
    ASSERT(source->u.tex != NULL);
}



/*************************************************************************************************/
/*  Visual baking helpers                                                                        */
/*************************************************************************************************/

static void _prop_copy(DvzVisual* visual, DvzProp* prop)
{
    ASSERT(prop != NULL);

    DvzSource* source = prop->source;
    ASSERT(source != NULL);

    VkDeviceSize col_size = _get_dtype_size(prop->dtype);
    ASSERT(col_size > 0);

    DvzArray* arr = _prop_array(prop);
    if (arr->data == NULL)
    {
        log_debug("visual prop %d #%d not set", prop->prop_type, prop->prop_idx);
        return;
    }

    // Do not copy props that have no automatic copy set up.
    if (prop->copy_type == DVZ_ARRAY_COPY_NONE)
        return;

    ASSERT(arr->data != NULL);
    ASSERT(source->arr.data != NULL);
    ASSERT(arr->item_count <= source->arr.item_count);

    dvz_array_column(
        &source->arr, prop->offset, col_size, 0, source->arr.item_count, //
        arr->item_count, arr->data,                                      //
        prop->arr_orig.dtype, prop->target_dtype,                        // optional cast
        prop->copy_type, prop->reps);
}



static void _source_alloc(DvzVisual* visual, DvzSource* source, uint32_t count)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    // Resize the source source.
    log_trace(
        "alloc %d elements for source %d #%d", count, source->source_type, source->source_idx);
    DvzArray* arr = &source->arr;
    ASSERT(dvz_obj_is_created(&arr->obj));
    dvz_array_resize(arr, count);
}



static void _source_fill(DvzVisual* visual, DvzSource* source)
{
    ASSERT(visual != NULL);
    ASSERT(source != NULL);

    // Copy all associated props to the source array.
    DvzProp* prop = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&visual->props);
    while (iter.item != NULL)
    {
        prop = iter.item;
        if (prop->source == source)
            _prop_copy(visual, prop);
        dvz_container_iter(&iter);
    }
}



// Get the first source of a given type for the given pipeline, or none.
static DvzSource*
_get_pipeline_source(DvzVisual* visual, DvzSourceType source_type, uint32_t pipeline_idx)
{
    ASSERT(visual != NULL);
    DvzSource* source = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&visual->sources);
    while (iter.item != NULL)
    {
        source = iter.item;
        if (source->source_type == source_type && source->pipeline_idx == pipeline_idx)
            return source;
        dvz_container_iter(&iter);
    }
    return NULL;
}



static void _bake_source(DvzVisual* visual, DvzSource* source)
{
    ASSERT(visual != NULL);
    if (source == NULL)
        return;

    // The baking function doesn't run if the VERTEX source is handled by the user.
    if (source->origin != DVZ_SOURCE_ORIGIN_LIB)
        return;
    if (source->obj.request != DVZ_VISUAL_REQUEST_UPLOAD)
    {
        log_trace(
            "skip bake source for source %d that doesn't need updating", source->source_kind);
        return;
    }

    // The number of vertices corresponds to the largest prop.
    uint32_t count = _source_size(visual, source);
    if (count == 0)
    {
        log_debug("empty source %d", source->source_type);
        return;
    }

    log_debug("baking source %d", source->source_kind);

    // Allocate the source array.
    _source_alloc(visual, source, count);

    // Copy all corresponding props to the array.
    _source_fill(visual, source);
}



static void _bake_uniforms(DvzVisual* visual)
{
    DvzContainerIterator iter = dvz_container_iterator(&visual->sources);
    DvzSource* source = NULL;
    // UNIFORM sources.

    while (iter.item != NULL)
    {
        source = iter.item;
        if (source->obj.request != DVZ_VISUAL_REQUEST_UPLOAD)
        {
            log_trace("skip bake source for uniform source that doesn't need updating");
            dvz_container_iter(&iter);
            continue;
        }

        // Allocate the UNIFORM sources, using the number of items in the props, and fill them
        // with the props.
        if (source->source_kind == DVZ_SOURCE_KIND_UNIFORM &&
            source->origin == DVZ_SOURCE_ORIGIN_LIB)
        {
            uint32_t count = _source_size(visual, source);
            ASSERT(count > 0);
            _source_alloc(visual, source, count);
            _source_fill(visual, source);
        }
        dvz_container_iter(&iter);
    }
}



/*************************************************************************************************/
/*  Visual default callbacks                                                                     */
/*************************************************************************************************/

static void _default_visual_bake(DvzVisual* visual, DvzVisualDataEvent ev)
{
    ASSERT(visual != NULL);

    // VERTEX source.
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    _bake_source(visual, source);

    // INDEX source.
    source = dvz_source_get(visual, DVZ_SOURCE_TYPE_INDEX, 0);
    _bake_source(visual, source);
}



static void _default_visual_fill(DvzVisual* visual, DvzVisualFillEvent ev)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    DvzCommands* cmds = ev.cmds;
    uint32_t idx = ev.cmd_idx;
    VkViewport viewport = ev.viewport.viewport;

    ASSERT(viewport.width > 0);
    ASSERT(viewport.height > 0);

    // Draw all valid graphics pipelines.
    DvzBindings* bindings = NULL;
    for (uint32_t pipeline_idx = 0; pipeline_idx < visual->graphics_count; pipeline_idx++)
    {
        ASSERT(dvz_obj_is_created(&visual->graphics[pipeline_idx]->obj));

        bindings = dvz_container_get(&visual->bindings, pipeline_idx);
        ASSERT(dvz_obj_is_created(&bindings->obj));

        DvzSource* vertex_source =
            _get_pipeline_source(visual, DVZ_SOURCE_TYPE_VERTEX, pipeline_idx);
        ASSERT(vertex_source != NULL);
        ASSERT(vertex_source->pipeline_idx == pipeline_idx);

        uint32_t vertex_count = vertex_source->arr.item_count;
        if (vertex_count == 0)
        {
            log_warn("skip this graphics pipeline as the vertex buffer is empty");
            continue;
        }
        ASSERT(vertex_count > 0);

        // Bind the vertex buffer.
        DvzBufferRegions* vertex_buf = &vertex_source->u.br;
        ASSERT(vertex_buf != NULL);
        dvz_cmd_bind_vertex_buffer(cmds, idx, *vertex_buf, 0);

        // Index buffer?
        DvzSource* index_source =
            _get_pipeline_source(visual, DVZ_SOURCE_TYPE_INDEX, pipeline_idx);
        uint32_t index_count = 0;
        DvzBufferRegions* index_buf = NULL;
        if (index_source != NULL)
        {
            index_count = index_source->arr.item_count;
            if (index_count > 0)
            {
                index_buf = &index_source->u.br;
                ASSERT(index_buf != NULL);
                dvz_cmd_bind_index_buffer(cmds, idx, *index_buf, 0);
            }
        }

        // Draw command.
        dvz_cmd_bind_graphics(cmds, idx, visual->graphics[pipeline_idx], bindings, 0);

        if (index_count == 0)
        {
            log_debug("draw %d vertices", vertex_count);
            // Make sure the bound vertex buffer is large enough.
            ASSERT(vertex_buf->size >= vertex_count * vertex_source->arr.item_size);
            dvz_cmd_draw(cmds, idx, 0, vertex_count);
        }
        else
        {
            log_debug("draw %d indices", index_count);
            // Make sure the bound index buffer is large enough.
            ASSERT(index_buf->size >= index_count * sizeof(DvzIndex));
            dvz_cmd_draw_indexed(cmds, idx, 0, 0, index_count);
        }
    }
}



#endif
