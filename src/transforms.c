#include "../include/datoviz/transforms.h"
#include "../include/datoviz/panel.h"
#include "transforms_utils.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_transform_pos(DvzDataCoords coords, DvzArray* pos_in, DvzArray* pos_out, bool inverse)
{
    // NOTE: this CPU transformation function is not optimized at all

    ASSERT(pos_in != NULL);
    ASSERT(pos_out != NULL);
    ASSERT(pos_out->item_count == pos_in->item_count);
    ASSERT(pos_out->item_size == pos_in->item_size);
    ASSERT(pos_out->dtype == pos_in->dtype);
    // TODO: support other dtypes
    ASSERT(pos_out->dtype == DVZ_DTYPE_DVEC3);

    log_debug("data normalization on %d position elements", pos_in->item_count);
    DvzTransform tr = {0};

    // First, handle non-cartesian transforms.
    if (coords.transform == DVZ_TRANSFORM_EARTH_MERCATOR_WEB)
    {
        tr = _transform(coords.transform);
        if (inverse)
            tr = _transform_inv(&tr);
        _transform_array(&tr, pos_in, pos_out);
    }
    // TODO: more non-cartesian transforms.

    // Then, linearly rescale to NDC.
    tr = _transform_interp(coords.box, DVZ_BOX_NDC);
    _transform_array(&tr, pos_in, pos_out);
}



void dvz_transform(DvzPanel* panel, DvzCDS source, dvec3 in, DvzCDS target, dvec3 out)
{
    ASSERT(panel != NULL);
    DvzTransformChain tc = _transforms_cds(panel, source, target);
    _transforms_apply(&tc, in, out);
}
