/*************************************************************************************************/
/* Slice                                                                                         */
/*************************************************************************************************/

#ifndef DVZ_HEADER_SLICE
#define DVZ_HEADER_SLICE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../viewport.h"
#include "../visual.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzSliceVertex DvzSliceVertex;
typedef struct DvzSliceParams DvzSliceParams;

// Forward declarations.
typedef struct DvzBatch DvzBatch;
typedef struct DvzVisual DvzVisual;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzSliceVertex
{
    vec3 pos; /* position */
    vec3 uvw; /* texture coordinates */
};

struct DvzSliceParams
{
    float alpha;
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 *
 */
DVZ_EXPORT DvzVisual* dvz_slice(DvzBatch* batch, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_slice_position(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* p0, vec3* p1, vec3* p2, vec3* p3, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_slice_texcoords(
    DvzVisual* slice, uint32_t first, uint32_t count, //
    vec3* uvw0, vec3* uvw1, vec3* uvw2, vec3* uvw3, int flags);



/**
 *
 */
DVZ_EXPORT void dvz_slice_texture(
    DvzVisual* visual, DvzId tex, DvzFilter filter, DvzSamplerAddressMode address_mode);



/**
 *
 */
DVZ_EXPORT void dvz_slice_alloc(DvzVisual* slice, uint32_t item_count);



/**
 *
 */
DVZ_EXPORT DvzId dvz_tex_slice(
    DvzBatch* batch, DvzFormat format, uint32_t width, uint32_t height, uint32_t depth,
    void* data);



/**
 *
 */
DVZ_EXPORT void dvz_slice_alpha(DvzVisual* visual, float alpha);



EXTERN_C_OFF

#endif
