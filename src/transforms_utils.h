#ifndef DVZ_TRANSFORMS_UTILS_HEADER
#define DVZ_TRANSFORMS_UTILS_HEADER

#include "../include/datoviz/interact.h"
#include "../include/datoviz/panel.h"
#include "../include/datoviz/scene.h"



/*************************************************************************************************/
/*  Position normalization                                                                       */
/*************************************************************************************************/

static void _check_box(DvzBox box)
{
    for (uint32_t i = 0; i < 3; i++)
        ASSERT(box.p0[i] <= box.p1[i]);
}



// Return the bounding box of a set of dvec3 points.
static DvzBox _box_bounding(DvzArray* points_in)
{
    ASSERT(points_in != NULL);
    ASSERT(points_in->item_count > 0);
    ASSERT(points_in->item_size > 0);

    dvec3* pos = NULL;
    DvzBox box = DVZ_BOX_INF;
    for (uint32_t i = 0; i < points_in->item_count; i++)
    {
        pos = (dvec3*)dvz_array_item(points_in, i);
        ASSERT(pos != NULL);
        for (uint32_t j = 0; j < 3; j++)
        {
            box.p0[j] = MIN(box.p0[j], (*pos)[j]);
            box.p1[j] = MAX(box.p1[j], (*pos)[j]);
        }
    }
    return box;
}



static DvzBox _box_merge(uint32_t count, DvzBox* boxes)
{
    if (count == 0)
        return DVZ_BOX_NDC;
    DvzBox merged = DVZ_BOX_INF;
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = 0; j < 3; j++)
        {
            merged.p0[j] = MIN(merged.p0[j], boxes[i].p0[j]);
            merged.p1[j] = MAX(merged.p1[j], boxes[i].p1[j]);
        }
    }
    for (uint32_t j = 0; j < 3; j++)
    {
        if (merged.p0[j] == merged.p1[j])
        {
            merged.p0[j] -= 1;
            merged.p1[j] += 1;
        }
    }
    return merged;
}



static void _box_print(DvzBox box)
{
    log_info(
        "box [%f, %f] [%f, %f] [%f %f]", box.p0[0], box.p1[0], box.p0[1], box.p1[1], box.p0[2],
        box.p1[2]);
}



// Make a box cubic/square (if need to keep fixed aspect ratio).
static DvzBox _box_cube(DvzBox box)
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

    DvzBox out = box;
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

static DvzTransform _transform(DvzTransformType type)
{
    DvzTransform tr = {0};
    tr.type = type;
    _dmat4_identity(tr.mat);
    return tr;
}



static DvzTransform _transform_interp(DvzBox box_in, DvzBox box_out)
{
    DvzTransform tr = _transform(DVZ_TRANSFORM_CARTESIAN);

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



static DvzTransform _transform_inv(DvzTransform* tr)
{
    ASSERT(tr != NULL);
    DvzTransform tri = {0};
    tri.type = tr->type;
    if (tr->type == DVZ_TRANSFORM_CARTESIAN)
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



static inline void _transform_apply(DvzTransform* tr, dvec3 in, dvec3 out)
{
    ASSERT(tr != NULL);
    if (tr->type == DVZ_TRANSFORM_CARTESIAN)
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



static DvzTransform _transform_mvp(DvzMVP* mvp)
{
    DvzTransform tr = _transform(DVZ_TRANSFORM_CARTESIAN);
    dmat4 mat;

    _dmat4_mat4(mvp->model, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    _dmat4_mat4(mvp->view, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    _dmat4_mat4(mvp->proj, mat);
    _dmat4_mul(mat, tr.mat, tr.mat);

    // flip y axis and renormalize z coordinate to account for differences between OpenGL and
    // Vulkan conventions (because we want to use cglm MVP helpers that use the OpenGL convention).
    _dmat4_mul(DVZ_TRANSFORM_MATRIX_VULKAN, tr.mat, tr.mat);
    return tr;
}



// transformation from a CDS to the next
static DvzTransform _transform_cds(DvzPanel* panel, DvzCDS source)
{
    ASSERT(panel != NULL);
    DvzTransform tr = _transform(DVZ_TRANSFORM_CARTESIAN);
    DvzBox box_data = panel->data_coords.box;
    DvzBox box_ndc = DVZ_BOX_NDC;
    DvzViewport viewport = panel->viewport;

    ASSERT(panel->grid != NULL);
    DvzCanvas* canvas = panel->grid->canvas;
    ASSERT(canvas != NULL);

    DvzMVP mvp = {0};
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
    case DVZ_CDS_DATA: // to SCENE
        tr = _transform_interp(box_data, box_ndc);
        break;

    case DVZ_CDS_SCENE: // to VULKAN
        tr = _transform_mvp(&mvp);
        break;

    case DVZ_CDS_VULKAN:; // to FRAMEBUFFER
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

            DvzBox box0 = (DvzBox){{-1, -1, 0}, {+1, +1, 1}};
            DvzBox box1 = (DvzBox){{x + ml, y + mt, 0}, {x + w - mr, y + h - mb, 1}};
            tr = _transform_interp(box0, box1);
        }
        break;

    case DVZ_CDS_FRAMEBUFFER:; // to WINDOW
        {
            // Canvas size, in screen and framebuffer coordinates
            double ws = viewport.size_screen[0];
            double hs = viewport.size_screen[1];
            double wf = viewport.size_framebuffer[0];
            double hf = viewport.size_framebuffer[1];

            DvzBox box0 = (DvzBox){{0, 0, 0}, {wf, hf, 1}};
            DvzBox box1 = (DvzBox){{0, 0, 0}, {ws, hs, 1}};
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

static DvzTransformChain _transforms()
{
    DvzTransformChain tc = {0};
    return tc;
}



static void _transforms_append(DvzTransformChain* tc, DvzTransform tr)
{
    ASSERT(tc != NULL);
    ASSERT(tc->count < DVZ_TRANSFORM_CHAIN_MAX_SIZE - 1);
    tc->transforms[tc->count++] = tr;
}



static inline void _transforms_apply(DvzTransformChain* tc, dvec3 in, dvec3 out)
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



static DvzTransformChain _transforms_inv(DvzTransformChain* tc)
{
    ASSERT(tc != NULL);
    DvzTransformChain tci = {0};
    tci.count = tc->count;
    for (uint32_t i = 0; i < tc->count; i++)
    {
        tci.transforms[i] = _transform_inv(&tc->transforms[tc->count - 1 - i]);
    }
    return tci;
}



static DvzTransformChain _transforms_cds(DvzPanel* panel, DvzCDS source, DvzCDS target)
{
    ASSERT(panel != NULL);
    DvzTransformChain tc = _transforms();
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
            _transforms_append(&tc, _transform_cds(panel, (DvzCDS)((int32_t)source + i)));
        }
    }
    return tc;
}



#endif
