#ifndef VKL_VISUALS_HEADER
#define VKL_VISUALS_HEADER

#include "vklite2.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum {
    VKL_PROP_NONE,
    VKL_PROP_POS,
    VKL_PROP_COLOR,
    VKL_PROP_TYPE,
} VklPropType;


typedef enum {
    VKL_DTYPE_NONE,

    VKL_DTYPE_FLOAT, // 32 bits
    VKL_DTYPE_VEC2,
    VKL_DTYPE_VEC3,
    VKL_DTYPE_VEC4,

    VKL_DTYPE_DOUBLE, // 64 bits
    VKL_DTYPE_DVEC2,
    VKL_DTYPE_DVEC3,
    VKL_DTYPE_DVEC4,

    VKL_DTYPE_UINT, // 32 bits, unsigned
    VKL_DTYPE_UVEC2,
    VKL_DTYPE_UVEC3,
    VKL_DTYPE_UVEC4,

    VKL_DTYPE_CHAR, // 8 bits, unsigned
    VKL_DTYPE_CVEC2,
    VKL_DTYPE_CVEC3,
    VKL_DTYPE_CVEC4,

    VKL_DTYPE_INT, // 32 bits, signed
    VKL_DTYPE_IVEC2,
    VKL_DTYPE_IVEC3,
    VKL_DTYPE_IVEC4,
} VklDataType;


typedef enum {
    VKL_PROP_LOC_NONE,

    VKL_PROP_LOC_ATTRIBUTE,
    VKL_PROP_LOC_UNIFORM,
    VKL_PROP_LOC_STORAGE,
    VKL_PROP_LOC_SAMPLER,
    VKL_PROP_LOC_PUSH,
} VklPropLoc;


typedef enum {
    VKL_PROP_BINDING_NONE,
    VKL_PROP_BINDING_CPU,
    VKL_PROP_BINDING_BUFFER, // only compatible with prop loc attribute, uniform, storage
    VKL_PROP_BINDING_TEXTURE, // only compatible with prop loc sampler
} VklPropBinding;


typedef enum {
    VKL_VISUAL_VARIANT_NONE,
    VKL_VISUAL_VARIANT_RAW,
    VKL_VISUAL_VARIANT_AGG,
    VKL_VISUAL_VARIANT_TEXTURED,
    VKL_VISUAL_VARIANT_TEXTURED_MULTI,
    VKL_VISUAL_VARIANT_SHADED,
} VklVisualVariant;




typedef enum {
    VKL_VISUAL_NONE,

    VKL_VISUAL_SCATTER, // raw, agg
    VKL_VISUAL_SEGMENT, // raw, agg
    VKL_VISUAL_ARROW,
    VKL_VISUAL_PATH, // raw, agg
    VKL_VISUAL_TEXT, // raw, agg
    VKL_VISUAL_TRIANGLE,
    VKL_VISUAL_RECTANGLE,
    VKL_VISUAL_IMAGE, // single, multi
    VKL_VISUAL_DISC,
    VKL_VISUAL_SECTOR,
    VKL_VISUAL_MESH, // raw, textured, textured_multi, shaded
    VKL_VISUAL_POLYGON,
    VKL_VISUAL_PSLG,
    VKL_VISUAL_HISTOGRAM,
    VKL_VISUAL_AREA,
    VKL_VISUAL_CANDLE,
    VKL_VISUAL_GRAPH,
    VKL_VISUAL_SURFACE,
    VKL_VISUAL_VOLUME,
    VKL_VISUAL_FAKE_SPHERE,
} VklVisualBuiltin;







/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/





/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/





/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/




#endif
