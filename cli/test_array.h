#include "../include/visky/array.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

static int vklite2_array_1(VkyTestContext* context)
{
    uint8_t values[] = {1, 2, 3, 4, 5, 6};

    VklArray arr = vkl_array(6, VKL_DTYPE_CHAR);

    // 1:6
    vkl_array_data(&arr, 0, 6, 6, values);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    // Partial copy of data.
    vkl_array_data(&arr, 2, 3, 3, values); // put [1, 2, 3] in arr[2:5]
    AT(memcmp(arr.data, (uint8_t[]){1, 2, 1, 2, 3, 6}, sizeof(values)) == 0);

    vkl_array_destroy(&arr);
    return 0;
}



static int vklite2_array_2(VkyTestContext* context)
{
    float values[] = {1, 2, 3, 4, 5, 6};

    VklArray arr = vkl_array(6, VKL_DTYPE_FLOAT);

    // 1:6
    vkl_array_data(&arr, 0, 6, 6, values);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    // Partial copy of data.
    vkl_array_data(&arr, 2, 3, 3, values); // put [1, 2, 3] in arr[2:5]
    AT(memcmp(arr.data, (float[]){1, 2, 1, 2, 3, 6}, sizeof(values)) == 0);

    // Resize with less elements. Should not lose the original data.
    vkl_array_resize(&arr, 3);
    AT(memcmp(arr.data, (float[]){1, 2, 1}, 3 * sizeof(float)) == 0);

    // Resize with more elements.
    vkl_array_resize(&arr, 9);
    // Resizing should repeat the last element.
    AT(memcmp(arr.data, (float[]){1, 2, 1, 2, 3, 6, 6, 6, 6}, 9 * sizeof(float)) == 0);

    // Fill the array with a constant value.
    float value = 12.0f;
    ASSERT(arr.item_count == 9);
    vkl_array_data(&arr, 0, 9, 1, &value);
    for (uint32_t i = 0; i < 9; i++)
    {
        AT(((float*)arr.data)[i] == value);
    }

    vkl_array_destroy(&arr);
    return 0;
}



static int vklite2_array_3(VkyTestContext* context)
{
    float values[] = {1, 2, 3, 4, 5, 6};

    VklArray arr = vkl_array(6, VKL_DTYPE_FLOAT);

    // 1:6
    vkl_array_data(&arr, 0, 6, 6, values);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    // Partial copy of data.
    vkl_array_data(&arr, 2, 3, 3, values); // put [1, 2, 3] in arr[2:5]
    AT(memcmp(arr.data, (float[]){1, 2, 1, 2, 3, 6}, sizeof(values)) == 0);

    // for (uint32_t i = 0; i < 9; i++)
    // {
    //     DBGF(((float*)arr.data)[i]);
    // }

    vkl_array_destroy(&arr);
    return 0;
}
