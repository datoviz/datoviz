#ifndef VKL_TRANSFORMS_UTILS_HEADER
#define VKL_TRANSFORMS_UTILS_HEADER

#include "../include/visky/interact.h"
#include "../include/visky/panel.h"
#include "../include/visky/scene.h"



/*************************************************************************************************/
/*  Position normalization                                                                       */
/*************************************************************************************************/

static void _check_box(VklBox box)
{
    for (uint32_t i = 0; i < 3; i++)
        ASSERT(box.p0[i] <= box.p1[i]);
}



// Return the bounding box of a set of dvec3 points.
static VklBox _box_bounding(VklArray* points_in)
{
    ASSERT(points_in != NULL);
    ASSERT(points_in->item_count > 0);
    ASSERT(points_in->item_size > 0);

    dvec3* pos = NULL;
    VklBox box = VKL_BOX_INF;
    for (uint32_t i = 0; i < points_in->item_count; i++)
    {
        pos = (dvec3*)vkl_array_item(points_in, i);
        ASSERT(pos != NULL);
        for (uint32_t j = 0; j < 3; j++)
        {
            box.p0[j] = MIN(box.p0[j], (*pos)[j]);
            box.p1[j] = MAX(box.p1[j], (*pos)[j]);
        }
    }
    return box;
}



static VklBox _box_merge(uint32_t count, VklBox* boxes)
{
    VklBox merged = VKL_BOX_INF;
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            merged.p0[j] = MIN(merged.p0[j], boxes[i].p0[j]);
            merged.p1[j] = MAX(merged.p1[j], boxes[i].p1[j]);
        }
    }
    return merged;
}



static void _box_print(VklBox box)
{
    log_info(
        "box [%f, %f] [%f, %f] [%f %f]", box.p0[0], box.p1[0], box.p0[1], box.p1[1], box.p0[2],
        box.p1[2]);
}



// Make a box cubic/square (if need to keep fixed aspect ratio).
static VklBox _box_cube(VklBox box)
{
    double edge_common = 0;
    double vmin = 0;
    double vmax = 0;

    dvec3 edge = {0};
    dvec3 center = {0};

    for (uint32_t j = 0; j < 3; j++)
    {
        vmin = box.p0[j];
        vmax = box.p1[j];

        if (vmin == +INFINITY && vmax == -INFINITY)
        {
            vmin = -1;
            vmax = +1;
        }
        ASSERT(vmin < +INFINITY);
        ASSERT(vmax > -INFINITY);

        center[j] = .5 * (vmin + vmax);
        ASSERT(center[j] != NAN);

        ASSERT(vmin <= center[j] && center[j] <= vmax);

        edge[j] = MAX(vmax - center[j], center[j] - vmin);
    }

    // Max edge.
    for (uint32_t j = 0; j < 3; j++)
        edge_common = MAX(edge_common, edge[j]);
    if (edge_common == 0)
        edge_common = 1;
    ASSERT(edge_common > 0);

    VklBox out = box;
    for (uint32_t j = 0; j < 3; j++)
    {
        // Find the edge on each axis. Do not extend the range if an axis range is degenerate.
        out.p0[j] = center[j] - (edge[j] == 0 ? 0 : edge_common);
        out.p1[j] = center[j] + (edge[j] == 0 ? 0 : edge_common);
    }
    return out;
}



/*************************************************************************************************/
/*  Internal transform API                                                                       */
/*************************************************************************************************/

static VklTransform _transform(VklTransformType type)
{
    VklTransform tr = {0};
    tr.type = type;
    _dmat4_identity(tr.mat);
    return tr;
}



static VklTransform _transform_interp(VklBox box_in, VklBox box_out)
{
    VklTransform tr = _transform(VKL_TRANSFORM_CARTESIAN);

    for (uint32_t j = 0; j < 3; j++)
    {
        // scaling coefficient
        tr.mat[j][j] = (box_out.p1[j] - box_out.p0[j]) / (box_in.p1[j] - box_in.p0[j]);
        // translation coefficient
        tr.mat[3][j] = (box_out.p0[j] * box_in.p1[j] - box_out.p1[j] * box_in.p0[j]) /
                       (box_in.p1[j] - box_in.p0[j]);
    }
    tr.mat[3][3] = 1;

    return tr;
}



static VklTransform _transform_inv(VklTransform* tr)
{
    ASSERT(tr != NULL);
    VklTransform tri = {0};
    tri.type = tr->type;
    if (tr->type == VKL_TRANSFORM_CARTESIAN)
    {
        tri.inverse = false; // we inverse the matrix instead
        _dmat4_inv(tr->mat, tri.mat);
    }
    else
    {
        tri.inverse = true;
        // it will be up to _transform_apply() to take the inverse field into account for
        // non-cartesian transforms
    }
    return tri;
}



static inline void _transform_apply(VklTransform* tr, dvec3 in, dvec3 out)
{
    ASSERT(tr != NULL);
    if (tr->type == VKL_TRANSFORM_CARTESIAN)
    {
        ASSERT(!tr->inverse);
        // TODO: implement log scale? available in tr->flags
        _dmat4_mulv3(tr->mat, in, 1, out);
    }
    else
    {
        log_error("non-cartesian transforms not yet implemented");
    }
}



static VklTransform _transform_mvp(VklMVP* mvp)
{
    VklTransform tr = _transform(VKL_TRANSFORM_CARTESIAN);
    dmat4 mat;

    _dmat4_mat4(mvp->model, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    _dmat4_mat4(mvp->view, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    _dmat4_mat4(mvp->proj, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    // flip y axis and renormalize z coordinate to account for differences between OpenGL and
    // Vulkan conventions (because we want to use cglm MVP helpers that use the OpenGL convention).
    _dmat4_mul(VKL_TRANSFORM_MATRIX_VULKAN, tr.mat, tr.mat);
    return tr;
}



// transformation from a CDS to the next
static VklTransform _transform_cds(VklPanel* panel, VklCDS source)
{
    ASSERT(panel != NULL);
    VklTransform tr = _transform(VKL_TRANSFORM_CARTESIAN);
    VklBox box_data = panel->data_coords.box;
    VklBox box_ndc = VKL_BOX_NDC;
    VklViewport viewport = panel->viewport;

    ASSERT(panel->grid != NULL);
    VklCanvas* canvas = panel->grid->canvas;
    ASSERT(canvas != NULL);

    VklMVP mvp = {0};
    if (panel->controller != NULL && panel->controller->interact_count > 0)
        mvp = panel->controller->interacts[0].mvp;
    else
    {
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
    }

    switch (source)
    {
    case VKL_CDS_DATA: // to SCENE
        tr = _transform_interp(box_data, box_ndc);
        break;

    case VKL_CDS_SCENE: // to VULKAN
        tr = _transform_mvp(&mvp);
        break;

    case VKL_CDS_VULKAN:; // to FRAMEBUFFER
        {
            // Viewport size, in framebuffer coordinates
            double x = viewport.viewport.x;
            double y = viewport.viewport.y;
            double w = viewport.viewport.width;
            double h = viewport.viewport.height;
            ASSERT(w > 0);
            ASSERT(h > 0);

            // Margins.
            double mt = 2 * viewport.margins[0] / h;
            double mr = 2 * viewport.margins[1] / w;
            double mb = 2 * viewport.margins[2] / h;
            double ml = 2 * viewport.margins[3] / w;

            VklBox box0 = (VklBox){{-1, -1, 0}, {+1, +1, 1}};
            VklBox box1 = (VklBox){{x + ml, y + mt, 0}, {x + w - mr, y + h - mb, 1}};
            tr = _transform_interp(box0, box1);
        }
        break;

    case VKL_CDS_FRAMEBUFFER:; // to WINDOW
        {
            // Canvas size, in screen and framebuffer coordinates
            double ws = viewport.size_screen[0];
            double hs = viewport.size_screen[1];
            double wf = viewport.size_framebuffer[0];
            double hf = viewport.size_framebuffer[1];

            VklBox box0 = (VklBox){{0, 0, 0}, {wf, hf, 1}};
            VklBox box1 = (VklBox){{0, 0, 0}, {ws, hs, 1}};
            tr = _transform_interp(box0, box1);
        }
        break;

    default:
        log_error("invalid CDS transform from %d to %d", source, source + 1);
        break;
    }

    return tr;
}



/*************************************************************************************************/
/*  Internal transform chain API                                                                 */
/*************************************************************************************************/

static VklTransformChain _transforms()
{
    VklTransformChain tc = {0};
    return tc;
}



static void _transforms_append(VklTransformChain* tc, VklTransform tr)
{
    ASSERT(tc != NULL);
    ASSERT(tc->count < VKL_TRANSFORM_CHAIN_MAX_SIZE - 1);
    tc->transforms[tc->count++] = tr;
}



static inline void _transforms_apply(VklTransformChain* tc, dvec3 in, dvec3 out)
{
    ASSERT(tc != NULL);
    dvec3 temp0, temp1;
    _dvec3_copy(in, temp0);
    for (uint32_t i = 0; i < tc->count; i++)
    {
        _transform_apply(&tc->transforms[i], temp0, temp1);
        _dvec3_copy(temp1, temp0);
    }
    _dvec3_copy(temp1, out);
}



static VklTransformChain _transforms_inv(VklTransformChain* tc)
{
    ASSERT(tc != NULL);
    VklTransformChain tci = {0};
    tci.count = tc->count;
    for (uint32_t i = 0; i < tc->count; i++)
    {
        tci.transforms[i] = _transform_inv(&tc->transforms[tc->count - 1 - i]);
    }
    return tci;
}



static VklTransformChain _transforms_cds(VklPanel* panel, VklCDS source, VklCDS target)
{
    ASSERT(panel != NULL);
    VklTransformChain tc = _transforms();
    int32_t d = (int32_t)target - (int32_t)source;
    if (d < 0)
    {
        tc = _transforms_cds(panel, target, source);
        tc = _transforms_inv(&tc);
    }
    else
    {
        // d == 0 ? do nothing, empty transform chain
        // d == 1 ? single loop iteration
        for (int32_t i = 0; i < d; i++)
        {
            _transforms_append(&tc, _transform_cds(panel, (VklCDS)((int32_t)source + i)));
        }
    }
    return tc;
}



#endif
