/*************************************************************************************************/
/*  Simple transform system for data normalization                                               */
/*************************************************************************************************/

#ifndef DVZ_TRANSFORMS_HEADER
#define DVZ_TRANSFORMS_HEADER

#include "array.h"
#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_BOX_NDC                                                                               \
    (DvzBox)                                                                                      \
    {                                                                                             \
        {-1, -1, -1}, { +1, +1, +1 }                                                              \
    }
#define DVZ_BOX_INF                                                                               \
    (DvzBox)                                                                                      \
    {                                                                                             \
        {+INFINITY, +INFINITY, +INFINITY}, { -INFINITY, -INFINITY, -INFINITY }                    \
    }

#define DVZ_TRANSFORM_CHAIN_MAX_SIZE 32

#define DVZ_TRANSFORM_MATRIX_VULKAN                                                               \
    (dmat4)                                                                                       \
    {                                                                                             \
        {1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, .5, 0}, { 0, 0, .5, 1 }                               \
    }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Transformations.
typedef enum
{
    DVZ_TRANSFORM_NONE,
    DVZ_TRANSFORM_CARTESIAN,
    DVZ_TRANSFORM_POLAR,
    DVZ_TRANSFORM_CYLINDRICAL,
    DVZ_TRANSFORM_SPHERICAL,
    DVZ_TRANSFORM_EARTH_MERCATOR_WEB,
} DvzTransformType;



// Transformation flags.
typedef enum
{
    DVZ_TRANSFORM_FLAGS_NONE = 0x0000,
    DVZ_TRANSFORM_FLAGS_LOGX = 0x0001,
    DVZ_TRANSFORM_FLAGS_LOGY = 0x0002,
    DVZ_TRANSFORM_FLAGS_LOGLOG = 0x0003,
    DVZ_TRANSFORM_FLAGS_FIXED_ASPECT = 0x0008,
} DvzTransformFlags;



// Coordinate systems.
typedef enum
{
    DVZ_CDS_NONE,
    DVZ_CDS_DATA,
    DVZ_CDS_SCENE,
    DVZ_CDS_VULKAN,
    DVZ_CDS_FRAMEBUFFER,
    DVZ_CDS_WINDOW,
} DvzCDS;



// CDS transpose.
typedef enum
{
    DVZ_CDS_TRANSPOSE_NONE,   // x right  y up     z front
    DVZ_CDS_TRANSPOSE_XFYRZU, // x front  y right  z up
    DVZ_CDS_TRANSPOSE_XBYDZL, // x back   y down   z left
    DVZ_CDS_TRANSPOSE_XLYBZD, // x left   y back   z down
} DvzCDSTranspose;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzDataCoords DvzDataCoords;
typedef struct DvzBox DvzBox;
typedef struct DvzTransform DvzTransform;
typedef struct DvzTransformChain DvzTransformChain;

// Forward declarations.
typedef struct DvzPanel DvzPanel;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzBox
{
    dvec3 p0, p1;
};



struct DvzDataCoords
{
    DvzBox box;                // in data coordinate system
    DvzCDSTranspose transpose; // possible transposition of the data coordinate system
    DvzTransformType transform;
    int flags; // come from the panel
    // TODO: union with transform parameters?
};



struct DvzTransform
{
    DvzTransformType type;
    dmat4 mat; // transformation matrix for linear transforms, parameters for other transforms
    int flags; // come from the panel
    bool inverse;
};



struct DvzTransformChain
{
    uint32_t count;
    DvzTransform transforms[DVZ_TRANSFORM_CHAIN_MAX_SIZE];
};



/*************************************************************************************************/
/*  Transposition functions                                                                      */
/*************************************************************************************************/

#define MAKE_TRANSPOSE(T)                                                                         \
    static void _transpose_##T(DvzCDSTranspose transpose, T* src, T* dst)                         \
    {                                                                                             \
        ASSERT(src != NULL);                                                                      \
        ASSERT(dst != NULL);                                                                      \
        T src_ = {0};                                                                             \
        memcpy(&src_, src, sizeof(T));                                                            \
        switch (transpose)                                                                        \
        {                                                                                         \
                                                                                                  \
        case DVZ_CDS_TRANSPOSE_NONE:                                                              \
            memcpy(dst, src_, sizeof(T));                                                         \
            break;                                                                                \
                                                                                                  \
        case DVZ_CDS_TRANSPOSE_XBYDZL:                                                            \
            (*dst)[0] = -src_[2];                                                                 \
            (*dst)[1] = -src_[1];                                                                 \
            (*dst)[2] = -src_[0];                                                                 \
            break;                                                                                \
                                                                                                  \
        case DVZ_CDS_TRANSPOSE_XFYRZU:                                                            \
            (*dst)[0] = -src_[1];                                                                 \
            (*dst)[1] = +src_[2];                                                                 \
            (*dst)[2] = +src_[0];                                                                 \
            break;                                                                                \
                                                                                                  \
        case DVZ_CDS_TRANSPOSE_XLYBZD:                                                            \
            (*dst)[0] = -src_[0];                                                                 \
            (*dst)[1] = -src_[2];                                                                 \
            (*dst)[2] = -src_[1];                                                                 \
            break;                                                                                \
                                                                                                  \
        default:                                                                                  \
            log_error("unknown CDS transpose %d", transpose);                                     \
            break;                                                                                \
        }                                                                                         \
    }

MAKE_TRANSPOSE(dvec3)
MAKE_TRANSPOSE(vec3)



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Apply a CPU builtin transformation on position data.
 *
 * @param coords the data coordinate system and bounds
 * @param pos_in input array of dvec3 values
 * @param[out] pos_out output array of dvec3 values
 * @param inverse whether to use the inverse or forward transformation
 */
DVZ_EXPORT void
dvz_transform_pos(DvzDataCoords coords, DvzArray* pos_in, DvzArray* pos_out, bool inverse);

/**
 * Convert a 3D position from a coordinate system to another.
 *
 * @param panel the panel
 * @param source the source coordinate system
 * @param in the input position
 * @param target the target coordinate system
 * @param[out] out the output (transformed) position
 */
DVZ_EXPORT void dvz_transform(DvzPanel* panel, DvzCDS source, dvec3 in, DvzCDS target, dvec3 out);



#endif
