/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#ifndef DVZ_VIDEO_FILE_UTILS_HEADER
#define DVZ_VIDEO_FILE_UTILS_HEADER

#include <stdint.h>
#include <stdio.h>
#if !defined(_WIN32) && !defined(_MSC_VER)
#include <sys/types.h>
#endif



static inline int dvz_video_file_seek64(FILE* fp, int64_t offset, int origin)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return _fseeki64(fp, offset, origin);
#else
    return fseeko(fp, (off_t)offset, origin);
#endif
}



static inline int64_t dvz_video_file_tell64(FILE* fp)
{
#if defined(_WIN32) || defined(_MSC_VER)
    return _ftelli64(fp);
#else
    return (int64_t)ftello(fp);
#endif
}

#endif
