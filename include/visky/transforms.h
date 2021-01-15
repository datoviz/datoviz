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
    vec2 xlim;
    vec2 ylim;
    vec2 zlim;
};



struct VklDataCoords
{
    dvec4 data; // (blx, bly, trx, try)
    vec4 gpu;   // (blx, bly, trx, try)
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



// Return the smallest cube surrounding a box.
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
    edge = MAX(edge, MAX(xmax - xcenter, xcenter - xmin));
    edge = MAX(edge, MAX(ymax - ycenter, ycenter - ymin));
    edge = MAX(edge, MAX(zmax - zcenter, zcenter - zmin));
    if (edge == 0)
        edge = 1;
    ASSERT(edge > 0);

    VklBox out = {0};
    out.xlim[0] = xcenter - edge;
    out.xlim[1] = xcenter + edge;
    out.ylim[0] = ycenter - edge;
    out.ylim[1] = ycenter + edge;
    out.zlim[0] = zcenter - edge;
    out.zlim[1] = zcenter + edge;

    return out;
}



static void _normalize_pos(VklBox box, VklArray* points_in, VklArray* points_out)
{
    ASSERT(points_out->item_count == points_in->item_count);
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



#endif
