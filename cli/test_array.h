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



typedef struct _TestDtype _TestDtype;
struct _TestDtype
{
    uint8_t a;
    float b;
};

static int vklite2_array_3(VkyTestContext* context)
{
    // uint8, float32
    VklArray arr = vkl_array_struct(3, sizeof(_TestDtype));

    _TestDtype value = {1, 2};
    vkl_array_data(&arr, 0, 3, 1, &value);
    for (uint32_t i = 0; i < 3; i++)
    {
        AT(((_TestDtype*)(vkl_array_item(&arr, i)))->a == 1);
        AT(((_TestDtype*)(vkl_array_item(&arr, i)))->b == 2);
    }

    // Copy data to the second column.
    float b = 20.0f;
    vkl_array_column(&arr, offsetof(_TestDtype, b), sizeof(float), 1, 2, 1, &b);

    // Row #0.
    AT(((_TestDtype*)(vkl_array_item(&arr, 0)))->a == 1);
    AT(((_TestDtype*)(vkl_array_item(&arr, 0)))->b == 2);

    // Row #1.
    AT(((_TestDtype*)(vkl_array_item(&arr, 1)))->a == 1);
    AT(((_TestDtype*)(vkl_array_item(&arr, 1)))->b == 20);

    // Row #2.
    AT(((_TestDtype*)(vkl_array_item(&arr, 2)))->a == 1);
    AT(((_TestDtype*)(vkl_array_item(&arr, 2)))->b == 20);

    // Resize.
    vkl_array_resize(&arr, 4);
    // Row #3
    AT(((_TestDtype*)(vkl_array_item(&arr, 3)))->a == 1);
    AT(((_TestDtype*)(vkl_array_item(&arr, 3)))->b == 20);

    vkl_array_destroy(&arr);
    return 0;
}



static int vklite2_array_4(VkyTestContext* context)
{
    uint8_t values[] = {1, 2, 3, 4, 5, 6};

    VklArray arr = vkl_array(3, VKL_DTYPE_CHAR);

    // Check automatic resize when setting larger data array.
    AT(arr.item_count == 3);
    vkl_array_data(&arr, 0, 6, 6, values);
    AT(arr.item_count == 6);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    vkl_array_destroy(&arr);
    return 0;
}
