#include "../include/visky/visky.h"

#include "../include/visky/transform.h"


/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VkyAxesTransform vky_axes_transform_inv(VkyAxesTransform tr)
{
    ASSERT(tr.scale[0] != 0);
    ASSERT(tr.scale[1] != 0);

    VkyAxesTransform tri = {0};
    tri.scale[0] = 1. / tr.scale[0];
    tri.scale[1] = 1. / tr.scale[1];
    tri.shift[0] = -tr.scale[0] * tr.shift[0];
    tri.shift[1] = -tr.scale[1] * tr.shift[1];
    return tri;
}

VkyAxesTransform vky_axes_transform_mul(VkyAxesTransform tr0, VkyAxesTransform tr1)
{
    VkyAxesTransform trm = {0};
    trm.scale[0] = tr0.scale[0] * tr1.scale[0];
    trm.scale[1] = tr0.scale[1] * tr1.scale[1];
    trm.shift[0] = tr0.shift[0] + tr1.shift[0] / tr0.scale[0];
    trm.shift[1] = tr0.shift[1] + tr1.shift[1] / tr0.scale[1];
    return trm;
}

VkyAxesTransform vky_axes_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout)
{
    ASSERT(qin[0] != pin[0]);
    ASSERT(qin[1] != pin[1]);
    ASSERT(qout[0] != pout[0]);
    ASSERT(qout[1] != pout[1]);

    VkyAxesTransform tr = {0};
    tr.scale[0] = (qout[0] - pout[0]) / (qin[0] - pin[0]);
    tr.scale[1] = (qout[1] - pout[1]) / (qin[1] - pin[1]);
    tr.shift[0] = (pin[0] * qout[0] - pout[0] * qin[0]) / (qout[0] - pout[0]);
    tr.shift[1] = (pin[1] * qout[1] - pout[1] * qin[1]) / (qout[1] - pout[1]);
    return tr;
}

void vky_axes_transform_apply(VkyAxesTransform* tr, dvec2 in, dvec2 out)
{
    out[0] = tr->scale[0] * (in[0] - tr->shift[0]);
    out[1] = tr->scale[1] * (in[1] - tr->shift[1]);
}

VkyAxesTransform vky_axes_transform(VkyPanel* panel, VkyCDS source, VkyCDS target)
{
    VkyAxesTransform tr = {{1, 1}, {0, 0}}; // identity
    if (source == target)
    {
        return tr;
    }
    else if (source > target)
    {
        return vky_axes_transform_inv(vky_axes_transform(panel, target, source));
    }
    else if (target - source >= 2)
    {
        for (uint32_t k = source; k <= target - 1; k++)
        {
            tr = vky_axes_transform_mul(tr, vky_axes_transform(panel, (VkyCDS)k, (VkyCDS)(k + 1)));
        }
    }
    else if (target - source == 1)
    {
        switch (source)
        {

        case VKY_CDS_DATA:
            // linear normalization based on axes range
            ASSERT(target == VKY_CDS_NDC);
            break;

        case VKY_CDS_NDC:
            // apply panzoom
            ASSERT(target == VKY_CDS_PANEL);
            break;

        case VKY_CDS_PANEL:
            // using inner viewport
            ASSERT(target == VKY_CDS_CANVAS_NDC);
            break;

        case VKY_CDS_CANVAS_NDC:
            // multiply by canvas size
            ASSERT(target == VKY_CDS_CANVAS_PX);
            break;

        default:
            break;
        }
    }
    return tr;
}
