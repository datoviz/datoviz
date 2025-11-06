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
#include "vulkan_core.h"



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

    // Load the shaders.
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);

    // Get the graphics pipeline
    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);

    // Slots
    DvzSlots* slots = dvz_proto_slots(&proto);
    ANN(slots);
    dvz_slots_create(slots);
    dvz_graphics_layout(graphics, dvz_slots_handle(slots));

    // Create the graphics pipeline.
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



int test_technique_render_texture(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    // Load the shaders.
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_square.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_square.frag.spv", &fs_size);

    // Initialize graphics pipeline.
    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);

    // Slots
    DvzSlots* slots = dvz_proto_slots(&proto);
    ANN(slots);
    dvz_slots_binding(
        slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    dvz_slots_create(slots);
    dvz_graphics_layout(graphics, dvz_slots_handle(slots));

    // Create the graphics pipeline.
    AT(dvz_graphics_create(graphics) == 0);


    // Texture.
    DvzImages* tex = &proto.tex;
    ANN(tex);
    dvz_images(&proto.bootstrap.device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, tex);
    dvz_images_format(tex, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_images_size(tex, 256, 256, 1);
    dvz_images_layout(tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    dvz_images_usage(tex, VK_IMAGE_USAGE_SAMPLED_BIT);
    dvz_images_create(tex);

    // Image views.
    DvzImageViews* tex_view = &proto.tex_view;
    ANN(tex_view);
    dvz_image_views(tex, tex_view);
    dvz_image_views_create(tex_view);

    // Image transition.
    dvz_proto_transition(
        &proto, tex, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    DvzSampler* sampler = &proto.sampler;
    ANN(sampler);
    dvz_sampler(&proto.bootstrap.device, sampler);
    dvz_sampler_min_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_anisotropy(sampler, 8);
    AT(dvz_sampler_create(sampler) == 0);

    // Descriptors.
    dvz_descriptors(&proto.slots, &proto.desc);
    dvz_descriptors_image(
        &proto.desc, 0, 0, 0, tex->layout, tex_view->vk_views[0], sampler->vk_sampler);

    // Record the command buffer.
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_barriers(cmds, 0, &proto.barriers);
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, &proto.graphics);
    dvz_cmd_bind_descriptors(cmds, 0, VK_PIPELINE_BIND_POINT_GRAPHICS, &proto.desc, 0, 1, 0, NULL);
    dvz_cmd_draw(cmds, 0, 0, 6, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);
    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Save a screenshot.
    dvz_proto_screenshot(&proto, "build/technique_render_texture.png");

    // Cleanup.
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);
    return 0;
}
