/*************************************************************************************************/
/* Volume                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VOLUME
#define DVZ_HEADER_VOLUME



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVolumeVertex DvzVolumeVertex;
typedef struct DvzVolumeParams DvzVolumeParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;
typedef struct DvzShape DvzShape;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVolumeVertex
{
    vec3 pos; /* position */
};

struct DvzVolumeParams
{
    vec4 box_size; /* size of the box containing the volume, in NDC */
    vec4 uvw0;     /* texture coordinates of the 2 corner points */
    vec4 uvw1;     /* texture coordinates of the 2 corner points */
    // vec4 clip;            /* plane normal vector for volume slicing */
    // vec2 transfer_xrange; /* x coords of the endpoints of the transfer function */
    // float color_coef;     /* scaling coefficient when fetching voxel color */
    // int32_t cmap;         /* colormap */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_volume(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_volume_alloc(DvzVisual* volume, uint32_t item_count);



/**
 *
 */
DVZ_EXPORT DvzId dvz_volume_texture(
    DvzVisual* image, uvec3 shape, DvzFormat format, DvzFilter filter, DvzSize size, void* data);



EXTERN_C_OFF

#endif
