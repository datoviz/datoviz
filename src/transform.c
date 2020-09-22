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
    // dvec2 ZERO = {0, 0};
    dvec2 NDC0 = {-1, -1};
    dvec2 NDC1 = {+1, +1};

    ASSERT(panel->controller_type == VKY_CONTROLLER_AXES_2D);
    VkyAxes* axes = ((VkyControllerAxes2D*)panel->controller)->axes;

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
            {
                dvec2 ll = {axes->xscale_orig.vmin, axes->yscale_orig.vmin};
                dvec2 ur = {axes->xscale_orig.vmax, axes->yscale_orig.vmax};
                tr = vky_axes_transform_interp(ll, NDC0, ur, NDC1);
            }
            break;

        case VKY_CDS_NDC:
            // apply panzoom
            ASSERT(target == VKY_CDS_PANEL);
            VkyPanzoom* panzoom = ((VkyControllerAxes2D*)panel->controller)->panzoom;
            dvec2 p = {panzoom->camera_pos[0], panzoom->camera_pos[1]};
            dvec2 s = {panzoom->zoom[0], panzoom->zoom[1]};
            tr.scale[0] = s[0];
            tr.scale[1] = s[1];
            tr.shift[0] = p[0]; // / s[0];
            tr.shift[1] = p[1]; // / s[1];
            break;

        case VKY_CDS_PANEL:
            // using inner viewport
            ASSERT(target == VKY_CDS_CANVAS_NDC);
            VkyViewport v = panel->viewport;
            VkyAxesTransform trm = {0}; // from inner to outer viewport
            VkyAxesTransform trc = {0}; // from outer viewport to canvas

            // Margins.
            double cw = panel->scene->canvas->size.framebuffer_width;
            double ch = panel->scene->canvas->size.framebuffer_height;
            double mt = panel->margins[0] / ch;
            double mr = panel->margins[1] / cw;
            double mb = panel->margins[2] / ch;
            double ml = panel->margins[3] / cw;

            trm = vky_axes_transform_interp(
                NDC0, (dvec2){-1 + ml, -1 + mb}, NDC1, (dvec2){+1 - mr, +1 - mt});

            // From outer to inner viewport.
            dvec2 ll, ur;
            ll[0] = -1 + 2 * v.x;
            ll[1] = +1 - 2 * (v.y + v.h);
            ur[0] = -1 + 2 * (v.x + v.w);
            ur[1] = +1 - 2 * v.y;
            trc = vky_axes_transform_interp(NDC0, ll, NDC1, ur);

            // DEBUG
            tr = vky_axes_transform_mul(trm, trc);
            break;

        case VKY_CDS_CANVAS_NDC:
            // multiply by canvas size
            ASSERT(target == VKY_CDS_CANVAS_PX);
            double w = panel->scene->canvas->size.window_width;
            double h = panel->scene->canvas->size.window_height;
            tr = vky_axes_transform_interp(NDC0, (dvec2){0, h}, NDC1, (dvec2){w, 0});
            break;

        default:
            log_error("unknown coordinate systems");
            break;
        }
    }
    return tr;
}
