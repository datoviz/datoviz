#ifndef VKL_ARRAY_HEADER
#define VKL_ARRAY_HEADER

#include "common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_DTYPE_NONE,

    VKL_DTYPE_CHAR, // 8 bits, unsigned
    VKL_DTYPE_CVEC2,
    VKL_DTYPE_CVEC3,
    VKL_DTYPE_CVEC4,

    VKL_DTYPE_UINT, // 32 bits, unsigned
    VKL_DTYPE_UVEC2,
    VKL_DTYPE_UVEC3,
    VKL_DTYPE_UVEC4,

    VKL_DTYPE_INT, // 32 bits, signed
    VKL_DTYPE_IVEC2,
    VKL_DTYPE_IVEC3,
    VKL_DTYPE_IVEC4,

    VKL_DTYPE_FLOAT, // 32 bits
    VKL_DTYPE_VEC2,
    VKL_DTYPE_VEC3,
    VKL_DTYPE_VEC4,

    VKL_DTYPE_DOUBLE, // 64 bits
    VKL_DTYPE_DVEC2,
    VKL_DTYPE_DVEC3,
    VKL_DTYPE_DVEC4,
} VklDataType;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/



#endif
