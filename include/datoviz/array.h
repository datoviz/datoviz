/*************************************************************************************************/
/*  Array API                                                                                    */
/*  Provides a simplistic 1D array object mostly used by the Visual API                          */
/*************************************************************************************************/

#ifndef DVZ_HEADER_ARRAY
#define DVZ_HEADER_ARRAY



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzArray DvzArray;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Data types.
typedef enum
{
    DVZ_DTYPE_NONE,
    DVZ_DTYPE_CUSTOM, // used for structured arrays (aka record arrays)
    DVZ_DTYPE_STR,    // 64 bits, pointer

    DVZ_DTYPE_CHAR, // 8 bits, unsigned int
    DVZ_DTYPE_CVEC2,
    DVZ_DTYPE_CVEC3,
    DVZ_DTYPE_CVEC4,

    DVZ_DTYPE_USHORT, // 16 bits, unsigned int
    DVZ_DTYPE_USVEC2,
    DVZ_DTYPE_USVEC3,
    DVZ_DTYPE_USVEC4,

    DVZ_DTYPE_SHORT, // 16 bits, signed int
    DVZ_DTYPE_SVEC2,
    DVZ_DTYPE_SVEC3,
    DVZ_DTYPE_SVEC4,

    DVZ_DTYPE_UINT, // 32 bits, unsigned int
    DVZ_DTYPE_UVEC2,
    DVZ_DTYPE_UVEC3,
    DVZ_DTYPE_UVEC4,

    DVZ_DTYPE_INT, // 32 bits, signed int
    DVZ_DTYPE_IVEC2,
    DVZ_DTYPE_IVEC3,
    DVZ_DTYPE_IVEC4,

    DVZ_DTYPE_FLOAT, // 32 bits float
    DVZ_DTYPE_VEC2,
    DVZ_DTYPE_VEC3,
    DVZ_DTYPE_VEC4,

    DVZ_DTYPE_DOUBLE, // 64 bits double
    DVZ_DTYPE_DVEC2,
    DVZ_DTYPE_DVEC3,
    DVZ_DTYPE_DVEC4,

    DVZ_DTYPE_MAT2, // matrices of floats
    DVZ_DTYPE_MAT3,
    DVZ_DTYPE_MAT4,
} DvzDataType;



// Array copy types.
typedef enum
{
    DVZ_ARRAY_COPY_NONE,
    DVZ_ARRAY_COPY_REPEAT,
    DVZ_ARRAY_COPY_SINGLE,
} DvzArrayCopyType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzArray
{
    DvzObject obj;
    DvzDataType dtype;
    uint32_t components; // number of components, ie 2 for vec2, 3 for dvec3, etc.
    DvzSize item_size;
    uint32_t item_count;
    DvzSize buffer_size;
    void* data;

    // 3D arrays
    uint32_t ndims; // 1, 2, or 3
    uvec3 shape;    // only for 3D arrays
};



/*************************************************************************************************/
/*  Inline functions                                                                             */
/*************************************************************************************************/

/**
 * Retrieve a single element from an array.
 *
 * @param array the array
 * @param idx the index of the element to retrieve
 * @returns a pointer to the requested element
 */
static inline void* dvz_array_item(DvzArray* array, uint32_t idx)
{
    ASSERT(array != NULL);
    idx = CLIP(idx, 0, array->item_count - 1);
    return (void*)((int64_t)array->data + (int64_t)(idx * array->item_size));
}



#endif
