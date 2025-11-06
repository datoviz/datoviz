/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Proto                                                                                        */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/vk/bootstrap.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/sampler.h"
#include "datoviz/vklite/shader.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_PROTO_WIDTH  800
#define DVZ_PROTO_HEIGHT 600
#define DVZ_PROTO_CLEAR_COLOR                                                                     \
    {                                                                                             \
        .1, .2, .3, 1                                                                             \
    }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzProto DvzProto;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzProto
{
    DvzBootstrap bootstrap;
    DvzShader vs;
    DvzShader fs;
    DvzGraphics graphics;
    DvzCompute compute;
    DvzSlots slots;
    DvzDescriptors desc;
    DvzRendering rendering;
    DvzImages img;
    DvzImageViews view;
    DvzImages tex;
    DvzImageViews tex_view;
    DvzSampler sampler;
    DvzBarriers barriers;
    DvzBarrierImage* bimg;
    DvzAttachment* attachment; // main color attachment
    DvzCommands cmds;
    DvzBuffer staging;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON



/**
 * Initialize a proto.
 *
 * @param proto the proto
 */
DVZ_EXPORT void dvz_proto(DvzProto* proto);



/**
 * Get slots.
 *
 * @param proto the proto
 * @returns the slots
 */
DVZ_EXPORT DvzSlots* dvz_proto_slots(DvzProto* proto);



/**
 * Get the graphics pipeline.
 *
 * @param proto the proto
 * @param vs_size the size of the buffer with the SPIR-V code of the vertex shader
 * @param fs_size the size of the buffer with the SPIR-V code of the fragment shader
 * @returns the graphics pipeline
 */
DVZ_EXPORT DvzGraphics* dvz_proto_graphics(
    DvzProto* proto, DvzSize vs_size, uint32_t* vs_spv, DvzSize fs_size, uint32_t* fs_spv);



/**
 * Get the command buffers.
 *
 * @param proto the proto
 * @returns the commands
 */
DVZ_EXPORT DvzCommands* dvz_proto_commands(DvzProto* proto);



DVZ_EXPORT void dvz_proto_transition(
    DvzProto* proto, DvzImages* img, VkAccessFlags2 access, VkImageLayout dst_layout);



/**
 * Save a screenshot.
 *
 * @param proto the proto
 * @param filename the path to the PNG RGBA file
 */
DVZ_EXPORT void dvz_proto_screenshot(DvzProto* proto, const char* filename);



/**
 * Destroy the proto.
 *
 * @param proto the proto
 */
DVZ_EXPORT void dvz_proto_destroy(DvzProto* proto);



EXTERN_C_OFF
