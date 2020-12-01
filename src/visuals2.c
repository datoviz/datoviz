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



void vkl_visual_create(VklVisual* visual) {}

void vkl_visual_destroy(VklVisual* visual) {}



/*************************************************************************************************/
/*  Builtin visuals                                                                              */
/*************************************************************************************************/

void vkl_visual_builtin(VklVisual* visual, VklVisualBuiltin builtin) {}

void vkl_visual_variant(VklVisual* visual, VklVisualVariant variant) {}

void vkl_visual_transform(VklVisual* visual, VklTransformAxis transform_axis) {}



/*************************************************************************************************/
/*  Custom visuals                                                                               */
/*************************************************************************************************/

void vkl_visual_prop(
    VklVisual* visual, VklPropType prop, uint32_t idx, VklDataType dtype, VklPropLoc loc,
    uint32_t binding_idx, uint32_t field_idx, VkDeviceSize offset)
{
}

// check graphics has been created
void vkl_visual_graphics(VklVisual* visual, VklGraphics* graphics) {}

void vkl_visual_compute(VklVisual* visual, VklCompute* compute) {}

void vkl_visual_bake(VklVisual* visual, VklVisualDataCallback callback) {}

void vkl_visual_fill(VklVisual* visual, VklVisualFillCallback callback) {}



/*************************************************************************************************/
/*  User-facing functions                                                                        */
/*************************************************************************************************/

void vkl_visual_size(VklVisual* visual, uint32_t item_count, uint32_t group_count) {}

void vkl_visual_group(VklVisual* visual, uint32_t group_idx, uint32_t size) {}

void vkl_visual_data(
    VklVisual* visual, VklPropType type, uint32_t idx, VkDeviceSize size, const void* data)
{
}

void vkl_visual_data_buffer(
    VklVisual* visual, VklPropType type, uint32_t idx, VklBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size)
{
}

void vkl_visual_data_texture(
    VklVisual* visual, VklPropType type, uint32_t idx, VklTexture* texture, uvec2 offset,
    uvec2 shape)
{
}
