/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing color                                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <math.h>
#include <stddef.h>

#include "_assertions.h"
#include "datoviz/common/types.h"
#include "test_math.h"
#include "testing.h"



/*************************************************************************************************/
/*  Color tests                                                                                  */
/*************************************************************************************************/

int test_color_layout(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    AT(sizeof(DvzColor) == 4);
    AT(offsetof(DvzColor, r) == 0);
    AT(offsetof(DvzColor, g) == 1);
    AT(offsetof(DvzColor, b) == 2);
    AT(offsetof(DvzColor, a) == 3);

    DvzColor colors[2] = {dvz_color_rgba(1, 2, 3, 4), dvz_color_rgba(5, 6, 7, 8)};
    const uint8_t* bytes = (const uint8_t*)colors;
    AT(bytes[0] == 1);
    AT(bytes[1] == 2);
    AT(bytes[2] == 3);
    AT(bytes[3] == 4);
    AT(bytes[4] == 5);
    AT(bytes[5] == 6);
    AT(bytes[6] == 7);
    AT(bytes[7] == 8);

    return 0;
}



int test_color_helpers(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    AT(dvz_color_u8(NAN) == 0);
    AT(dvz_color_u8(-INFINITY) == 0);
    AT(dvz_color_u8(-0.1f) == 0);
    AT(dvz_color_u8(0.0f) == 0);
    AT(dvz_color_u8(0.5f) == 128);
    AT(dvz_color_u8(1.0f / 255.0f) == 1);
    AT(dvz_color_u8(0.999f) == 255);
    AT(dvz_color_u8(1.0f) == 255);
    AT(dvz_color_u8(INFINITY) == 0);

    DvzColor color = dvz_color_from_unit(1.0f, 0.5f, 0.0f, 0.25f);
    AT(color.r == 255);
    AT(color.g == 128);
    AT(color.b == 0);
    AT(color.a == 64);

    color = dvz_color_hex_rgb(0x123456);
    AT(color.r == 0x12);
    AT(color.g == 0x34);
    AT(color.b == 0x56);
    AT(color.a == 0xff);

    color = dvz_color_hex_rgba(0x12345678);
    AT(color.r == 0x12);
    AT(color.g == 0x34);
    AT(color.b == 0x56);
    AT(color.a == 0x78);

    return 0;
}



int test_color_linear_roundtrip(TstContext* suite, const TstCase* tstitem)
{
    ANN(suite);

    const DvzColor source = dvz_color_rgba(18, 127, 250, 64);
    const DvzColorf linear = dvz_color_to_linear(source);
    const DvzColor roundtrip = dvz_color_from_linear(linear);

    AT(roundtrip.r == source.r);
    AT(roundtrip.g == source.g);
    AT(roundtrip.b == source.b);
    AT(roundtrip.a == source.a);

    const DvzColor clipped = dvz_color_from_linear(dvz_colorf(-1.0f, 0.0f, 4.0f, 2.0f));
    AT(clipped.r == 0);
    AT(clipped.g == 0);
    AT(clipped.b == 255);
    AT(clipped.a == 255);

    return 0;
}
