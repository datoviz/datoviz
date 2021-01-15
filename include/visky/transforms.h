#ifndef VKL_TRANSFORMS_HEADER
#define VKL_TRANSFORMS_HEADER

#include "array.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



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
    // TODO: double
    vec2 xlim;
    vec2 ylim;
    vec2 zlim;
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

// Return the bounding box of a set of vec3 points.
static VklBox _box_bounding(VklArray* points_in)
{
    ASSERT(points_in != NULL);
    ASSERT(points_in->item_count > 0);
    ASSERT(points_in->item_size > 0);

    float xmin = INFINITY;
    float ymin = INFINITY;
    float zmin = INFINITY;

    float xmax = -INFINITY;
    float ymax = -INFINITY;
    float zmax = -INFINITY;

    vec3* pos = NULL;
    for (uint32_t i = 0; i < points_in->item_count; i++)
    {
        pos = (vec3*)vkl_array_item(points_in, i);
        xmin = MIN(xmin, (*pos)[0]);
        xmax = MAX(xmax, (*pos)[0]);
        ymin = MIN(ymin, (*pos)[1]);
        ymax = MAX(ymax, (*pos)[1]);
        zmin = MIN(zmin, (*pos)[2]);
        zmax = MAX(zmax, (*pos)[2]);
    }
    VklBox box = {{xmin, xmax}, {ymin, ymax}, {zmin, zmax}};
    return box;
}



static VklBox _box_merge(uint32_t count, VklBox* boxes)
{
    VklBox merged = {{INFINITY, -INFINITY}, {INFINITY, -INFINITY}, {INFINITY, -INFINITY}};
    for (uint32_t i = 0; i < count; i++)
    {
        merged.xlim[0] = MIN(merged.xlim[0], boxes[i].xlim[0]);
        merged.xlim[1] = MAX(merged.xlim[1], boxes[i].xlim[1]);
        merged.ylim[0] = MIN(merged.ylim[0], boxes[i].ylim[0]);
        merged.ylim[1] = MAX(merged.ylim[1], boxes[i].ylim[1]);
        merged.zlim[0] = MIN(merged.zlim[0], boxes[i].zlim[0]);
        merged.zlim[1] = MAX(merged.zlim[1], boxes[i].zlim[1]);
    }
    return merged;
}



static void _box_print(VklBox box)
{
    log_info(
        "box x [%.3f, %.3f], y [%.3f, %.3f], z [%.3f, %.3f]", //
        box.xlim[0], box.xlim[1], box.ylim[0], box.ylim[1], box.zlim[0], box.zlim[1]);
}



// Make a box cubic/square (if need to keep fixed aspect ratio).
static VklBox _box_cube(VklBox box)
{
    float xmin = box.xlim[0];
    float xmax = box.xlim[1];
    float ymin = box.ylim[0];
    float ymax = box.ylim[1];
    float zmin = box.zlim[0];
    float zmax = box.zlim[1];

    float xcenter = .5 * (xmin + xmax);
    float ycenter = .5 * (ymin + ymax);
    float zcenter = .5 * (zmin + zmax);

    ASSERT(xmin <= xcenter && xcenter <= xmax);
    ASSERT(ymin <= ycenter && ycenter <= ymax);
    ASSERT(zmin <= zcenter && zcenter <= zmax);

    float edge = 0;
    float edge_x = MAX(xmax - xcenter, xcenter - xmin);
    float edge_y = MAX(ymax - ycenter, ycenter - ymin);
    float edge_z = MAX(zmax - zcenter, zcenter - zmin);

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
    out.xlim[0] = xcenter - edge_x;
    out.xlim[1] = xcenter + edge_x;
    // }
    // if (edge_y > 0)
    // {
    out.ylim[0] = ycenter - edge_y;
    out.ylim[1] = ycenter + edge_y;
    // }
    // if (edge_z > 0)
    // {
    out.zlim[0] = zcenter - edge_z;
    out.zlim[1] = zcenter + edge_z;
    // }
    return out;
}



static void _normalize_pos(VklBox box, VklArray* points_in, VklArray* points_out)
{
    ASSERT(points_out->item_count == points_in->item_count);
    DBG(points_out->item_size);
    DBG(points_in->item_size);
    ASSERT(points_out->item_size == points_in->item_size);

    vec3* pos_in = NULL;
    vec3* pos_out = NULL;

    for (uint32_t i = 0; i < points_in->item_count; i++)
    {
        pos_in = vkl_array_item(points_in, i);
        pos_out = vkl_array_item(points_out, i);

        (*pos_out)[0] = -1.0 + 2.0 * ((*pos_in)[0] - box.xlim[0]) / (box.xlim[1] - box.xlim[0]);
        (*pos_out)[1] = -1.0 + 2.0 * ((*pos_in)[1] - box.ylim[0]) / (box.ylim[1] - box.ylim[0]);
        (*pos_out)[2] = -1.0 + 2.0 * ((*pos_in)[2] - box.zlim[0]) / (box.zlim[1] - box.zlim[0]);
    }
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vkl_transform(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out);



#endif
