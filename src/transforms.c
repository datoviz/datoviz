#include "../include/datoviz/transforms.h"
#include "../include/datoviz/panel.h"
#include "transforms_utils.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void dvz_transform_data(DvzDataCoords coords, DvzArray* pos_in, DvzArray* pos_out, bool inverse)
{
    // NOTE: this CPU transformation function is not optimized at all

    ASSERT(pos_in != NULL);
    ASSERT(pos_out != NULL);

    log_debug("data normalization on %d position elements", pos_in->item_count);
    DvzTransform tr = {0};

    if (coords.transform == DVZ_TRANSFORM_CARTESIAN)
        tr = _transform_interp(coords.box, DVZ_BOX_NDC);
    if (coords.transform == DVZ_TRANSFORM_EARTH_MERCATOR_WEB)
        tr = _transform(coords.transform);

    if (inverse)
        tr = _transform_inv(&tr);

    ASSERT(pos_out->item_count == pos_in->item_count);
    ASSERT(pos_out->item_size == pos_in->item_size);

    ASSERT(pos_out->dtype == pos_in->dtype);

    // TODO: support other dtypes
    ASSERT(
        pos_out->dtype == DVZ_DTYPE_DVEC3
        // pos_out->dtype == DVZ_DTYPE_DOUBLE || //
        // pos_out->dtype == DVZ_DTYPE_DVEC2 ||  //
        // pos_out->dtype == DVZ_DTYPE_DVEC3 ||  //
        // pos_out->dtype == DVZ_DTYPE_DVEC4     //
    );

    const uint32_t components = pos_in->components;
    ASSERT(pos_out->components == components);
    ASSERT(1 <= components && components <= 4);

    dvec3* in = NULL;
    dvec3* out = NULL;

    for (uint32_t i = 0; i < pos_in->item_count; i++)
    {
        in = (dvec3*)dvz_array_item(pos_in, i);
        out = (dvec3*)dvz_array_item(pos_out, i);
        _transform_apply(&tr, *in, *out);
    }
}



void dvz_transform(DvzPanel* panel, DvzCDS source, dvec3 in, DvzCDS target, dvec3 out)
{
    ASSERT(panel != NULL);
    DvzTransformChain tc = _transforms_cds(panel, source, target);
    _transforms_apply(&tc, in, out);
}
