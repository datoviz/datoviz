#include "../include/datoviz/visuals.h"
#include "../include/datoviz/canvas.h"
#include "../include/datoviz/graphics.h"
#include "visuals_utils.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

DvzVisual dvz_visual(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);

    DvzVisual visual = {0};
    visual.canvas = canvas;
    visual.props =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzProp), DVZ_OBJECT_TYPE_PROP);
    visual.sources =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzSource), DVZ_OBJECT_TYPE_SOURCE);
    visual.bindings =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBindings), DVZ_OBJECT_TYPE_BINDINGS);
    visual.bindings_comp =
        dvz_container(DVZ_CONTAINER_DEFAULT_COUNT, sizeof(DvzBindings), DVZ_OBJECT_TYPE_BINDINGS);

    // Default callbacks.
    visual.callback_fill = _default_visual_fill;
    visual.callback_bake = _default_visual_bake;

    dvz_obj_created(&visual.obj);
    return visual;
}



static void dvz_prop_destroy(DvzProp* prop)
{
    ASSERT(prop != NULL);
    log_trace("destroy prop");
    dvz_array_destroy(&prop->arr_orig);
    dvz_array_destroy(&prop->arr_trans);
    dvz_array_destroy(&prop->arr_staging);
    if (prop->default_value != NULL)
        FREE(prop->default_value)
    dvz_obj_destroyed(&prop->obj);
}

static void dvz_source_destroy(DvzSource* source)
{
    ASSERT(source != NULL);
    log_trace("destroy source");
    dvz_array_destroy(&source->arr);
    dvz_obj_destroyed(&source->obj);
}

void dvz_visual_destroy(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    log_trace("destroy visual");

    CONTAINER_DESTROY_ITEMS(DvzProp, visual->props, dvz_prop_destroy)
    dvz_container_destroy(&visual->props);

    CONTAINER_DESTROY_ITEMS(DvzSource, visual->sources, dvz_source_destroy)
    dvz_container_destroy(&visual->sources);

    CONTAINER_DESTROY_ITEMS(DvzBindings, visual->bindings, dvz_bindings_destroy)
    dvz_container_destroy(&visual->bindings);

    CONTAINER_DESTROY_ITEMS(DvzBindings, visual->bindings_comp, dvz_bindings_destroy)
    dvz_container_destroy(&visual->bindings_comp);

    dvz_obj_destroyed(&visual->obj);
}



/*************************************************************************************************/
/*  Visual creation                                                                              */
/*************************************************************************************************/

void dvz_visual_source(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, //
    DvzPipelineType pipeline, uint32_t pipeline_idx,                   //
    uint32_t slot_idx, VkDeviceSize item_size, int flags)
{
    ASSERT(visual != NULL);
    ASSERT(dvz_source_get(visual, source_type, source_idx) == NULL);

    DvzSource* source = dvz_container_alloc(&visual->sources);
    dvz_obj_init(&source->obj);
    source->visual = visual;
    source->source_type = source_type;
    source->source_kind = _get_source_kind(source_type);
    source->source_idx = source_idx;
    source->pipeline = pipeline;
    source->pipeline_idx = pipeline_idx;
    source->slot_idx = slot_idx;
    source->flags = flags;

    if (source->source_kind < DVZ_SOURCE_KIND_TEXTURE_1D)
        source->arr = dvz_array_struct(0, item_size);
    else
    {
        // Textures.
        uint32_t ndims = _get_texture_ndims(source->source_kind);
        source->arr = dvz_array_3D(ndims, 0, 0, 0, item_size);
    }

    // source origin (GPU object) not set yet
    source->origin = DVZ_SOURCE_ORIGIN_NONE;

    // NOTE: exception for INDEX source, most frequently automatically handled by the library
    if (source->source_kind == DVZ_SOURCE_KIND_INDEX)
    {
        source->origin = DVZ_SOURCE_ORIGIN_LIB;
        _source_set_changed(source, true);
        // source->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
    }
}



void dvz_visual_source_share(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, uint32_t other_idx)
{
    ASSERT(visual != NULL);
    DvzSource* source = dvz_source_get(visual, source_type, source_idx);
    ASSERT(source != NULL);
    source->other_idxs[source->other_count++] = other_idx;
}



DvzProp* dvz_visual_prop(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, DvzDataType dtype,
    DvzSourceType source_type, uint32_t source_idx)
{
    ASSERT(visual != NULL);

    DvzProp* prop = dvz_container_alloc(&visual->props);
    dvz_obj_init(&prop->obj);

    prop->prop_type = prop_type;
    prop->prop_idx = prop_idx;
    prop->dtype = dtype;
    prop->item_size = _get_dtype_size(dtype);
    prop->dpi_scaling = 1;
    prop->source = dvz_source_get(visual, source_type, source_idx);
    if (prop->source == NULL && source_type != DVZ_SOURCE_TYPE_NONE)
    {
        log_error("source of type %d #%d not found", source_type, source_idx);
    }

    // NOTE: we do not use prop arrays for texture sources at the moment
    if ((prop->source == NULL || prop->source->source_kind < DVZ_SOURCE_KIND_TEXTURE_1D) &&
        prop->dtype != DVZ_DTYPE_CUSTOM)
        prop->arr_orig = dvz_array(0, prop->dtype);

    return prop;
}



void dvz_visual_prop_default(DvzProp* prop, void* default_value)
{
    ASSERT(prop != NULL);
    ASSERT(prop->arr_orig.item_size > 0);

    // Makes a copy of the default value to the prop.
    if (prop->default_value == NULL)
        prop->default_value = calloc(1, prop->arr_orig.item_size);
    memcpy(prop->default_value, default_value, prop->arr_orig.item_size);

    // Declaring a default value to the prop is equivalent to setting a single value to the prop.
    dvz_visual_data(prop->source->visual, prop->prop_type, prop->prop_idx, 1, default_value);
}



void dvz_visual_prop_copy(
    DvzProp* prop, uint32_t field_idx, VkDeviceSize offset, //
    DvzArrayCopyType copy_type, uint32_t reps)
{
    ASSERT(prop != NULL);

    prop->field_idx = field_idx;
    prop->offset = offset;
    prop->copy_type = copy_type;
    prop->reps = reps;
    prop->target_dtype = DVZ_DTYPE_NONE;
}



void dvz_visual_prop_size(DvzProp* prop, VkDeviceSize item_size)
{
    ASSERT(prop != NULL);
    ASSERT(item_size > 0);

    prop->item_size = item_size;

    if (prop->source == NULL || prop->source->source_kind < DVZ_SOURCE_KIND_TEXTURE_1D)
        prop->arr_orig = dvz_array_struct(0, prop->item_size);
}



void dvz_visual_prop_cast(
    DvzProp* prop, uint32_t field_idx, VkDeviceSize offset, DvzDataType target_dtype, //
    DvzArrayCopyType copy_type, uint32_t reps)
{
    ASSERT(prop != NULL);

    prop->field_idx = field_idx;
    prop->offset = offset;
    prop->copy_type = copy_type;
    prop->reps = reps;
    prop->target_dtype = target_dtype;
}



void dvz_visual_prop_dpi(DvzProp* prop, float dpi_scaling)
{
    ASSERT(prop != NULL);
    ASSERT(dpi_scaling > 0);
    prop->dpi_scaling = dpi_scaling;
}



void dvz_visual_graphics(DvzVisual* visual, DvzGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(dvz_obj_is_created(&graphics->obj));
    if (visual->graphics_count >= DVZ_MAX_GRAPHICS_PER_VISUAL)
    {
        log_error("maximum number of graphics per visual reached");
        return;
    }
    visual->graphics[visual->graphics_count] = graphics;

    DvzBindings* bindings = dvz_container_alloc(&visual->bindings);
    ASSERT(visual->bindings.count == visual->graphics_count + 1);
    *bindings = dvz_bindings(&graphics->slots, visual->canvas->swapchain.img_count);
    visual->graphics_count++;
}



void dvz_visual_compute(DvzVisual* visual, DvzCompute* compute)
{
    ASSERT(visual != NULL);
    ASSERT(compute != NULL);
    ASSERT(dvz_obj_is_created(&compute->obj));
    if (visual->compute_count >= DVZ_MAX_COMPUTES_PER_VISUAL)
    {
        log_error("maximum number of computes per visual reached");
        return;
    }
    visual->computes[visual->compute_count] = compute;

    DvzBindings* bindings = dvz_container_alloc(&visual->bindings);
    ASSERT(visual->bindings.count == visual->compute_count + 1);
    *bindings = dvz_bindings(&compute->slots, visual->canvas->swapchain.img_count);
    visual->compute_count++;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void dvz_visual_group(DvzVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
    if (group_idx >= DVZ_MAX_VISUAL_GROUPS)
    {
        log_error("maximum number of groups reached");
        return;
    }
    visual->group_count = MAX(visual->group_count, group_idx + 1);
    visual->group_sizes[group_idx] = size;
}



static void _visual_data(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data,
    bool do_resize)
{
    ASSERT(visual != NULL);
    uint32_t count = first_item + item_count;
    ASSERT(count > 0);
    ASSERT(data_item_count > 0);

    // Get the associated prop.
    DvzProp* prop = dvz_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);

    // Get the associated source.
    DvzSource* source = prop->source;
    if (source != NULL)
        log_trace(
            "found source type %d #%d for prop type %d #%d", //
            source->source_type, source->source_idx, prop_type, prop_idx);

    if (source != NULL && source->source_kind == DVZ_SOURCE_KIND_UNIFORM &&
        (first_item > 0 || item_count > 1))
    {
        log_debug(
            "discarding uniform data after the first item (number of items was %d)", item_count);
        first_item = 0;
        item_count = 1;
        data_item_count = 1;
        count = 1;
    }

    // Make sure the array has the right size.
    if (!do_resize)
        count = MAX(count, prop->arr_orig.item_count);
    dvz_array_resize(&prop->arr_orig, count);

    // Copy the specified array to the prop array.
    dvz_array_data(&prop->arr_orig, first_item, item_count, data_item_count, data);

    prop->obj.request = DVZ_VISUAL_REQUEST_UPLOAD;

    if (source != NULL)
    {
        log_trace("source type %d #%d handled by lib", source->source_type, source->source_idx);
        source->origin = DVZ_SOURCE_ORIGIN_LIB;
        _source_set_changed(source, true);
    }
}



void dvz_visual_data(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, //
    uint32_t count, const void* data)
{
    ASSERT(visual != NULL);
    _visual_data(visual, prop_type, prop_idx, 0, count, count, data, true);
}



void dvz_visual_data_partial(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
{
    _visual_data(
        visual, prop_type, prop_idx, first_item, item_count, data_item_count, data, false);
}



void dvz_visual_data_append(
    DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, //
    uint32_t count, const void* data)
{
    ASSERT(visual != NULL);
    DvzProp* prop = dvz_prop_get(visual, prop_type, prop_idx);
    ASSERT(prop != NULL);
    uint32_t first_item = prop->arr_orig.item_count;
    dvz_visual_data_partial(visual, prop_type, prop_idx, first_item, count, count, data);
}



static DvzSource*
_assert_source_exists(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx)
{
    DvzSource* source = dvz_source_get(visual, source_type, source_idx);

    // // Check if the requested source is not a shared source.
    // if (source == NULL)
    // {
    //     DvzSource* src = dvz_container_iterator(&visual->sources);
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
    //         src = dvz_container_iter(&visual->sources);
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

void dvz_visual_data_source(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, //
    uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
{
    ASSERT(visual != NULL);
    uint32_t count = first_item + item_count;
    ASSERT(count > 0);
    ASSERT(data_item_count > 0);

    // Get the associated source.
    DvzSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);
    ASSERT(source->source_type == source_type);

    // Make sure the array has the right size.
    dvz_array_resize(&source->arr, count);

    // Copy the specified array to the prop array.
    dvz_array_data(&source->arr, first_item, item_count, data_item_count, data);

    source->origin = DVZ_SOURCE_ORIGIN_NOBAKE;
    // source->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
    // visual->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
    _source_set_changed(source, true);
}



// Means that no data updates will be done by datoviz, it is up to the user to update the bound
// buffer
void dvz_visual_buffer(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, DvzBufferRegions br)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    DvzSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);

    VkDeviceSize size = br.size;
    ASSERT(size > 0);
    ASSERT(br.buffer != VK_NULL_HANDLE);

    source->u.br = br;
    source->origin = DVZ_SOURCE_ORIGIN_USER;
    _source_set_changed(source, true);
    // source->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
    // visual->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;

    // Set the pipeline bindings with the source buffer.
    _set_source_bindings(visual, source);
}



void dvz_visual_texture(
    DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, DvzTexture* texture)
{
    ASSERT(visual != NULL);
    ASSERT(visual != NULL);
    DvzSource* source = _assert_source_exists(visual, source_type, source_idx);
    ASSERT(source != NULL);
    ASSERT(texture != NULL);

    source->u.tex = texture;
    source->origin = DVZ_SOURCE_ORIGIN_USER;
    _source_set_changed(source, true);

    DvzBindings* bindings = _get_bindings(visual, source);
    ASSERT(bindings != NULL);
    ASSERT(texture->image != NULL);
    ASSERT(texture->sampler != NULL);
    dvz_bindings_texture(bindings, source->slot_idx, texture);
}



void dvz_visual_flags(DvzVisual* visual, int flags)
{
    ASSERT(visual != NULL);
    visual->flags = flags;
    // Update the vertex buffer at the next call to dvz_visual_update().
    DvzSource* source = _get_pipeline_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    _source_set_changed(source, true);
}



uint32_t dvz_visual_item_count(DvzVisual* visual)
{
    DvzProp* prop = dvz_prop_get(visual, DVZ_PROP_POS, 0);
    return dvz_prop_size(prop);
}



/*************************************************************************************************/
/*  Visual events                                                                                */
/*************************************************************************************************/

void dvz_visual_callback_bake(DvzVisual* visual, DvzVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->callback_bake = callback;
}



void dvz_visual_fill_callback(DvzVisual* visual, DvzVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    visual->callback_fill = callback;
}



void dvz_visual_fill_event(
    DvzVisual* visual, VkClearColorValue clear_color, DvzCommands* cmds, uint32_t cmd_idx,
    DvzViewport viewport, void* user_data)
{
    ASSERT(visual != NULL);
    ASSERT(visual->callback_fill != NULL);

    DvzVisualFillEvent ev = {0};
    ev.clear_color = clear_color;
    ev.cmds = cmds;
    ev.cmd_idx = cmd_idx;
    ev.viewport = viewport;
    ev.user_data = user_data;

    visual->callback_fill(visual, ev);
}



void dvz_visual_fill_begin(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
}



void dvz_visual_fill_end(DvzCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas != NULL);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Baking helpers                                                                               */
/*************************************************************************************************/

DvzSource* dvz_source_get(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx)
{
    ASSERT(visual != NULL);
    DvzSource* source = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&visual->sources);
    DvzSource* out = NULL;
    while (iter.item != NULL)
    {
        source = iter.item;
        if (source->source_type == source_type && source->source_idx == source_idx)
        {
            // Check there is only 1 source with a given type and idx.
            ASSERT(out == NULL);
            out = source;
        }
        dvz_container_iter(&iter);
    }
    return out;
}



DvzArray* dvz_source_array(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx)
{
    ASSERT(visual != NULL);
    DvzSource* source = dvz_source_get(visual, source_type, source_idx);
    return _source_array(source);
}



DvzProp* dvz_prop_get(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx)
{
    ASSERT(visual != NULL);
    DvzProp* prop = NULL;
    DvzContainerIterator iter = dvz_container_iterator(&visual->props);
    DvzProp* out = NULL;
    while (iter.item != NULL)
    {
        prop = iter.item;
        if (prop->prop_type == prop_type && prop->prop_idx == prop_idx)
        {
            ASSERT(out == NULL);
            out = prop;
        }
        dvz_container_iter(&iter);
    }
    if (out == NULL)
        log_debug("prop with type %d #%d not found", prop_type, prop_idx);
    // ASSERT(out != NULL);
    return out;
}



DvzArray* dvz_prop_array(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx)
{
    ASSERT(visual != NULL);
    DvzProp* prop = dvz_prop_get(visual, prop_type, prop_idx);
    return _prop_array(prop, DVZ_PROP_ARRAY_DEFAULT);
}



uint32_t dvz_prop_size(DvzProp* prop)
{
    DvzArray* arr = _prop_array(prop, DVZ_PROP_ARRAY_DEFAULT);
    return arr->item_count;
}



void* dvz_prop_item(DvzProp* prop, uint32_t prop_idx)
{
    ASSERT(prop != NULL);
    DvzArray* arr = _prop_array(prop, DVZ_PROP_ARRAY_DEFAULT);
    void* res = prop->default_value;
    if (prop_idx < arr->item_count)
        res = dvz_array_item(arr, prop_idx);
    if (res == NULL)
    {
        log_debug("no default value for prop %d #%d", prop->prop_type, prop->prop_idx);
    }
    return res;
}



/*************************************************************************************************/
/*  Data update                                                                                  */
/*************************************************************************************************/

void dvz_visual_update(
    DvzVisual* visual, DvzViewport viewport, DvzDataCoords coords, const void* user_data)
{
    ASSERT(visual != NULL);
    log_debug("visual update");

    DvzVisualDataEvent ev = {0};
    ev.viewport = viewport;
    ev.coords = coords;
    ev.user_data = user_data;

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
    DvzArray* arr = NULL;
    DvzBufferRegions* br = NULL;
    DvzCanvas* canvas = visual->canvas;
    DvzContext* ctx = canvas->gpu->context;
    DvzTexture* texture = NULL;
    bool to_upload = false;

    DvzContainerIterator iter = dvz_container_iterator(&visual->sources);
    DvzSource* source = NULL;
    DvzBindings* bindings = NULL;
    while (iter.item != NULL)
    {
        source = iter.item;
        // No source set: using default source or skipping.
        if (source->origin == DVZ_SOURCE_ORIGIN_NONE)
        {
            if (_source_is_texture(source->source_kind))
            {
                log_debug(
                    "source type %d #%d is not set, using default texture", source->source_type,
                    source->source_idx);

                DvzTexture* default_tex = _default_source_texture(ctx, source->source_kind);

                ASSERT(default_tex != NULL);
                ASSERT(default_tex->image != NULL);
                ASSERT(default_tex->image->images[0] != VK_NULL_HANDLE);
                ASSERT(default_tex->sampler != NULL);
                ASSERT(dvz_obj_is_created(&default_tex->obj));

                dvz_visual_texture(visual, source->source_type, source->source_idx, default_tex);
            }
            else if (_source_is_buffer(source->source_kind))
            {
                log_debug(
                    "source type %d #%d is not set", source->source_type, source->source_idx);
                if (source->source_type == DVZ_SOURCE_TYPE_VERTEX)
                {
                    log_warn("skipping visual data upload as VERTEX source is not set");
                    visual->obj.status = DVZ_OBJECT_STATUS_INVALID;

                    // NOTE: this is needed to avoid an infinite loop. We need to let
                    // _enqueue_all_visuals_changed() that this visual is invalid and should not
                    // trigger a visual_changed event.
                    _source_set_changed(source, false);

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
                // bindings = dvz_container_get(&visual->bindings, source->pipeline_idx);
                // ASSERT(bindings != NULL);
                // bindings->obj.status = DVZ_OBJECT_STATUS_INVALID;
                // DvzSource* other = NULL;
                // for (uint32_t j = 0; j < source->other_count; j++)
                // {
                //     // Get other source.
                //     other = dvz_bake_source(visual, source->source_type, source->other_idxs[j]);
                //     ASSERT(other != NULL);
                //     // Get bindings corresponding to graphics pipeline of that other source/
                //     bindings = dvz_container_get(&visual->bindings, other->pipeline_idx);
                //     ASSERT(bindings != NULL);
                //     bindings->obj.status = DVZ_OBJECT_STATUS_INVALID;
                // }
            }

            dvz_container_iter(&iter);
            continue;
        }

        // Upload only for sources manages by datoviz.
        to_upload =
            source->origin == DVZ_SOURCE_ORIGIN_LIB || source->origin == DVZ_SOURCE_ORIGIN_NOBAKE;
        if (!to_upload)
        {
            log_trace(
                "skip data upload for source type %d #%d, origin %d, that is handled by user", //
                source->source_type, source->source_idx, source->origin);
            dvz_container_iter(&iter);
            continue;
        }
        if (!_source_is_set(source))
        {
            log_error("data source %d #%d was never set", source->source_type, source->source_idx);
            dvz_container_iter(&iter);
            continue;
        }
        else if (!_source_has_changed(source))
        {
            log_trace(
                "skip data upload for source %d that doesn't need to be updated",
                source->source_type);
            dvz_container_iter(&iter);
            continue;
        }

        arr = &source->arr;

        // Update buffer sources.
        if (_source_is_buffer(source->source_kind))
        {
            if (arr->item_count == 0)
            {
                log_debug("empty source %d", source->source_type);
                dvz_container_iter(&iter);
                continue;
            }
            log_debug("uploading new data for source %d", source->source_type);

            br = &source->u.br;

            // NOTE: the source array MUST have been allocated by the baking function,
            // or directly by the user via dvz_visual_data_source() (NOBAKE origin)
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

            if (br->buffer->type == DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE)
            {
                // dvz_canvas_buffers(canvas, *br, 0, size, arr->data);
                for (uint32_t i = 0; i < canvas->swapchain.img_count; i++)
                    dvz_buffer_upload(br->buffer, br->offsets[i], size, arr->data);
            }
            else
                dvz_upload_buffer(ctx, *br, 0, size, arr->data);
            _source_set(source);
            // source->obj.status = DVZ_OBJECT_STATUS_CREATED;
            // visual->obj.status = DVZ_OBJECT_STATUS_CREATED;
        }

        // Update textures.
        else if (_source_is_texture(source->source_kind))
        {
            // Make sure the GPU texture exists and is allocated with the right shape.
            _source_texture(visual, source);
            texture = source->u.tex;

            ASSERT(texture != NULL);
            ASSERT(dvz_obj_is_created(&texture->obj));
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
            dvz_upload_texture(
                ctx, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, arr->item_count * arr->item_size,
                arr->data);
            _source_set(source);
        }

        dvz_container_iter(&iter);
    }

    // Update the bindings that need to be updated.
    for (uint32_t i = 0; i < visual->graphics_count; i++)
    {
        bindings = dvz_container_get(&visual->bindings, i);
        ASSERT(bindings != NULL);
        if (bindings->obj.status == DVZ_OBJECT_STATUS_NEED_UPDATE)
            dvz_bindings_update(bindings);
    }
    for (uint32_t i = 0; i < visual->compute_count; i++)
    {
        bindings = dvz_container_get(&visual->bindings_comp, i);
        ASSERT(bindings != NULL);
        if (bindings->obj.status == DVZ_OBJECT_STATUS_NEED_UPDATE)
            dvz_bindings_update(bindings);
    }
}
