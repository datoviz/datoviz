/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing techniques                                                                           */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <float.h>
#include <inttypes.h>
#include <stdbool.h>

#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vklite/proto.h"
#include "test_vklite.h"
#include "testing.h"



/*************************************************************************************************/
/*  Techniques                                                                                   */
/*************************************************************************************************/

int test_technique_triangle(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    // DvzSlots* slots = dvz_proto_slots(&proto);
    // ANN(slots);

    // Load the shaders.
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);

    // Create the graphics pipeline.
    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);
    AT(dvz_graphics_create(graphics) == 0);

    // Record the command buffer.
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_barriers(cmds, 0, &proto.barriers);
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, &proto.graphics);
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);
    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Save a screenshot.
    dvz_proto_screenshot(&proto, "build/technique_triangle.png");

    // Cleanup.
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);
    return 0;
}
