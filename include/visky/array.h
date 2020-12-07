#ifndef VKL_ARRAY_HEADER
#define VKL_ARRAY_HEADER

#include "vklite2.h"



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

typedef enum
{
    VKL_DTYPE_NONE,
    VKL_DTYPE_CUSTOM,

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
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VkDeviceSize _get_dtype_size(VklDataType dtype)
{
    switch (dtype)
    {
    case VKL_DTYPE_CHAR:
        return 1;
    case VKL_DTYPE_CVEC2:
        return 1 * 2;
    case VKL_DTYPE_CVEC3:
        return 1 * 3;
    case VKL_DTYPE_CVEC4:
        return 1 * 4;


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


    case VKL_DTYPE_DOUBLE:
        return 8;
    case VKL_DTYPE_DVEC2:
        return 8 * 2;
    case VKL_DTYPE_DVEC3:
        return 8 * 3;
    case VKL_DTYPE_DVEC4:
        return 8 * 4;

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

static VklArray vkl_array(uint32_t item_count, VklDataType dtype)
{
    VklArray arr = {0};
    arr.obj.type = VKL_OBJECT_TYPE_ARRAY;
    arr.dtype = dtype;
    ASSERT(dtype != VKL_DTYPE_NONE);
    ASSERT(dtype != VKL_DTYPE_CUSTOM);
    arr.item_size = _get_dtype_size(dtype);
    arr.item_count = item_count;
    arr.buffer_size = item_count * arr.item_size;
    arr.data = calloc(item_count, arr.item_size);
    obj_created(&arr.obj);
    return arr;
}



static VklArray vkl_array_struct(uint32_t item_count, VkDeviceSize item_size)
{
    VklArray arr = {0};
    arr.obj.type = VKL_OBJECT_TYPE_ARRAY;
    arr.dtype = VKL_DTYPE_CUSTOM;
    arr.item_size = item_size;
    arr.item_count = item_count;
    arr.buffer_size = item_count * arr.item_size;
    arr.data = calloc(item_count, arr.item_size);
    obj_created(&arr.obj);
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



static void vkl_array_data(
    VklArray* array, uint32_t first_item, uint32_t item_count, //
    uint32_t data_item_count, const void* data)
{
    ASSERT(array != NULL);
    ASSERT(data_item_count > 0);
    ASSERT(array->data != NULL);
    ASSERT(data != NULL);
    ASSERT(item_count > 0);

    VkDeviceSize item_size = array->item_size;
    ASSERT(item_size > 0);

    void* dst = array->data;
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



static void vkl_array_resize(VklArray* array, uint32_t item_count)
{
    ASSERT(array != NULL);
    uint32_t old_item_count = array->item_count;
    VkDeviceSize old_size = array->buffer_size;
    VkDeviceSize new_size = item_count * array->item_size;

    if (new_size > old_size)
    {
        log_trace("resize array from %d to %d items", old_item_count, item_count);
        REALLOC(array->data, item_count * array->item_size);
        // Repeat the last element when resizing.
        _repeat_last(old_size / array->item_size, array->item_size, array->data, item_count);
        array->buffer_size = new_size;
    }
    array->item_count = item_count;
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
        memcpy((void*)dst_byte, (void*)src_byte, col_size);
        src_byte += (int64_t)src_stride;
        dst_byte += (int64_t)dst_stride;
    }
}



static void vkl_array_destroy(VklArray* array)
{
    ASSERT(array != NULL);
    obj_destroyed(&array->obj);
    FREE(array->data) //
}



#endif
