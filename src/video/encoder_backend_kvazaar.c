/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Kvazaar backend placeholder                                                                  */
/*************************************************************************************************/

#include "encoder_backend.h"

#include <string.h>

#include "_log.h"



/*************************************************************************************************/
/*  Forward declarations                                                                          */
/*************************************************************************************************/

static bool kvazaar_probe(const DvzVideoEncoderConfig* cfg);
static int kvazaar_init(DvzVideoEncoder* enc);
static int kvazaar_start(DvzVideoEncoder* enc);
static int kvazaar_submit(DvzVideoEncoder* enc, uint64_t timeline_value);
static int kvazaar_stop(DvzVideoEncoder* enc);
static void kvazaar_destroy(DvzVideoEncoder* enc);



/*************************************************************************************************/
/*  Public backend                                                                               */
/*************************************************************************************************/

const DvzVideoBackend DVZ_VIDEO_BACKEND_KVAZAAR = {
    .name = "kvazaar",
    .probe = kvazaar_probe,
    .init = kvazaar_init,
    .start = kvazaar_start,
    .submit = kvazaar_submit,
    .stop = kvazaar_stop,
    .destroy = kvazaar_destroy,
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static bool kvazaar_probe(const DvzVideoEncoderConfig* cfg)
{
    (void)cfg;
    return false;
}

static int kvazaar_init(DvzVideoEncoder* enc)
{
    (void)enc;
    return -1;
}

static int kvazaar_start(DvzVideoEncoder* enc)
{
    (void)enc;
    log_warn("kvazaar backend is not implemented yet");
    return -1;
}

static int kvazaar_submit(DvzVideoEncoder* enc, uint64_t timeline_value)
{
    (void)enc;
    (void)timeline_value;
    return -1;
}

static int kvazaar_stop(DvzVideoEncoder* enc)
{
    (void)enc;
    return 0;
}

static void kvazaar_destroy(DvzVideoEncoder* enc)
{
    (void)enc;
}
