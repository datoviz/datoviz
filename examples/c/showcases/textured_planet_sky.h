/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "datoviz/common/types.h"
#include "datoviz/math/types.h"



typedef struct TexturedPlanetSkyLayer
{
    vec3* positions;
    DvzColor* colors;
    float* sizes;
    uint32_t count;
} TexturedPlanetSkyLayer;


typedef struct TexturedPlanetSkyTexture
{
    uint8_t* rgba;
    uint32_t width;
    uint32_t height;
    float transform[9];
} TexturedPlanetSkyTexture;



typedef struct TexturedPlanetSkyModel
{
    TexturedPlanetSkyLayer stars;
    TexturedPlanetSkyTexture galaxy;
    char snapshot_utc[32];
} TexturedPlanetSkyModel;



/**
 * Load a prepared Gaia/2MASS celestial-sky binary.
 *
 * @param path binary path
 * @param model output model
 * @return whether the model loaded successfully
 */
bool textured_planet_sky_model_load(const char* path, TexturedPlanetSkyModel* model);



/**
 * Free arrays owned by a prepared celestial-sky model.
 *
 * @param model model to clear
 */
void textured_planet_sky_model_destroy(TexturedPlanetSkyModel* model);
