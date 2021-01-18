#ifndef VKL_TRANSFORMS_HEADER
#define VKL_TRANSFORMS_HEADER

#include "array.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define VKL_BOX_NDC                                                                               \
    (VklBox)                                                                                      \
    {                                                                                             \
        {-1, -1, -1}, { +1, +1, +1 }                                                              \
    }
#define VKL_BOX_INF                                                                               \
    (VklBox)                                                                                      \
    {                                                                                             \
        {+INFINITY, +INFINITY, +INFINITY}, { -INFINITY, -INFINITY, -INFINITY }                    \
    }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_TRANSFORM_NONE,
    VKL_TRANSFORM_CARTESIAN,
    VKL_TRANSFORM_POLAR,
    VKL_TRANSFORM_CYLINDRICAL,
    VKL_TRANSFORM_SPHERICAL,
    VKL_TRANSFORM_EARTH_MERCATOR_WEB,
} VklTransform;



typedef enum
{
    VKL_TRANSFORM_FLAGS_NONE = 0x0000,
    VKL_TRANSFORM_FLAGS_LOGX = 0x0001,
    VKL_TRANSFORM_FLAGS_LOGY = 0x0002,
    VKL_TRANSFORM_FLAGS_LOGLOG = 0x0003,
    VKL_TRANSFORM_FLAGS_FIXED_ASPECT = 0x0010,
} VklTransformFlags;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklDataCoords VklDataCoords;
typedef struct VklBox VklBox;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklBox
{
    // dvec2 xlim;
    // dvec2 ylim;
    // dvec2 zlim;
    dvec3 p0, p1;
};



struct VklDataCoords
{
    VklBox box; // in data coordinate system
    VklTransform transform;
    int flags;
    // TODO: union with transform parameters?
};



/*************************************************************************************************/
/*  Position normalization                                                                       */
/*************************************************************************************************/

// TODO: use double instead

// Return the bounding box of a set of dvec3 points.
static VklBox _box_bounding(VklArray* points_in)
{
    ASSERT(points_in != NULL);
    ASSERT(points_in->item_count > 0);
    ASSERT(points_in->item_size > 0);

    // double xmin = INFINITY;
    // double ymin = INFINITY;
    // double zmin = INFINITY;

    // double xmax = -INFINITY;
    // double ymax = -INFINITY;
    // double zmax = -INFINITY;

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



// static void _box_print(VklBox box)
// {
//     log_info(
//         "box x [%.3f, %.3f], y [%.3f, %.3f], z [%.3f, %.3f]", //
//         box.xlim[0], box.xlim[1], box.ylim[0], box.ylim[1], box.zlim[0], box.zlim[1]);
// }



// Make a box cubic/square (if need to keep fixed aspect ratio).
static VklBox _box_cube(VklBox box)
{
    double xmin = box.p0[0];
    double xmax = box.p1[0];
    double ymin = box.p0[1];
    double ymax = box.p1[1];
    double zmin = box.p0[2];
    double zmax = box.p1[2];

    double xcenter = .5 * (xmin + xmax);
    double ycenter = .5 * (ymin + ymax);
    double zcenter = .5 * (zmin + zmax);

    ASSERT(xmin <= xcenter && xcenter <= xmax);
    ASSERT(ymin <= ycenter && ycenter <= ymax);
    ASSERT(zmin <= zcenter && zcenter <= zmax);

    double edge = 0;
    double edge_x = MAX(xmax - xcenter, xcenter - xmin);
    double edge_y = MAX(ymax - ycenter, ycenter - ymin);
    double edge_z = MAX(zmax - zcenter, zcenter - zmin);

    edge = MAX(edge, edge_x);
    edge = MAX(edge, edge_y);
    edge = MAX(edge, edge_z);
    if (edge == 0)
        edge = 1;
    ASSERT(edge > 0);

    // Find the edge on each axis. Do not extend the range if an axis range is degenerate.
    edge_x = edge_x > 0 ? edge : edge_x;
    edge_y = edge_y > 0 ? edge : edge_y;
    edge_z = edge_z > 0 ? edge : edge_z;

    VklBox out = box;
    // if (edge_x > 0)
    // {
    out.p0[0] = xcenter - edge_x;
    out.p1[0] = xcenter + edge_x;
    // }
    // if (edge_y > 0)
    // {
    out.p0[1] = ycenter - edge_y;
    out.p1[1] = ycenter + edge_y;
    // }
    // if (edge_z > 0)
    // {
    out.p0[2] = zcenter - edge_z;
    out.p1[2] = zcenter + edge_z;
    // }
    return out;
}



// static inline void _transform_point_linear(dvec2* in_lim, double* in, dvec2* out_lim, double*
// out)
// {
//     *out = (*out_lim)[0] +
//            (out_lim[1] - out_lim[0]) * ((*in) - (*in_lim)[0]) / ((*in_lim)[1] - (*in_lim)[0]);
// }



static void _transform_linear(
    VklBox box_in, VklArray* points_in, //
    VklBox box_out, VklArray* points_out)
{
    ASSERT(points_out->item_count == points_in->item_count);
    ASSERT(points_out->item_size == points_in->item_size);

    ASSERT(points_out->dtype == points_in->dtype);
    ASSERT(
        points_out->dtype == VKL_DTYPE_DOUBLE || //
        points_out->dtype == VKL_DTYPE_DVEC2 ||  //
        points_out->dtype == VKL_DTYPE_DVEC3 ||  //
        points_out->dtype == VKL_DTYPE_DVEC4     //
    );

    const uint32_t components = points_in->components;
    ASSERT(points_out->components == components);
    ASSERT(1 <= components && components <= 4);

    dvec3* pos_in = NULL;
    dvec3* pos_out = NULL;

    dvec3 a = {0};
    dvec3 b = {0};
    for (uint32_t j = 0; j < components; j++)
    {
        a[j] = (box_out.p1[j] - box_out.p0[j]) / (box_in.p1[j] - box_in.p0[j]);
        b[j] = box_out.p0[j] * box_in.p1[j] - box_out.p1[j] * box_in.p0[j];
    }

    for (uint32_t i = 0; i < points_in->item_count; i++)
    {
        pos_in = (dvec3*)vkl_array_item(points_in, i);
        pos_out = (dvec3*)vkl_array_item(points_out, i);

        for (uint32_t j = 0; j < components; j++)
            (*pos_out)[j] = a[j] * (*pos_in)[j] + b[j];
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vkl_transform(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out);



#endif
