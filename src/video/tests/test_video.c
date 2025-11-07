/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing video                                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_assertions.h"
#include "_log.h"

#include "test_video.h"
#include "testing.h"



/*************************************************************************************************/
/*  Video tests                                                                                  */
/*************************************************************************************************/

int test_video_1(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    log_info("hello world");

    return 0;
}



/*************************************************************************************************/
/*  Entry-point                                                                                  */
/*************************************************************************************************/

int test_video(TstSuite* suite)
{
    ANN(suite);

    const char* tags = "video";

    TEST_SIMPLE(test_video_1);



    return 0;
}
