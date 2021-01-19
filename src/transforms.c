#include "../include/visky/transforms.h"
#include "../include/visky/panel.h"
#include "transforms_utils.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void vkl_transform_data(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out, bool inverse)
{
    // NOTE: this CPU transformation function is not optimized at all

    ASSERT(pos_in != NULL);
    ASSERT(pos_out != NULL);

    log_debug("data normalization on %d position elements", pos_in->item_count);
    VklTransform tr = _transform_interp(coords.box, VKL_BOX_NDC);
    if (inverse)
        tr = _transform_inv(&tr);

    ASSERT(pos_out->item_count == pos_in->item_count);
    ASSERT(pos_out->item_size == pos_in->item_size);

    ASSERT(pos_out->dtype == pos_in->dtype);

    // TODO: support other dtypes
    ASSERT(
        pos_out->dtype == VKL_DTYPE_DVEC3
        // pos_out->dtype == VKL_DTYPE_DOUBLE || //
        // pos_out->dtype == VKL_DTYPE_DVEC2 ||  //
        // pos_out->dtype == VKL_DTYPE_DVEC3 ||  //
        // pos_out->dtype == VKL_DTYPE_DVEC4     //
    );

    const uint32_t components = pos_in->components;
    ASSERT(pos_out->components == components);
    ASSERT(1 <= components && components <= 4);

    dvec3* in = NULL;
    dvec3* out = NULL;

    for (uint32_t i = 0; i < pos_in->item_count; i++)
    {
        in = (dvec3*)vkl_array_item(pos_in, i);
        out = (dvec3*)vkl_array_item(pos_out, i);
        _transform_apply(&tr, *in, *out);
    }
}



void vkl_transform(VklPanel* panel, VklCDS source, dvec3 in, VklCDS target, dvec3 out)
{
    ASSERT(panel != NULL);
    VklTransformChain tc = _transforms_cds(panel, source, target);
    _transforms_apply(&tc, in, out);
}
