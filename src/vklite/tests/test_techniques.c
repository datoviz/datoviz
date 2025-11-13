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
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/proto.h"
#include "datoviz/vklite/rendering.h"
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
    DvzImages tex = {0};
    DvzImageViews tex_view = {0};
    {
        ANN(&tex);
        dvz_images(&proto.bootstrap.device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &tex);
        dvz_images_format(&tex, VK_FORMAT_R8G8B8A8_UNORM);
        dvz_images_size(&tex, DVZ_PROTO_WIDTH / 2, DVZ_PROTO_HEIGHT / 2, 1);
        dvz_images_usage(&tex, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        dvz_images_create(&tex);

        // Image views.
        ANN(&tex_view);
        dvz_image_views(&tex, &tex_view);
        dvz_image_views_create(&tex_view);
    }


    // Inner rendering.
    DvzRendering irendering = {0};
    {
        dvz_rendering(&irendering);
        dvz_rendering_area(&irendering, 0, 0, DVZ_PROTO_WIDTH / 2, DVZ_PROTO_HEIGHT / 2);

        // Attachments.
        DvzAttachment* attachment = dvz_rendering_color(&irendering, 0);
        dvz_attachment_image(
            attachment, dvz_image_views_handle(&tex_view, 0),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(attachment, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        dvz_attachment_clear(attachment, (VkClearValue){.color.float32 = {.5, .5, .5, 1}});
    }


    // Sampler.
    DvzSampler sampler = {0};
    {
        ANN(&sampler);
        dvz_sampler(&proto.bootstrap.device, &sampler);
        dvz_sampler_min_filter(&sampler, VK_FILTER_LINEAR);
        dvz_sampler_mag_filter(&sampler, VK_FILTER_LINEAR);
        dvz_sampler_address_mode(
            &sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        dvz_sampler_anisotropy(&sampler, 8);
        AT(dvz_sampler_create(&sampler) == 0);
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
    DvzBarrierImage* ibimg = dvz_barriers_image(&ibarriers, dvz_image_handle(&tex, 0));
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
            tex_view.vk_views[0], sampler.vk_sampler);
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
    dvz_images_destroy(&tex);
    dvz_image_views_destroy(&tex_view);
    dvz_sampler_destroy(&sampler);
    dvz_graphics_destroy(&igraphics);
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    return proto.bootstrap.instance.n_errors > 0;
}



int test_technique_stencil(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    // Mask pipeline.
    DvzGraphics mgraphics = {0};
    {
        DvzDevice* device = &proto.bootstrap.device;
        ANN(device);

        // Initialize the graphics pipeline.
        dvz_graphics(device, &mgraphics);

        // Load the shaders.
        DvzSize vs_size = 0;
        DvzSize fs_size = 0;
        DvzShader vs = {0};
        DvzShader fs = {0};
        uint32_t* vs_spv = dvz_test_shader_load("fullscreen.vert.spv", &vs_size);
        uint32_t* fs_spv = dvz_test_shader_load("disc_mask.frag.spv", &fs_size);

        // Shaders.
        dvz_shader(device, vs_size, vs_spv, &vs);
        dvz_shader(device, fs_size, fs_spv, &fs);
        dvz_graphics_shader(&mgraphics, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&vs));
        dvz_graphics_shader(&mgraphics, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs));

        // Attachments.
        dvz_graphics_attachment_color(&mgraphics, 0, VK_FORMAT_R8G8B8A8_UNORM);
        dvz_graphics_attachment_depth(&mgraphics, VK_FORMAT_D32_SFLOAT_S8_UINT);
        dvz_graphics_attachment_stencil(&mgraphics, VK_FORMAT_D32_SFLOAT_S8_UINT);
        dvz_graphics_blend_color(&mgraphics, 0, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, 0, 0);

        // Fixed state.
        dvz_graphics_primitive(
            &mgraphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

        // Dynamic state.
        dvz_graphics_viewport(
            &mgraphics, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);

        dvz_graphics_scissor(
            &mgraphics, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);

        dvz_graphics_stencil(
            &mgraphics, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE,
            VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0xFF, 0xFF, 1, DVZ_GRAPHICS_FLAGS_FIXED);

        dvz_graphics_depth(
            &mgraphics, false, false, VK_COMPARE_OP_ALWAYS, DVZ_GRAPHICS_FLAGS_FIXED);

        // Slots
        DvzSlots slots = {0};
        dvz_slots(device, &slots);
        dvz_slots_create(&slots);
        dvz_graphics_layout(&mgraphics, dvz_slots_handle(&slots));

        // Create the graphics pipeline.
        AT(dvz_graphics_create(&mgraphics) == 0);

        // Cleanup.
        dvz_slots_destroy(&slots);
        dvz_shader_destroy(&vs);
        dvz_shader_destroy(&fs);
        dvz_free(vs_spv);
        dvz_free(fs_spv);
    }

    // Mask rendering.
    DvzRendering mrendering = {0};
    {
        dvz_rendering(&mrendering);
        dvz_rendering_area(&mrendering, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT);

        // Attachments.
        DvzAttachment* catt = dvz_rendering_color(&mrendering, 0);
        dvz_attachment_image(
            catt, dvz_image_views_handle(&proto.view, 0),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(
            catt, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        // Attachments.
        DvzAttachment* datt = dvz_rendering_depth(&mrendering);
        dvz_attachment_image(
            datt, dvz_image_views_handle(&proto.dview, 0),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(datt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        dvz_attachment_clear(datt, (VkClearValue){.depthStencil = {1.0f, 0}});

        DvzAttachment* satt = dvz_rendering_stencil(&mrendering);
        dvz_attachment_image(
            satt, dvz_image_views_handle(&proto.dview, 0),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        dvz_attachment_ops(satt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
        dvz_attachment_clear(satt, (VkClearValue){.depthStencil = {1.0f, 0}});
    }

    // Load the shaders.
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);

    // Get the graphics pipeline
    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);

    dvz_graphics_stencil(
        graphics, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
        VK_STENCIL_OP_KEEP, VK_COMPARE_OP_EQUAL, 0xFF, 0, 1, DVZ_GRAPHICS_FLAGS_FIXED);

    dvz_graphics_depth(graphics, false, false, VK_COMPARE_OP_ALWAYS, DVZ_GRAPHICS_FLAGS_FIXED);

    // Slots
    DvzSlots* slots = dvz_proto_slots(&proto);
    ANN(slots);
    dvz_slots_create(slots);
    dvz_graphics_layout(graphics, dvz_slots_handle(slots));

    // Create the graphics pipeline.
    AT(dvz_graphics_create(graphics) == 0);

    // Change the depth attachment settings to LOAD the depth values.
    DvzAttachment* datt = dvz_rendering_depth(&proto.rendering);
    dvz_attachment_ops(datt, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    DvzAttachment* satt = dvz_rendering_stencil(&proto.rendering);
    dvz_attachment_ops(satt, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    // Record the command buffer.
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    dvz_cmd_begin(cmds);
    dvz_cmd_barriers(cmds, 0, &proto.barriers);

    // Mask rendering.
    dvz_cmd_rendering_begin(cmds, 0, &mrendering);
    dvz_cmd_bind_graphics(cmds, 0, &mgraphics);
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);

    // Triangle rendering.
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, &proto.graphics);
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);

    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Save a screenshot.
    dvz_proto_screenshot(&proto, "build/technique_stencil.png");

    // Cleanup.
    dvz_graphics_destroy(&mgraphics);
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    return proto.bootstrap.instance.n_errors > 0;
}



int test_technique_msaa(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize the proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    VkSampleCountFlags sample_count = VK_SAMPLE_COUNT_4_BIT;

    // Multisampled image.
    DvzImages msimg = {0};
    DvzImageViews msimg_view = {0};
    DvzImages msdepth = {0};
    DvzImageViews msdepth_view = {0};
    {
        ANN(&msimg);
        dvz_images(
            &proto.bootstrap.device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &msimg);
        dvz_images_format(&msimg, VK_FORMAT_R8G8B8A8_UNORM);
        dvz_images_samples(&msimg, sample_count);
        dvz_images_size(&msimg, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
        dvz_images_usage(&msimg, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        dvz_images_create(&msimg);

        // Image views.
        ANN(&msimg_view);
        dvz_image_views(&msimg, &msimg_view);
        dvz_image_views_create(&msimg_view);

        ANN(&msdepth);
        dvz_images(
            &proto.bootstrap.device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &msdepth);
        dvz_images_format(&msdepth, VK_FORMAT_D32_SFLOAT_S8_UINT);
        dvz_images_samples(&msdepth, sample_count);
        dvz_images_size(&msdepth, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
        dvz_images_usage(&msdepth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        dvz_images_create(&msdepth);

        // Image views.
        ANN(&msdepth_view);
        dvz_image_views(&msdepth, &msdepth_view);
        dvz_image_views_aspect(
            &msdepth_view, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        dvz_image_views_create(&msdepth_view);
    }



    // Load the shaders.
    DvzSize vs_size = 0;
    DvzSize fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_triangle.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_triangle.frag.spv", &fs_size);

    // Get the graphics pipeline
    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);

    dvz_graphics_multisampling(graphics, sample_count, 0.5, 0);

    // Slots
    DvzSlots* slots = dvz_proto_slots(&proto);
    ANN(slots);
    dvz_slots_create(slots);
    dvz_graphics_layout(graphics, dvz_slots_handle(slots));

    // Create the graphics pipeline.
    AT(dvz_graphics_create(graphics) == 0);


    // Resolve in rendering.
    DvzRendering* rendering = &proto.rendering;
    ANN(rendering);
    DvzAttachment* att = dvz_rendering_color(rendering, 0);
    ANN(att);
    dvz_attachment_image(
        att, dvz_image_views_handle(&msimg_view, 0), VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    dvz_attachment_resolve(
        att, VK_RESOLVE_MODE_AVERAGE_BIT, dvz_image_views_handle(&proto.view, 0),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(att, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    DvzAttachment* datt = dvz_rendering_depth(rendering);
    dvz_attachment_image(
        datt, dvz_image_views_handle(&msdepth_view, 0),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_attachment_resolve(
        datt, VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, dvz_image_views_handle(&proto.dview, 0),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    DvzAttachment* satt = dvz_rendering_stencil(rendering);
    dvz_attachment_image(
        satt, dvz_image_views_handle(&msdepth_view, 0),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_attachment_resolve(
        satt, VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, dvz_image_views_handle(&proto.dview, 0),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    // Record the command buffer.
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);
    dvz_cmd_begin(cmds);
    // NOTE: prevent a write after write sync hazard: we don't do the initial image transition on
    // the resolve image since it is written by the MSAA rendering
    // dvz_cmd_barriers(cmds, 0, &proto.barriers);
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, &proto.graphics);
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);
    dvz_cmd_end(cmds);

    // Submit the command buffer.
    dvz_cmd_submit(cmds);

    // Save a screenshot.
    dvz_proto_screenshot(&proto, "build/technique_msaa.png");

    // Cleanup.
    dvz_image_views_destroy(&msimg_view);
    dvz_image_views_destroy(&msdepth_view);
    dvz_images_destroy(&msimg);
    dvz_images_destroy(&msdepth);
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    return proto.bootstrap.instance.n_errors > 0;
}



int test_technique_compute_graphics(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    DvzDevice* device = &proto.bootstrap.device;
    ANN(device);

    // Step 1: create storage/vertex buffer

    // Positions of 4 vertices (square; triangle strip)
    vec2 positions[4] = {
        {-0.5f, -0.5f},
        {0.5f, -0.5f},
        {-0.5f, 0.5f},
        {0.5f, 0.5f},
    };

    DvzBuffer buf = {0};
    dvz_buffer(device, &proto.bootstrap.allocator, &buf);
    dvz_buffer_size(&buf, sizeof(positions));
    dvz_buffer_usage(
        &buf, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_flags(&buf, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    AT(dvz_buffer_create(&buf) == 0);

    // Upload initial data
    dvz_buffer_upload(&buf, 0, sizeof(positions), positions);


    // Step 2: set up slots (descriptor layout)
    DvzSlots slots = {0};
    dvz_slots(device, &slots);
    // set = 0, binding = 0, storage buffer
    dvz_slots_binding(
        &slots, 0, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_slots_create(&slots);


    // Step 3: create compute pipeline
    DvzSize cs_size = 0;
    uint32_t* cs_spv = dvz_test_shader_load("compute_increment.comp.spv", &cs_size);

    DvzShader cs = {0};
    dvz_shader(device, cs_size, cs_spv, &cs);

    DvzCompute compute = {0};
    dvz_compute(device, &compute);
    dvz_compute_shader(&compute, dvz_shader_handle(&cs));
    dvz_compute_layout(&compute, dvz_slots_handle(&slots));
    AT(dvz_compute_create(&compute) == 0);

    // Descriptors for compute
    DvzDescriptors desc = {0};
    dvz_descriptors(&slots, &desc);
    dvz_descriptors_buffer(&desc, 0, 0, 0, dvz_buffer_handle(&buf), 0, sizeof(positions));


    // Step 4: create graphics pipeline
    DvzSize vs_size = 0, fs_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("hello_compute.vert.spv", &vs_size);
    uint32_t* fs_spv = dvz_test_shader_load("hello_compute.frag.spv", &fs_size);

    DvzGraphics* graphics = dvz_proto_graphics(&proto, vs_size, vs_spv, fs_size, fs_spv);
    ANN(graphics);

    // One color attachment, no depth/stencil needed
    dvz_graphics_attachment_color(graphics, 0, VK_FORMAT_R8G8B8A8_UNORM);

    // Vertex input: vec2
    dvz_graphics_vertex_binding(graphics, 0, sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32_SFLOAT, 0);

    // Layout: reuse same slots (even though graphics doesn't use the storage buffer)
    dvz_graphics_layout(graphics, dvz_slots_handle(&slots));
    dvz_graphics_primitive(
        graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, DVZ_GRAPHICS_FLAGS_FIXED);
    AT(dvz_graphics_create(graphics) == 0);


    // Step 5: record command buffer
    DvzCommands* cmds = dvz_proto_commands(&proto);
    ANN(cmds);

    dvz_cmd_begin(cmds);

    // Compute pass
    dvz_cmd_bind_compute(cmds, 0, &compute);
    dvz_cmd_bind_descriptors(cmds, 0, VK_PIPELINE_BIND_POINT_COMPUTE, &desc, 0, 1, 0, NULL);

    // Dispatch 4 workgroups → each vertex is shifted in compute shader
    dvz_cmd_dispatch(cmds, 0, 4, 1, 1);

    // Barrier: compute write → graphics vertex read
    {
        DvzBarrierBuffer* b =
            dvz_barriers_buffer(&proto.barriers, dvz_buffer_handle(&buf), 0, sizeof(positions));
        dvz_barrier_buffer_stage(
            b, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT);
        dvz_barrier_buffer_access(
            b, VK_ACCESS_2_SHADER_WRITE_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);
        dvz_cmd_barriers(cmds, 0, &proto.barriers);
    }

    // Graphics pass
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, graphics);
    dvz_cmd_bind_vertex_buffers(cmds, 0, 0, 1, &buf, (DvzSize[]){0});
    dvz_cmd_draw(cmds, 0, 0, 4, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);
    dvz_cmd_end(cmds);
    dvz_cmd_submit(cmds);


    // Step 6: save screenshot
    dvz_proto_screenshot(&proto, "build/technique_compute_graphics.png");


    // Step 7: cleanup
    dvz_slots_destroy(&slots);
    dvz_shader_destroy(&cs);
    dvz_compute_destroy(&compute);
    dvz_graphics_destroy(graphics);
    dvz_buffer_destroy(&buf);
    dvz_proto_destroy(&proto);
    dvz_free(cs_spv);
    dvz_free(vs_spv);
    dvz_free(fs_spv);

    return proto.bootstrap.instance.n_errors > 0;
}



int test_technique_picking(TstSuite* suite, TstItem* tstitem)
{
    ANN(suite);
    ANN(tstitem);

    // Initialize proto.
    DvzProto proto = {0};
    dvz_proto(&proto);

    DvzDevice* device = &proto.bootstrap.device;
    ANN(device);

    DvzSlots* slots = dvz_proto_slots(&proto);
    ANN(slots);
    AT(dvz_slots_create(slots) == 0);


    // Step 1: vertex buffer with 3 vertices and ID 42
    typedef struct
    {
        vec2 pos;
        uint32_t id;
    } Vertex;
    Vertex verts[3] = {
        {{-0.5f, -0.5f}, 42},
        {{0.5f, -0.5f}, 42},
        {{0.0f, 0.5f}, 42},
    };

    DvzBuffer vbuf = {0};
    dvz_buffer(device, &proto.bootstrap.allocator, &vbuf);
    dvz_buffer_size(&vbuf, sizeof(verts));
    dvz_buffer_usage(&vbuf, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_flags(&vbuf, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    AT(dvz_buffer_create(&vbuf) == 0);
    dvz_buffer_upload(&vbuf, 0, sizeof(verts), verts);


    // Step 2: picking attachment (R32_UINT)
    DvzImages pickimg = {0};
    dvz_images(device, &proto.bootstrap.allocator, VK_IMAGE_TYPE_2D, 1, &pickimg);
    dvz_images_format(&pickimg, VK_FORMAT_R32_UINT);
    dvz_images_size(&pickimg, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 1);
    dvz_images_usage(
        &pickimg, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_vma_flags(&pickimg, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    AT(dvz_images_create(&pickimg) == 0);

    DvzImageViews pickview = {0};
    dvz_image_views(&pickimg, &pickview);
    dvz_image_views_aspect(&pickview, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_image_views_type(&pickview, VK_IMAGE_VIEW_TYPE_2D);
    dvz_image_views_create(&pickview);



    // Transition pickimg → COLOR_ATTACHMENT_OPTIMAL before using it as a render target.
    {
        DvzBarriers barriers = {0};
        dvz_barriers(&barriers);

        DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(&pickimg, 0));
        dvz_barrier_image_stage(
            bimg, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
        dvz_barrier_image_access(bimg, 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        dvz_barrier_image_layout(
            bimg, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_COLOR_BIT);
        dvz_barrier_image_mip(bimg, 0, 1);
        dvz_barrier_image_layers(bimg, 0, 1);

        DvzCommands* cmds = dvz_proto_commands(&proto);
        dvz_cmd_begin(cmds);
        dvz_cmd_barriers(cmds, 0, &barriers);
        dvz_cmd_end(cmds);
        dvz_cmd_submit(cmds); // simple blocking submit in vklite
    }



    // Step 3: load shaders
    DvzSize vs_size = 0, cs_size = 0, ps_size = 0;
    uint32_t* vs_spv = dvz_test_shader_load("standard.vert.spv", &vs_size);
    uint32_t* fs_color_spv = dvz_test_shader_load("standard.frag.spv", &cs_size);
    uint32_t* fs_pick_spv = dvz_test_shader_load("pick.frag.spv", &ps_size);


    // Step 4: graphics pipeline A — color
    DvzGraphics* gfx_color = dvz_proto_graphics(&proto, vs_size, vs_spv, cs_size, fs_color_spv);
    ANN(gfx_color);

    dvz_graphics_attachment_color(gfx_color, 0, VK_FORMAT_R8G8B8A8_UNORM);

    dvz_graphics_vertex_binding(gfx_color, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(gfx_color, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
    dvz_graphics_vertex_attr(gfx_color, 0, 1, VK_FORMAT_R32_UINT, offsetof(Vertex, id));

    dvz_graphics_layout(gfx_color, dvz_slots_handle(slots));
    AT(dvz_graphics_create(gfx_color) == 0);


    // Step 5: graphics pipeline B — picking (same VS, different FS)
    DvzGraphics gfx_pick = {0};
    dvz_graphics(device, &gfx_pick);

    // Shared vertex shader.
    DvzShader vs = {0};
    dvz_shader(device, vs_size, vs_spv, &vs);
    dvz_graphics_shader(&gfx_pick, VK_SHADER_STAGE_VERTEX_BIT, dvz_shader_handle(&vs));

    // Picking fragment shader.
    DvzShader fs_pick = {0};
    dvz_shader(device, ps_size, fs_pick_spv, &fs_pick);
    dvz_graphics_shader(&gfx_pick, VK_SHADER_STAGE_FRAGMENT_BIT, dvz_shader_handle(&fs_pick));

    dvz_graphics_attachment_color(&gfx_pick, 0, VK_FORMAT_R32_UINT);

    dvz_graphics_vertex_binding(&gfx_pick, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(&gfx_pick, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos));
    dvz_graphics_vertex_attr(&gfx_pick, 0, 1, VK_FORMAT_R32_UINT, offsetof(Vertex, id));

    dvz_graphics_viewport(
        &gfx_pick, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(
        &gfx_pick, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_layout(&gfx_pick, dvz_slots_handle(slots));

    dvz_graphics_primitive(
        &gfx_pick, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);

    AT(dvz_graphics_create(&gfx_pick) == 0);


    // Step 5.5: create staging buffer for reading back one uint32 ID
    DvzBuffer staging = {0};
    dvz_buffer(device, &proto.bootstrap.allocator, &staging);
    dvz_buffer_size(&staging, sizeof(uint32_t));
    dvz_buffer_usage(&staging, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_flags(
        &staging, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
    AT(dvz_buffer_create(&staging) == 0);


    // Step 6: record command buffer
    DvzCommands* cmds = dvz_proto_commands(&proto);
    dvz_cmd_begin(cmds);

    /* COLOR PASS */
    dvz_cmd_rendering_begin(cmds, 0, &proto.rendering);
    dvz_cmd_bind_graphics(cmds, 0, gfx_color);
    dvz_cmd_bind_vertex_buffers(cmds, 0, 0, 1, &vbuf, (DvzSize[]){0});
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);

    /* PICKING PASS */
    DvzRendering pickrend = {0};
    dvz_rendering(&pickrend);
    dvz_rendering_area(&pickrend, 0, 0, DVZ_PROTO_WIDTH, DVZ_PROTO_HEIGHT);

    DvzAttachment* patt = dvz_rendering_color(&pickrend, 0);
    dvz_attachment_image(
        patt, dvz_image_views_handle(&pickview, 0), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_attachment_ops(patt, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(patt, (VkClearValue){.color = {.uint32 = {0}}});

    dvz_cmd_rendering_begin(cmds, 0, &pickrend);
    dvz_cmd_bind_graphics(cmds, 0, &gfx_pick);
    dvz_cmd_bind_vertex_buffers(cmds, 0, 0, 1, &vbuf, (DvzSize[]){0});
    dvz_cmd_draw(cmds, 0, 0, 3, 0, 1);
    dvz_cmd_rendering_end(cmds, 0);

    /* TRANSITION pickimg → TRANSFER_SRC_OPTIMAL + COPY 1 PIXEL TO STAGING */

    // Build a one-off barrier for pickimg.
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* bimg = dvz_barriers_image(&barriers, dvz_image_handle(&pickimg, 0));
    dvz_barrier_image_stage(
        bimg, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    dvz_barrier_image_access(
        bimg, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    dvz_barrier_image_layout(
        bimg, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_barrier_image_aspect(bimg, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_barrier_image_mip(bimg, 0, 1);
    dvz_barrier_image_layers(bimg, 0, 1);

    dvz_cmd_barriers(cmds, 0, &barriers);

    // Define the region: 1 pixel at the center of the image.
    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_offset(&region, DVZ_PROTO_WIDTH / 2, DVZ_PROTO_HEIGHT / 2, 0);
    dvz_image_region_extent(&region, 1, 1, 1);
    dvz_image_region_aspect(&region, VK_IMAGE_ASPECT_COLOR_BIT);

    // Copy that pixel into the staging buffer.
    dvz_cmd_copy_image_to_buffer(
        cmds, 0, dvz_image_handle(&pickimg, 0), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &region,
        dvz_buffer_handle(&staging), 0);

    dvz_cmd_end(cmds);
    dvz_cmd_submit(cmds); // blocks until done in vklite


    // Step 7: screenshot
    dvz_proto_screenshot(&proto, "build/technique_picking.png");


    // Step 8: read back picked ID from staging buffer
    uint32_t picked_id = 0;
    dvz_buffer_download(&staging, 0, sizeof(uint32_t), &picked_id);
    printf("Picked ID at center pixel = %u\n", picked_id);


    // Step 9: cleanup
    dvz_buffer_destroy(&staging);
    dvz_graphics_destroy(&gfx_pick);
    dvz_graphics_destroy(gfx_color);
    dvz_shader_destroy(&vs);
    dvz_shader_destroy(&fs_pick);
    dvz_buffer_destroy(&vbuf);
    dvz_image_views_destroy(&pickview);
    dvz_images_destroy(&pickimg);
    dvz_proto_destroy(&proto);
    dvz_free(vs_spv);
    dvz_free(fs_color_spv);
    dvz_free(fs_pick_spv);

    return proto.bootstrap.instance.n_errors > 0;
}
