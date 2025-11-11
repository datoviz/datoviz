/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream tests                                                                                 */
/*************************************************************************************************/

#include "test_stream.h"

#include "_alloc.h"
#include "datoviz/stream/frame_stream.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

static int test_stream_attach_video(TstSuite* suite, TstItem* item)
{
    (void)item;
    ANN(suite);

    DvzFrameStreamConfig cfg = dvz_frame_stream_default_config();
    DvzFrameStream* stream = dvz_frame_stream_create(NULL, &cfg);
    AT(stream != NULL);

    DvzVideoSinkConfig vc = dvz_video_sink_default_config();
    AT(dvz_frame_stream_attach_sink(stream, dvz_video_sink_backend(), &vc) == 0);

    dvz_frame_stream_destroy(stream);
    return 0;
}

int test_stream(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "stream";
    TEST_SIMPLE(test_stream_attach_video);
    return 0;
}
