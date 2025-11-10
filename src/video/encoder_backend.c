/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Video encoder backend registry                                                               */
/*************************************************************************************************/

#include "encoder_backend.h"

#include <string.h>

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"



/*************************************************************************************************/
/*  Extern backends                                                                              */
/*************************************************************************************************/

extern const DvzVideoBackend DVZ_VIDEO_BACKEND_NVENC;
extern const DvzVideoBackend DVZ_VIDEO_BACKEND_KVAZAAR;

static const DvzVideoBackend* const BACKENDS[] = {
    &DVZ_VIDEO_BACKEND_NVENC,
    &DVZ_VIDEO_BACKEND_KVAZAAR,
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

static const DvzVideoBackend* dvz_video_backend_find(const char* name)
{
    if (!name || name[0] == '\0')
    {
        return NULL;
    }
    for (size_t i = 0; i < DVZ_ARRAY_COUNT(BACKENDS); ++i)
    {
        const DvzVideoBackend* backend = BACKENDS[i];
        if (backend && backend->name && strcmp(backend->name, name) == 0)
        {
            return backend;
        }
    }
    return NULL;
}

const DvzVideoBackend* dvz_video_backend_pick(const DvzVideoEncoderConfig* cfg)
{
    const char* sel = (cfg && cfg->backend) ? cfg->backend : "auto";
    bool auto_pick = (sel == NULL) || (strcmp(sel, "auto") == 0);

    if (!auto_pick)
    {
        const DvzVideoBackend* backend = dvz_video_backend_find(sel);
        if (backend)
        {
            if (!backend->probe || backend->probe(cfg))
            {
                return backend;
            }
            log_warn("video backend '%s' unavailable, falling back to auto", sel);
        }
        auto_pick = true;
    }

    if (auto_pick)
    {
        for (size_t i = 0; i < DVZ_ARRAY_COUNT(BACKENDS); ++i)
        {
            const DvzVideoBackend* backend = BACKENDS[i];
            if (!backend)
            {
                continue;
            }
            if (!backend->probe || backend->probe(cfg))
            {
                return backend;
            }
        }
    }

    return NULL;
}
