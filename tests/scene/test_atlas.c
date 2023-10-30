/*************************************************************************************************/
/*  Testing atlas                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_atlas.h"
#include "scene/atlas.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Atlas test utils                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Atlas tests                                                                                  */
/*************************************************************************************************/

int test_atlas_1(TstSuite* suite)
{
    ANN(suite);
    unsigned long ttf_size = 0;
    unsigned char* ttf_bytes = dvz_resource_font("Roboto_Medium", &ttf_size);
    ASSERT(ttf_size > 0);
    ANN(ttf_bytes);

    DvzAtlas* atlas = dvz_atlas(ttf_size, ttf_bytes);

    // dvz_atlas_string(atlas, "ABCabc"); // By default, ASCII
    // dvz_atlas_codepoints(atlas, 2, (uint32_t[]){9785, 9786});

    // Generate the atlas.
    AT(!dvz_atlas_valid(atlas));
    dvz_atlas_generate(atlas);
    AT(dvz_atlas_valid(atlas));

    // Atlas size.
    uvec3 shape = {0};
    dvz_atlas_shape(atlas, shape);

    // Save the atlas PNG.
    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/atlas.png", ARTIFACTS_DIR);
    dvz_atlas_png(atlas, imgpath);

    // Show a glyph's coordinates.
    vec4 coords = {0};
    if (dvz_atlas_glyph(atlas, 97, coords) == 0)
        glm_vec4_print(coords, stdout);
    AT(coords[0] > 0);
    AT(coords[1] > 0);
    AT(coords[2] > 0);
    AT(coords[3] > 0);

    dvz_atlas_destroy(atlas);
    return 0;
}
