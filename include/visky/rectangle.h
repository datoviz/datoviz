#ifndef VKY_RECTANGLE_HEADER
#define VKY_RECTANGLE_HEADER

#include "constants.h"
#include "scene.h"



/*************************************************************************************************/
/*  Rectangle visual                                                                             */
/*************************************************************************************************/

typedef struct VkyRectangleParams VkyRectangleParams;
struct VkyRectangleParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

typedef struct VkyRectangleData VkyRectangleData;
struct VkyRectangleData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    vec2 size; // size of the rectangle
    VkyColorBytes color;
};

VKY_EXPORT VkyVisual* vky_visual_rectangle(VkyScene* scene, VkyRectangleParams params);



/*************************************************************************************************/
/*  Area visual                                                                                  */
/*************************************************************************************************/

typedef struct VkyAreaParams VkyAreaParams;
struct VkyAreaParams
{
    vec3 origin;
    vec3 u;
    vec3 v;
};

typedef struct VkyAreaVertex VkyAreaVertex;
struct VkyAreaVertex
{
    vec3 pos;
    VkyColorBytes color;
    uint32_t area_idx;
};

typedef struct VkyAreaData VkyAreaData;
struct VkyAreaData
{
    // position of the lower left corner in the 2D coordinate system defined by (origin, u, v)
    vec2 p;
    float h;
    VkyColorBytes color;
    uint32_t area_idx;
};

VKY_EXPORT VkyVisual* vky_visual_area(VkyScene* scene, VkyAreaParams params);



/*************************************************************************************************/
/*  Axis rectangle visual                                                                        */
/*************************************************************************************************/

typedef struct VkyRectangleAxisData VkyRectangleAxisData;
struct VkyRectangleAxisData
{
    vec2 ab;
    uint8_t span_axis; // 0 or 1
    VkyColorBytes color;
};

VKY_EXPORT VkyVisual* vky_visual_rectangle_axis(VkyScene* scene);



#endif
