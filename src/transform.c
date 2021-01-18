#include "../include/visky/transforms.h"



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

void vkl_transform(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out)
{
    ASSERT(pos_in != NULL);
    ASSERT(pos_out != NULL);

    log_debug("data normalization on %d position elements", pos_in->item_count);

    switch (coords.transform)
    {

    case VKL_TRANSFORM_CARTESIAN:
        _transform_linear(coords.box, pos_in, VKL_BOX_NDC, pos_out);
        break;

        // TODO: other transforms
        // TODO: support for semilog/loglog plots

    default:
        break;
    }
}
