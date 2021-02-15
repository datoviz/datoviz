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

    log_debug(
        "data normalization on %d position elements, transform %d", pos_in->item_count,
        coords.transform);

    // Default transform.
    DvzTransform tr = _transform(DVZ_TRANSFORM_CARTESIAN);

    // // HACK: for now we only support DVEC3. For axes we need support for FLOAT just for linear
    // so
    // // we do that here.
    // if (pos_out->dtype == DVZ_DTYPE_DOUBLE && coords.transform == DVZ_TRANSFORM_CARTESIAN)
    // {
    //     _box_print(coords.box);
    //     double a = 2. / (coords.box.p1[0] - coords.box.p0[0]);
    //     double b = -1 - 2 * coords.box.p0[0] / (coords.box.p1[0] - coords.box.p0[0]);
    //     double x = 0;
    //     for (uint32_t i = 0; i < pos_in->item_count; i++)
    //     {
    //         x = ((double*)pos_in->data)[i];
    //         ((double*)pos_out->data)[i] = a * x + b;
    //         log_debug("%f %f", x, ((double*)pos_out->data)[i]);
    //     }
    //     return;
    //     // -1 + 2 * (t - u) / (v - u);
    // }

    // TODO: support other dtypes
    ASSERT(pos_out->dtype == DVZ_DTYPE_DVEC3);

    DvzArray* pos_temp = pos_in;

    // First, handle non-cartesian transforms.
    if (coords.transform == DVZ_TRANSFORM_EARTH_MERCATOR_WEB)
    {
        tr = _transform(coords.transform);
        if (inverse)
            tr = _transform_inv(&tr);
        _transform_array(&tr, pos_in, pos_out);
        pos_temp = pos_out;
    }
    // TODO: more non-cartesian transforms.

    // Transform the box.
    // NOTE: assuming a box is transformed to a box...
    DvzBox box = {0};
    _transform_apply(&tr, coords.box.p0, box.p0);
    _transform_apply(&tr, coords.box.p1, box.p1);

    // Then, linearly rescale to NDC, using the transformed box.
    tr = _transform_interp(box, DVZ_BOX_NDC);

    // Apply the transformation.
    _transform_array(&tr, pos_temp, pos_out);
    // dvz_array_print(pos_temp);
    // dvz_array_print(pos_out);
}



void dvz_transform(DvzPanel* panel, DvzCDS source, dvec3 pos_in, DvzCDS target, dvec3 pos_out)
{
    ASSERT(panel != NULL);
    DvzTransformChain tc = _transforms_cds(panel, source, target);
    _transforms_apply(&tc, pos_in, pos_out);
}
