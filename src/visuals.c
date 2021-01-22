#include "../include/visky/visuals.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"
#include "visuals_utils.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);

    VklVisual visual = {0};
    visual.canvas = canvas;
    visual.props =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklProp), VKL_OBJECT_TYPE_PROP);
    visual.sources =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklSource), VKL_OBJECT_TYPE_SOURCE);
    visual.bindings =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklBindings), VKL_OBJECT_TYPE_BINDINGS);
    visual.bindings_comp =
        vkl_container(VKL_CONTAINER_DEFAULT_COUNT, sizeof(VklBindings), VKL_OBJECT_TYPE_BINDINGS);

    // Default callbacks.
    visual.callback_fill = _default_visual_fill;
    visual.callback_bake = _default_visual_bake;

    obj_created(&visual.obj);
    return visual;
}



void vkl_visual_destroy(VklVisual* visual)
{
    ASSERT(visual != NULL);

    // Free the props.
    VklProp* prop = vkl_container_iter_init(&visual->props);
    while (prop != NULL)
    {
        vkl_array_destroy(&prop->arr_orig);
        vkl_array_destroy(&prop->arr_trans);
        if (prop->default_value != NULL)
        {
            FREE(prop->default_value)
        }
        obj_destroyed(&prop->obj);
        prop = vkl_container_iter(&visual->props);
    }
    vkl_container_destroy(&visual->props);

    // Free the data sources.
    VklSource* source = vkl_container_iter_init(&visual->sources);
    while (source != NULL)
    {
        vkl_array_destroy(&source->arr);
        obj_destroyed(&source->obj);
        source = vkl_container_iter(&visual->sources);
    }
    vkl_container_destroy(&visual->sources);

    CONTAINER_DESTROY_ITEMS(VklBindings, visual->bindings, vkl_bindings_destroy)
    CONTAINER_DESTROY_ITEMS(VklBindings, visual->bindings_comp, vkl_bindings_destroy)

    obj_destroyed(&visual->obj);
}



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

static void _source_set_changed(VklSource* source, bool value)
{
    ASSERT(source != NULL);
    int req = value ? VKL_VISUAL_REQUEST_UPLOAD : VKL_VISUAL_REQUEST_NOT_SET;
    source->obj.request = req;
    ASSERT(source->visual != NULL);
    source->visual->obj.request = req;
}



static void _source_set(VklSource* source)
{
    ASSERT(source != NULL);
    source->obj.request = VKL_VISUAL_REQUEST_SET;
    ASSERT(source->visual != NULL);
    source->visual->obj.request = VKL_VISUAL_REQUEST_SET;
}



static bool _source_has_changed(VklSource* source)
{
    ASSERT(source != NULL);
    return source->obj.request == VKL_VISUAL_REQUEST_UPLOAD;
}



static bool _source_is_set(VklSource* source)
{
    ASSERT(source != NULL);
    return source->obj.request != VKL_VISUAL_REQUEST_NOT_SET;
}



void vkl_visual_source(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, //
    VklPipelineType pipeline, uint32_t pipeline_idx,                   //
    uint32_t slot_idx, VkDeviceSize item_size, int flags)
{
    ASSERT(visual != NULL);
    ASSERT(vkl_source_get(visual, source_type, source_idx) == NULL);

    VklSource* source = vkl_container_alloc(&visual->sources);
    obj_init(&source->obj);
    source->visual = visual;
    source->source_type = source_type;
    source->source_kind = _get_source_kind(source_type);
    source->source_idx = source_idx;
    source->pipeline = pipeline;
    source->pipeline_idx = pipeline_idx;
    source->slot_idx = slot_idx;
    source->flags = flags;

    if (source->source_kind < VKL_SOURCE_KIND_TEXTURE_1D)
        source->arr = vkl_array_struct(0, item_size);
    else
    {
        // Textures.
        uint32_t ndims = _get_texture_ndims(source->source_kind);
        source->arr = vkl_array_3D(ndims, 0, 0, 0, item_size);
    }

    // source origin (GPU object) not set yet
    source->origin = VKL_SOURCE_ORIGIN_NONE;

    // NOTE: exception for INDEX source, most frequently automatically handled by the library
    if (source->source_kind == VKL_SOURCE_KIND_INDEX)
    {
        source->origin = VKL_SOURCE_ORIGIN_LIB;
        _source_set_changed(source, true);
        // source->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    }
}



void vkl_visual_source_share(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, uint32_t other_idx)
{
    ASSERT(visual != NULL);
    VklSource* source = vkl_source_get(visual, source_type, source_idx);
    ASSERT(source != NULL);
    source->other_idxs[source->other_count++] = other_idx;
}



void vkl_visual_prop(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, VklDataType dtype,
    VklSourceType source_type, uint32_t source_idx)
{
    ASSERT(visual != NULL);

    VklProp* prop = vkl_container_alloc(&visual->props);
    obj_init(&prop->obj);

    prop->prop_type = prop_type;
    prop->prop_idx = prop_idx;
    prop->dtype = dtype;
    prop->source = vkl_source_get(visual, source_type, source_idx);
    if (prop->source == NULL)
    {
        log_error("source of type %d #%d not found", source_type, source_idx);
    }
    ASSERT(prop->source != NULL);

    // NOTE: we do not use prop arrays for texture sources at the moment
    if (prop->source->source_kind < VKL_SOURCE_KIND_TEXTURE_1D)
        prop->arr_orig = vkl_array(0, prop->dtype);
}



void vkl_visual_prop_default(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, void* default_value)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);
    ASSERT(prop->arr_orig.item_size > 0);

    // Makes a copy of the default value to the prop.
    if (prop->default_value == NULL)
        prop->default_value = calloc(1, prop->arr_orig.item_size);
    memcpy(prop->default_value, default_value, prop->arr_orig.item_size);

    // Declaring a default value to the prop is equivalent to setting a single value to the prop.
    vkl_visual_data(visual, prop_type, prop_idx, 1, default_value);
}



void vkl_visual_prop_copy(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, //
    uint32_t field_idx, VkDeviceSize offset,                     //
    VklArrayCopyType copy_type, uint32_t reps)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);

    prop->field_idx = field_idx;
    prop->offset = offset;
    prop->copy_type = copy_type;
    prop->reps = reps;
    prop->target_dtype = VKL_DTYPE_NONE;
}



void vkl_visual_prop_cast(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx,       //
    uint32_t field_idx, VkDeviceSize offset, VklDataType target_dtype, //
    VklArrayCopyType copy_type, uint32_t reps)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);

    prop->field_idx = field_idx;
    prop->offset = offset;
    prop->copy_type = copy_type;
    prop->reps = reps;
    prop->target_dtype = target_dtype;
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

    VklBindings* bindings = vkl_container_alloc(&visual->bindings);
    ASSERT(visual->bindings.count == visual->graphics_count + 1);
    *bindings = vkl_bindings(&graphics->slots, visual->canvas->swapchain.img_count);
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
    visual->computes[visual->compute_count] = compute;

    VklBindings* bindings = vkl_container_alloc(&visual->bindings);
    ASSERT(visual->bindings.count == visual->compute_count + 1);
    *bindings = vkl_bindings(&compute->slots, visual->canvas->swapchain.img_count);
    visual->compute_count++;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

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



void vkl_visual_data(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data)
{
    ASSERT(visual != NULL);
    vkl_visual_data_partial(visual, prop_type, prop_idx, 0, count, count, data);
}



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
{
    ASSERT(visual != NULL);
    uint32_t count = first_item + item_count;
    ASSERT(count > 0);
    ASSERT(data_item_count > 0);

    // Get the associated prop.
    VklProp* prop = vkl_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);

    // Make sure the array has the right size.
    vkl_array_resize(&prop->arr_orig, count);

    // Copy the specified array to the prop array.
    vkl_array_data(&prop->arr_orig, first_item, item_count, data_item_count, data);
    // Mark the prop has set, will need to be transposed only once.
    prop->obj.request = 0;

    // Get the associated source.
    VklSource* source = prop->source;
    ASSERT(source != NULL);
    if (source != NULL)
    {
        log_trace("source type %d #%d handled by lib", source->source_type, source->source_idx);
        source->origin = VKL_SOURCE_ORIGIN_LIB;
        // source->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
        // visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
        _source_set_changed(source, true);
    }
}



void vkl_visual_data_append(
    VklVisual* visual, VklPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);
    uint32_t first_item = prop->arr_orig.item_count;
    vkl_visual_data_partial(visual, prop_type, prop_idx, first_item, count, count, data);
}



static VklSource*
_assert_source_exists(VklVisual* visual, VklSourceType source_type, uint32_t source_idx)
{
    VklSource* source = vkl_source_get(visual, source_type, source_idx);

    // // Check if the requested source is not a shared source.
    // if (source == NULL)
    // {
    //     VklSource* src = vkl_container_iter_init(&visual->sources);
    //     while (src != NULL)
    //     {
    //         for (uint32_t j = 0; j < src->other_count; j++)
    //         {
    //             if (src->other_idxs[j] == source_idx)
    //             {
    //                 source = src;
    //                 break;
    //             }
    //         }
    //         src = vkl_container_iter(&visual->sources);
    //     }
    // }

    if (source == NULL)
    {
        log_error("source of type %d #%d not found", source_type, source_idx);
        return NULL;
    }
    ASSERT(source != NULL);
    return source;
}

void vkl_visual_data_source(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
{
    ASSERT(visual != NULL);
    uint32_t count = first_item + item_count;
    ASSERT(count > 0);
    ASSERT(data_item_count > 0);

    // Get the associated source.
    VklSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);
    ASSERT(source->source_type == source_type);

    // Make sure the array has the right size.
    vkl_array_resize(&source->arr, count);

    // Copy the specified array to the prop array.
    vkl_array_data(&source->arr, first_item, item_count, data_item_count, data);

    source->origin = VKL_SOURCE_ORIGIN_NOBAKE;
    // source->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    // visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    _source_set_changed(source, true);
}



// Means that no data updates will be done by visky, it is up to the user to update the bound
// buffer
void vkl_visual_buffer(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklBufferRegions br)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);

    VkDeviceSize size = br.size;
    ASSERT(size > 0);
    ASSERT(br.buffer != VK_NULL_HANDLE);

    source->u.br = br;
    source->origin = VKL_SOURCE_ORIGIN_USER;
    _source_set_changed(source, true);
    // source->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    // visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;

    // Set the pipeline bindings with the source buffer.
    _set_source_bindings(visual, source);
}



void vkl_visual_texture(
    VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklTexture* texture)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    VklSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);
    ASSERT(texture != NULL);

    source->u.tex = texture;
    source->origin = VKL_SOURCE_ORIGIN_USER;
    _source_set_changed(source, true);
    // source->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;
    // visual->obj.status = VKL_OBJECT_STATUS_NEED_UPDATE;

    VklBindings* bindings = _get_bindings(visual, source);
    ASSERT(bindings != NULL);
    ASSERT(texture->image != NULL);
    ASSERT(texture->sampler != NULL);
    vkl_bindings_texture(bindings, source->slot_idx, texture);
}



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

void vkl_visual_callback_bake(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->callback_bake = callback;
}



void vkl_visual_fill_callback(VklVisual* visual, VklVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    visual->callback_fill = callback;
}



void vkl_visual_fill_event(
    VklVisual* visual, VkClearColorValue clear_color, VklCommands* cmds, uint32_t cmd_idx,
    VklViewport viewport, void* user_data)
{
    ASSERT(visual != NULL);
    ASSERT(visual->callback_fill != NULL);

    VklVisualFillEvent ev = {0};
    ev.clear_color = clear_color;
    ev.cmds = cmds;
    ev.cmd_idx = cmd_idx;
    ev.viewport = viewport;
    ev.user_data = user_data;

    visual->callback_fill(visual, ev);
}



void vkl_visual_fill_begin(VklCanvas* canvas, VklCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
}



void vkl_visual_fill_end(VklCanvas* canvas, VklCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

VklSource* vkl_source_get(VklVisual* visual, VklSourceType source_type, uint32_t source_idx)
{
    ASSERT(visual != NULL);
    VklSource* source = vkl_container_iter_init(&visual->sources);
    VklSource* out = NULL;
    while (source != NULL)
    {
        if (source->source_type == source_type && source->source_idx == source_idx)
        {
            // Check there is only 1 source with a given type and idx.
            ASSERT(out == NULL);
            out = source;
        }
        source = vkl_container_iter(&visual->sources);
    }
    return out;
}



VklProp* vkl_prop_get(VklVisual* visual, VklPropType prop_type, uint32_t prop_idx)
{
    ASSERT(visual != NULL);
    VklProp* prop = vkl_container_iter_init(&visual->props);
    VklProp* out = NULL;
    while (prop != NULL)
    {
        if (prop->prop_type == prop_type && prop->prop_idx == prop_idx)
        {
            ASSERT(out == NULL);
            out = prop;
        }
        prop = vkl_container_iter(&visual->props);
    }
    if (out == NULL)
        log_trace("prop with type %d #%d not found", prop_type, prop_idx);
    // ASSERT(out != NULL);
    return out;
}



uint32_t vkl_prop_size(VklProp* prop)
{
    VklArray* arr = _prop_array(prop);
    return arr->item_count;
}



void* vkl_prop_item(VklProp* prop, uint32_t prop_idx)
{
    ASSERT(prop != NULL);
    VklArray* arr = _prop_array(prop);
    void* res = prop->default_value;
    if (prop_idx < arr->item_count)
        res = vkl_array_item(arr, prop_idx);
    if (res == NULL)
    {
        log_debug("no default value for prop %d #%d", prop->prop_type, prop->prop_idx);
    }
    return res;
}



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

void vkl_visual_update(
    VklVisual* visual, VklViewport viewport, VklDataCoords coords, const void* user_data)
{
    ASSERT(visual != NULL);
    log_trace("visual update");

    VklVisualDataEvent ev = {0};
    ev.viewport = viewport;
    ev.coords = coords;
    ev.user_data = user_data;

    // if (visual->callback_transform != NULL)
    // {
    //     log_debug("visual transform callback");
    //     // This callback updates some props data_trans
    //     visual->callback_transform(visual, ev);
    // }

    if (visual->callback_bake != NULL)
    {
        log_trace("visual bake callback");

        // This callback does the following:
        // 1. Determine vertex count and index count
        // 2. Resize the VERTEX and INDEX array sources accordingly.
        // 3. Possibly resize other sources.
        // 4. Take the props and fill the array sources.
        visual->callback_bake(visual, ev);
    }
    // NOTE: we bake the UNIFORM sources here.
    _bake_uniforms(visual);


    // Here, we assume that all sources are correctly allocated, which includes VERTEX and INDEX
    // arrays, and that they have their data ready for upload.
    // Upload the buffers and textures
    VklArray* arr = NULL;
    VklBufferRegions* br = NULL;
    VklCanvas* canvas = visual->canvas;
    VklContext* ctx = canvas->gpu->context;
    VklTexture* texture = NULL;
    bool to_upload = false;

    VklSource* source = vkl_container_iter_init(&visual->sources);
    VklBindings* bindings = NULL;
    while (source != NULL)
    {
        // No source set: using default source or skipping.
        if (source->origin == VKL_SOURCE_ORIGIN_NONE)
        {
            if (_source_is_texture(source->source_kind))
            {
                log_warn(
                    "source type %d #%d is not set, using default texture (colormap array)",
                    source->source_type, source->source_idx);
                ASSERT(ctx->color_texture.texture != NULL);
                ASSERT(ctx->color_texture.texture->image != NULL);
                ASSERT(ctx->color_texture.texture->image->images[0] != VK_NULL_HANDLE);
                ASSERT(ctx->color_texture.texture->sampler != NULL);
                ASSERT(is_obj_created(&ctx->color_texture.texture->obj));

                vkl_visual_texture(
                    visual, source->source_type, source->source_idx, ctx->color_texture.texture);
            }
            else if (_source_is_buffer(source->source_kind))
            {
                log_debug(
                    "source type %d #%d is not set", source->source_type, source->source_idx);
                if (source->source_type == VKL_SOURCE_TYPE_VERTEX)
                {
                    log_warn("skipping visual data upload as VERTEX source is not set");
                    visual->obj.status = VKL_OBJECT_STATUS_INVALID;
                    return;
                }
            }
            else
            {
                log_error(
                    "unknown source type %d #%d, aborting", //
                    source->source_type, source->source_idx);
                ASSERT(0);

                // NOTE: The following is useless??

                // // NOTE: mark the binding corresponding to the source's pipeline as invalid.
                // bindings = vkl_container_get(&visual->bindings, source->pipeline_idx);
                // ASSERT(bindings != NULL);
                // bindings->obj.status = VKL_OBJECT_STATUS_INVALID;
                // VklSource* other = NULL;
                // for (uint32_t j = 0; j < source->other_count; j++)
                // {
                //     // Get other source.
                //     other = vkl_bake_source(visual, source->source_type, source->other_idxs[j]);
                //     ASSERT(other != NULL);
                //     // Get bindings corresponding to graphics pipeline of that other source/
                //     bindings = vkl_container_get(&visual->bindings, other->pipeline_idx);
                //     ASSERT(bindings != NULL);
                //     bindings->obj.status = VKL_OBJECT_STATUS_INVALID;
                // }
            }

            source = vkl_container_iter(&visual->sources);
            continue;
        }

        // Upload only for sources manages by visky.
        to_upload =
            source->origin == VKL_SOURCE_ORIGIN_LIB || source->origin == VKL_SOURCE_ORIGIN_NOBAKE;
        if (!to_upload)
        {
            log_trace(
                "skip data upload for source type %d #%d, origin %d, that is handled by user", //
                source->source_type, source->source_idx, source->origin);
            source = vkl_container_iter(&visual->sources);
            continue;
        }
        if (!_source_is_set(source))
        {
            log_error("data source %d #%d was never set", source->source_type, source->source_idx);
            source = vkl_container_iter(&visual->sources);
            continue;
        }
        else if (!_source_has_changed(source))
        {
            log_trace(
                "skip data upload for source %d that doesn't need to be updated",
                source->source_type);
            source = vkl_container_iter(&visual->sources);
            continue;
        }

        arr = &source->arr;

        // Update buffer sources.
        if (_source_is_buffer(source->source_kind))
        {
            if (arr->item_count == 0)
            {
                log_debug("empty source %d", source->source_type);
                source = vkl_container_iter(&visual->sources);
                continue;
            }

            br = &source->u.br;

            // NOTE: the source array MUST have been allocated by the baking function,
            // or directly by the user via vkl_visual_data_source() (NOBAKE origin)
            ASSERT(arr->item_count > 0);
            ASSERT(arr->item_size > 0);

            // Make sure the GPU buffer exists and is allocated with the right size.
            _source_buffer(visual, source);

            ASSERT(br->size > 0);
            VkDeviceSize size = arr->item_count * arr->item_size;
            ASSERT(br->size >= size);
            ASSERT(arr->buffer_size >= size);

            ASSERT(br->buffer != VK_NULL_HANDLE);

            log_trace(
                "upload buffer (%d items, buffer size %d bytes) for automatically-handled source "
                "%d #%d", //
                arr->item_count, br->size, source->source_type, source->source_idx);

            vkl_upload_buffers(canvas, *br, 0, size, arr->data);
            _source_set(source);
            // source->obj.status = VKL_OBJECT_STATUS_CREATED;
            // visual->obj.status = VKL_OBJECT_STATUS_CREATED;
        }

        // Update textures.
        else if (_source_is_texture(source->source_kind))
        {
            // Make sure the GPU texture exists and is allocated with the right shape.
            _source_texture(visual, source);
            texture = source->u.tex;

            ASSERT(texture != NULL);
            ASSERT(is_obj_created(&texture->obj));
            // NOTE: the source array MUST have been allocated by the baking function
            ASSERT(arr->item_count > 0);
            ASSERT(arr->item_size > 0);
            ASSERT(arr->ndims >= 1);
            ASSERT(arr->shape[0] > 0);
            ASSERT(arr->shape[1] > 0);
            ASSERT(arr->shape[2] > 0);

            log_debug(
                "upload texture for automatically-handled source %d #%d, shape %dx%dx%d", //
                source->source_type, source->source_idx,                                  //
                arr->shape[0], arr->shape[1], arr->shape[2]);
            vkl_upload_texture(
                canvas, texture, VKL_ZERO_OFFSET, VKL_ZERO_OFFSET,
                arr->item_count * arr->item_size, arr->data);
            _source_set(source);
        }

        source = vkl_container_iter(&visual->sources);
    }

    // Update the bindings that need to be updated.
    for (uint32_t i = 0; i < visual->graphics_count; i++)
    {
        bindings = vkl_container_get(&visual->bindings, i);
        ASSERT(bindings != NULL);
        if (bindings->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            vkl_bindings_update(bindings);
    }
    for (uint32_t i = 0; i < visual->compute_count; i++)
    {
        bindings = vkl_container_get(&visual->bindings_comp, i);
        ASSERT(bindings != NULL);
        if (bindings->obj.status == VKL_OBJECT_STATUS_NEED_UPDATE)
            vkl_bindings_update(bindings);
    }
}
