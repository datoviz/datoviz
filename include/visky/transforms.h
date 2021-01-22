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

#define VKL_TRANSFORM_CHAIN_MAX_SIZE 32

#define VKL_TRANSFORM_MATRIX_VULKAN                                                               \
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
    VKL_TRANSFORM_NONE,
    VKL_TRANSFORM_CARTESIAN,
    VKL_TRANSFORM_POLAR,
    VKL_TRANSFORM_CYLINDRICAL,
    VKL_TRANSFORM_SPHERICAL,
    VKL_TRANSFORM_EARTH_MERCATOR_WEB,
} VklTransformType;



// Transformation flags.
typedef enum
{
    VKL_TRANSFORM_FLAGS_NONE = 0x0000,
    VKL_TRANSFORM_FLAGS_LOGX = 0x0001,
    VKL_TRANSFORM_FLAGS_LOGY = 0x0002,
    VKL_TRANSFORM_FLAGS_LOGLOG = 0x0003,
    VKL_TRANSFORM_FLAGS_FIXED_ASPECT = 0x0008,
} VklTransformFlags;



// Coordinate systems.
typedef enum
{
    VKL_CDS_NONE,
    VKL_CDS_DATA,
    VKL_CDS_SCENE,
    VKL_CDS_VULKAN,
    VKL_CDS_FRAMEBUFFER,
    VKL_CDS_WINDOW,
} VklCDS;



typedef enum
{
    VKL_CDS_TRANSPOSE_DEFAULT, // x right  y up     z front
    VKL_CDS_TRANSPOSE_XFYRZU,  // x front  y right  z up
    VKL_CDS_TRANSPOSE_XBYDZL,  // x back   y down   z left
    VKL_CDS_TRANSPOSE_XLYBZD,  // x left   y back   z down
} VklCDSTranspose;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklDataCoords VklDataCoords;
typedef struct VklBox VklBox;
typedef struct VklTransform VklTransform;
typedef struct VklTransformChain VklTransformChain;

// Forward declarations.
typedef struct VklPanel VklPanel;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklBox
{
    dvec3 p0, p1;
};



struct VklDataCoords
{
    VklBox box;                // in data coordinate system
    VklCDSTranspose transpose; // possible transposition of the data coordinate system
    VklTransformType transform;
    int flags; // come from the panel
    // TODO: union with transform parameters?
};



struct VklTransform
{
    VklTransformType type;
    dmat4 mat; // transformation matrix for linear transforms, parameters for other transforms
    int flags; // come from the panel
    bool inverse;
};



struct VklTransformChain
{
    uint32_t count;
    VklTransform transforms[VKL_TRANSFORM_CHAIN_MAX_SIZE];
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void
vkl_transform_data(VklDataCoords coords, VklArray* pos_in, VklArray* pos_out, bool inverse);

VKY_EXPORT void vkl_transform(VklPanel* panel, VklCDS source, dvec3 in, VklCDS target, dvec3 out);



#endif
