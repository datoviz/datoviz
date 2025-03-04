/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Array API                                                                                    */
/*  Provides a simplistic 1D array object mostly used by the Visual API                          */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "scene/array.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

// Size in bytes of a single element of a given dtype.
static DvzSize _get_dtype_size(DvzDataType dtype)
{
    switch (dtype)
    {
    // 8 bits
    case DVZ_DTYPE_CHAR:
        return 1;
    case DVZ_DTYPE_CVEC2:
        return 1 * 2;
    case DVZ_DTYPE_CVEC3:
        return 1 * 3;
    case DVZ_DTYPE_CVEC4:
        return 1 * 4;

    // 16 bits
    case DVZ_DTYPE_USHORT:
    case DVZ_DTYPE_SHORT:
        return 2;
    case DVZ_DTYPE_SVEC2:
    case DVZ_DTYPE_USVEC2:
        return 2 * 2;
    case DVZ_DTYPE_SVEC3:
    case DVZ_DTYPE_USVEC3:
        return 2 * 3;
    case DVZ_DTYPE_SVEC4:
    case DVZ_DTYPE_USVEC4:
        return 2 * 4;

    // 32 bits
    case DVZ_DTYPE_FLOAT:
    case DVZ_DTYPE_UINT:
    case DVZ_DTYPE_INT:
        return 4;

    case DVZ_DTYPE_VEC2:
    case DVZ_DTYPE_UVEC2:
    case DVZ_DTYPE_IVEC2:
        return 4 * 2;

    case DVZ_DTYPE_VEC3:
    case DVZ_DTYPE_UVEC3:
    case DVZ_DTYPE_IVEC3:
        return 4 * 3;

    case DVZ_DTYPE_VEC4:
    case DVZ_DTYPE_UVEC4:
    case DVZ_DTYPE_IVEC4:
        return 4 * 4;

    // 64 bits
    case DVZ_DTYPE_DOUBLE:
        return 8;
    case DVZ_DTYPE_DVEC2:
        return 8 * 2;
    case DVZ_DTYPE_DVEC3:
        return 8 * 3;
    case DVZ_DTYPE_DVEC4:
        return 8 * 4;
    case DVZ_DTYPE_STR:
        return sizeof(char*);

    case DVZ_DTYPE_MAT2:
        return 2 * 2 * 4;
    case DVZ_DTYPE_MAT3:
        return 3 * 3 * 4;
    case DVZ_DTYPE_MAT4:
        return 4 * 4 * 4;

    default:
        break;
    }

    if (dtype != DVZ_DTYPE_NONE)
        log_trace("could not find the size of dtype %d, are we creating a struct array?", dtype);
    return 0;
}



// Number of components in a given dtype (e.g. 4 for vec4)
static uint32_t _get_components(DvzDataType dtype)
{
    switch (dtype)
    {
    case DVZ_DTYPE_CHAR:
    case DVZ_DTYPE_USHORT:
    case DVZ_DTYPE_SHORT:
    case DVZ_DTYPE_UINT:
    case DVZ_DTYPE_INT:
    case DVZ_DTYPE_FLOAT:
    case DVZ_DTYPE_DOUBLE:
        return 1;

    case DVZ_DTYPE_CVEC2:
    case DVZ_DTYPE_USVEC2:
    case DVZ_DTYPE_SVEC2:
    case DVZ_DTYPE_UVEC2:
    case DVZ_DTYPE_IVEC2:
    case DVZ_DTYPE_VEC2:
    case DVZ_DTYPE_DVEC2:
        return 2;

    case DVZ_DTYPE_CVEC3:
    case DVZ_DTYPE_USVEC3:
    case DVZ_DTYPE_SVEC3:
    case DVZ_DTYPE_UVEC3:
    case DVZ_DTYPE_IVEC3:
    case DVZ_DTYPE_VEC3:
    case DVZ_DTYPE_DVEC3:
        return 3;

    case DVZ_DTYPE_CVEC4:
    case DVZ_DTYPE_USVEC4:
    case DVZ_DTYPE_SVEC4:
    case DVZ_DTYPE_UVEC4:
    case DVZ_DTYPE_IVEC4:
    case DVZ_DTYPE_VEC4:
    case DVZ_DTYPE_DVEC4:
        return 4;

    default:
        return 0;
        break;
    }
    return 0;
}



// Create a new 1D array with a given dtype, number of elements, and item size (used for record
// arrays containing heterogeneous data)
static DvzArray* _create_array(uint32_t item_count, DvzDataType dtype, DvzSize item_size)
{
    log_trace("creating array with %d items of size %s each", item_count, pretty_size(item_size));
    DvzArray* arr = (DvzArray*)calloc(1, sizeof(DvzArray));
    memset(arr, 0, sizeof(DvzArray));
    arr->obj.type = DVZ_OBJECT_TYPE_ARRAY;
    arr->dtype = dtype;
    arr->components = _get_components(dtype);
    arr->item_size = item_size;
    ASSERT(item_size > 0);
    arr->item_count = item_count;
    arr->buffer_size = item_count * arr->item_size;
    if (item_count > 0)
        arr->data = calloc(item_count, arr->item_size);
    dvz_obj_created(&arr->obj);
    return arr;
}



// Cast a vector.
static inline void _cast(DvzDataType target_dtype, void* dst, DvzDataType source_dtype, void* src)
{
    if (source_dtype == DVZ_DTYPE_DOUBLE && target_dtype == DVZ_DTYPE_FLOAT)
    {
        ((vec3*)dst)[0][0] = ((dvec3*)src)[0][0];
    }
    else if (source_dtype == DVZ_DTYPE_DVEC2 && target_dtype == DVZ_DTYPE_VEC2)
    {
        ((vec3*)dst)[0][0] = ((dvec3*)src)[0][0];
        ((vec3*)dst)[0][1] = ((dvec3*)src)[0][1];
    }
    else if (source_dtype == DVZ_DTYPE_DVEC3 && target_dtype == DVZ_DTYPE_VEC3)
    {
        ((vec3*)dst)[0][0] = ((dvec3*)src)[0][0];
        ((vec3*)dst)[0][1] = ((dvec3*)src)[0][1];
        ((vec3*)dst)[0][2] = ((dvec3*)src)[0][2];
    }
    else
        log_error("unknown casting dtypes %d %d", source_dtype, target_dtype);
}



// Fill the remaining of an array with the last non-empty value.
static void
_repeat_last(uint32_t old_item_count, DvzSize item_size, void* data, uint32_t item_count)
{
    // Repeat the last item of an array.
    DvzSize old_size = old_item_count * item_size;
    int64_t dst_offset = (int64_t)data + (int64_t)old_size;
    int64_t src_offset = (int64_t)data + (int64_t)old_size - (int64_t)item_size;
    ASSERT(item_count > old_item_count);
    uint32_t repeat_count = item_count - old_item_count;
    for (uint32_t i = 0; i < repeat_count; i++)
    {
        memcpy((void*)dst_offset, (void*)src_offset, item_size);
        dst_offset += (int64_t)item_size;
    }
}



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
DvzArray* dvz_array(uint32_t item_count, DvzDataType dtype)
{
    ASSERT(dtype != DVZ_DTYPE_NONE);
    ASSERT(dtype != DVZ_DTYPE_CUSTOM);
    return _create_array(item_count, dtype, _get_dtype_size(dtype));
}



/**
 * Create a copy of an existing array.
 *
 * @param arr an array
 * @returns a new array
 */
DvzArray* dvz_array_copy(DvzArray* arr)
{
    DvzArray* arr_new = calloc(1, sizeof(DvzArray));
    memcpy(arr_new, arr, sizeof(DvzArray));
    arr_new->data = malloc(arr->buffer_size);
    memcpy(arr_new->data, arr->data, arr->buffer_size);
    return arr_new;
}



/**
 * Create an array with a single dvec3 position.
 *
 * @param pos initial number of elements
 * @returns a new array
 */
DvzArray* dvz_array_point(dvec3 pos)
{
    DvzArray* arr = dvz_array(1, DVZ_DTYPE_DVEC3);
    memcpy(arr->data, pos, sizeof(dvec3));
    return arr;
}



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
DvzArray* dvz_array_wrap(uint32_t item_count, DvzDataType dtype, void* data)
{
    DvzArray* arr = dvz_array(0, dtype); // do not allocate underlying buffer
    // Manual setting of struct fields with the passed buffer
    arr->item_count = item_count;
    arr->buffer_size = item_count * arr->item_size;
    arr->data = data;
    return arr;
}



/**
 * Create a 1D record array with heterogeneous data type.
 *
 * @param item_count number of elements
 * @param item_size size, in bytes, of each item
 * @returns the array
 */
DvzArray* dvz_array_struct(uint32_t item_count, DvzSize item_size)
{
    ASSERT(item_size > 0);
    return _create_array(item_count, DVZ_DTYPE_CUSTOM, item_size);
}



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
dvz_array_3D(uint32_t ndims, uint32_t width, uint32_t height, uint32_t depth, DvzSize item_size)
{
    ASSERT(ndims > 0);
    ASSERT(ndims <= 3);

    if (ndims == 1)
        ASSERT(height <= 1 && depth <= 1);
    if (ndims == 2)
        ASSERT(depth <= 1);

    uint32_t item_count = width * height * depth;

    DvzArray* arr = _create_array(item_count, DVZ_DTYPE_CUSTOM, item_size);
    arr->ndims = ndims;
    arr->shape[0] = width;
    arr->shape[1] = height;
    arr->shape[2] = depth;
    return arr;
}



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
void dvz_array_resize(DvzArray* array, uint32_t item_count)
{
    ANN(array);
    ASSERT(item_count > 0);
    ASSERT(array->item_size > 0);

    uint32_t old_item_count = array->item_count;

    // Do nothing if the size is the same.
    if (item_count == old_item_count)
        return;

    // If the array was not allocated, allocate it with the specified size.
    if (array->data == NULL)
    {
        array->data = calloc(item_count, array->item_size);
        array->item_count = item_count;

        // NOTE: using dvz_next_pow2() below causes a crash in scene_axes test
        array->buffer_size = item_count * array->item_size;
        // array->buffer_size = dvz_next_pow2(item_count * array->item_size);

        log_trace(
            "allocate array to contain %d elements (%s)", item_count,
            pretty_size(array->buffer_size));
        return;
    }

    // Here, the array was already allocated, and the requested size is different.
    DvzSize old_size = array->buffer_size;
    DvzSize new_size = item_count * array->item_size;
    ANN(array->data);

    // Only reallocate if the existing buffer is not large enough for the new item_count.
    if (new_size > old_size)
    {
        uint32_t new_item_count = 2 * old_item_count;
        while (new_item_count < item_count)
            new_item_count *= 2;
        ASSERT(new_item_count >= item_count);
        new_size = new_item_count * array->item_size;
        log_trace(
            "resize array from %d to %d items of size %d", //
            old_item_count, new_item_count, array->item_size);
        REALLOC(void*, array->data, new_size);
        // Repeat the last element when resizing.
        _repeat_last(old_size / array->item_size, array->item_size, array->data, new_item_count);
        array->buffer_size = new_size;
    }
    array->item_count = item_count;
}



/**
 * Reset to 0 the contents of an existing array.
 *
 * @param array the array to clear
 */
void dvz_array_clear(DvzArray* array)
{
    ANN(array);
    memset(array->data, 0, array->buffer_size);
}



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
void dvz_array_reshape(DvzArray* array, uint32_t width, uint32_t height, uint32_t depth)
{
    ANN(array);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);
    uint32_t item_count = width * height * depth;

    // If the shape is the same, do nothing.
    if (width == array->shape[0] && height == array->shape[1] && depth == array->shape[2])
        return;

    // Resize the underlying buffer.
    dvz_array_resize(array, item_count);

    // HACK: reset to 0 the array instead of having to deal with reshaping.
    log_trace("clearing the 3D array while reshaping it to %dx%dx%d", width, height, depth);
    dvz_array_clear(array);

    array->shape[0] = width;
    array->shape[1] = height;
    array->shape[2] = depth;
}



/**
 * Insert data in an array.
 *
 * @param array the array
 * @param offset the index of the first element of the inserted data in the new array
 * @param size the number of elements to insert
 * @param insert the data to insert
 */
void dvz_array_insert(DvzArray* array, uint32_t offset, uint32_t size, void* insert)
{
    ANN(array);
    ASSERT(size > 0);
    ANN(insert);

    // Size of the chunk to move to make place for the inserted buffer.
    DvzSize chunk1_size = (array->item_count - offset) * array->item_size;

    // Resize the array.
    dvz_array_resize(array, array->item_count + size);

    // Position of the second chunk before the insertion.
    void* chunk1_bef = (void*)((int64_t)array->data + (int64_t)((offset + 0) * array->item_size));

    // Position of the second chunk after the insertion.
    void* chunk1_aft =
        (void*)((int64_t)array->data + (int64_t)((offset + size) * array->item_size));

    // Move the second chunk after the inserted data.
    if (chunk1_size > 0 && chunk1_bef != chunk1_aft)
        memmove(chunk1_aft, chunk1_bef, chunk1_size);

    // Insert the data.
    ASSERT((int64_t)chunk1_bef + (int64_t)(size * array->item_size) == (int64_t)chunk1_aft);
    memcpy(chunk1_bef, insert, size * array->item_size);
}



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
    uint32_t item_count)
{
    ANN(src_arr);
    ANN(dst_arr);
    ASSERT(item_count > 0);
    ASSERT(src_offset + item_count <= src_arr->item_count);
    ASSERT(dst_offset + item_count <= dst_arr->item_count);
    ASSERT(src_arr->dtype == dst_arr->dtype);
    ASSERT(src_arr->item_size == dst_arr->item_size);

    void* src = (void*)((int64_t)src_arr->data + ((int64_t)(src_offset * src_arr->item_size)));
    void* dst = (void*)((int64_t)dst_arr->data + ((int64_t)(dst_offset * dst_arr->item_size)));
    DvzSize size = item_count * src_arr->item_size;
    memcpy(dst, src, size);
}



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
 *     dvz_array_data(arr, 0, 10, 1, &item);
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
    uint32_t data_item_count, const void* data)
{
    ANN(array);
    ASSERT(data_item_count > 0);
    ANN(array->data);
    if (data == NULL)
    {
        log_debug("skipping dvz_array_data() with NULL data");
        return;
    }
    ASSERT(item_count > 0);

    // Resize if necessary.
    if (first_item + item_count > array->item_count)
    {
        dvz_array_resize(array, first_item + item_count);
    }
    ASSERT(first_item + item_count <= array->item_count);
    ASSERT(array->item_size > 0);
    ASSERT(array->item_count > 0);

    DvzSize item_size = array->item_size;
    ASSERT(item_size > 0);

    void* dst = array->data;
    // Allocate the array if needed.
    if (dst == NULL)
        dst = array->data = calloc(first_item + array->item_count, array->item_size);
    ANN(dst);
    const void* src = data;
    ANN(src);

    DvzSize copy_size = MIN(item_count, data_item_count) * item_size;
    ASSERT(copy_size > 0);
    // log_trace(
    //     "copy %d elements (%d bytes) into array[%d:%d]", //
    //     data_item_count, copy_size, first_item, first_item + item_count);
    ASSERT(array->buffer_size >= (first_item + item_count) * item_size);
    memcpy((void*)((int64_t)dst + (int64_t)(first_item * item_size)), src, copy_size);

    // If the source data array is smaller than the destination array, repeat the last value.
    if (data_item_count < item_count)
    {
        _repeat_last(
            data_item_count, array->item_size,
            (void*)((int64_t)array->data + (int64_t)(first_item * item_size)), item_count);
    }
}



/**
 * Multiply all array elements by a scaling factor.
 *
 * !!! note
 *     Only FLOAT dtype is supported for now.
 *
 * @param array the array
 * @param scaling scaling factor
 */
void dvz_array_scale(DvzArray* arr, float scaling)
{
    ANN(arr);
    // TODO: support other dtypes.
    if (arr->dtype == DVZ_DTYPE_FLOAT)
    {
        for (uint32_t i = 0; i < arr->item_count; i++)
        {
            ((float*)arr->data)[i] *= scaling;
        }
    }
}



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
    DvzArrayCopyType copy_type, uint32_t reps)          //
{
    ANN(array);
    ASSERT(data_item_count > 0);
    ANN(array->data);
    ANN(data);
    ASSERT(item_count > 0);
    ASSERT(first_item + item_count <= array->item_count);

    DvzSize src_offset = 0;
    DvzSize src_stride = col_size;

    DvzSize dst_offset = offset;
    DvzSize dst_stride = array->item_size;

    void* dst = array->data;
    const void* src = data;

    ANN(src);
    ANN(dst);
    ASSERT(src_stride > 0);
    ASSERT(dst_stride > 0);
    ASSERT(item_count > 0);

    log_trace(
        "copy src offset %d stride %d, dst offset %d stride %d, item size %d count %d", //
        src_offset, src_stride, dst_offset, dst_stride, col_size, item_count);

    int64_t src_byte = (int64_t)src + (int64_t)src_offset;
    int64_t dst_byte = (int64_t)dst + (int64_t)(first_item * dst_stride) + (int64_t)dst_offset;

    uint32_t j = 0; // j: src index
    uint32_t m = 0;
    bool skip = false;
    for (uint32_t i = 0; i < item_count; i++) // i: dst index
    {
        if (reps > 1)
            m = i % reps;
        // Determine whether the current item copy should be skipped.
        skip = copy_type == DVZ_ARRAY_COPY_SINGLE && reps > 1 && m > 0;

        // NOTE: this function is not optimized and might benefit from being refactored.

        // Copy the current item, unless we are in SINGLE copy mode
        if (!skip)
        {
            if (source_dtype == target_dtype ||   //
                source_dtype == DVZ_DTYPE_NONE || //
                target_dtype == DVZ_DTYPE_NONE)   //
                memcpy((void*)dst_byte, (void*)src_byte, col_size);
            else
            {
                _cast(target_dtype, (void*)dst_byte, source_dtype, (void*)src_byte);
            }
        }

        // Advance the source pointer, unless we are in SINGLE copy mode
        skip = reps > 1 && m < reps - 1;
        if (j < data_item_count - 1 && !skip)
        {
            src_byte += (int64_t)src_stride;
            j++;
        }

        dst_byte += (int64_t)dst_stride;
    }
}



void dvz_array_print(DvzArray* array)
{
    ANN(array);
    dvec3* item = NULL;
    for (uint32_t i = 0; i < array->item_count; i++)
    {
        item = (dvec3*)dvz_array_item(array, i);
        if (array->dtype == DVZ_DTYPE_DVEC3)
        {
            log_info("%f %f %f", item[0][0], item[0][1], item[0][2]);
        }
    }
}



/**
 * Destroy an array.
 *
 * This function frees the allocated underlying data buffer.
 *
 * @param array the array to destroy
 */
void dvz_array_destroy(DvzArray* array)
{
    ANN(array);
    log_trace("destroying array with %d items", array->item_count);
    if (!dvz_obj_is_created(&array->obj))
        return;
    dvz_obj_destroyed(&array->obj);
    FREE(array->data);
    FREE(array);
}
