/*************************************************************************************************/
/*  Testing font                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_font.h"
#include "scene/font.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Font test utils                                                                              */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Font tests                                                                                   */
/*************************************************************************************************/

int test_font_1(TstSuite* suite)
{
    ANN(suite);

    // Load a font.
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);

    // Create the font object.
    DvzFont* font = dvz_font(ttf_size, ttf_bytes);

    const uint32_t n = 4;
    vec4* xywh = dvz_font_layout(font, n, (uint32_t[]){97, 98, 99, 100});
    for (uint32_t i = 0; i < n; i++)
    {
        glm_vec4_print(xywh[i], stdout);
    }

    // Cleanup.
    FREE(xywh);
    dvz_font_destroy(font);
    return 0;
}
