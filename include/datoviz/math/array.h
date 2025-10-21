/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Array API                                                                                    */
/*  Provides a simplistic 1D array object mostly used by the Visual API                          */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_obj.h"
#include "datoviz/math/types.h"



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
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a new 1D array.
 *
 * @param item_count initial number of elements
 * @param dtype the data type of the array
 * @returns a new array
 */
DvzArray* dvz_array(uint32_t item_count, DvzDataType dtype);



/**
 * Create a copy of an existing array.
 *
 * @param arr an array
 * @returns a new array
 */
DvzArray* dvz_array_copy(DvzArray* arr);



/**
 * Create an array with a single dvec3 position.
 *
 * @param pos initial number of elements
 * @returns a new array
 */
DvzArray* dvz_array_point(dvec3 pos);



/**
 * Create a 1D array from an existing compatible memory buffer.
 *
 * The created array does not allocate memory, it uses the passed buffer instead.
 *
 * !!! warning
 *     Destroying the array will free the passed pointer!
 *
 * @param item_count number of elements in the passed buffer
 * @param dtype the data type of the array
 * @returns the array wrapping the buffer
 */
DvzArray* dvz_array_wrap(uint32_t item_count, DvzDataType dtype, void* data);



/**
 * Create a 1D record array with heterogeneous data type.
 *
 * @param item_count number of elements
 * @param item_size size, in bytes, of each item
 * @returns the array
 */
DvzArray* dvz_array_struct(uint32_t item_count, DvzSize item_size);



/**
 * Create a 3D array holding a texture.
 *
 * @param ndims number of dimensions (1, 2, 3)
 * @param width number of elements along the 1st dimension
 * @param height number of elements along the 2nd dimension
 * @param depth number of elements along the 3rd dimension
 * @param item_size size of each item in bytes
 * @returns the array
 */
DvzArray*
dvz_array_3D(uint32_t ndims, uint32_t width, uint32_t height, uint32_t depth, DvzSize item_size);



/**
 * Resize an existing array.
 *
 * * If the new size is equal to the old size, do nothing.
 * * If the new size is smaller than the old size, change the size attribute but do not reallocate
 * * If the new size is larger than the old size, reallocate memory and copy over the old values
 *
 * @param array the array to resize
 * @param item_count the new number of items
 */
void dvz_array_resize(DvzArray* array, uint32_t item_count);



/**
 * Reset to 0 the contents of an existing array.
 *
 * @param array the array to clear
 */
void dvz_array_clear(DvzArray* array);



/**
 * Reshape a 3D array and *delete all the data in it*.
 *
 * !!! warning
 *     The contents of the array will be cleared. Copying the existing data would require more work
 *     and is not necessary at the moment.
 *
 * @param array the array to reshape and clear
 * @param width number of elements along the 1st dimension
 * @param height number of elements along the 2nd dimension
 * @param depth number of elements along the 3rd dimension
 */
void dvz_array_reshape(DvzArray* array, uint32_t width, uint32_t height, uint32_t depth);



/**
 * Insert data in an array.
 *
 * @param array the array
 * @param offset the index of the first element of the inserted data in the new array
 * @param size the number of elements to insert
 * @param insert the data to insert
 */
void dvz_array_insert(DvzArray* array, uint32_t offset, uint32_t size, void* insert);



/**
 * Copy a region of an array into another.
 *
 * @param src_arr the source array
 * @param dst_arr the destination array
 * @param src_offset the index, in the source array, of the first item to copy
 * @param dst_offset the destination index
 * @param item_count the number of items to copy
 */
void dvz_array_copy_region(
    DvzArray* src_arr, DvzArray* dst_arr, uint32_t src_offset, uint32_t dst_offset,
    uint32_t item_count);



/**
 * Copy data into an array.
 *
 * * There will be `item_count` values copied between `first_item` and `first_item + item_count` in
 *   the array.
 * * There are `data_item_count` values in the passed buffer.
 * * If `item_count > data_item_count`, the last value of `data` will be repeated until the last
 * value.
 *
 * Example:
 *
 * === "C"
 *     ```c
 *     // Create an array of 10 double numbers, initialize all elements with 1.23.
 *     DvzArray* arr = dvz_array(10, DVZ_DTYPE_DOUBLE);
 *     double item = 1.23;
 *     dvz_array_data(&arr, 0, 10, 1, &item);
 *     ```
 *
 * @param array the array
 * @param first_item first element in the array to be overwritten
 * @param item_count number of items to write
 * @param data_item_count number of elements in `data`
 * @param data the buffer containing the data to copy
 */
void dvz_array_data(
    DvzArray* array, uint32_t first_item, uint32_t item_count, //
    uint32_t data_item_count, const void* data);



/**
 * Multiply all array elements by a scaling factor.
 *
 * !!! note
 *     Only FLOAT dtype is supported for now.
 *
 * @param array the array
 * @param scaling scaling factor
 */
void dvz_array_scale(DvzArray* arr, float scaling);



/**
 * Copy data into the column of a record array.
 *
 * This function is used by the default visual baking function, which copies to the vertex buffer
 * (corresponding to a record array with as many fields as GLSL attributes in the vertex shader)
 * the user-specified visual props (data for the individual elements).
 *
 * @param array the array
 * @param offset the offset of the column start within an array item, in bytes
 * @param col_size stride in the source array, in bytes
 * @param first_item first element in the array to be overwritten
 * @param item_count number of elements to write
 * @param data_item_count number of elements in `data`
 * @param data the buffer containing the data to copy
 * @param source_dtype the source dtype (only used when casting)
 * @param target_dtype the target dtype (only used when casting)
 * @param copy_type the type of copy
 * @param reps the number of repeats for each copied element
 */
void dvz_array_column(
    DvzArray* array, DvzSize offset, DvzSize col_size,  //
    uint32_t first_item, uint32_t item_count,           //
    uint32_t data_item_count, const void* data,         //
    DvzDataType source_dtype, DvzDataType target_dtype, //
    DvzArrayCopyType copy_type, uint32_t reps);



void dvz_array_print(DvzArray* array);



/**
 * Destroy an array.
 *
 * This function frees the allocated underlying data buffer.
 *
 * @param array the array to destroy
 */
void dvz_array_destroy(DvzArray* array);



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
    ANN(array);
    idx = CLIP(idx, 0, array->item_count - 1);
    return (void*)((int64_t)array->data + (int64_t)(idx * array->item_size));
}
