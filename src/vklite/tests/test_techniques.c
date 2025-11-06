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

#include "../../vk/tests/test_vk.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "datoviz/vklite/proto.h"
#include "datoviz/vklite/slots.h"
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

    return proto.bootstrap.instance.n_errors > 0;
}



int test_technique_render_texture(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    // Outer graphics pipeline (textured square).

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


    // Inner graphics pipeline (triangle).
    DvzGraphics igraphics = {0};
    {
        DvzDevice* device = &proto.bootstrap.device;
        ANN(device);

        // Load the shaders.
        DvzSize ivs_size = 0;
        DvzSize ifs_size = 0;
        DvzShader ivs = {0};
        DvzShader ifs = {0};
        uint32_t* ivs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &ivs_size);
        uint32_t* ifs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &ifs_size);

        // Initialize the graphics pipeline.
        dvz_graphics(device, &igraphics);


        // Shaders.
        dvz_shader(device, ivs_size, ivs_spv, &ivs);
        dvz_shader(device, ifs_size, ifs_spv, &ifs);
        dvz_graphics_shader(&igraphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&ivs));
        dvz_graphics_shader(&igraphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&ifs));

        // Slots.
        DvzSlots islots = {0};
        dvz_slots(device, &islots);
        dvz_slots_create(&islots);

        // Attachments.
        dvz_graphics_attachment_color(&igraphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
        dvz_graphics_blend_color(
            &igraphics, 0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD, 0xF);
        dvz_graphics_blend_alpha(
            &igraphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);

        // Fixed state.
        dvz_graphics_primitive(
            &igraphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

        // Dynamic state.
        dvz_graphics_viewport(
            &igraphics, 0, 0, DVZ_PROTO_WIDTH / 2.0, DVZ_PROTO_HEIGHT / 2.0, 0, 1,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);
        dvz_graphics_scissor(
            &igraphics, 0, 0, DVZ_PROTO_WIDTH / 2.0, DVZ_PROTO_HEIGHT / 2.0,
            DVZ_GRAPHICS_FLAGS_DYNAMIC);

        // Slots
        dvz_graphics_layout(&igraphics, dvz_slots_handle(&islots));

        // Create the graphics pipeline.
        AT(dvz_graphics_create(&igraphics) == 0);

        // Cleanup.
        dvz_slots_destroy(&islots);
        dvz_shader_destroy(&ivs);
        dvz_shader_destroy(&ifs);
        dvz_free(ivs_spv);
        dvz_free(ifs_spv);
    }


    // Image to render to, and to use as a texture.
    DvzImages* tex = &proto.tex;
    DvzImageViews* tex_view = &proto.tex_view;
    {
        ANN(tex);
        dvz_images(&proto.bootstrap.device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, tex);
        dvz_images_format(tex, VK_FORMAT_R8G8B8A8_UNORM);
        dvz_images_size(tex, DVZ_PROTO_WIDTH / 2, DVZ_PROTO_HEIGHT / 2, 1);
        dvz_images_usage(tex, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        dvz_images_create(tex);

        // Image views.
        ANN(tex_view);
        dvz_image_views(tex, tex_view);
        dvz_image_views_create(tex_view);
    }


    // Inner rendering.
    DvzRendering irendering = {0};
    {
        dvz_rendering(&irendering);
        dvz_rendering_area(&irendering, 0, 0, DVZ_PROTO_WIDTH / 2, DVZ_PROTO_HEIGHT / 2);

        // Attachments.
        DvzAttachment* attachment = dvz_rendering_color(&irendering, 0);
        dvz_attachment_image(
            attachment, dvz_image_views_handle(tex_view, 0),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        dvz_attachment_clear(attachment, (VkClearValue){.color.float32 = {.8, .2, .2, .5}});
    }


    // Sampler.
    DvzSampler* sampler = &proto.sampler;
    {
        ANN(sampler);
        dvz_sampler(&proto.bootstrap.device, sampler);
        dvz_sampler_min_filter(sampler, VK_FILTER_LINEAR);
        dvz_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
        dvz_sampler_address_mode(
            sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        dvz_sampler_anisotropy(sampler, 8);
        AT(dvz_sampler_create(sampler) == 0);
    }


    // Barrier for the outer rendering.
    DvzBarrierImage* bimg = dvz_barriers_image(&proto.barriers, dvz_image_handle(&proto.img, 0));
    {
        ANN(bimg);
        dvz_barrier_image_stage(
            bimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        dvz_barrier_image_layout(
            bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }


    // Barrier for the inner rendering.
    DvzBarriers ibarriers = {0};
    dvz_barriers(&ibarriers);
    DvzBarrierImage* ibimg = dvz_barriers_image(&ibarriers, dvz_image_handle(tex, 0));
    ANN(ibimg);
    // Initial image transition for inner color attachment.
    {
        dvz_barrier_image_stage(
            ibimg, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        dvz_barrier_image_access(ibimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        dvz_barrier_image_layout(
            ibimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }


    // Descriptors.
    {
        dvz_descriptors(&proto.slots, &proto.desc);
        dvz_descriptors_image(
            &proto.desc, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, //
            tex_view->vk_views[0], sampler->vk_sampler);
    }


    // Record the command buffer.
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    dvz_cmd_begin(cmds);

    // Initial barrier for the texture: inner rendering.
    dvz_cmd_barriers(cmds, 0, &ibarriers); // image transition of inner image for render

    // Inner rendering.
    {
        dvz_cmd_rendering_begin(cmds, 0, &irendering);
        dvz_cmd_bind_graphics(cmds, 0, &igraphics);
        dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
        dvz_cmd_rendering_end(cmds, 0);
    }

    // Now, need to transition inner image from render to texture.
    {
        dvz_barrier_image_stage(
            ibimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        dvz_barrier_image_access(
            ibimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT);
        dvz_barrier_image_layout(
            ibimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        dvz_cmd_barriers(cmds, 0, &ibarriers);
    }

    // Outer rendering.
    dvz_cmd_barriers(cmds, 0, &proto.barriers); // image transition of outer image for render
    {
        dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
        dvz_cmd_bind_graphics(cmds, 0, &proto.graphics);
        dvz_cmd_bind_descriptors(
            cmds, 0, VK_PIPELINE_BIND_POINT_GRAPHICS, &proto.desc, 0, 1, 0, NULL);
        dvz_cmd_draw(cmds, 0, 0, 6, 0, 1);
        dvz_cmd_rendering_end(cmds, 0);
    }

    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Save a screenshot.
    dvz_proto_screenshot(&proto, "build/technique_render_texture.png");

    // Cleanup.
    dvz_graphics_destroy(&igraphics);
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    return proto.bootstrap.instance.n_errors > 0;
}
