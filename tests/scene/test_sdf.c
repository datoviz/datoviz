/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing sdf                                                                                  */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_sdf.h"
#include "fileio.h"
#include "scene/sdf.h"
#include "test.h"
#include "testing.h"
#include "testing_utils.h"



/*************************************************************************************************/
/*  Sdf test utils                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Sdf tests                                                                                    */
/*************************************************************************************************/

int test_sdf_single(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    const char* svg_path = "M10,10 L90,10 L90,90 L10,90 Z";
    uint32_t w = 100;
    uint32_t h = 100;

    float* sdf = dvz_sdf_from_svg(svg_path, w, h);
    if (sdf == NULL)
    {
        log_error("SDF generation failed");
        return 1;
    }
    uint8_t* rgb = dvz_sdf_to_rgb(sdf, w, h);

    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/sdf_single.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, w, h, rgb);

    FREE(sdf);
    FREE(rgb);
    return 0;
}



int test_sdf_multi(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);

    // const char* svg_path = "M10,10 L90,10 L90,90 L10,90 Z";
    const char* svg_path =
        "M50,10 L61.8,35.5 L90,42 L69,61 L75,90 L50,75 L25,90 L31,61 L10,42 L38.2,35.5 Z";
    uint32_t w = 100;
    uint32_t h = 100;

    float* msdf = dvz_msdf_from_svg(svg_path, w, h);
    if (msdf == NULL)
    {
        log_error("SDF generation failed");
        return 1;
    }
    uint8_t* rgb = dvz_msdf_to_rgb(msdf, w, h);

    char imgpath[1024] = {0};
    snprintf(imgpath, sizeof(imgpath), "%s/sdf_multi.png", ARTIFACTS_DIR);
    dvz_write_png(imgpath, w, h, rgb);

    FREE(msdf);
    FREE(rgb);
    return 0;
}
