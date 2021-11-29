/*************************************************************************************************/
/*  Testing alloc                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "resources.h"
#include "test.h"
#include "test_resources.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Resources tests                                                                              */
/*************************************************************************************************/

int test_resources_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);

    // Create some GPU objects, which should be automatically destroyed upon context destruction
    // (resources destruction).
    DvzBuffer* buffer = dvz_resources_buffer(&res, DVZ_BUFFER_TYPE_VERTEX, false, 64);
    ASSERT(buffer != NULL);

    uvec3 shape = {2, 4, 1};
    DvzImages* img = dvz_resources_image(&res, DVZ_TEX_2D, shape, VK_FORMAT_R8G8B8A8_UNORM);
    ASSERT(img != NULL);

    DvzSampler* sampler =
        dvz_resources_sampler(&res, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    ASSERT(sampler != NULL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_double.comp.spv", SPIRV_DIR);
    DvzCompute* compute = dvz_resources_compute(&res, path);
    ASSERT(compute != NULL);

    dvz_resources_destroy(&res);
    return 0;
}



int test_resources_dat_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);
    res.img_count = 3;

    // Allocate a dat.
    DvzSize size = 128;
    DvzDat* dat = dvz_dat(&res, DVZ_BUFFER_TYPE_VERTEX, size, 0);
    ASSERT(dat != NULL);

    dvz_dat_destroy(dat);
    dvz_resources_destroy(&res);
    return 0;
}



int test_resources_tex_1(TstSuite* suite)
{
    ASSERT(suite != NULL);
    DvzGpu* gpu = get_gpu(suite);
    ASSERT(gpu != NULL);

    // Create the resources object.
    DvzResources res = {0};
    dvz_resources(gpu, &res);

    uvec3 shape = {2, 4, 1};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // Allocate a tex.
    DvzTex* tex = dvz_tex(&res, DVZ_TEX_2D, shape, format, 0);
    ASSERT(tex != NULL);

    dvz_tex_destroy(tex);
    dvz_resources_destroy(&res);
    return 0;
}
