/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Testing fixture offscreen                                                                    */
/*************************************************************************************************/

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "datoviz/common/macros.h"
#include "datoviz/math/types.h"
#include "datoviz/vklite/commands.h"
#include "datoviz/vklite/descriptors.h"
#include "datoviz/vklite/graphics.h"
#include "datoviz/vklite/images.h"
#include "datoviz/vklite/rendering.h"
#include "datoviz/vklite/slots.h"
#include "datoviz/vklite/sync.h"
#include "fixture_gpu.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_FIXTURE_WIDTH  800
#define DVZ_FIXTURE_HEIGHT 600
#define DVZ_FIXTURE_CLEAR_COLOR                                                                  \
    {                                                                                            \
        .1, .2, .3, 1                                                                            \
    }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFixtureOffscreen DvzFixtureOffscreen;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

EXTERN_C_ON

/**
 * Create an offscreen rendering fixture borrowing a GPU fixture.
 *
 * @param gpu the parent GPU fixture
 * @param width width of the offscreen target
 * @param height height of the offscreen target
 * @return allocated offscreen fixture, or NULL on allocation failure
 */
DVZ_EXPORT DvzFixtureOffscreen*
dvz_fixture_offscreen(DvzFixtureGpu* gpu, uint32_t width, uint32_t height);



/**
 * Destroy an offscreen rendering fixture.
 *
 * @param fixture the offscreen fixture
 */
DVZ_EXPORT void dvz_fixture_offscreen_destroy(DvzFixtureOffscreen* fixture);



/**
 * Get the parent GPU fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed GPU fixture
 */
DVZ_EXPORT DvzFixtureGpu* dvz_fixture_offscreen_gpu(DvzFixtureOffscreen* fixture);



/**
 * Get the slots helper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed slots wrapper
 */
DVZ_EXPORT DvzSlots* dvz_fixture_offscreen_slots(DvzFixtureOffscreen* fixture);



/**
 * Get the graphics helper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @param vs_size size in bytes of the vertex shader SPIR-V buffer
 * @param vs_spv vertex shader SPIR-V buffer
 * @param fs_size size in bytes of the fragment shader SPIR-V buffer
 * @param fs_spv fragment shader SPIR-V buffer
 * @return borrowed graphics wrapper
 */
DVZ_EXPORT DvzGraphics* dvz_fixture_offscreen_graphics(
    DvzFixtureOffscreen* fixture, DvzSize vs_size, uint32_t* vs_spv, DvzSize fs_size,
    uint32_t* fs_spv);



/**
 * Get the descriptor wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed descriptor wrapper
 */
DVZ_EXPORT DvzDescriptors* dvz_fixture_offscreen_desc(DvzFixtureOffscreen* fixture);



/**
 * Get the command wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed command wrapper
 */
DVZ_EXPORT DvzCommands* dvz_fixture_offscreen_cmds(DvzFixtureOffscreen* fixture);



/**
 * Get the rendering wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed rendering wrapper
 */
DVZ_EXPORT DvzRendering* dvz_fixture_offscreen_rendering(DvzFixtureOffscreen* fixture);



/**
 * Get the barrier wrapper owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed barrier wrapper
 */
DVZ_EXPORT DvzBarriers* dvz_fixture_offscreen_barriers(DvzFixtureOffscreen* fixture);



/**
 * Get the main color image owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed color image wrapper
 */
DVZ_EXPORT DvzImages* dvz_fixture_offscreen_color(DvzFixtureOffscreen* fixture);



/**
 * Get the main color image view owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed color image-view wrapper
 */
DVZ_EXPORT DvzImageViews* dvz_fixture_offscreen_color_view(DvzFixtureOffscreen* fixture);



/**
 * Get the main depth-stencil image owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed depth image wrapper
 */
DVZ_EXPORT DvzImages* dvz_fixture_offscreen_depth(DvzFixtureOffscreen* fixture);



/**
 * Get the main depth-stencil image view owned by the fixture.
 *
 * @param fixture the offscreen fixture
 * @return borrowed depth image-view wrapper
 */
DVZ_EXPORT DvzImageViews* dvz_fixture_offscreen_depth_view(DvzFixtureOffscreen* fixture);



/**
 * Transition an image with a one-shot command recorded by the fixture.
 *
 * @param fixture the offscreen fixture
 * @param img the image to transition
 * @param access destination access flags
 * @param layout destination layout
 */
DVZ_EXPORT void dvz_fixture_offscreen_transition(
    DvzFixtureOffscreen* fixture, DvzImages* img, VkAccessFlags2 access, VkImageLayout layout);



/**
 * Save the fixture color target as a PNG screenshot.
 *
 * @param fixture the offscreen fixture
 * @param filename output PNG path
 */
DVZ_EXPORT void dvz_fixture_offscreen_png(DvzFixtureOffscreen* fixture, const char* filename);

EXTERN_C_OFF
