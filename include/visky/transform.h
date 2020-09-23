#ifndef VKY_TRANSFORM_HEADER
#define VKY_TRANSFORM_HEADER

#include "scene.h"
#include "types.h"


/*************************************************************************************************/
/*  Transform definitions                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_CDS_DATA = 1,       // data coordinate system
    VKY_CDS_GPU = 2,        // data coordinates normalized to -1,+1 and sent to the GPU
    VKY_CDS_PANZOOM = 3,    // normalized coords within the panel inner's viewport (w/ panzoom)
    VKY_CDS_PANEL = 4,      // NDC coordinates within the outer panel viewport
    VKY_CDS_CANVAS_NDC = 5, // normalized coords within the canvas
    VKY_CDS_CANVAS_PX = 6,  // same but in pixels, origin at the upper left
} VkyCDS;


typedef struct VkyAxesTransform VkyAxesTransform;

struct VkyAxesTransform
{
    dvec2 scale, shift;
};



/*************************************************************************************************/
/*  Transform functions                                                                          */
/*************************************************************************************************/

VkyAxesTransform vky_axes_transform(VkyPanel* panel, VkyCDS source, VkyCDS target);

VkyAxesTransform vky_axes_transform_inv(VkyAxesTransform);

VkyAxesTransform vky_axes_transform_mul(VkyAxesTransform, VkyAxesTransform);

VkyAxesTransform vky_axes_transform_interp(dvec2 pin, dvec2 pout, dvec2 qin, dvec2 qout);

void vky_axes_transform_apply(VkyAxesTransform*, dvec2 in, dvec2 out);


#endif
