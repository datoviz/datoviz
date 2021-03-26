#include "../include/datoviz/context.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Buffer                                                                                       */
/*************************************************************************************************/

int test_context_buffer_default(TestContext* tc)
{
    return 0; //
}



/*************************************************************************************************/
/*  Colormap                                                                                     */
/*************************************************************************************************/

int test_context_colormap_custom(TestContext* tc)
{
    DvzContext* ctx = tc->context; // dvz_context(gpu);
    ASSERT(ctx != NULL);

    // Make a custom colormap.
    uint8_t cmap = CMAP_CUSTOM;
    uint8_t color_count = 3;
    cvec4 colors[3] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
    };

    // Update it on the CPU.
    dvz_colormap_custom(cmap, color_count, colors);

    // Check that the CPU array has been updated.
    cvec4 out = {0};
    for (uint32_t i = 0; i < 3; i++)
    {
        dvz_colormap(cmap, i, out);
        AT(memcmp(out, colors[i], sizeof(cvec4)) == 0);
    }

    // Update the colormap texture on the GPU.
    dvz_context_colormap(ctx);

    // Check that the GPU texture has been updated.
    cvec4* arr = calloc(256 * 256, sizeof(cvec4));
    dvz_texture_download(
        ctx->color_texture.texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, //
        256 * 256 * sizeof(cvec4), arr);
    cvec2 ij = {0};
    dvz_colormap_idx(cmap, 0, ij);
    AT(memcmp(&arr[256 * ij[0] + ij[1]], colors, 3 * sizeof(cvec4)) == 0);
    FREE(arr);

    return 0;
}
