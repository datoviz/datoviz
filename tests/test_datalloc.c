/*************************************************************************************************/
/*  Testing Dat alloc                                                                            */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "datalloc.h"
#include "test.h"
#include "test_datalloc.h"
#include "test_resources.h"
#include "testing.h"



/*************************************************************************************************/
/*  Alloc tests                                                                                  */
/*************************************************************************************************/

int test_datalloc_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);

    DvzDatAlloc datalloc = {0};
    dvz_datalloc(gpu, &res, &datalloc);

    VkDeviceSize alignment = 0;
    VkDeviceSize size = 128;

    // 2 allocations in the staging buffer.
    DvzDat* dat = dvz_dat(&res, &datalloc, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ASSERT(dat != NULL);
    AT(dat->br.offsets[0] == 0);
    AT(dat->br.size == size);

    // Get the buffer alignment.
    alignment = dat->br.buffer->vma.alignment;

    DvzDat* dat_1 = dvz_dat(&res, &datalloc, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ASSERT(dat_1 != NULL);
    AT(dat_1->br.offsets[0] == _align(size, alignment));
    AT(dat_1->br.size == size);

    // Resize the second buffer.
    VkDeviceSize new_size = 196;
    dvz_dat_resize(dat_1, new_size);
    // The offset should be the same, just the size should change.
    AT(dat_1->br.offsets[0] == _align(size, alignment));
    AT(dat_1->br.size == new_size);

    // 1 allocation in the vertex buffer.
    DvzDat* dat_2 = dvz_dat(&res, &datalloc, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    ASSERT(dat_2 != NULL);
    AT(dat_2->br.offsets[0] == 0);
    AT(dat_2->br.size == size);


    // Delete the first staging allocation.
    dvz_dat_destroy(dat);

    // New allocation in the staging buffer.
    DvzDat* dat_3 = dvz_dat(&res, &datalloc, DVZ_BUFFER_TYPE_STAGING, size, 0);
    ASSERT(dat_3 != NULL);
    AT(dat_3->br.offsets[0] == 0);
    AT(dat_3->br.size == size);

    // Resize the lastly-created buffer, we should be in the first position.
    dvz_dat_resize(dat_3, new_size);
    AT(dat_3->br.offsets[0] == 0);
    AT(dat_3->br.size == new_size);

    // Resize the lastly-created buffer, we should now get a new position.
    new_size = 1024;
    dvz_dat_resize(dat_3, new_size);
    AT(dat_3->br.offsets[0] == 2 * alignment);
    AT(dat_3->br.size == new_size);

    dvz_datalloc_stats(&datalloc);

    // NOTE: resources destruction MUST occur before datalloc destruction.
    dvz_resources_destroy(&res);
    dvz_datalloc_destroy(&datalloc);
    return 0;
}
