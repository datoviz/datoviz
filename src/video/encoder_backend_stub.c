/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video backend stubs                                                                          */
/*************************************************************************************************/

#include "encoder_backend.h"

#include "_log.h"



/*************************************************************************************************/
/*  NVENC video backend stubs                                                                    */
/*************************************************************************************************/

#if !DVZ_HAS_CUDA



static bool nvenc_probe(const DvzVideoEncoderConfig* cfg)
{
    (void)cfg;
    return false;
}



static int nvenc_init(DvzVideoEncoder* enc)
{
    (void)enc;
    return -1;
}



static int nvenc_start(DvzVideoEncoder* enc)
{
    (void)enc;
    log_warn("NVENC backend unavailable; rebuild with DVZ_HAS_CUDA=ON");
    return -1;
}



static int nvenc_submit(DvzVideoEncoder* enc, uint64_t timeline_value)
{
    (void)enc;
    (void)timeline_value;
    return -1;
}



static int nvenc_stop(DvzVideoEncoder* enc)
{
    (void)enc;
    return 0;
}



static void nvenc_destroy(DvzVideoEncoder* enc) { (void)enc; }



const DvzVideoBackend DVZ_VIDEO_BACKEND_NVENC = {
    .name = "nvenc",
    .probe = nvenc_probe,
    .init = nvenc_init,
    .start = nvenc_start,
    .submit = nvenc_submit,
    .stop = nvenc_stop,
    .destroy = nvenc_destroy,
};



#endif



/*************************************************************************************************/
/*  NVENC video backend stubs                                                                    */
/*************************************************************************************************/

#if !DVZ_HAS_KVZ



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
    log_warn("kvazaar backend unavailable; rebuild with DVZ_HAS_KVZ=ON");
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



static void kvazaar_destroy(DvzVideoEncoder* enc) { (void)enc; }



const DvzVideoBackend DVZ_VIDEO_BACKEND_KVAZAAR = {
    .name = "kvazaar",
    .probe = kvazaar_probe,
    .init = kvazaar_init,
    .start = kvazaar_start,
    .submit = kvazaar_submit,
    .stop = kvazaar_stop,
    .destroy = kvazaar_destroy,
};



#endif
