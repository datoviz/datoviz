/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing external memory module                                                               */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdio.h>

#include "datoviz.h"
#include "datoviz_external.h"
#include "datoviz_protocol.h"
#include "renderer.h"
#include "scene/baker.h"
#include "scene/dual.h"
#include "scene/visual.h"
#include "test.h"
#include "test_external.h"
#include "test_resources.h"
#include "testing.h"



/*************************************************************************************************/
/*  External tests                                                                               */
/*************************************************************************************************/

int test_external_1(TstSuite* suite, TstItem* tstitem)
{
    GET_HOST_GPU

    DvzRenderer* rd = dvz_renderer(gpu, 0);
    DvzBatch* batch = dvz_batch();

    // // Create a dat.
    // DvzRequest req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    // DvzId dat_id = req.id;
    // ASSERT(dat_id != DVZ_ID_NONE);
    // dvz_renderer_request(rd, req);

    DvzVisual* visual = dvz_visual(batch, DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);
    // dvz_visual_shader(visual, "graphics_trivial");
    dvz_visual_attr(visual, 0, 0, sizeof(vec3), DVZ_FORMAT_R32G32B32_SFLOAT, 0);
    dvz_visual_attr(visual, 1, sizeof(vec3), sizeof(DvzColor), DVZ_FORMAT_COLOR, 0);
    // dvz_visual_slot(visual, 0, DVZ_SLOT_DAT);
    // dvz_visual_slot(visual, 1, DVZ_SLOT_DAT);

    // DvzMVP mvp = dvz_mvp_default();
    // dvz_visual_mvp(visual, &mvp);

    // DvzViewport viewport = dvz_viewport_default(800, 600);
    // dvz_visual_viewport(visual, &viewport);

    uint32_t n = 10;
    dvz_visual_alloc(visual, n, n, 0);
    // dvz_visual_update(visual);

    dvz_renderer_requests(rd, dvz_batch_size(batch), dvz_batch_requests(batch));


    DvzSize offset = 0;
    int fd = dvz_external_vertex(rd, visual, 0, &offset);
    log_info("external handle is %d", fd);

    // Destroy the requester and renderer.
    dvz_visual_destroy(visual);

    dvz_batch_destroy(batch);
    dvz_renderer_destroy(rd);

    return fd == 0;
}
