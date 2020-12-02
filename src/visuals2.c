#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};

    // TODO

    obj_created(&visual.obj);
    return visual;
}



void vkl_visual_destroy(VklVisual* visual)
{
    ASSERT(visual != NULL);
    // TODO
}



/*************************************************************************************************/
/*  Custom visuals                                                                               */
/*************************************************************************************************/

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



void vkl_visual_bake(VklVisual* visual, VklVisualDataCallback callback)
{
    ASSERT(visual != NULL);
    visual->data_callback = callback;
}



void vkl_visual_fill(VklVisual* visual, VklVisualFillCallback callback)
{
    ASSERT(visual != NULL);
    visual->fill_callback = callback;
}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count)
{
    ASSERT(visual != NULL);
    visual->item_count = item_count;
    visual->group_count = group_count;
}



void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
    // TODO
}



void vkl_visual_data(VklVisual* visual, VklPropType type, uint32_t idx, const void* data)
{
    ASSERT(visual != NULL);
    vkl_visual_data_partial(visual, type, idx, 0, visual->item_count, data);
}



void vkl_visual_data_partial(
    VklVisual* visual, VklPropType type, uint32_t idx, uint32_t first_item, uint32_t item_count,
    const void* data)
{
    ASSERT(visual != NULL);
    // TODO
}



void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, VklBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size)
{
    ASSERT(visual != NULL);
    // TODO
}



void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, VklTexture* texture, uvec2 offset,
    uvec2 shape)
{
    ASSERT(visual != NULL);
    // TODO
}
