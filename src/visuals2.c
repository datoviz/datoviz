#include "../include/visky/visuals2.h"
#include "../include/visky/canvas.h"
#include "../include/visky/graphics.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VklVisual vkl_visual(VklCanvas* canvas)
{
    VklVisual visual = {0};

    return visual;
}



void vkl_visual_create(VklVisual* visual) { ASSERT(visual != NULL); }



void vkl_visual_destroy(VklVisual* visual) { ASSERT(visual != NULL); }



/*************************************************************************************************/
/*  Custom visuals                                                                               */
/*************************************************************************************************/

void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset)
{
    ASSERT(visual != NULL);
}



void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics)
{
    ASSERT(visual != NULL);
    ASSERT(graphics != NULL);
    ASSERT(is_obj_created(&graphics->obj));
}



void vkl_visual_compute(VklVisual* visual, VklCompute* compute)
{
    ASSERT(visual != NULL);
    ASSERT(compute != NULL);
    ASSERT(is_obj_created(&compute->obj));
}



void vkl_visual_bake(VklVisual* visual, VklVisualDataCallback callback) { ASSERT(visual != NULL); }



void vkl_visual_fill(VklVisual* visual, VklVisualFillCallback callback) { ASSERT(visual != NULL); }



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count)
{
    ASSERT(visual != NULL);
}



void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size)
{
    ASSERT(visual != NULL);
}



void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t idx, VkDeviceSize size, const void* data)
{
    ASSERT(visual != NULL);
}



void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, VklBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size)
{
    ASSERT(visual != NULL);
}



void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, VklTexture* texture, uvec2 offset,
    uvec2 shape)
{
    ASSERT(visual != NULL);
}
