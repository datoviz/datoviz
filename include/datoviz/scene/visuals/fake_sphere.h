/*************************************************************************************************/
/* FakeSphere                                                                                    */
/*************************************************************************************************/

#ifndef DVZ_HEADER_FAKE_SPHERE
#define DVZ_HEADER_FAKE_SPHERE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFakeSphereVertex DvzFakeSphereVertex;
typedef struct DvzFakeSphereParams DvzFakeSphereParams;

// Forward declarations.
typedef struct DvzRequester DvzRequester;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFakeSphereVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* size */
};



struct DvzFakeSphereParams
{
    vec4 light_pos;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_fake_sphere(DvzRequester* rqr, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_position(DvzVisual* visual, uint32_t first, uint32_t count, vec3* pos, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_color(DvzVisual* visual, uint32_t first, uint32_t count, cvec4* color, int flags);



/**
 *
 */
DVZ_EXPORT void
dvz_fake_sphere_size(DvzVisual* visual, uint32_t first, uint32_t count, float* size, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_fake_sphere_alloc(DvzVisual* visual, uint32_t item_count);



/**
 *
 */
DVZ_EXPORT void dvz_fake_sphere_light_pos(DvzVisual* visual, vec3 pos);



EXTERN_C_OFF

#endif
