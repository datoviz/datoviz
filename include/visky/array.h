#ifndef VKL_ARRAY_HEADER
#define VKL_ARRAY_HEADER

#include "vklite.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklArray VklArray;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Data types.
typedef enum
{
    VKL_DTYPE_NONE,
    VKL_DTYPE_CUSTOM, // used for structured arrays (aka record arrays)

    VKL_DTYPE_CHAR, // 8 bits, unsigned int
    VKL_DTYPE_CVEC2,
    VKL_DTYPE_CVEC3,
    VKL_DTYPE_CVEC4,

    VKL_DTYPE_USHORT, // 16 bits, unsigned int
    VKL_DTYPE_USVEC2,
    VKL_DTYPE_USVEC3,
    VKL_DTYPE_USVEC4,

    VKL_DTYPE_SHORT, // 16 bits, signed int
    VKL_DTYPE_SVEC2,
    VKL_DTYPE_SVEC3,
    VKL_DTYPE_SVEC4,

    VKL_DTYPE_UINT, // 32 bits, unsigned int
    VKL_DTYPE_UVEC2,
    VKL_DTYPE_UVEC3,
    VKL_DTYPE_UVEC4,

    VKL_DTYPE_INT, // 32 bits, signed int
    VKL_DTYPE_IVEC2,
    VKL_DTYPE_IVEC3,
    VKL_DTYPE_IVEC4,

    VKL_DTYPE_FLOAT, // 32 bits float
    VKL_DTYPE_VEC2,
    VKL_DTYPE_VEC3,
    VKL_DTYPE_VEC4,

    VKL_DTYPE_DOUBLE, // 64 bits double
    VKL_DTYPE_DVEC2,
    VKL_DTYPE_DVEC3,
    VKL_DTYPE_DVEC4,

    VKL_DTYPE_MAT2, // matrices of floats
    VKL_DTYPE_MAT3,
    VKL_DTYPE_MAT4,
} VklDataType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklArray
{
    VklObject obj;
    VklDataType dtype;
    VkDeviceSize item_size;
    uint32_t item_count;
    VkDeviceSize buffer_size;
    void* data;

    // 3D arrays
    uint32_t ndims; // 1, 2, or 3
    uvec3 shape;    // only for 3D arrays
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VkDeviceSize _get_dtype_size(VklDataType dtype)
{
    switch (dtype)
    {
    // 8 bits
    case VKL_DTYPE_CHAR:
        return 1;
    case VKL_DTYPE_CVEC2:
        return 1 * 2;
    case VKL_DTYPE_CVEC3:
        return 1 * 3;
    case VKL_DTYPE_CVEC4:
        return 1 * 4;

    // 16 bits
    case VKL_DTYPE_USHORT:
    case VKL_DTYPE_SHORT:
        return 2;
    case VKL_DTYPE_USVEC2:
        return 2 * 2;
    case VKL_DTYPE_USVEC3:
        return 2 * 3;
    case VKL_DTYPE_USVEC4:
        return 2 * 4;

    // 32 bits
    case VKL_DTYPE_FLOAT:
    case VKL_DTYPE_UINT:
    case VKL_DTYPE_INT:
        return 4;

    case VKL_DTYPE_VEC2:
    case VKL_DTYPE_UVEC2:
    case VKL_DTYPE_IVEC2:
        return 4 * 2;

    case VKL_DTYPE_VEC3:
    case VKL_DTYPE_UVEC3:
    case VKL_DTYPE_IVEC3:
        return 4 * 3;

    case VKL_DTYPE_VEC4:
    case VKL_DTYPE_UVEC4:
    case VKL_DTYPE_IVEC4:
        return 4 * 4;

    // 64 bits
    case VKL_DTYPE_DOUBLE:
        return 8;
    case VKL_DTYPE_DVEC2:
        return 8 * 2;
    case VKL_DTYPE_DVEC3:
        return 8 * 3;
    case VKL_DTYPE_DVEC4:
        return 8 * 4;

    case VKL_DTYPE_MAT2:
        return 2 * 2 * 4;
    case VKL_DTYPE_MAT3:
        return 3 * 3 * 4;
    case VKL_DTYPE_MAT4:
        return 4 * 4 * 4;

    default:
        break;
    }

    if (dtype != VKL_DTYPE_NONE)
        log_error("could not find the size of dtype %d", dtype);
    return 0;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static VklArray _create_array(uint32_t item_count, VklDataType dtype, VkDeviceSize item_size)
{
    VklArray arr = {0};
    arr.obj.type = VKL_OBJECT_TYPE_ARRAY;
    arr.dtype = dtype;
    arr.item_size = item_size;
    arr.item_count = item_count;
    arr.buffer_size = item_count * arr.item_size;
    if (item_count > 0)
        arr.data = calloc(item_count, arr.item_size);
    obj_created(&arr.obj);
    return arr;
}



static VklArray vkl_array(uint32_t item_count, VklDataType dtype)
{
    ASSERT(dtype != VKL_DTYPE_NONE);
    ASSERT(dtype != VKL_DTYPE_CUSTOM);
    return _create_array(item_count, dtype, _get_dtype_size(dtype));
}



static VklArray vkl_array_struct(uint32_t item_count, VkDeviceSize item_size)
{
    ASSERT(item_size > 0);
    return _create_array(item_count, VKL_DTYPE_CUSTOM, item_size);
}



static VklArray vkl_array_3D(
    uint32_t ndims, uint32_t width, uint32_t height, uint32_t depth, VkDeviceSize item_size)
{
    ASSERT(ndims > 0);
    ASSERT(ndims <= 3);

    if (ndims == 1)
        ASSERT(height <= 1 && depth <= 1);
    if (ndims == 2)
        ASSERT(depth <= 1);

    uint32_t item_count = width * height * depth;

    VklArray arr = _create_array(item_count, VKL_DTYPE_CUSTOM, item_size);
    arr.ndims = ndims;
    arr.shape[0] = width;
    arr.shape[1] = height;
    arr.shape[2] = depth;
    return arr;
}



static void
_repeat_last(uint32_t old_item_count, VkDeviceSize item_size, void* data, uint32_t item_count)
{
    // Repeat the last item of an array.
    VkDeviceSize old_size = old_item_count * item_size;
    int64_t dst_offset = (int64_t)data + (int64_t)old_size;
    int64_t src_offset = (int64_t)data + (int64_t)old_size - (int64_t)item_size;
    ASSERT(item_count > old_item_count);
    uint32_t repeat_count = item_count - old_item_count;
    log_trace("repeat %d items in array", repeat_count);
    for (uint32_t i = 0; i < repeat_count; i++)
    {
        memcpy((void*)dst_offset, (void*)src_offset, item_size);
        dst_offset += (int64_t)item_size;
    }
}



static void vkl_array_resize(VklArray* array, uint32_t item_count)
{
    ASSERT(array != NULL);
    ASSERT(item_count > 0);

    uint32_t old_item_count = array->item_count;

    // Do nothing if the size is the same.
    if (item_count == old_item_count)
        return;

    // If the array was not allocated, allocate it with the specified size.
    if (array->data == NULL)
    {
        array->data = calloc(item_count, array->item_size);
        array->item_count = item_count;
        array->buffer_size = item_count * array->item_size;
        return;
    }

    // Here, the array was already allocated, and the requested size is different.
    VkDeviceSize old_size = array->buffer_size;
    VkDeviceSize new_size = item_count * array->item_size;
    ASSERT(array->data != NULL);

    // Only reallocate if the existing buffer is not large enough for the new item_count.
    if (new_size > old_size)
    {
        log_debug(
            "resize array from %d to %d items of size %d", old_item_count, item_count,
            array->item_size);
        REALLOC(array->data, item_count * array->item_size);
        // Repeat the last element when resizing.
        _repeat_last(old_size / array->item_size, array->item_size, array->data, item_count);
        array->buffer_size = new_size;
    }
    array->item_count = item_count;
}



// WARNING: this command currently loses all the data in the array.
static void vkl_array_reshape(VklArray* array, uint32_t width, uint32_t height, uint32_t depth)
{
    ASSERT(array != NULL);
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(depth > 0);
    uint32_t item_count = width * height * depth;

    // If the shape is the same, do nothing.
    if (width == array->shape[0] && height == array->shape[1] && depth == array->shape[2])
        return;

    // Resize the underlying buffer.
    vkl_array_resize(array, item_count);

    // HACK: reset to 0 the array instead of having to deal with reshaping.
    log_trace("clearing the 3D array while reshaping it to %dx%dx%d", width, height, depth);
    memset(array->data, 0, array->buffer_size);

    array->shape[0] = width;
    array->shape[1] = height;
    array->shape[2] = depth;
}



static void vkl_array_data(
    VklArray* array, uint32_t first_item, uint32_t item_count, //
    uint32_t data_item_count, const void* data)
{
    ASSERT(array != NULL);
    ASSERT(data_item_count > 0);
    ASSERT(array->data != NULL);
    ASSERT(data != NULL);
    ASSERT(item_count > 0);

    // Resize if necessary.
    if (first_item + item_count > array->item_count)
    {
        vkl_array_resize(array, first_item + item_count);
    }
    ASSERT(first_item + item_count <= array->item_count);

    VkDeviceSize item_size = array->item_size;
    ASSERT(item_size > 0);

    void* dst = array->data;
    // Allocate the array if needed.
    if (dst == NULL)
        dst = array->data = calloc(first_item + array->item_count, array->item_size);
    ASSERT(dst != NULL);
    const void* src = data;
    ASSERT(src != NULL);

    VkDeviceSize copy_size = MIN(item_count, data_item_count) * item_size;
    log_trace(
        "copy %d elements (%d bytes) into array[%d:%d]", //
        data_item_count, copy_size, first_item, first_item + item_count);
    memcpy((void*)((int64_t)dst + (int64_t)(first_item * item_size)), src, copy_size);

    // If the source data array is smaller than the destination array, repeat the last value.
    if (data_item_count < item_count)
    {
        _repeat_last(data_item_count, array->item_size, array->data, item_count);
    }
}



static inline void* vkl_array_item(VklArray* array, uint32_t idx)
{
    ASSERT(array != NULL);
    idx = CLIP(idx, 0, array->item_count);
    return (void*)((int64_t)array->data + (int64_t)(idx * array->item_size));
}



static void vkl_array_column(
    VklArray* array, VkDeviceSize offset, VkDeviceSize col_size, //
    uint32_t first_item, uint32_t item_count,                    //
    uint32_t data_item_count, const void* data)
{
    ASSERT(array != NULL);
    ASSERT(data_item_count > 0);
    ASSERT(array->data != NULL);
    ASSERT(data != NULL);
    ASSERT(item_count > 0);
    ASSERT(first_item + item_count <= array->item_count);

    VkDeviceSize src_offset = 0;
    VkDeviceSize src_stride = col_size;

    VkDeviceSize dst_offset = offset;
    VkDeviceSize dst_stride = array->item_size;

    void* dst = array->data;
    const void* src = data;

    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    ASSERT(src_stride > 0);
    ASSERT(dst_stride > 0);
    ASSERT(item_count > 0);

    log_trace(
        "copy src offset %d stride %d, dst offset %d stride %d, item size %d count %d", //
        src_offset, src_stride, dst_offset, dst_stride, col_size, item_count);

    int64_t src_byte = (int64_t)src + (int64_t)src_offset;
    int64_t dst_byte = (int64_t)dst + (int64_t)(first_item * dst_stride) + (int64_t)dst_offset;
    for (uint32_t i = 0; i < item_count; i++)
    {
        // log_trace("copy from %d to %d", src_byte, dst_byte);
        memcpy((void*)dst_byte, (void*)src_byte, col_size);
        if (i < data_item_count - 1)
            src_byte += (int64_t)src_stride;
        dst_byte += (int64_t)dst_stride;
    }
}



static void vkl_array_destroy(VklArray* array)
{
    ASSERT(array != NULL);
    if (!is_obj_created(&array->obj))
        return;
    obj_destroyed(&array->obj);
    FREE(array->data) //
}



#endif
