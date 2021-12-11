/*************************************************************************************************/
/*  Testing file io                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_fileio.h"
#include "fileio.h"
#include "test.h"
#include "testing.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_utils_png_1(TstSuite* suite)
{
    ASSERT(suite != NULL);

    int32_t width = 256, height = 128;
    uint8_t* rgb = calloc((uint32_t)(width * height), 3);
    for (int32_t i = 0; i < width; i++)
    {
        for (int32_t j = 0; j < height; j++)
        {
            if ((i - width / 2) * (j - height / 2) < 0)
            {
                rgb[3 * i * height + 3 * j + 0] = 128;
                rgb[3 * i * height + 3 * j + 1] = 32;
                rgb[3 * i * height + 3 * j + 2] = 16;
            }
        }
    }

    DvzSize size = 0;
    void* out = NULL;
    dvz_make_png((uint32_t)width, (uint32_t)height, rgb, &size, &out);
    AT(size > 0);
    AT(out != NULL);

    // test
    // FILE* fp = fopen("a.png", "wb");
    // fwrite(out, size, size, fp);
    // fclose(fp);

    FREE(out);
    FREE(rgb);
    return 0;
}
