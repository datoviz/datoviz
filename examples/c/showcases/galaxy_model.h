/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Deterministic density-wave galaxy model used by the galaxy showcase. */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/scene.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define GALAXY_STAR_COUNT     35000u
#define GALAXY_DUST_COUNT     26250u
#define GALAXY_HII_COUNT      200u
#define GALAXY_PARTICLE_COUNT (GALAXY_STAR_COUNT + GALAXY_DUST_COUNT + 2u * GALAXY_HII_COUNT)



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum GalaxyParticleType
{
    GALAXY_PARTICLE_STAR = 0,
    GALAXY_PARTICLE_DUST,
    GALAXY_PARTICLE_HII_GLOW,
    GALAXY_PARTICLE_HII_CORE,
} GalaxyParticleType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct GalaxyParticle
{
    float phase_degrees;
    float angular_velocity;
    float orientation;
    float major_radius;
    float minor_radius;
    float temperature;
    float brightness;
    float base_size_px;
    float height;
    GalaxyParticleType type;
} GalaxyParticle;


typedef struct GalaxyModel
{
    uint32_t particle_count;
    GalaxyParticle* particles;
    vec3* positions;
    DvzColor* colors;
    float* sizes;
} GalaxyModel;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

bool galaxy_model_init(GalaxyModel* model, uint32_t seed);

void galaxy_model_update(GalaxyModel* model, double elapsed_years);

void galaxy_model_destroy(GalaxyModel* model);
